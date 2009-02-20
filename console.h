/*
 * console.h
 *
 *  Created on: 19.02.2009
 *      Author: misha
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_


#include <windows.h>
#include <string>


class Console
{
public:
	Console() {}
	HWND Init(HWND parent);
	void SetPrompt(const std::wstring & prompt);
	long MinHeight() { return m_minHeight; }
	// only for Client window message forwarding
	LRESULT Input(unsigned int msg, WPARAM wparam, LPARAM lparam);
	void Log(const std::wstring & str);
	bool HasInput();
private:
	HWND m_handle;
	HWND m_histWnd;
	HWND m_editWnd;
	std::wstring m_prompt;
	std::wstring m_history[50];
	unsigned m_begin;
	unsigned m_end;
	unsigned m_pos;
	HFONT m_font;
	SIZE m_promptSize;
	long m_lineHeight;
	long m_minHeight;
	WNDPROC m_defEditProc;

	LRESULT CALLBACK WndProc(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	LRESULT CALLBACK EditWndProc(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	static LRESULT CALLBACK WndProcDispatch(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	static LRESULT CALLBACK EditWndProcDispatch(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	void AlignSubWindows(long windowWidth, long windowHeight);
	Console(const Console &);
	Console & operator=(const Console &);
};


extern Console g_console;


#endif /* CONSOLE_H_ */
