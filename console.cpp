/*
 * console.cpp
 *
 *  Created on: 19.02.2009
 *      Author: misha
 */

#include "console.h"
#include "globals.h"
#include <cassert>


using namespace std;


const wchar_t CONSOLEWNDCLASS[] = L"GCadConsoleWindow";


Console g_console;


LRESULT CALLBACK Console::WndProc(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	// warning: this variables not support multiple instances of this class
	static long windowHeight;
	static long windowWidth;
	const int HISTORYID = 1;
	const int EDITID = 2;
	switch (msg)
	{
	case WM_CREATE:
		do
		{
			ClientDC hdc(0);
			LOGFONTW logfont = {0};
			logfont.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			logfont.lfPitchAndFamily = FIXED_PITCH;
			wcscpy(logfont.lfFaceName, L"Courier");
			m_font = CreateFontIndirectW(&logfont);
			if (m_font == 0)
				assert(0);
			if (SelectObject(hdc, m_font) == 0)
				assert(0);
			TEXTMETRIC tm = {0};
			if (!GetTextMetrics(hdc, &tm))
				assert(0);
			m_lineHeight = tm.tmHeight + tm.tmExternalLeading;
			m_minHeight = m_lineHeight * 5 + 1;
			if (!GetTextExtentPoint32W(hdc, m_prompt.c_str(), m_prompt.size(), &m_promptSize))
				assert(0);
			m_histWnd = CreateWindowExW(0, L"EDIT", 0, WS_CHILD |
					WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
					0,0,0,0, hwnd, reinterpret_cast<HMENU>(HISTORYID), g_hInstance, 0);
			if (m_histWnd == 0)
				assert(0);
			(void)SendMessageW(m_histWnd, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), true);
			m_editWnd = CreateWindowExW(0, L"EDIT", 0, WS_CHILD | WS_VISIBLE |
					ES_AUTOHSCROLL,
					0,0,0,0, hwnd, reinterpret_cast<HMENU>(EDITID), g_hInstance, 0);
			if (m_editWnd == 0)
				assert(0);
			(void)SendMessageW(m_editWnd, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), true);
			m_defEditProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(m_editWnd, GWLP_WNDPROC));
			assert(m_defEditProc != 0);
			if (SetWindowLongPtrW(m_editWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&EditWndProcDispatch)) == 0)
				assert(0);
		}
		while (false);
		return 0;
	case WM_SIZE:
		do
		{
			windowWidth = LOWORD(lparam);
			windowHeight = HIWORD(lparam);
			AlignSubWindows(windowWidth, windowHeight);
		}
		while (false);
		return 0;
	case WM_PAINT:
		do
		{
			PAINTSTRUCT ps;
			if (BeginPaint(hwnd, &ps) == 0)
				assert(0);
			HPEN hpen = reinterpret_cast<HPEN>(GetStockObject(DC_PEN));
			if (!hpen)
				assert(0);
			if (!SelectObject(ps.hdc, hpen))
				assert(0);
			if (SetDCPenColor(ps.hdc, RGB(128, 128, 128)) == CLR_INVALID)
				assert(0);
			MoveToEx(ps.hdc, 0, windowHeight - m_lineHeight - 1, 0);
			LineTo(ps.hdc, windowWidth, windowHeight - m_lineHeight - 1);
			if (!SelectObject(ps.hdc, m_font))
				assert(0);
			if (!TextOutW(ps.hdc, 0, windowHeight - m_lineHeight, m_prompt.c_str(), m_prompt.size()))
				assert(0);
			EndPaint(hwnd, &ps);
		}
		while (false);
		return 0;
	default:
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
}


LRESULT CALLBACK Console::WndProcDispatch(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	return g_console.WndProc(hwnd, msg, wparam, lparam);
}


LRESULT CALLBACK Console::EditWndProc(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_UP:
			if (m_pos == m_begin)
				return 0;
			if (m_pos == 0)
				m_pos = sizeof(m_history)/sizeof(m_history[0]) - 1;
			else
				m_pos--;
			if (!SetWindowTextW(m_editWnd, m_history[m_pos].c_str()))
				assert(0);
			(void)SendMessageW(m_editWnd, EM_SETSEL, m_history[m_pos].size(), m_history[m_pos].size());
			return 0;
		case VK_DOWN:
			if (m_pos == m_end || m_pos == (m_end == 0 ? sizeof(m_history)/sizeof(m_history[0]) - 1 : m_end - 1))
				return 0;
			m_pos++;
			if (m_pos >= sizeof(m_history)/sizeof(m_history[0]))
				m_pos -= sizeof(m_history)/sizeof(m_history[0]);
			if (!SetWindowTextW(m_editWnd, m_history[m_pos].c_str()))
				assert(0);
			(void)SendMessageW(m_editWnd, EM_SETSEL, m_history[m_pos].size(), m_history[m_pos].size());
			return 0;
		case VK_RETURN:
		{
			int length = GetWindowTextLength(m_editWnd) + 1;
			if (length == 1)
				return 0;
			wchar_t * textbuf = new wchar_t[length];
			if (GetWindowTextW(m_editWnd, textbuf, length) == 0)
				assert(0);
			m_history[m_end] = wstring(textbuf, length - 1);
			delete [] textbuf;
			wstring & text = m_history[m_end];
			m_end += 1;
			if (m_end >= sizeof(m_history)/sizeof(m_history[0]))
				m_end -= sizeof(m_history)/sizeof(m_history[0]);
			if (m_end == m_begin)
			{
				m_begin++;
				if (m_begin >= sizeof(m_history)/sizeof(m_history[0]))
					m_begin -= sizeof(m_history)/sizeof(m_history[0]);
			}
			m_pos = m_end;
			if (!SetWindowTextW(m_editWnd, L""))
				assert(0);
			LogCommand(text);
			g_curTool->Command(text);
		}
			return 0;
		case VK_ESCAPE:
			Cancel();
			return 0;
		default:
			return CallWindowProc(m_defEditProc, hwnd, msg, wparam, lparam);
		}
	default:
		return CallWindowProc(m_defEditProc, hwnd, msg, wparam, lparam);
	}
}

LRESULT CALLBACK Console::EditWndProcDispatch(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	return g_console.EditWndProc(hwnd, msg, wparam, lparam);
}

HWND Console::Init(HWND parent)
{
	WNDCLASSW cls = {0};
	cls.hInstance = g_hInstance;
	cls.lpfnWndProc = &WndProcDispatch;
	cls.lpszClassName = CONSOLEWNDCLASS;
	cls.hCursor = LoadCursor(0, IDC_IBEAM);
	if (RegisterClassW(&cls) == 0)
		assert(0);

	m_handle = CreateWindowExW(0, CONSOLEWNDCLASS, 0, WS_CHILD | WS_VISIBLE |
			WS_CLIPCHILDREN,
			0,0,0,0, parent, 0, g_hInstance, 0);
	if (m_handle == 0)
		assert(0);
	return m_handle;
}


LRESULT Console::Input(unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	return EditWndProc(m_editWnd, msg, wparam, lparam);
}


void Console::Log(const wstring & str)
{
	int histLen = GetWindowTextLength(m_histWnd);
	if (histLen != 0)
	{
		(void)SendMessageW(m_histWnd, EM_SETSEL, histLen, histLen);
		(void)SendMessageW(m_histWnd, EM_REPLACESEL, false, reinterpret_cast<LPARAM>(L"\r\n"));
	}
	(void)SendMessageW(m_histWnd, EM_SETSEL, histLen + 2, histLen + 2);
	(void)SendMessageW(m_histWnd, EM_REPLACESEL, false, reinterpret_cast<LPARAM>(str.c_str()));
}


void Console::LogCommand(const wstring & cmd)
{
	Log(m_prompt + cmd);
}


bool Console::HasInput()
{
	return GetWindowTextLength(m_editWnd) > 0;
}


wstring Console::GetInput()
{
	int length = GetWindowTextLength(m_editWnd);
	if (length == 0)
		return wstring();
	wchar_t * textbuf = new wchar_t[length + 1];
	if (GetWindowTextW(m_editWnd, textbuf, length + 1) == 0)
		assert(0);
	return wstring(textbuf, length);
}


void Console::ClearInput()
{
	if (!SetWindowTextW(m_editWnd, L""))
		assert(0);
}


void Console::AlignSubWindows(long windowWidth, long windowHeight)
{
	if (!MoveWindow(m_histWnd,
			0, 0,
			windowWidth, windowHeight - m_lineHeight - 1, true))
	{
		assert(0);
	}
	if (!MoveWindow(m_editWnd,
			m_promptSize.cx, windowHeight - m_lineHeight,
			windowWidth - m_promptSize.cy, m_lineHeight, true))
	{
		assert(0);
	}
}


void Console::SetPrompt(const wstring & prompt)
{
	m_prompt = prompt;
	ClientDC hdc(0);
	if (SelectObject(hdc, m_font) == 0)
		assert(0);
	if (!GetTextExtentPoint32W(hdc, m_prompt.c_str(), m_prompt.size(), &m_promptSize))
		assert(0);
	RECT rect;
	if (!GetClientRect(m_handle, &rect))
		assert(0);
	AlignSubWindows(rect.right - rect.left, rect.bottom - rect.top);
	if (!InvalidateRect(m_handle, 0, true))
		assert(0);
}
