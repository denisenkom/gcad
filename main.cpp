#define _WIN32_IE 0x0500
#define _WIN32_WINNT 0x0501
#include "globals.h"
#include "resource.h"
#include <windows.h>
#include <windowsx.h>
#include <afxres.h>
#include <commctrl.h>
#include <list>
#include <cassert>
#include <cmath>


using namespace std;


HINSTANCE g_hInstance = 0;
HWND g_hmainWindow = 0;
HWND g_topRebar = 0;
HWND g_leftRebar = 0;
HWND g_rightRebar = 0;
HWND g_htoolbar = 0;
HWND g_htoolbarDraw = 0;
HWND g_htoolbarMod = 0;
HWND g_statusBar = 0;
HWND g_hclientWindow = 0;
HPEN g_gridHPen = 0;
HCURSOR g_cursorHandle = 0;
HPEN g_cursorHPen = 0;
HPEN g_lineHPen = 0;
HPEN g_selectedLineHPen = 0;
HBRUSH g_manipHBrush = 0;
HBRUSH g_selectedManipHBrush = 0;
HPEN g_objectSnapHPen = 0;

bool g_objSnapDrawn = false;
PointType g_objSnapType;
Point<int> g_objSnapPos;


void DrawObjectSnap(HDC hdc, Point<int> pos, PointType type)
{
	SelectObject(hdc, g_objectSnapHPen);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, pos.X - 5, pos.Y - 5, pos.X + 6, pos.Y + 6);
}


LRESULT CALLBACK ClientWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
		{
		g_gridHPen = CreatePen(PS_SOLID, 0, RGB(50, 50, 50));
		if (!g_gridHPen)
			assert(0);
		g_cursorHPen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
		if (!g_cursorHPen)
			assert(0);
		g_lineHPen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
		if (!g_lineHPen)
			assert(0);
		{
			LOGBRUSH logbrush;
			logbrush.lbStyle = BS_SOLID;
			logbrush.lbColor = RGB(255, 255, 255);
			DWORD style[] = {1, 1};
		g_selectedLineHPen = ExtCreatePen(PS_COSMETIC | PS_USERSTYLE, 1, &logbrush, sizeof(style)/sizeof(style[0]), style);
		if (!g_selectedLineHPen)
			assert(0);
		}
		g_manipHBrush = CreateSolidBrush(RGB(0, 0, 255));
		assert(g_manipHBrush);
		g_selectedManipHBrush = CreateSolidBrush(RGB(255, 0, 0));
		assert(g_selectedManipHBrush);
		g_objectSnapHPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
		assert(g_objectSnapHPen);

		{SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
		si.nMin = g_vscrollMin;
		si.nMax = g_vscrollMax;
		si.nPos = g_vscrollPos;
		SetScrollInfo(hwnd, SB_VERT, &si, true);}

		{SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
		si.nMin = g_hscrollMin;
		si.nMax = g_hscrollMax;
		si.nPos = g_hscrollPos;
		SetScrollInfo(hwnd, SB_HORZ, &si, true);}

		g_defaultTool.Start(list<CadObject*>());
		}
		return 0;
	case WM_DESTROY:
		if (!DeleteObject(g_objectSnapHPen))
			assert(0);
		if (!DeleteObject(g_selectedLineHPen))
			assert(0);
		if (!DeleteObject(g_lineHPen))
			assert(0);
		if (!DeleteObject(g_cursorHPen))
			assert(0);
		if (!DeleteObject(g_gridHPen))
			assert(0);
		if (!DeleteObject(g_manipHBrush))
			assert(0);
		if (!DeleteObject(g_selectedManipHBrush))
			assert(0);
		g_selector.SelectHandler = Loki::Functor<void, LOKI_TYPELIST_2(CadObject *, bool)>();
		g_fantomManager.RecalcFantomsHandler = Loki::Functor<void>();
		return 0;
	case WM_PAINT:
		{
		PAINTSTRUCT paintStruct;
		HDC hdc = BeginPaint(hwnd, &paintStruct);
		if (hdc == 0)
			return 0;
		Point<float> updateMin = ScreenToWorld(paintStruct.rcPaint.left, paintStruct.rcPaint.bottom);
		Point<float> updateMax = ScreenToWorld(paintStruct.rcPaint.right, paintStruct.rcPaint.top);
		Point<float> gridMin, gridMax;
		gridMin.X = max(updateMin.X, static_cast<float>(g_extentMin.X));
		gridMin.Y = max(updateMin.Y, static_cast<float>(g_extentMin.Y));
		gridMax.X = min(updateMax.X, static_cast<float>(g_extentMax.X));
		gridMax.Y = min(updateMax.Y, static_cast<float>(g_extentMax.Y));
		if (gridMin.X <= gridMax.X && gridMin.Y <= gridMax.Y)
		{
			float gridStepF = static_cast<float>(g_gridStep);
			float startx = floor(gridMin.X / gridStepF) * gridStepF;
			float starty = floor(gridMin.Y / gridStepF) * gridStepF;
			float endx = ceil(gridMax.X / gridStepF) * gridStepF;
			float endy = ceil(gridMax.Y / gridStepF) * gridStepF;
			SelectObject(hdc, g_gridHPen);
			for (float y = starty; y <= endy; y += gridStepF)
			{
				Point<int> from = WorldToScreen(startx, y);
				Point<int> to = WorldToScreen(endx, y);
				MoveToEx(hdc, from.X, from.Y, 0);
				LineTo(hdc, to.X, to.Y);
			}
			for (float x = startx; x <= endx; x += gridStepF)
			{
				Point<int> from = WorldToScreen(x, starty);
				Point<int> to = WorldToScreen(x, endy);
				MoveToEx(hdc, from.X, from.Y, 0);
				LineTo(hdc, to.X, to.Y);
			}
		}

		for (list<CadObject *>::const_iterator i = g_doc.Objects.begin();
			i != g_doc.Objects.end(); i++)
		{
			(*i)->Draw(hdc, IsSelected(*i));
		}

		g_defaultTool.DrawManipulators(hdc);

		if (g_cursorDrawn)
			DrawCursor(hdc);
		g_fantomManager.DrawFantoms(hdc);
		g_objSnapDrawn = false;
		EndPaint(hwnd, &paintStruct);
		}
		return 0;
	case WM_SIZE:
		{
		g_viewHeight = HIWORD(lparam);
		g_viewWidth = LOWORD(lparam);

		{SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_DISABLENOSCROLL;
		si.nPage = g_viewHeight;
		ExtendVScrollLimits(si);
		/*if (g_viewHeight >= g_vscrollMax - g_vscrollMin + 1)
		{
			si.fMask |= SIF_POS;
			si.nPos = g_vscrollPos = (g_vscrollMax - g_vscrollMin + 1 - g_viewHeight) / 2;
		}
		else if (g_vscrollPos > g_vscrollMax - g_viewHeight + 1)
		{
			si.fMask |= SIF_POS;
			si.nPos = g_vscrollPos = g_vscrollMax - g_viewHeight + 1;
		}*/
		SetScrollInfo(hwnd, SB_VERT, &si, true);}

		{SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_DISABLENOSCROLL;
		si.nPage = g_viewWidth;
		ExtendHScrollLimits(si);
		/*if (g_viewWidth >= g_hscrollMax - g_hscrollMin + 1)
		{
			si.fMask |= SIF_POS;
			si.nPos = g_hscrollPos = (g_hscrollMax - g_hscrollMin + 1 - g_viewWidth) / 2;
		}
		else if (g_hscrollPos > g_hscrollMax - g_viewWidth + 1)
		{
			si.fMask |= SIF_POS;
			si.nPos = g_hscrollPos = g_hscrollMax - g_viewWidth + 1;
		}*/
		SetScrollInfo(hwnd, SB_HORZ, &si, true);}
		}
		return 0;
	case WM_VSCROLL:
		{
		int oldPos = g_vscrollPos;
		switch (LOWORD(wparam))
		{
        case SB_PAGEDOWN:
            g_vscrollPos += g_viewHeight;
			g_vscrollPos = min(g_vscrollPos, g_vrangeMax - g_viewHeight + 1);
            break;
        case SB_PAGEUP:
            g_vscrollPos -= g_viewHeight;
			g_vscrollPos = max(g_vscrollPos, g_vrangeMin);
            break;
        case SB_LINEUP:
            g_vscrollPos -= 10; // TODO: make it DPI dependent
            break;
        case SB_LINEDOWN:
            g_vscrollPos += 10; // TODO: make it DPI dependent
            break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
			{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_TRACKPOS;
			if (!GetScrollInfo(hwnd, SB_VERT, &si))
				assert(0);
			g_vscrollPos = si.nTrackPos;
			}
            break;
        case SB_TOP:
            g_vscrollPos = g_vscrollMin;
            break;
        case SB_BOTTOM:
            g_vscrollPos = g_vscrollMax - g_viewHeight + 1;
            break;
        case SB_ENDSCROLL:
            break;
		default:
			assert(0);
			break;
		}
        if (oldPos != g_vscrollPos)
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_DISABLENOSCROLL;
			si.nPos = g_vscrollPos;
			ExtendVScrollLimits(si);
			SetScrollInfo(hwnd, SB_VERT, &si, true);

            ScrollWindow(hwnd, 0, oldPos - g_vscrollPos, 0, 0);
		}
		}
		return 0;
	case WM_HSCROLL:
		{
		int oldPos = g_hscrollPos;
		switch (LOWORD(wparam))
		{
        case SB_PAGEDOWN:
            g_hscrollPos += g_viewWidth;
			g_hscrollPos = min(g_hscrollPos, g_hrangeMax - g_viewWidth + 1);
            break;
        case SB_PAGEUP:
            g_hscrollPos -= g_viewWidth;
			g_hscrollPos = max(g_hscrollPos, g_hrangeMin);
            break;
        case SB_LINEUP:
            g_hscrollPos -= 10; // TODO: make it DPI dependent
            break;
        case SB_LINEDOWN:
            g_hscrollPos += 10; // TODO: make it DPI dependent
            break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
			{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_TRACKPOS;
			if (!GetScrollInfo(hwnd, SB_HORZ, &si))
				assert(0);
			g_hscrollPos = si.nTrackPos;
			}
            break;
        case SB_TOP:
            g_hscrollPos = g_hscrollMin;
            break;
        case SB_BOTTOM:
            g_hscrollPos = g_hscrollMax - g_viewWidth + 1;
            break;
        case SB_ENDSCROLL:
            break;
		default:
			assert(0);
			break;
		}
        if (oldPos != g_hscrollPos)
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_DISABLENOSCROLL;
			si.nPos = g_hscrollPos;
			ExtendHScrollLimits(si);
			SetScrollInfo(hwnd, SB_HORZ, &si, true);

            ScrollWindow(hwnd, oldPos - g_hscrollPos, 0, 0, 0);
		}
		}
		return 0;
	case WM_MOUSEWHEEL:
		if (GET_KEYSTATE_WPARAM(wparam) & MK_CONTROL)
		{
			POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
			if (!ScreenToClient(hwnd, &pt))
				assert(0);
			Zoom(g_magification, g_magification * (GET_WHEEL_DELTA_WPARAM(wparam) * 0.2f / WHEEL_DELTA),
				g_hscrollPos, g_vscrollPos, pt.x, pt.y);
			g_fantomManager.RecalcFantoms();
		}
		else
		{
			if (g_viewHeight < g_vscrollMax - g_vscrollMin + 1)
			{
				int oldPos = g_vscrollPos;
				int newPos = g_vscrollPos - GET_WHEEL_DELTA_WPARAM(wparam) * 30 / WHEEL_DELTA; // TODO: make dpi dependent
				newPos = min(g_vscrollMax - g_viewHeight + 1, max(g_vscrollMin, newPos));
				if (oldPos != newPos)
				{
					try
					{
						ClientDC hdc(hwnd);
						if (g_cursorDrawn)
							DrawCursor(hdc);
						g_fantomManager.DrawFantoms(hdc);
					}
					catch (WindowsError &)
					{
					}

					SCROLLINFO si;
					si.cbSize = sizeof(si);
					si.fMask = SIF_POS;
					si.nPos = g_vscrollPos = newPos;
					SetScrollInfo(hwnd, SB_VERT, &si, true);

					ScrollWindow(hwnd, 0, oldPos - newPos, 0, 0);

					try
					{
						ClientDC hdc(hwnd);
						if (g_cursorDrawn)
							DrawCursor(hdc);
						g_fantomManager.RecalcFantoms();
						g_fantomManager.DrawFantoms(hdc);
					}
					catch (WindowsError &)
					{
					}
				}
			}
		}
		return 0;
	case WM_MOUSEHWHEEL:
		if (g_viewWidth < g_hscrollMax - g_hscrollMin + 1)
		{
			int oldPos = g_hscrollPos;
			int newPos = g_hscrollPos + GET_WHEEL_DELTA_WPARAM(wparam) * 30 / WHEEL_DELTA; // TODO: make dpi dependent
			newPos = min(g_hscrollMax - g_viewWidth + 1, max(g_hscrollMin, newPos));
			if (oldPos != newPos)
			{
				try
				{
					ClientDC hdc(hwnd);
					if (g_cursorDrawn)
						DrawCursor(hdc);
					g_fantomManager.DrawFantoms(hdc);
				}
				catch (WindowsError &)
				{
				}

				SCROLLINFO si;
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS;
				si.nPos = g_hscrollPos = newPos;
				SetScrollInfo(hwnd, SB_HORZ, &si, true);

				ScrollWindow(hwnd, oldPos - newPos, 0, 0, 0);

				try
				{
					ClientDC hdc(hwnd);
					if (g_cursorDrawn)
						DrawCursor(hdc);
					g_fantomManager.RecalcFantoms();
					g_fantomManager.DrawFantoms(hdc);
				}
				catch (WindowsError &)
				{
				}
			}
		}
		return 1;
	case WM_MOUSEMOVE:
		if (!g_mouseInsideClient)
		{
			TRACKMOUSEEVENT tme = {0};
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;
			if (TrackMouseEvent(&tme))
			{
				g_mouseInsideClient = true;
				switch (g_cursorType)
				{
				case CursorTypeManual:
					ShowCursor(false);
					break;
				case CursorTypeSystem:
					SetCursor(g_cursorHandle);
					break;
				default:
					assert(0);
					break;
				}
			}
		}
		switch (g_cursorType)
		{
		case CursorTypeManual:
			{
				try
				{
					ClientDC hdc(hwnd);
					SetROP2(hdc, R2_XORPEN);
					if (g_cursorDrawn)
						DrawCursorRaw(hdc, g_cursorScn.X, g_cursorScn.Y);
					g_fantomManager.DrawFantoms(hdc);
					g_cursorScn.X = GET_X_LPARAM(lparam);
					g_cursorScn.Y = GET_Y_LPARAM(lparam);
					g_cursorWrld = ScreenToWorld(g_cursorScn);
					bool snapped;
					snapped = false;
					// checking for object snap
					if (g_canSnap)
					{
						if (g_objectSnapEnable)
						{
							bool first = true;
							double minDist;
							Point<double> best;
							PointType bestType;
							// finding closest point
							for (list<CadObject *>::iterator i = g_doc.Objects.begin();
								i != g_doc.Objects.end(); i++)
							{
								vector<pair<Point<double>, PointType> > points = (*i)->GetPoints();
								for (vector<pair<Point<double>, PointType> >::iterator j = points.begin();
									j != points.end(); j++)
								{
									double dx = j->first.X - g_cursorWrld.X;
									double dy = j->first.Y - g_cursorWrld.Y;
									double dist = sqrt(dx*dx + dy*dy);
									if (dist < minDist || first)
									{
										first = false;
										minDist = dist;
										best = j->first;
										bestType = j->second;
									}
								}
							}
							// if found
							if (!first)
							{
								Point<int> bestScn = WorldToScreen(best);
								bool show = false;
								if (minDist * g_magification < 50)
								{
									show = true;
								}
								if (minDist * g_magification < 5)
								{
									g_cursorWrld = best;
									g_cursorScn = bestScn;
									snapped = true;
								}
								if (show)
								{
									bool needDraw = false;
									if (g_objSnapDrawn)
									{
										if (g_objSnapPos != bestScn || g_objSnapType != bestType)
										{
											// erasing
											DrawObjectSnap(hdc, g_objSnapPos, g_objSnapType);
											needDraw = true;
										}
									}
									else
									{
										needDraw = true;
									}
									if (needDraw)
									{
										DrawObjectSnap(hdc, bestScn, bestType);
										g_objSnapDrawn = true;
										g_objSnapPos = bestScn;
										g_objSnapType = bestType;
									}
								}
								else
								{
									if (g_objSnapDrawn)
									{
										DrawObjectSnap(hdc, g_objSnapPos, g_objSnapType);
										g_objSnapDrawn = false;
									}
								}
							}
						}
						if (!snapped && g_snapEnable)
						{
							g_cursorWrld.X = floor((g_cursorWrld.X + g_snapStep/2)/g_snapStep)*g_snapStep;
							g_cursorWrld.Y = floor((g_cursorWrld.Y + g_snapStep/2)/g_snapStep)*g_snapStep;
							g_cursorScn = WorldToScreen(g_cursorWrld);
						}
					}
					DrawCursorRaw(hdc, g_cursorScn.X, g_cursorScn.Y);
					g_fantomManager.RecalcFantoms();
					g_fantomManager.DrawFantoms(hdc);
					g_cursorDrawn = true;
				}
				catch (WindowsError &)
				{
				}
			}
			{
			wchar_t buffer[32];
			int len = swprintf(buffer, L"%#.4f, %#.4f", g_cursorWrld.X, g_cursorWrld.Y);
			assert(len > 0);
			assert(static_cast<size_t>(len) < sizeof(buffer)/sizeof(buffer[0]) - 1);
			SetWindowTextW(g_statusBar, buffer);
			}
			break;
		case CursorTypeSystem:
			break;
		default:
			assert(0);
			break;
		}
		g_curTool->ProcessInput(hwnd, msg, wparam, lparam);
		return 0;
	case WM_MOUSELEAVE:
		g_mouseInsideClient = false;
		switch (g_cursorType)
		{
		case CursorTypeManual:
			ShowCursor(true);
			if (g_cursorDrawn)
			{
				try
				{
					ClientDC hdc(hwnd);
					DrawCursor(hdc);
					g_cursorDrawn = false;
				}
				catch (WindowsError &)
				{
				}
			}
			break;
		case CursorTypeSystem:
			break;
		default:
			assert(0);
			break;
		}
		return 0;
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_ESCAPE:
			g_fantomManager.DeleteFantoms(false);
			g_curTool->Cancel();
			if (g_curTool != &g_defaultTool)
			{
				g_curTool = &g_defaultTool;
				g_curTool->Start(list<CadObject *>());
			}
			InvalidateRect(hwnd, 0, true);
			UpdateWindow(hwnd);
			break;
		default:
			g_curTool->ProcessInput(hwnd, msg, wparam, lparam);
			break;
		}
		return 0;
	}
	g_curTool->ProcessInput(hwnd, msg, wparam, lparam);
	return DefWindowProc(hwnd, msg, wparam, lparam);
}


class DxfReader
{
public:
	explicit DxfReader(HANDLE hfile) : m_hfile(hfile), m_lastPos(0), m_bufSize(0)
	{
		assert(hfile != 0 && hfile != INVALID_HANDLE_VALUE);
	}
	std::string ReadLine();
	bool ReadItem(std::pair<int, std::string> & result);
private:
	HANDLE m_hfile;
	char m_buffer[4096];
	unsigned int m_lastPos;
	unsigned int m_bufSize;
};


class DxfIOError
{
public:
	DxfIOError() : m_err(GetLastError()) {}
	LocalAllocStr GetStr() { return GetWinErrorStr(m_err); }
private:
	unsigned long m_err;
};


string DxfReader::ReadLine()
{
	string line;
	while (true)
	{
		if (m_lastPos == m_bufSize)
		{
			unsigned long read;
			if (!ReadFile(m_hfile, m_buffer, sizeof(m_buffer), &read, 0))
				throw DxfIOError();
			assert(read <= sizeof(m_buffer));
			m_lastPos = 0;
			m_bufSize = static_cast<unsigned int>(read);
			if (m_bufSize == 0)
				return line;
		}
		char ch = m_buffer[m_lastPos];
		m_lastPos++;
		switch (ch)
		{
		case '\r': break;
		case '\n': return line;
		default: line += ch;
		}
	}
}


bool DxfReader::ReadItem(pair<int, string> & result)
{
	string codeStr;
	string valStr;
	try
	{
		codeStr = ReadLine();
		valStr = ReadLine();
	}
	catch (DxfIOError & ex)
	{
		throw wstring(L"Ошибка чтения файла: ") + ex.GetStr().c_str();
	}
	if (codeStr.size() == 0)
		return false;
	int code;
	int ret = sscanf(codeStr.c_str(), "%d", &code);
	if (ret != 1)
		throw wstring(L"Файл имеет неправильный формат");
	result = make_pair(code, valStr);
	return true;
}


double DxfStrToDouble(const char * str)
{
	char * endp;
	double result = strtod(str, &endp);
	if (str == endp)
		throw wstring(L"Файл имеет неправильный формат");
	return result;
}



LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
		g_topRebar = CreateWindowExW(0, REBARCLASSNAMEW, 0,
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
				RBS_VARHEIGHT | CCS_NODIVIDER | RBS_BANDBORDERS | CCS_NORESIZE, 0, 0, 0, 0,
				hwnd, 0, g_hInstance, 0);
		if (g_topRebar == 0)
			return -1;
		g_leftRebar = CreateWindowExW(0, REBARCLASSNAMEW, 0,
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
				RBS_VARHEIGHT | CCS_NODIVIDER | RBS_BANDBORDERS | CCS_VERT | CCS_NORESIZE, 0, 0, 0, 0,
				hwnd, 0, g_hInstance, 0);
		if (g_leftRebar == 0)
			return -1;
		g_rightRebar = CreateWindowExW(0, REBARCLASSNAMEW, 0,
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
				RBS_VARHEIGHT | CCS_NODIVIDER | RBS_BANDBORDERS | CCS_VERT | CCS_NORESIZE, 0, 0, 0, 0,
				hwnd, 0, g_hInstance, 0);
		if (g_rightRebar == 0)
			return -1;
		g_statusBar = CreateWindowExW(0, STATUSCLASSNAMEW, 0, WS_CHILD | WS_VISIBLE |
				WS_CLIPSIBLINGS | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, 0, g_hInstance, 0);
		if (g_statusBar == 0)
			return -1;
		do
		{
			g_htoolbar = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"", WS_CHILD |
					WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_FLAT |
					CCS_NORESIZE | CCS_NODIVIDER, 0, 0, 0, 0, hwnd, 0, g_hInstance, 0);
			if (!g_htoolbar)
				return -1;
			//SendMessageW(g_htoolbar, TB_SETIMAGELIST, 0, g_himageList);
			TBADDBITMAP tbAddBitmap = { g_hInstance, reinterpret_cast<INT_PTR>(MAKEINTRESOURCEW(IDB_TOOLBAR)) };
			if (SendMessageW(g_htoolbar, TB_ADDBITMAP, 2, reinterpret_cast<LPARAM>(&tbAddBitmap)) == -1)
				return -1;
			SendMessageW(g_htoolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			const TBBUTTON buttons[] = {
				{0, ID_VIEW_PAN, TBSTATE_ENABLED, 0, 0, 0 },
				{1, ID_VIEW_ZOOM, TBSTATE_ENABLED, 0, 0, 0 }};
			if (!SendMessageW(g_htoolbar, TB_ADDBUTTONS, sizeof(buttons) / sizeof(buttons[0]), reinterpret_cast<LPARAM>(buttons)))
				return -1;
			long buttonSize = SendMessageW(g_htoolbar, TB_GETBUTTONSIZE, 0, 0);
			int buttonHeight = HIWORD(buttonSize);
			int buttonWidth = LOWORD(buttonSize);
			MoveWindow(g_htoolbar, 0, 0, buttonWidth * sizeof(buttons) / sizeof(buttons[0]), buttonHeight, false);
		}
		while (false);

		do
		{
			g_htoolbarDraw = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"", WS_CHILD |
					WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_FLAT |
					CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_WRAPABLE, 0, 0, 0, 0, hwnd, 0, g_hInstance, 0);
			if (!g_htoolbarDraw)
				return -1;
			TBADDBITMAP tbAddBitmap = { g_hInstance, reinterpret_cast<INT_PTR>(MAKEINTRESOURCEW(IDB_TBDRAW)) };
			if (SendMessageW(g_htoolbarDraw, TB_ADDBITMAP, 2, reinterpret_cast<LPARAM>(&tbAddBitmap)) == -1)
				return -1;
			SendMessageW(g_htoolbarDraw, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			const TBBUTTON buttons[] = {
				{0, ID_DRAW_LINES, TBSTATE_ENABLED, 0, 0, 0 },
				{1, ID_DRAW_ARCS, TBSTATE_ENABLED, 0, 0, 0 },
				{2, ID_DRAW_CIRCLE, TBSTATE_ENABLED, 0, 0, 0 }};
			if (!SendMessageW(g_htoolbarDraw, TB_ADDBUTTONS, sizeof(buttons) / sizeof(buttons[0]), reinterpret_cast<LPARAM>(buttons)))
				return -1;
			long buttonSize = SendMessageW(g_htoolbarDraw, TB_GETBUTTONSIZE, 0, 0);
			int buttonHeight = HIWORD(buttonSize);
			int buttonWidth = LOWORD(buttonSize);
			MoveWindow(g_htoolbarDraw, 0, 0, buttonWidth * sizeof(buttons) / sizeof(buttons[0]), buttonHeight, false);
		}
		while (false);

		do
		{
			g_htoolbarMod = CreateWindowExW(0, TOOLBARCLASSNAMEW, L"", WS_CHILD |
					WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_FLAT |
					CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_WRAPABLE, 0, 0, 0, 0, hwnd, 0, g_hInstance, 0);
			if (!g_htoolbarMod)
				return -1;
			TBADDBITMAP tbAddBitmap = { g_hInstance, reinterpret_cast<INT_PTR>(MAKEINTRESOURCEW(IDB_TBMODIFY)) };
			if (SendMessageW(g_htoolbarMod, TB_ADDBITMAP, 2, reinterpret_cast<LPARAM>(&tbAddBitmap)) == -1)
				return -1;
			SendMessageW(g_htoolbarMod, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
			const TBBUTTON buttons[] = {
				{0, ID_MODIFY_MOVE, TBSTATE_ENABLED, 0, 0, 0 },
				};
			if (!SendMessageW(g_htoolbarMod, TB_ADDBUTTONS, sizeof(buttons) / sizeof(buttons[0]), reinterpret_cast<LPARAM>(buttons)))
				return -1;
			long buttonSize = SendMessageW(g_htoolbarMod, TB_GETBUTTONSIZE, 0, 0);
			int buttonHeight = HIWORD(buttonSize);
			int buttonWidth = LOWORD(buttonSize);
			MoveWindow(g_htoolbarMod, 0, 0, buttonWidth * sizeof(buttons) / sizeof(buttons[0]), buttonHeight, false);
		}
		while (false);

		do
		{
			do
			{
				REBARBANDINFOW bi = {0};
				bi.cbSize = sizeof(bi);
				bi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE;
				bi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
				bi.hwndChild = g_htoolbar;
				RECT rect;
				GetWindowRect(g_htoolbar, &rect);
				bi.cyMinChild = rect.bottom - rect.top;
				bi.cxMinChild = rect.right - rect.left;
				if (!SendMessageW(g_topRebar, RB_INSERTBAND, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&bi)))
					return -1;
			}
			while (false);
			do
			{
				REBARBANDINFOW bi = {0};
				bi.cbSize = sizeof(bi);
				bi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE;
				bi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
				bi.hwndChild = g_htoolbarDraw;
				RECT rect;
				GetWindowRect(g_htoolbarDraw, &rect);
				bi.cyMinChild = rect.bottom - rect.top;
				bi.cxMinChild = rect.right - rect.left;
				if (!SendMessageW(g_leftRebar, RB_INSERTBAND, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&bi)))
					return -1;
			}
			while (false);
			do
			{
				REBARBANDINFOW bi = {0};
				bi.cbSize = sizeof(bi);
				bi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE;
				bi.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;
				bi.hwndChild = g_htoolbarMod;
				RECT rect;
				GetWindowRect(g_htoolbarMod, &rect);
				bi.cyMinChild = rect.bottom - rect.top;
				bi.cxMinChild = rect.right - rect.left;
				if (!SendMessageW(g_rightRebar, RB_INSERTBAND, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&bi)))
					return -1;
			}
			while (false);
		}
		while (false);

		g_hclientWindow = CreateWindowExW(0, MAINCLIENTCLASS, L"", WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(1), g_hInstance, 0);
		if (g_hclientWindow == 0)
			return -1;
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_NOTIFY:
		NMHDR * nmhdr;
		nmhdr = reinterpret_cast<NMHDR*>(lparam);
		if (nmhdr->hwndFrom == g_topRebar)
		{
			switch (nmhdr->code)
			{
			case RBN_HEIGHTCHANGE:
				RECT toolbarRect;
				GetWindowRect(g_topRebar, &toolbarRect);
				RECT clientRect;
				GetClientRect(hwnd, &clientRect);
				int width = clientRect.right;
				int height = clientRect.bottom - (toolbarRect.bottom - toolbarRect.top);
				MoveWindow(g_hclientWindow, 0, toolbarRect.bottom - toolbarRect.top,
					width, height, true);
				break;
			}
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	case WM_COMMAND:
		switch (LOWORD(wparam))
		{
		case ID_FILE_CLOSE:
			DestroyWindow(hwnd);
			return 0;
		case ID_FILE_IMPORTDXF:
			do
			{
				wchar_t fileBuf[MAX_PATH] = {0};
				OPENFILENAMEW ofn = {0};
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hwnd;
				ofn.lpstrFilter = L"Файлы DXF\0*.dxf\0Все файлы\0*.*\0\0";
				ofn.lpstrFile = fileBuf;
				ofn.nMaxFile = sizeof(fileBuf)/sizeof(fileBuf[0]);
				ofn.lpstrTitle = L"Импорт DXF";
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
				if (!GetOpenFileNameW(&ofn))
				{
					if (CommDlgExtendedError() != 0)
						assert(0);
					return 0;
				}
				try
				{
					WinHandle hfile = CreateFileW(fileBuf, FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
					if (!hfile)
					{
						throw wstring(L"Не удалось открыть файл: ") + fileBuf +
										GetWinErrorStr().c_str();
					}
					DxfReader rdr(hfile.get());
					bool foundEntities = false;
					pair<int, string> item;
					while (rdr.ReadItem(item))
					{
						if (item.first == 2 && item.second == "ENTITIES")
						{
							foundEntities = true;
							break;
						}
					}
					if (!foundEntities)
					{
						MessageBoxW(hwnd, L"Файл не содержит рисунка", 0, MB_ICONEXCLAMATION);
						return 0;
					}
					if (!rdr.ReadItem(item) || item.first != 0)
						throw wstring(L"Неправильный формат dxf файла");
					bool done = false;
					while (!done)
					{
						if (item.second == "LINE")
						{
							enum Flags
							{
								P1X = 1,
								P1Y = 2,
								P2X = 4,
								P2Y = 8,
							};
							int flags = 0;
							Point<double> p1, p2;
							bool lineDone = false;
							while (!lineDone && rdr.ReadItem(item))
							{
								switch (item.first)
								{
								case 0:
									lineDone = true;
									break;
								case 10:
									p1.X = DxfStrToDouble(item.second.c_str());
									flags |= P1X;
									break;
								case 20:
									p1.Y = DxfStrToDouble(item.second.c_str());
									flags |= P1Y;
									break;
								case 11:
									p2.X = DxfStrToDouble(item.second.c_str());
									flags |= P2X;
									break;
								case 21:
									p2.Y = DxfStrToDouble(item.second.c_str());
									flags |= P2Y;
									break;
								}
							}
							if (!lineDone || flags != (P1X | P1Y | P2X | P2Y))
								throw wstring(L"Неправильный формат dxf файла");
							CadLine * line = new CadLine();
							line->Point1 = p1;
							line->Point2 = p2;
							g_doc.Objects.push_back(line);
						}
						else if (item.second == "ARC")
						{
							Point<double> center;
							double radius;
							double ang1, ang2;
							enum Flags
							{
								CX = 1,
								CY = 2,
								RADIUS = 4,
								ANGLE1 = 8,
								ANGLE2 = 16,
							};
							int flags = 0;
							bool arcDone = false;
							while (!arcDone && rdr.ReadItem(item))
							{
								switch (item.first)
								{
								case 0:
									arcDone = true;
									break;
								case 10:
									center.X = DxfStrToDouble(item.second.c_str());
									flags |= CX;
									break;
								case 20:
									center.Y = DxfStrToDouble(item.second.c_str());
									flags |= CY;
									break;
								case 40:
									radius = DxfStrToDouble(item.second.c_str());
									flags |= RADIUS;
									break;
								case 50:
									ang1 = DxfStrToDouble(item.second.c_str());
									flags |= ANGLE1;
									break;
								case 51:
									ang2 = DxfStrToDouble(item.second.c_str());
									flags |= ANGLE2;
									break;
								}
							}
							if (!arcDone || flags != (CX | CY | RADIUS | ANGLE1 | ANGLE2))
								throw wstring(L"Неправильный формат dxf файла");
							CadArc * arc = new CadArc();
							arc->Center = center;
							arc->Radius = radius;
							arc->Ccw = true;
							arc->Start.X = cos(ang1*M_PI/180)*radius + center.X;
							arc->Start.Y = sin(ang1*M_PI/180)*radius + center.Y;
							arc->End.X = cos(ang2*M_PI/180)*radius + center.X;
							arc->End.Y = sin(ang2*M_PI/180)*radius + center.Y;
							g_doc.Objects.push_back(arc);
						}
						else if (item.second == "ENDSEC")
						{
							done = true;
							break;
						}
						else
						{
							while (rdr.ReadItem(item) && item.first != 0)
								;
						}
					}
					InvalidateRect(g_hclientWindow, 0, true);
				}
				catch (wstring & err)
				{
					if (MessageBoxW(hwnd, err.c_str(), 0, MB_ICONERROR) == 0)
						assert(0);
					return 0;
				}
			}
			while (false);
			return 0;
		case ID_VIEW_PAN:
		case ID_VIEW_ZOOM:
		case ID_DRAW_LINES:
		case ID_DRAW_CIRCLE:
		case ID_DRAW_ARCS:
		case ID_MODIFY_MOVE:
			SelectTool(LOWORD(wparam));
			return 0;
		}
		break;
	case WM_SIZE:
		do
		{
			unsigned int topRebarHeight = static_cast<unsigned int>(SendMessageW(g_topRebar, RB_GETBARHEIGHT, 0, 0));
			unsigned int leftRebarWidth = static_cast<unsigned int>(SendMessageW(g_leftRebar, RB_GETBARHEIGHT, 0, 0));
			unsigned int rightRebarWidth = static_cast<unsigned int>(SendMessageW(g_leftRebar, RB_GETBARHEIGHT, 0, 0));
			SendMessageW(g_statusBar, WM_SIZE, 0, 0);
			RECT sbRect;
			if (!GetWindowRect(g_statusBar, &sbRect))
				assert(0);
			int sbHeight = sbRect.bottom - sbRect.top;
			int width = LOWORD(lparam);
			int height = HIWORD(lparam);
			MoveWindow(g_topRebar, 0, 0, width, topRebarHeight, true);
			MoveWindow(g_leftRebar, 0, topRebarHeight, leftRebarWidth, height - topRebarHeight - sbHeight, true);
			MoveWindow(g_rightRebar, width - rightRebarWidth, topRebarHeight, rightRebarWidth, height - topRebarHeight - sbHeight, true);
			MoveWindow(g_hclientWindow, leftRebarWidth, topRebarHeight,
				width - leftRebarWidth - rightRebarWidth, height - topRebarHeight - sbHeight, true);
		}
		while (false);
		return 0;
	case WM_SETFOCUS:
		SetFocus(g_hclientWindow);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wparam, lparam);
}


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE /*hPrevInst*/, LPSTR /*cmdLine*/, int cmdShow)
{
	g_hInstance = hInst;
	WNDCLASSW cls = {0};
	cls.hInstance = hInst;
	cls.lpfnWndProc = MainWndProc;
	cls.lpszClassName = MAINWNDCLASS;
	cls.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
	if (RegisterClassW(&cls) == 0)
	{
		MessageBoxW(0, L"��� ������� ��������� ��������� Windows NT", 0, MB_ICONHAND);
		return 1;
	}

	WNDCLASSW clientCls = {0};
	clientCls.style = CS_HREDRAW | CS_VREDRAW;
	clientCls.hInstance = hInst;
	clientCls.lpfnWndProc = ClientWndProc;
	clientCls.lpszClassName = MAINCLIENTCLASS;
	clientCls.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	if (RegisterClassW(&clientCls) == 0)
	{
		MessageBoxW(0, L"��� ������� ��������� ��������� Windows NT", 0, MB_ICONHAND);
		return 1;
	}

	InitCommonControls();

	g_hmainWindow = CreateWindowExW(0, MAINWNDCLASS, L"GCad", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInst, 0);
	if (g_hmainWindow == 0)
	{
		MessageBoxW(0, L"������ ��� �������� �������� ����", 0, MB_ICONHAND);
		return 1;
	}

	ShowWindow(g_hmainWindow, cmdShow);

	MSG msg;
	while (GetMessageW(&msg, 0, 0, 0))
	{
		DispatchMessageW(&msg);
	}
	return msg.wParam;
}
