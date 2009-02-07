/*
 * globals.cpp
 *
 *  Created on: 28.01.2009
 *      Author: misha
 */
#include "globals.h"
#include "exmath.h"
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>


using namespace std;


Point<double> g_extentMin = Point<double>(0, 0);
Point<double> g_extentMax = Point<double>(2000, 2000);
double g_gridStep = 100;
bool g_snapEnable = false;
double g_snapStep = 10;
bool g_objectSnapEnable = true;

float g_magification = 1;
int g_viewHeight = 0;
int g_viewWidth = 0;
int g_vrangeMin = static_cast<int>(g_extentMin.Y * g_magification);
int g_vrangeMax = static_cast<int>(g_extentMax.Y * g_magification);
int g_vscrollMin = g_vrangeMin;
int g_vscrollMax = g_vrangeMax;
int g_vscrollPos = g_vscrollMin;
int g_hrangeMin = static_cast<int>(g_extentMin.X * g_magification);
int g_hrangeMax = static_cast<int>(g_extentMax.X * g_magification);
int g_hscrollMin = g_hrangeMin;
int g_hscrollMax = g_hrangeMax;
int g_hscrollPos = g_hscrollMin;

CursorType g_cursorType = CursorTypeManual;
CustomCursorType g_customCursorType = CustomCursorTypeSelect;
bool g_cursorDrawn = false;
bool g_mouseInsideClient = false;
Point<int> g_cursorScn;
Point<double> g_cursorWrld;

int g_checkedMenu = 0;

Document g_doc;

SelectorTool g_selectTool;
PanTool g_panTool;
ZoomTool g_zoomTool;
DrawLinesTool g_drawLinesTool;
DrawArcsTool g_drawArcsTool;
Tool * g_curTool = &g_selectTool;

list<Fantom *> g_fantoms;


void DrawCursorRaw(HDC hdc, int x, int y)
{
	// TODO: make dpi dependent
	SelectObject(hdc, g_cursorHPen);
	switch (g_customCursorType)
	{
	case CustomCursorTypeSelect:
		MoveToEx(hdc, x - 20, y, 0);
		LineTo(hdc, x + 20, y);
		MoveToEx(hdc, x, y - 20, 0);
		LineTo(hdc, x, y + 20);

		MoveToEx(hdc, x - 3, y - 3, 0);
		LineTo(hdc, x - 3, y + 3);
		LineTo(hdc, x + 3, y + 3);
		LineTo(hdc, x + 3, y - 3);
		LineTo(hdc, x - 3, y - 3);
		break;
	case CustomCursorTypeCross:
		MoveToEx(hdc, x - 20, y, 0);
		LineTo(hdc, x + 20, y);
		MoveToEx(hdc, x, y - 20, 0);
		LineTo(hdc, x, y + 20);
		break;
	default:
		assert(0);
		break;
	}
}


void CadLine::Draw(HDC hdc, bool selected) const
{
	HPEN hpen = selected ? g_selectedLineHPen : g_lineHPen;
	if (SelectObject(hdc, hpen) == NULL)
		assert(0);
	if (SetBkColor(hdc, RGB(0, 0, 0)) == CLR_INVALID)
		assert(0);
	Point<int> fromScn = WorldToScreen(Point1.X, Point1.Y);
	Point<int> toScn = WorldToScreen(Point2.X, Point2.Y);
	MoveToEx(hdc, fromScn.X, fromScn.Y, 0);
	LineTo(hdc, toScn.X, toScn.Y);
	LineTo(hdc, toScn.X + 1, toScn.Y);
}


bool CadLine::IntersectsRect(double x1, double y1, double x2, double y2) const
{
	return LineIntersectsRect(Point1.X, Point1.Y, Point2.X, Point2.Y, x1, y1, x2, y2);
}


vector<Point<double> > CadLine::GetManipulators() const
{
	vector<Point<double> > result(2);
	result[0] = Point1;
	result[1] = Point2;
	return result;
}


vector<pair<Point<double>, PointType> > CadLine::GetPoints() const
{
	vector<pair<Point<double>, PointType> > result(3);
	result[0] = make_pair(Point1, PointTypeEndPoint);
	Point<double> middle((Point1.X + Point2.X)/2, (Point1.Y + Point2.Y)/2);
	result[1] = make_pair(middle, PointTypeMiddle);
	result[2] = make_pair(Point2, PointTypeEndPoint);
	return result;
}


Fantom * CadLine::CreateFantom(int param)
{
	int iparam = reinterpret_cast<int>(param);
	FantomLine * result = new FantomLine(iparam, iparam == 1 ? Point1 : Point2);
	return result;
}


void FantomLine::Draw(HDC hdc) const
{
	SelectObject(hdc, g_lineHPen);
	Point<int> startScn = WorldToScreen(m_fixedPt);
	MoveToEx(hdc, startScn.X, startScn.Y, 0);
	LineTo(hdc, g_cursorScn.X, g_cursorScn.Y);
	LineTo(hdc, g_cursorScn.X + 1, g_cursorScn.Y);
}


CadObject * FantomLine::CreateCad() const
{
	CadLine * result = new CadLine();
	AssignToCad(result);
	return result;
}


void FantomLine::AssignToCad(CadObject * to) const
{
	CadLine * line = dynamic_cast<CadLine*>(to);
	assert(line);
	AssignToCad(line);
}


void FantomLine::AssignToCad(CadLine * to) const
{
	Point<double> movingPt = g_cursorWrld;
	to->Point1 = m_movingPtNo == 0 ? movingPt : m_fixedPt;
	to->Point2 = m_movingPtNo == 1 ? movingPt : m_fixedPt;
}


void CadArc::Draw(HDC hdc, bool selected) const
{
	HPEN hpen = selected ? g_selectedLineHPen : g_lineHPen;
	if (SelectObject(hdc, hpen) == NULL)
		assert(0);
	if (SetBkColor(hdc, RGB(0, 0, 0)) == CLR_INVALID)
		assert(0);
	Point<int> center = WorldToScreen(Center);
	Point<int> start = WorldToScreen(Start);
	Point<int> end = WorldToScreen(End);
	int r = static_cast<int>(Radius * g_magification);
	if (!Ccw)
	{
		Point<int> temp = start;
		start = end;
		end = temp;
	}
	MoveToEx(hdc, start.X, start.Y, 0);
	ArcTo(hdc, center.X - r, center.Y - r, center.X + r + 1, center.Y + r + 1, start.X, start.Y, end.X, end.Y);
	LineTo(hdc, end.X + 1, end.Y);
}


bool CadArc::IntersectsRect(double x1, double y1, double x2, double y2) const
{
	return ArcIntersectsRect(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw, x1, y1, x2, y2);
}


vector<Point<double> > CadArc::GetManipulators() const
{
	Point<double> middle;
	CalcMiddlePointOfArc(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw, middle.X, middle.Y);
	vector<Point<double> > result(3);
	result[0] = Start;
	result[1] = middle;
	result[2] = End;
	return result;
}


vector<pair<Point<double>, PointType> > CadArc::GetPoints() const
{
	Point<double> middle;
	CalcMiddlePointOfArc(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw, middle.X, middle.Y);
	vector<pair<Point<double>, PointType> > result(3);
	result[0] = make_pair(Start, PointTypeEndPoint);
	result[1] = make_pair(middle, PointTypeMiddle);
	result[2] = make_pair(End, PointTypeEndPoint);
	return result;
}


Fantom * CadArc::CreateFantom(int param)
{
	Point<double> middle;
	CalcMiddlePointOfArc(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw, middle.X, middle.Y);
	FantomArc * result = new FantomArc(param, Start, middle, End);
	return result;
}


void FantomArc::Recalc() const
{
	Point<double> movingPt = g_cursorWrld;
	Point<double> p1 = m_first = m_movingPtNo == 0 ? movingPt : m_first;
	Point<double> p2 = m_movingPtNo == 1 ? movingPt : m_second;
	Point<double> p3 = m_third = m_movingPtNo == 2 ? movingPt : m_third;
	double sx1sy1 = p1.X*p1.X + p1.Y*p1.Y;
	double sx2sy2 = p2.X*p2.X + p2.Y*p2.Y;
	double sx3sy3 = p3.X*p3.X + p3.Y*p3.Y;
	double m11[][3] = {
			{p1.X, p1.Y, 1},
			{p2.X, p2.Y, 1},
			{p3.X, p3.Y, 1}};
	double m12[][3] = {
			{sx1sy1, p1.Y, 1},
			{sx2sy2, p2.Y, 1},
			{sx3sy3, p3.Y, 1}};
	double m13[][3] = {
			{sx1sy1, p1.X, 1},
			{sx2sy2, p2.X, 1},
			{sx3sy3, p3.X, 1}};
	double m14[][3] = {
			{sx1sy1, p1.X, p1.Y},
			{sx2sy2, p2.X, p2.Y},
			{sx3sy3, p3.X, p3.Y}};
	double dm11 = det3(m11);
	double dm12 = det3(m12);
	double dm13 = det3(m13);
	double dm14 = det3(m14);
	double cx = +.5f * dm12/dm11;
	double cy = -.5f * dm13/dm11;
	m_center = Point<double>(cx, cy);
	m_radius = sqrt(cx*cx + cy*cy + dm14/dm11);
	m_ccw = dm11 > 0;
}


void FantomArc::Draw(HDC hdc) const
{
	SelectObject(hdc, g_lineHPen);
	Point<int> center = WorldToScreen(m_center);
	Point<int> from = WorldToScreen(m_first);
	Point<int> to = WorldToScreen(m_third);
	int radius = static_cast<int>(m_radius * g_magification);
	if (!m_ccw)
	{
		Point<int> temp = from;
		from = to;
		to = temp;
	}
	Arc(hdc, center.X - radius, center.Y - radius, center.X + radius + 1, center.Y + radius + 1, from.X, from.Y, to.X, to.Y);
}


CadObject * FantomArc::CreateCad() const
{
	CadArc * result = new CadArc;
	AssignToCad(result);
	return result;
}


void FantomArc::AssignToCad(CadObject * to) const
{
	CadArc * arc = dynamic_cast<CadArc*>(to);
	AssignToCad(arc);
}


void FantomArc::AssignToCad(CadArc * to) const
{
	Recalc();
	to->Center = m_center;
	to->Start = m_first;
	to->End = m_third;
	to->Radius = m_radius;
	to->Ccw = m_ccw;
}


void ExtendHScrollLimits(SCROLLINFO & si)
{
	if (g_hscrollPos < g_hscrollMin)
	{
		si.fMask |= SIF_RANGE;
		g_hscrollMin = g_hscrollPos;
	}
	else if (g_hscrollPos > g_hscrollMin && g_hscrollMin < g_hrangeMin)
	{
		si.fMask |= SIF_RANGE;
		g_hscrollMin = min(g_hrangeMin, g_hscrollPos);
	}
	if (g_hscrollPos > g_hscrollMax - g_viewWidth + 1)
	{
		si.fMask |= SIF_RANGE;
		g_hscrollMax = g_hscrollPos + g_viewWidth - 1;
	}
	else if (g_hscrollPos < g_hscrollMax - g_viewWidth + 1 && g_hscrollMax > g_hrangeMax)
	{
		si.fMask |= SIF_RANGE;
		g_hscrollMax = max(g_hscrollPos + g_viewWidth - 1, g_hrangeMax);
	}
	if (si.fMask & SIF_RANGE)
	{
		si.nMin = g_hscrollMin;
		si.nMax = g_hscrollMax;
	}
}


void ExtendVScrollLimits(SCROLLINFO & si)
{
	if (g_vscrollPos < g_vscrollMin)
	{
		si.fMask |= SIF_RANGE;
		g_vscrollMin = g_vscrollPos;
	}
	else if (g_vscrollPos > g_vscrollMin && g_vscrollMin < g_vrangeMin)
	{
		si.fMask |= SIF_RANGE;
		g_vscrollMin = min(g_vrangeMin, g_vscrollPos);
	}
	if (g_vscrollPos > g_vscrollMax - g_viewHeight + 1)
	{
		si.fMask |= SIF_RANGE;
		g_vscrollMax = g_vscrollPos + g_viewHeight - 1;
	}
	else if (g_vscrollPos < g_vscrollMax - g_viewHeight + 1 && g_vscrollMax > g_vrangeMax)
	{
		si.fMask |= SIF_RANGE;
		g_vscrollMax = max(g_vscrollPos + g_viewHeight - 1, g_vrangeMax);
	}
	if (si.fMask & SIF_RANGE)
	{
		si.nMin = g_vscrollMin;
		si.nMax = g_vscrollMax;
	}
}


void SelectTool(int toolId)
{
	if (g_checkedMenu == toolId)
		return;
	HMENU hmenu = GetMenu(g_hmainWindow);

	if (g_checkedMenu != 0)
	{
		const ToolInfo & toolInfo = ToolById(g_checkedMenu);
		if (toolInfo.HasMenuItem)
		{
			MENUITEMINFOW iteminfo = {0};
			iteminfo.cbSize = sizeof(iteminfo);
			iteminfo.fMask = MIIM_STATE | MIIM_FTYPE;
			iteminfo.fType = MFT_STRING | MFT_RADIOCHECK;
			iteminfo.fState = MFS_ENABLED;
			if (!SetMenuItemInfoW(hmenu, g_checkedMenu, false, &iteminfo))
				assert(0);
		}

		if (toolInfo.HasToolbarButton)
		{
			if (!SendMessageW(g_htoolbar, TB_CHECKBUTTON, g_checkedMenu, 0))
				assert(0);
		}
	}

	const ToolInfo & toolInfo = ToolById(toolId);
	if (toolInfo.HasMenuItem)
	{
		MENUITEMINFOW iteminfo = {0};
		iteminfo.cbSize = sizeof(iteminfo);
		iteminfo.fMask = MIIM_STATE | MIIM_FTYPE;
		iteminfo.fType = MFT_STRING | MFT_RADIOCHECK;
		iteminfo.fState = MFS_CHECKED | MFS_ENABLED;
		if (!SetMenuItemInfoW(hmenu, toolId, false, &iteminfo))
			assert(0);
	}
	if (toolInfo.HasToolbarButton)
	{
		if (!SendMessageW(g_htoolbar, TB_CHECKBUTTON, toolId, 1))
			assert(0);
	}
	g_curTool = &toolInfo.Tool;
	switch (toolId)
	{
	case ID_VIEW_SELECT:
		g_cursorType = CursorTypeManual;
		g_cursorHandle = 0;
		g_customCursorType = CustomCursorTypeSelect;
		break;
	case ID_VIEW_PAN:
		g_cursorType = CursorTypeSystem;
		g_cursorHandle = LoadCursorW(g_hInstance, MAKEINTRESOURCEW(IDC_PAN));
		break;
	case ID_VIEW_ZOOM:
		g_cursorType = CursorTypeSystem;
		g_cursorHandle = LoadCursorW(g_hInstance, MAKEINTRESOURCEW(IDC_ZOOM));
		break;
	case ID_DRAW_LINES:
		g_cursorType = CursorTypeManual;
		g_cursorHandle = 0;
		g_customCursorType = CustomCursorTypeCross;
		break;
	case ID_DRAW_ARCS:
		g_cursorType = CursorTypeManual;
		g_cursorHandle = 0;
		g_customCursorType = CustomCursorTypeCross;
		g_drawArcsTool.Start();
		break;
	default:
		assert(0);
		break;
	}
	g_checkedMenu = toolId;
}


void SelectorTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		switch (m_state)
		{
		case Selecting:
		{
			Point<double> leftbot = ScreenToWorld(g_cursorScn.X - 3, g_cursorScn.Y + 3);
			Point<double> righttop = ScreenToWorld(g_cursorScn.X + 3, g_cursorScn.Y - 3);
			if (GetKeyState(VK_SHIFT) & 0x8000)
			{
				// removing selection
				for (list<CadObject *>::reverse_iterator i = m_selected.rbegin();
					i != m_selected.rend(); i++)
				{
					if ((*i)->IntersectsRect(leftbot.X, leftbot.Y, righttop.X, righttop.Y))
					{
						RemoveManipulators(*i);
						m_selected.erase((++i).base());
						InvalidateRect(hwnd, 0, true);
						UpdateWindow(hwnd);
						break;
					}
				}
			}
			else
			{
				// begin moving manipulator if clicked in it
				// selecting closest manipulator
				Point<double> cursorPos = g_cursorWrld;
				double range;
				bool found = false;
				for (list<Manipulator>::iterator i = m_manipulators.begin();
					i != m_manipulators.end(); i++)
				{
					Point<int> posScn = WorldToScreen(i->Position);
					if (posScn.X - MANIP_SIZE/2 <= GET_X_LPARAM(lparam) && GET_X_LPARAM(lparam) <= posScn.X + MANIP_SIZE/2 &&
							posScn.Y - MANIP_SIZE/2 <= GET_Y_LPARAM(lparam) && GET_Y_LPARAM(lparam) <= posScn.Y + MANIP_SIZE/2)
					{
						double dx = cursorPos.X - i->Position.X;
						double dy = cursorPos.Y - i->Position.Y;
						double newRange = sqrt(dx*dx + dy*dy);
						if (!found || newRange < range)
						{
							found = true;
							range = newRange;
							m_selManip = i;
						}
					}
				}
				if (found)
				{
					ClientDC hdc(hwnd);
					Point<int> posScn = WorldToScreen(m_selManip->Position);
					RECT rect = {posScn.X - MANIP_SIZE/2, posScn.Y - MANIP_SIZE/2, posScn.X + MANIP_SIZE/2, posScn.Y + MANIP_SIZE/2};
					if (g_cursorDrawn)
						DrawCursor(hdc);
					FillRect(hdc, &rect, g_selectedManipHBrush);
					for (list<pair<CadObject*, int> >::iterator i = m_selManip->Links.begin();
						i != m_selManip->Links.end(); i++)
					{
						g_fantoms.push_back(i->first->CreateFantom(i->second));
						m_manipulated.push_back(i->first);
					}
					g_fantoms.push_back(new FantomLine(m_selManip->Position));
					DrawFantoms(hdc);
					g_customCursorType = CustomCursorTypeCross;
					if (g_cursorDrawn)
						DrawCursor(hdc);
					m_state = MovingManip;
					break;
				}

				// selecting single cad object, if clicked on it
				for (list<CadObject *>::reverse_iterator i = g_doc.Objects.rbegin();
					i != g_doc.Objects.rend(); i++)
				{
					if (IsSelected(*i))
						continue;
					if ((*i)->IntersectsRect(leftbot.X, leftbot.Y, righttop.X, righttop.Y))
					{
						m_selected.push_back(*i);
						AddManipulators(*i);
						InvalidateRect(hwnd, 0, true);
						UpdateWindow(hwnd);
						break;
					}
				}
			}
		}
			break;
		case MovingManip:
			list<Fantom*>::iterator ifan = g_fantoms.begin();
			list<CadObject*>::iterator i = m_manipulated.begin();
			for (; i != m_manipulated.end(); i++, ifan++)
			{
				list<CadObject*>::iterator pos = find(g_doc.Objects.begin(), g_doc.Objects.end(), *i);
				assert(pos != g_doc.Objects.end());
				RemoveManipulators(*pos);
				(*ifan)->AssignToCad(*pos);
				AddManipulators(*pos);
			}
			m_manipulated.clear();
			DeleteFantoms(false);
			g_customCursorType = CustomCursorTypeSelect;
			m_state = Selecting;
			InvalidateRect(hwnd, 0, true);
			UpdateWindow(hwnd);
			break;
		}
		break;
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_ESCAPE:
			switch (m_state)
			{
			case Selecting:
				if (m_selected.size() != 0)
				{
					m_manipulators.clear();
					m_selected.clear();
					InvalidateRect(hwnd, 0, true);
				}
				break;
			case MovingManip:
				m_manipulated.clear();
				DeleteFantoms(false);
				g_customCursorType = CustomCursorTypeSelect;
				m_state = Selecting;
				InvalidateRect(hwnd, 0, true);
				break;
			}
			break;
		case VK_DELETE:
			switch (m_state)
			{
			case Selecting:
			{
				bool needRefresh = false;
				while (m_selected.size() != 0)
				{
					needRefresh = true;
					bool found = false;
					list<CadObject *>::iterator i = m_selected.begin();
					for (list<CadObject *>::iterator j = g_doc.Objects.begin();
						j != g_doc.Objects.end(); j++)
					{
						if (*i == *j)
						{
							CadObject * obj = *i;
							m_selected.erase(i);
							g_doc.Objects.erase(j);
							RemoveManipulators(obj);
							delete obj;
							found = true;
							break;
						}
					}
					assert(found);
				}
				if (needRefresh)
					InvalidateRect(hwnd, 0, true);
			}
				break;
			case MovingManip:
				break;
			}
			break;
		}
		break;
	}
}


void SelectorTool::AddManipulators(CadObject * obj)
{
	vector<Point<double> > manips = obj->GetManipulators();
	size_t ulManipsNum = manips.size();
	assert(ulManipsNum <= INT_MAX);
	int manipsNum = static_cast<int>(ulManipsNum);
	for (int i = 0; i < manipsNum; i++)
	{
		list<Manipulator>::iterator pmanip = m_manipulators.begin();
		for (; pmanip != m_manipulators.end(); pmanip++)
		{
			if (pmanip->Position == manips[i])
				break;
		}
		if (pmanip == m_manipulators.end())
		{
			Manipulator manip;
			manip.Position = manips[i];
			manip.Links.push_back(make_pair(obj, i));
			m_manipulators.push_back(manip);
		}
		else
		{
			pmanip->Links.push_back(make_pair(obj, i));
		}
	}
}


void SelectorTool::RemoveManipulators(CadObject * obj)
{
	for (list<Manipulator>::iterator i = m_manipulators.begin();
		i != m_manipulators.end();)
	{
		for (list<pair<CadObject*, int> >::iterator j = i->Links.begin();
			j != i->Links.end();)
		{
			if (j->first == obj)
				j = i->Links.erase(j);
			else
				j++;
		}
		if (i->Links.size() == 0)
			i = m_manipulators.erase(i);
		else
			i++;
	}
}


void SelectorTool::DrawManipulators(HDC hdc)
{
	for (list<Manipulator>::const_iterator i = m_manipulators.begin();
		i != m_manipulators.end(); i++)
	{
		Point<int> pos = WorldToScreen(i->Position);
		RECT rect = {pos.X - MANIP_SIZE/2, pos.Y - MANIP_SIZE/2, pos.X + MANIP_SIZE/2, pos.Y + MANIP_SIZE/2};
		FillRect(hdc, &rect, g_manipHBrush);
	}
}


void Zoom(float prevMag, float deltaMag, int hscrollPos, int vscrollPos, int x, int y)
{
	g_magification = prevMag + deltaMag;
	g_hscrollPos = static_cast<int>((hscrollPos + x) / prevMag * g_magification) - x;
	g_vscrollPos = static_cast<int>((vscrollPos + y) / prevMag * g_magification) - y;

	g_hrangeMin = static_cast<int>(g_extentMin.X * g_magification);
	g_hrangeMax = static_cast<int>(g_extentMax.X * g_magification);
	g_vrangeMin = static_cast<int>(g_extentMin.Y * g_magification);
	g_vrangeMax = static_cast<int>(g_extentMax.Y * g_magification);
	g_hscrollMin = min(g_hrangeMin, g_hscrollPos);
	g_hscrollMax = max(g_hrangeMax, g_hscrollPos + g_viewWidth - 1);
	g_vscrollMin = min(g_vrangeMin, g_vscrollPos);
	g_vscrollMax = max(g_vrangeMax, g_vscrollPos + g_viewHeight - 1);
	{SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nPos = g_hscrollPos;
	si.nMin = g_hscrollMin;
	si.nMax = g_hscrollMax;
	SetScrollInfo(g_hclientWindow, SB_HORZ, &si, true);}
	{SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nPos = g_vscrollPos;
	si.nMin = g_vscrollMin;
	si.nMax = g_vscrollMax;
	SetScrollInfo(g_hclientWindow, SB_VERT, &si, true);}

	InvalidateRect(g_hclientWindow, 0, true);
}


void ZoomTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_MOUSEMOVE:
		if (m_zooming)
		{
			Zoom(m_startMag, -0.005f * (GET_Y_LPARAM(lparam) - m_startY),
				m_startHscroll, m_startVscroll, m_vectorX, m_vectorY);
		}
		break;
	case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        m_startY = GET_Y_LPARAM(lparam);
		m_startMag = g_magification;
		m_startHscroll = g_hscrollPos;
		m_startVscroll = g_vscrollPos;
        m_vectorX = GET_X_LPARAM(lparam);
		m_vectorY = GET_Y_LPARAM(lparam);
        m_zooming = true;
		break;
	case WM_LBUTTONUP:
		SetCapture(0);
		m_zooming = false;
		break;
	}
}


void PanTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		m_prevX = GET_X_LPARAM(lparam);
		m_prevY = GET_Y_LPARAM(lparam);
		m_panning = true;
		break;
	case WM_MOUSEMOVE:
		if (m_panning)
		{
			int x = GET_X_LPARAM(lparam);
			int y = GET_Y_LPARAM(lparam);
			int dx = x - m_prevX;
			int dy = y - m_prevY;
			g_hscrollPos -= dx;
			g_vscrollPos -= dy;
			ScrollWindow(hwnd, dx, dy, 0, 0);
			m_prevX = x;
			m_prevY = y;

			{SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_DISABLENOSCROLL;
			si.nPos = g_hscrollPos;
			ExtendHScrollLimits(si);
			SetScrollInfo(hwnd, SB_HORZ, &si, true);}
			{SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_DISABLENOSCROLL;
			si.nPos = g_vscrollPos;
			ExtendVScrollLimits(si);
			SetScrollInfo(hwnd, SB_VERT, &si, true);}
		}
		break;
	case WM_LBUTTONUP:
		SetCapture(0);
		m_panning = false;
		break;
	}
}


void DrawLinesTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		if (m_selectingSecondPoint)
		{
			try
			{
				ClientDC hdc(hwnd);
				if (g_cursorDrawn)
					DrawCursor(hdc);
				DeleteFantoms(hdc);
				CadObject * line = m_fantomLine->CreateCad();
				g_doc.Objects.push_back(line);
				SetROP2(hdc, R2_COPYPEN);
				line->Draw(hdc, false);
				if (g_cursorDrawn)
					DrawCursor(hdc);
			}
			catch (WindowsError &)
			{
			}
		}
		else
		{
			m_selectingSecondPoint = true;
		}
		m_fantomLine = new FantomLine(g_cursorWrld);
		g_fantoms.push_back(m_fantomLine);
		break;
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_ESCAPE:
			try
			{
				ClientDC hdc(hwnd);
				DeleteFantoms(hdc);
				if (g_cursorDrawn)
					DrawCursor(hdc);
			}
			catch (WindowsError &)
			{
			}
			m_selectingSecondPoint = false;
			SelectTool(ID_VIEW_SELECT);
			if (g_cursorDrawn)
			{
				try
				{
					ClientDC hdc(hwnd);
					DrawCursor(hdc);
				}
				catch (WindowsError &)
				{
				}
			}
			break;
		}
		break;
	}
}


void DrawArcsTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		switch (m_state)
		{
		case StateSelectingFirstPoint:
			m_firstPoint = g_cursorWrld;
			g_fantoms.push_back(new FantomLine(g_cursorWrld));
			m_state = StateSelectingSecondPoint;
			break;
		case StateSelectingSecondPoint:
			DeleteFantoms(true);
			m_fantomArc = new FantomArc(m_firstPoint, g_cursorWrld);
			g_fantoms.push_back(m_fantomArc);
			m_state = StateSelectingThirdPoint;
			break;
		case StateSelectingThirdPoint:
		{
			m_state = StateSelectingFirstPoint;
			ClientDC hdc(hwnd);
			if (g_cursorDrawn)
				DrawCursor(hdc);
			CadObject * arc = m_fantomArc->CreateCad();
			DeleteFantoms(hdc);
			g_doc.Objects.push_back(arc);
			SetROP2(hdc, R2_COPYPEN);
			arc->Draw(hdc, false);
			if (g_cursorDrawn)
				DrawCursor(hdc);
		}
			break;
		}
		break;
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_ESCAPE:
			try
			{
				ClientDC hdc(hwnd);
				DeleteFantoms(hdc);
				if (g_cursorDrawn)
					DrawCursor(hdc);
			}
			catch (WindowsError &)
			{
			}
			SelectTool(ID_VIEW_SELECT);
			if (g_cursorDrawn)
			{
				try
				{
					ClientDC hdc(hwnd);
					DrawCursor(hdc);
				}
				catch (WindowsError &)
				{
				}
			}
			break;
		}
		break;
	}
}


void DrawFantoms(HDC hdc)
{
	SetROP2(hdc, R2_XORPEN);
	for (list<Fantom *>::iterator i = g_fantoms.begin();
		i != g_fantoms.end(); i++)
	{
		(*i)->Draw(hdc);
	}
}


void RecalcFantoms()
{
	for (list<Fantom *>::iterator i = g_fantoms.begin();
		i != g_fantoms.end(); i++)
	{
		(*i)->Recalc();
	}
}


void DeleteFantoms(bool update)
{
	if (g_fantoms.size() == 0)
		return;
	if (update)
	{
		ClientDC hdc(g_hclientWindow);
		DrawFantoms(hdc);
	}
	g_fantoms.clear();
}


void DeleteFantoms(HDC hdc)
{
	DrawFantoms(hdc);
	g_fantoms.clear();
}
