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
#include <loki/functor.h>
#include <loki/typelistmacros.h>
#include <algorithm>
#include <cmath>
#include <cfloat>


using namespace std;
using namespace Loki;


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

Document g_doc;

Selector g_selector;
DefaultTool g_defaultTool;
PanTool g_panTool;
ZoomTool g_zoomTool;
DrawLinesTool g_drawLinesTool;
DrawCircleTool g_drawCircleTool;
DrawArcsTool g_drawArcsTool;
MoveTool g_moveTool;
PasteTool g_pasteTool;
Tool * g_curTool = &g_defaultTool;

FantomManager g_fantomManager;

UndoManager g_undoManager;

bool g_canSnap = false;

std::list<CadObject *> g_selected;

void DrawCursorRaw(HDC hdc, int x, int y)
{
	// TODO: make dpi dependent
	SelectObject(hdc, g_cursorHPen);
	switch (g_customCursorType)
	{
	case CustomCursorTypeSelect:
		MoveToEx(hdc, x - 30, y, 0);
		LineTo(hdc, x + 30, y);
		MoveToEx(hdc, x, y - 30, 0);
		LineTo(hdc, x, y + 30);

		MoveToEx(hdc, x - 3, y - 3, 0);
		LineTo(hdc, x - 3, y + 3);
		LineTo(hdc, x + 3, y + 3);
		LineTo(hdc, x + 3, y - 3);
		LineTo(hdc, x - 3, y - 3);
		break;
	case CustomCursorTypeCross:
		MoveToEx(hdc, x - 30, y, 0);
		LineTo(hdc, x + 30, y);
		MoveToEx(hdc, x, y - 30, 0);
		LineTo(hdc, x, y + 30);
		break;
	case CustomCursorTypeBox:
		MoveToEx(hdc, x - 3, y - 3, 0);
		LineTo(hdc, x - 3, y + 3);
		LineTo(hdc, x + 3, y + 3);
		LineTo(hdc, x + 3, y - 3);
		LineTo(hdc, x - 3, y - 3);
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
	vector<Point<double> > result(3);
	result[0] = Point1;
	result[1] = (Point1 + Point2)/2;
	result[2] = Point2;
	return result;
}


void CadLine::UpdateManip(const Point<double> & pt, int id)
{
	switch (id)
	{
	case 0: Point1 = pt; break;
	case 1: {
		Point<double> displace = pt - (Point1 + Point2)/2;
		Point1 += displace;
		Point2 += displace;
		}
	break;
	case 2: Point2 = pt; break;
	default: assert(0); break;
	}
}


vector<pair<Point<double>, PointType> > CadLine::GetPoints() const
{
	vector<pair<Point<double>, PointType> > result(3);
	result[0] = make_pair(Point1, PointTypeEndPoint);
	result[1] = make_pair((Point1 + Point2)/2, PointTypeMiddle);
	result[2] = make_pair(Point2, PointTypeEndPoint);
	return result;
}


void CadLine::Move(Point<double> displacement)
{
	Point1.X += displacement.X;
	Point1.Y += displacement.Y;
	Point2.X += displacement.X;
	Point2.Y += displacement.Y;
}


CadLine * CadLine::Clone()
{
	return new CadLine(*this);
}


void CadLine::Assign(CadObject * rhs)
{
	CadLine * line = dynamic_cast<CadLine *>(rhs);
	assert(line != 0);
	Assign(line);
}


void CadLine::Assign(CadLine * line)
{
	*this = *line;
}


size_t CadLine::Serialize(unsigned char * ptr) const
{
	size_t result = 0;
	result += WritePtr(ptr, ID);
	result += WritePtr(ptr, Point1.X);
	result += WritePtr(ptr, Point1.Y);
	result += WritePtr(ptr, Point2.X);
	result += WritePtr(ptr, Point2.Y);
	return result;
}


void CadLine::Load(unsigned char const *& ptr, size_t & size)
{
	ReadPtr(ptr, Point1.X, size);
	ReadPtr(ptr, Point1.Y, size);
	ReadPtr(ptr, Point2.X, size);
	ReadPtr(ptr, Point2.Y, size);
}


void CadCircle::Draw(HDC hdc, bool selected) const
{
	HPEN hpen = selected ? g_selectedLineHPen : g_lineHPen;
	if (SelectObject(hdc, hpen) == NULL)
		assert(0);
	if (SetBkColor(hdc, RGB(0, 0, 0)) == CLR_INVALID)
		assert(0);
	Point<int> center = WorldToScreen(Center);
	int r = static_cast<int>(Radius * g_magification);
	if (!Arc(hdc, center.X - r, center.Y - r, center.X + r + 1, center.Y + r + 1, 0, 0, 0, 0))
		assert(0);
}


bool CadCircle::IntersectsRect(double x1, double y1, double x2, double y2) const
{
	Rect<double> brect = GetBoundingRect();
	if (!IsRectsIntersects(Rect<double>(x1, y1, x2, y2), brect))
		return false;
	if (IsLeftContainsRight(Rect<double>(x1, y1, x2, y2), brect))
		return true;
	pair<bool, double> res;
	res = VertLineIntersectsCircle(x1, Center, Radius);
	if (res.first)
	{
		if (y1 <= Center.Y + res.second && Center.Y + res.second <= y2)
			return true;
		else if (res.second != 0 && y1 <= Center.Y - res.second && Center.Y - res.second <= y2)
			return true;
	}
	res = VertLineIntersectsCircle(x2, Center, Radius);
	if (res.first)
	{
		if (y1 <= Center.Y + res.second && Center.Y + res.second <= y2)
			return true;
		else if (res.second != 0 && y1 <= Center.Y - res.second && Center.Y - res.second <= y2)
			return true;
	}
	res = HorzLineIntersectsCircle(y1, Center, Radius);
	if (res.first)
	{
		if (x1 <= Center.X + res.second && Center.X + res.second <= x2)
			return true;
		else if (res.second != 0 && x1 <= Center.X - res.second && Center.X - res.second <= x2)
			return true;
	}
	res = HorzLineIntersectsCircle(y2, Center, Radius);
	if (res.first)
	{
		if (x1 <= Center.X + res.second && Center.X + res.second <= x2)
			return true;
		else if (res.second != 0 && x1 <= Center.X - res.second && Center.X - res.second <= x2)
			return true;
	}
	return false;
}


Rect<double> CadCircle::GetBoundingRect() const
{
	Point<double> rad(Radius, Radius);
	return Rect<double>(Center - rad, Center + rad);
}


vector<Point<double> > CadCircle::GetManipulators() const
{
	Point<double> manips[] = {
			Point<double>(Center.X + Radius, Center.Y),
			Point<double>(Center.X, Center.Y + Radius),
			Point<double>(Center.X - Radius, Center.Y),
			Point<double>(Center.X, Center.Y - Radius),
			Point<double>(Center.X, Center.Y),
	};
	return vector<Point<double> >(manips, manips + sizeof(manips)/sizeof(manips[0]));
}


void CadCircle::UpdateManip(const Point<double> & pt, int id)
{
	switch (id)
	{
	case 0: Radius = (pt - Center).Length(); break;
	case 1: Radius = (pt - Center).Length(); break;
	case 2: Radius = (pt - Center).Length(); break;
	case 3: Radius = (pt - Center).Length(); break;
	case 4: Center = pt; break;
	default: assert(0); break;
	}
}


std::vector<std::pair<Point<double>, PointType> > CadCircle::GetPoints() const
{
	pair<Point<double>, PointType> points[] = {
			pair<Point<double>, PointType>(Point<double>(Center.X + Radius, Center.Y), PointTypeQuadrant),
			pair<Point<double>, PointType>(Point<double>(Center.X, Center.Y + Radius), PointTypeQuadrant),
			pair<Point<double>, PointType>(Point<double>(Center.X - Radius, Center.Y), PointTypeQuadrant),
			pair<Point<double>, PointType>(Point<double>(Center.X, Center.Y - Radius), PointTypeQuadrant),
			pair<Point<double>, PointType>(Point<double>(Center.X, Center.Y), PointTypeCenter),
	};
	return vector<pair<Point<double>, PointType > >(points, points + sizeof(points)/sizeof(points[0]));
}


void CadCircle::Move(Point<double> displacement)
{
	Center += displacement;
}


CadCircle * CadCircle::Clone()
{
	return new CadCircle(*this);
}


void CadCircle::Assign(CadObject * rhs)
{
	CadCircle * rhsCircle = dynamic_cast<CadCircle *>(rhs);
	assert(rhsCircle != 0);
	*this = *rhsCircle;
}


size_t CadCircle::Serialize(unsigned char * ptr) const
{
	size_t result = 0;
	result += WritePtr(ptr, ID);
	result += WritePtr(ptr, Center.X);
	result += WritePtr(ptr, Center.Y);
	result += WritePtr(ptr, Radius);
	return result;
}


void CadCircle::Load(unsigned char const *& ptr, size_t & size)
{
	ReadPtr(ptr, Center.X, size);
	ReadPtr(ptr, Center.Y, size);
	ReadPtr(ptr, Radius, size);
}


void CadArc::Draw(HDC hdc, bool selected) const
{
	HPEN hpen = selected ? g_selectedLineHPen : g_lineHPen;
	if (SelectObject(hdc, hpen) == NULL)
		assert(0);
	if (SetBkColor(hdc, RGB(0, 0, 0)) == CLR_INVALID)
		assert(0);
	if (Radius == 0)
	{
		Point<int> startScn = WorldToScreen(Start);
		Point<int> endScn = WorldToScreen(End);
		MoveToEx(hdc, startScn.X, startScn.Y, 0);
		LineTo(hdc, endScn.X, endScn.Y);
		LineTo(hdc, endScn.X + 1, endScn.Y);
	}
	else
	{
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
		//MoveToEx(hdc, start.X, start.Y, 0);
		Arc(hdc, center.X - r, center.Y - r, center.X + r + 1, center.Y + r + 1, start.X, start.Y, end.X, end.Y);
		//LineTo(hdc, end.X + 1, end.Y);
	}
}


bool CadArc::IntersectsRect(double x1, double y1, double x2, double y2) const
{
	return ArcIntersectsRect(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw, x1, y1, x2, y2);
}


Rect<double> CadArc::GetBoundingRect() const
{
	return ArcsBoundingRect(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw);
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


void CadArc::UpdateManip(const Point<double> & pt, int id)
{
	Point<double> middle;
	CalcMiddlePointOfArc(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw, middle.X, middle.Y);
	switch (id)
	{
	case 0: Start = pt; break;
	case 1: middle = pt; break;
	case 2: End = pt; break;
	default: assert(0); break;
	}
	CircleArc<double> arc = ArcFrom3Pt(Start, middle, End);
	Center = arc.Center;
	Radius = arc.Radius;
}


vector<pair<Point<double>, PointType> > CadArc::GetPoints() const
{
	Point<double> middle;
	CalcMiddlePointOfArc(Center.X, Center.Y, Radius, Start.X, Start.Y, End.X, End.Y, Ccw, middle.X, middle.Y);
	vector<pair<Point<double>, PointType> > result(4);
	result[0] = make_pair(Start, PointTypeEndPoint);
	result[1] = make_pair(middle, PointTypeMiddle);
	result[2] = make_pair(End, PointTypeEndPoint);
	result[3] = make_pair(Center, PointTypeCenter);
	return result;
}


void CadArc::Move(Point<double> displacement)
{
	Center.X += displacement.X;
	Center.Y += displacement.Y;
	Start.X += displacement.X;
	Start.Y += displacement.Y;
	End.X += displacement.X;
	End.Y += displacement.Y;
}


CadArc * CadArc::Clone()
{
	return new CadArc(*this);
}


void CadArc::Assign(CadObject * rhs)
{
	CadArc * arc = dynamic_cast<CadArc *>(rhs);
	assert(arc != 0);
	Assign(arc);
}


void CadArc::Assign(CadArc * arc)
{
	*this = *arc;
}


size_t CadArc::Serialize(unsigned char * ptr) const
{
	size_t result = 0;
	result += WritePtr(ptr, ID);
	result += WritePtr(ptr, Center.X);
	result += WritePtr(ptr, Center.Y);
	result += WritePtr(ptr, Radius);
	result += WritePtr(ptr, Start.X);
	result += WritePtr(ptr, Start.Y);
	result += WritePtr(ptr, End.X);
	result += WritePtr(ptr, End.Y);
	result += WritePtr(ptr, Ccw);
	return result;
}


void CadArc::Load(unsigned char const *& ptr, size_t & size)
{
	ReadPtr(ptr, Center.X, size);
	ReadPtr(ptr, Center.Y, size);
	ReadPtr(ptr, Radius, size);
	ReadPtr(ptr, Start.X, size);
	ReadPtr(ptr, Start.Y, size);
	ReadPtr(ptr, End.X, size);
	ReadPtr(ptr, End.Y, size);
	ReadPtr(ptr, Ccw, size);
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
	const ToolInfo & toolInfo = ToolById(toolId);
	list<CadObject *> selected;
	if (g_curTool == &g_defaultTool)
		selected = g_selected;
	g_curTool->Exiting();
	g_curTool = &toolInfo.Tool;
	g_curTool->Start(selected);
	InvalidateRect(g_hclientWindow, 0, true);
	UpdateWindow(g_hclientWindow);
}


void ExitTool()
{
	g_fantomManager.DeleteFantoms(false);
	g_fantomManager.RecalcFantomsHandler = Functor<void>();
	g_curTool->Exiting();
	g_curTool = &g_defaultTool;
	g_curTool->Start(list<CadObject*>());
	InvalidateRect(g_hclientWindow, 0, true);
	UpdateWindow(g_hclientWindow);
}


bool IsSelected(const CadObject * obj)
{
	for (std::list<CadObject *>::const_iterator i = g_selected.begin();
		i != g_selected.end(); i++)
	{
		if (*i == obj)
			return true;
	}
	return false;
}


void MyRectangle(HDC hdc, Point<int> pt1, Point<int> pt2)
{
	int left = min(pt1.X, pt2.X);
	int top = min(pt1.Y, pt2.Y);
	int right = max(pt1.X, pt2.X) + 1;
	int bottom = max(pt1.Y, pt2.Y) + 1;
	if (!Rectangle(hdc, left, top, right, bottom))
		assert(0);
}


void Selector::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		m_lassoPt2 = m_lassoPt1 = ScreenToWorld(g_cursorScn.X, g_cursorScn.Y);
		m_lassoOn = true;
		break;
	case WM_MOUSEMOVE:
		if (m_lassoOn)
		{
			ClientDC hdc(hwnd);
			SelectObject(hdc, GetStockObject(NULL_BRUSH));
			if (m_lassoPt2.X < m_lassoPt1.X)
				SelectObject(hdc, g_selectedLineHPen);
			else
				SelectObject(hdc, g_lineHPen);
			SetROP2(hdc, R2_XORPEN);
			Point<int> scnPt1 = WorldToScreen(m_lassoPt1);
			Point<int> scnPt2 = WorldToScreen(m_lassoPt2);
			if (m_lassoDrawn)
				MyRectangle(hdc, scnPt1, scnPt2);
			m_lassoPt2 = ScreenToWorld(g_cursorScn.X, g_cursorScn.Y);
			scnPt2 = g_cursorScn;
			if (m_lassoPt2.X < m_lassoPt1.X)
				SelectObject(hdc, g_selectedLineHPen);
			else
				SelectObject(hdc, g_lineHPen);
			MyRectangle(hdc, scnPt1, scnPt2);
			m_lassoDrawn = true;
		}
		break;
	case WM_LBUTTONUP:
		Rect<double> testRect;
		bool multiselect;
		bool intersect;
		if (m_lassoOn && m_lassoPt1 != m_lassoPt2)
		{
			testRect = Rect<double>(m_lassoPt1, m_lassoPt2).Normalized();
			multiselect = true;
			intersect = m_lassoPt1.X > m_lassoPt2.X;
		}
		else
		{
			testRect.Pt1 = ScreenToWorld(g_cursorScn.X - 3, g_cursorScn.Y + 3);
			testRect.Pt2 = ScreenToWorld(g_cursorScn.X + 3, g_cursorScn.Y - 3);
			multiselect = false;
			intersect = true;
		}
		m_lassoOn = false;
		if (m_lassoDrawn)
		{
			m_lassoDrawn = false;
			InvalidateRect(hwnd, 0, true);
		}
		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			// removing selection
			for (list<CadObject *>::iterator i = g_selected.end();
				i != g_selected.begin();)
			{
				i--;
				bool good;
				if (intersect)
					good = (*i)->IntersectsRect(testRect);
				else
					good = IsLeftContainsRight(testRect, (*i)->GetBoundingRect());
				if (good)
				{
					if (SelectHandler)
						SelectHandler(*i, false);
					i = g_selected.erase(i);
					InvalidateRect(hwnd, 0, true);
					if (!multiselect)
						break;
				}
			}
		}
		else
		{
			// selecting single cad object, if clicked on it
			for (list<CadObject *>::reverse_iterator i = g_doc.Objects.rbegin();
				i != g_doc.Objects.rend(); i++)
			{
				if (IsSelected(*i))
					continue;
				bool good;
				if (intersect)
					good = (*i)->IntersectsRect(testRect);
				else
					good = IsLeftContainsRight(testRect, (*i)->GetBoundingRect());
				if (good)
				{
					g_selected.push_back(*i);
					if (SelectHandler != 0)
						SelectHandler(*i, true);
					InvalidateRect(hwnd, 0, true);
					if (!multiselect)
						break;
				}
			}
		}
		break;
	}
}


void Selector::Cancel()
{
	m_lassoOn = false;
	if (m_lassoDrawn)
	{
		m_lassoDrawn = false;
		InvalidateRect(g_hclientWindow, 0, true);
	}
	if (g_selected.size() != 0)
		g_selected.clear();
}

void DefaultTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (m_state)
	{
	case Selecting:
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			do
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
						Manipulated manip;
						manip.Original = i->first;
						manip.Copy = manip.Original->Clone();
						manip.ManipId = i->second;
						m_manipulated.push_back(manip);
						g_fantomManager.AddFantom(manip.Copy);
					}
					m_fantomLine.reset(new CadLine());
					m_fantomLine->Point1 = m_fantomLine->Point2 = m_selManip->Position;
					g_fantomManager.AddFantom(m_fantomLine.get());
					g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DefaultTool::RecalcFantomsHandler);
					g_fantomManager.DrawFantoms(hdc);
					g_customCursorType = CustomCursorTypeCross;
					if (g_cursorDrawn)
						DrawCursor(hdc);
					m_state = MovingManip;
					g_canSnap = true;
					break;
				}
				else
				{
					g_selector.ProcessInput(hwnd, msg, wparam, lparam);
				}
			}
			while (false);
			break;
		case WM_KEYDOWN:
			switch (wparam)
			{
			case VK_DELETE:
				DeleteSelectedObjects();
				break;
			}
			break;
		default:
			g_selector.ProcessInput(hwnd, msg, wparam, lparam);
			break;
		}
		break;
	case MovingManip:
		switch (msg)
		{
		case WM_LBUTTONDOWN:
		{
			auto_ptr<GroupUndoItem> group(new GroupUndoItem);
			for (list<Manipulated>::iterator i = m_manipulated.begin();
				i != m_manipulated.end(); i++)
			{
				group->AddItem(new AssignObjectUndoItem(i->Original, i->Copy));
				i->Copy = 0;
				RemoveManipulators(i->Original);
			}
			g_undoManager.AddWork(group.release());
			for (list<Manipulated>::iterator i = m_manipulated.begin();
				i != m_manipulated.end(); i++)
			{
				AddManipulators(i->Original);
			}
			ClearManipulated();
			g_fantomManager.DeleteFantoms(false);
			g_customCursorType = CustomCursorTypeSelect;
			g_canSnap = false;
			m_state = Selecting;
			InvalidateRect(hwnd, 0, true);
			UpdateWindow(hwnd);
		}
			break;
		}
		break;
	}
}


void DefaultTool::Start(const std::list<CadObject *> & /*selected*/)
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeSelect;
	Functor<void, LOKI_TYPELIST_2(CadObject*,bool)> handler(this, &DefaultTool::SelectHandler);
	g_selector.SelectHandler = handler;
	g_canSnap = false;
}


void DefaultTool::Cancel()
{
	switch (m_state)
	{
	case Selecting:
		g_selector.Cancel();
		m_manipulators.clear();
		break;
	case MovingManip:
		ClearManipulated();
		g_customCursorType = CustomCursorTypeSelect;
		m_state = Selecting;
		g_canSnap = false;
		break;
	}
}


void DefaultTool::Exiting()
{
	switch (m_state)
	{
	case MovingManip:
		ClearManipulated();
		g_customCursorType = CustomCursorTypeSelect;
		m_state = Selecting;
		g_canSnap = false;
	case Selecting:
		if (g_selected.size() != 0)
		{
			m_manipulators.clear();
			g_selected.clear();
		}
		break;
	}
}


void DefaultTool::SelectHandler(CadObject * obj, bool select)
{
	if (select)
		AddManipulators(obj);
	else
		RemoveManipulators(obj);
}


void DefaultTool::AddManipulators(CadObject * obj)
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


void DefaultTool::RemoveManipulators(CadObject * obj)
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


void DefaultTool::DrawManipulators(HDC hdc)
{
	for (list<Manipulator>::const_iterator i = m_manipulators.begin();
		i != m_manipulators.end(); i++)
	{
		Point<int> pos = WorldToScreen(i->Position);
		RECT rect = {pos.X - MANIP_SIZE/2, pos.Y - MANIP_SIZE/2, pos.X + MANIP_SIZE/2, pos.Y + MANIP_SIZE/2};
		FillRect(hdc, &rect, g_manipHBrush);
	}
}


void DefaultTool::RecalcFantomsHandler()
{
	if (m_fantomLine.get())
		m_fantomLine->Point2 = g_cursorWrld;
	for (list<Manipulated>::const_iterator i = m_manipulated.begin();
		i != m_manipulated.end(); i++)
	{
		i->Copy->UpdateManip(g_cursorWrld, i->ManipId);
	}
}


void DefaultTool::ClearManipulated()
{
	for (list<Manipulated>::const_iterator i = m_manipulated.begin();
		i != m_manipulated.end(); i++)
	{
		delete i->Copy;
	}
	m_manipulated.clear();
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


void ZoomTool::Start(const std::list<CadObject *> & /*selected*/)
{
	g_cursorType = CursorTypeSystem;
	g_cursorHandle = LoadCursorW(g_hInstance, MAKEINTRESOURCEW(IDC_ZOOM));
	g_canSnap = false;
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


void PanTool::Start(const std::list<CadObject *> & /*selected*/)
{
	g_cursorType = CursorTypeSystem;
	g_cursorHandle = LoadCursorW(g_hInstance, MAKEINTRESOURCEW(IDC_PAN));
	g_canSnap = false;
}


void DrawLinesTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		if (m_selectingSecondPoint)
		{
			g_fantomManager.DeleteFantoms(false);
			g_undoManager.AddWork(new AddObjectUndoItem(m_fantomLine.release()));
		}
		else
		{
			m_selectingSecondPoint = true;
		}
		m_fantomLine.reset(new CadLine());
		m_fantomLine->Point1 = m_fantomLine->Point2 = g_cursorWrld;
		g_fantomManager.AddFantom(m_fantomLine.get());
		g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawLinesTool::RecalcFantomsHandler);
		InvalidateRect(hwnd, 0, true);
		break;
	}
}


void DrawLinesTool::RecalcFantomsHandler()
{
	m_fantomLine->Point2 = g_cursorWrld;
}



void DrawLinesTool::Start(const std::list<CadObject *> & /*selected*/)
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	m_selectingSecondPoint = false;
	g_canSnap = true;
}


void DrawCircleTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (m_state)
	{
	case StateSelectingCenter:
		switch (msg)
		{
		case WM_LBUTTONUP:
			m_cadCircle.reset(new CadCircle());
			m_cadCircle->Center = g_cursorWrld;
			m_cadCircle->Radius = 0;
			m_radiusLine.reset(new CadLine());
			m_radiusLine->Point1 = m_radiusLine->Point2 = g_cursorWrld;
			m_state = StateSelectingRadius;
			g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawCircleTool::RecalcFantomsHandler);
			g_fantomManager.AddFantom(m_cadCircle.get());
			g_fantomManager.AddFantom(m_radiusLine.get());
			break;
		}
		break;
	case StateSelectingRadius:
		switch (msg)
		{
		case WM_LBUTTONUP:
			m_radiusLine.reset(0);
			g_undoManager.AddWork(new AddObjectUndoItem(m_cadCircle.release()));
			ExitTool();
			break;
		}
		break;
	}
}


void DrawCircleTool::RecalcFantomsHandler()
{
	m_cadCircle->Radius = (g_cursorWrld - m_cadCircle->Center).Length();
	m_radiusLine->Point2 = g_cursorWrld;
}


void DrawCircleTool::Start(const std::list<CadObject *> & selected)
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	m_state = StateSelectingCenter;
	g_canSnap = true;
}


void DrawArcsTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		switch (m_state)
		{
		case StateSelectingFirstPoint:
			m_fantomLine.reset(new CadLine());
			m_fantomLine->Point1 = m_fantomLine->Point2 = g_cursorWrld;
			g_fantomManager.AddFantom(m_fantomLine.get());
			g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawArcsTool::RecalcFantomsHandler);
			m_state = StateSelectingSecondPoint;
			InvalidateRect(hwnd, 0, true);
			break;
		case StateSelectingSecondPoint:
			g_fantomManager.DeleteFantoms(false);
			m_fantomArc.reset(new CadArc());
			m_fantomArc->Start = m_fantomLine->Point1;
			m_fantomArc->End = m_secondPoint = m_fantomLine->Point2;
			m_fantomArc->Radius = 0;
			m_fantomLine.reset(0);
			g_fantomManager.AddFantom(m_fantomArc.get());
			g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawArcsTool::RecalcFantomsHandler);
			m_state = StateSelectingThirdPoint;
			InvalidateRect(hwnd, 0, true);
			break;
		case StateSelectingThirdPoint:
			g_undoManager.AddWork(new AddObjectUndoItem(m_fantomArc.release()));
			ExitTool();
			break;
		}
		break;
	}
}


void DrawArcsTool::Start(const std::list<CadObject *> & /*selected*/)
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	m_state = StateSelectingFirstPoint;
	g_canSnap = true;
}


void DrawArcsTool::RecalcFantomsHandler()
{
	if (m_fantomLine.get())
		m_fantomLine->Point2 = g_cursorWrld;
	if (m_fantomArc.get())
	{
		CircleArc<double> arc = ArcFrom3Pt(m_fantomArc->Start, m_secondPoint, g_cursorWrld);
		m_fantomArc->Center = arc.Center;
		m_fantomArc->End = g_cursorWrld;
		m_fantomArc->Ccw = arc.Ccw;
		m_fantomArc->Radius = arc.Radius;
	}
}


void MoveTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (m_state)
	{
	case StateSelecting:
		switch (msg)
		{
		case WM_KEYDOWN:
			switch (wparam)
			{
			case VK_RETURN:
				m_state = StateChoosingBasePoint;
				do
				{
					ClientDC hdc(hwnd);
					if (g_cursorDrawn)
						DrawCursor(hdc);
					g_customCursorType = CustomCursorTypeCross;
					if (g_cursorDrawn)
						DrawCursor(hdc);
				}
				while (false);
				g_canSnap = true;
				break;
			default:
				g_selector.ProcessInput(hwnd, msg, wparam, lparam);
				break;
			}
			break;
		default:
			g_selector.ProcessInput(hwnd, msg, wparam, lparam);
			break;
		}
		break;
	case StateChoosingBasePoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			m_basePoint = g_cursorWrld;
			g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &MoveTool::RecalcFantomsHandler);
			m_fantomLine.reset(new CadLine());
			m_fantomLine->Point1 = m_fantomLine->Point2 = m_basePoint;
			g_fantomManager.AddFantom(m_fantomLine.get());
			m_state = StateChoosingDestPoint;
			DeleteCopies();
			m_objects.resize(g_selected.size());
			do
			{
				int no = 0;
				for (list<CadObject*>::iterator i = g_selected.begin();
					i != g_selected.end(); i++, no++)
				{
					m_objects[no] = (*i)->Clone();
					g_fantomManager.AddFantom(m_objects[no]);
				}
			}
			while (false);
			break;
		}
		break;
	case StateChoosingDestPoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			do
			{
				auto_ptr<GroupUndoItem> group(new GroupUndoItem);
				int no = 0;
				for (list<CadObject*>::iterator i = g_selected.begin();
					i != g_selected.end(); i++, no++)
				{
					group->AddItem(new AssignObjectUndoItem(*i, m_objects[no]));
					m_objects[no] = 0;
				}
				g_undoManager.AddWork(group.release());
				ExitTool();
			}
			while (false);
			break;
		}
		break;
	default:
		assert(0);
		break;
	}
}


void MoveTool::Start(const std::list<CadObject *> & selected)
{
	if (selected.size() == 0)
	{
		m_state = StateSelecting;
		g_cursorType = CursorTypeManual;
		g_cursorHandle = 0;
		g_customCursorType = CustomCursorTypeBox;
		g_selector.SelectHandler = Functor<void, LOKI_TYPELIST_2(CadObject*,bool)>();
		g_canSnap = false;
	}
	else
	{
		g_selected = selected;
		m_state = StateChoosingBasePoint;
		g_cursorType = CursorTypeManual;
		g_cursorHandle = 0;
		g_customCursorType = CustomCursorTypeCross;
		g_selector.SelectHandler = Functor<void, LOKI_TYPELIST_2(CadObject*,bool)>();
		g_canSnap = true;
	}
}


void MoveTool::Cancel()
{
	Exiting();
}


void MoveTool::Exiting()
{
	m_fantomLine.reset(0);
	DeleteCopies();
	g_selector.Cancel();
}


void MoveTool::DeleteCopies()
{
	for (vector<CadObject*>::iterator i = m_objects.begin();
		i != m_objects.end(); i++)
	{
		delete *i;
	}
	m_objects.resize(0);
}


void MoveTool::RecalcFantomsHandler()
{
	for (vector<CadObject*>::iterator i = m_objects.begin();
		i != m_objects.end(); i++)
	{
		(*i)->Move(g_cursorWrld - m_basePoint);
	}
	m_basePoint = g_cursorWrld;
}


void PasteTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		do
		{
			auto_ptr<GroupUndoItem> group(new GroupUndoItem);
			for (vector<CadObject*>::iterator i = m_objects.begin();
				i != m_objects.end(); i++)
			{
				group->AddItem(new AddObjectUndoItem(*i));
				*i = 0;
			}
			g_undoManager.AddWork(group.release());
			ExitTool();
		}
		while (false);
		break;
	}
}


void PasteTool::Start(const std::list<CadObject *> & /*selected*/)
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	g_canSnap = true;
	if (!OpenClipboard(g_hmainWindow))
		assert(0);
	HGLOBAL hglob = GetClipboardData(g_clipboardFormat);
	if (hglob != 0)
	{
		size_t size = GlobalSize(hglob);
		assert(size != 0);
		unsigned char const * ptr = reinterpret_cast<unsigned char *>(GlobalLock(hglob));
		m_basePoint = Point<double>(DBL_MAX, DBL_MAX);
		while (size != 0)
		{
			int id;
			ReadPtr(ptr, id, size);
			CadObject * obj;
			switch (id)
			{
			case CadLine::ID: obj = new CadLine(); break;
			case CadCircle::ID: obj = new CadCircle(); break;
			case CadArc::ID: obj = new CadArc(); break;
			default: assert(0); break;
			}
			obj->Load(ptr, size);
			m_objects.push_back(obj);
			g_fantomManager.AddFantom(obj);
			m_basePoint = Point<double>(min(m_basePoint.X, obj->GetBoundingRect().Pt1.X),
					min(m_basePoint.Y, obj->GetBoundingRect().Pt1.Y));
		}
		g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &PasteTool::RecalcFantomsHandler);
		// aligning base point of objects with cursor
		RecalcFantomsHandler();
	}
	else
	{
		ExitTool();
	}
	if (!CloseClipboard())
		assert(0);
}


void PasteTool::Cancel()
{
	Exiting();
}


void PasteTool::Exiting()
{
	DeleteCopies();
}


void PasteTool::DeleteCopies()
{
	for (vector<CadObject*>::iterator i = m_objects.begin();
		i != m_objects.end(); i++)
	{
		delete *i;
	}
	m_objects.resize(0);
}


void PasteTool::RecalcFantomsHandler()
{
	for (vector<CadObject*>::iterator i = m_objects.begin();
		i != m_objects.end(); i++)
	{
		(*i)->Move(g_cursorWrld - m_basePoint);
	}
	m_basePoint = g_cursorWrld;
}


Point<int> WorldToScreen(float x, float y)
{
	Point<int> result;
	float scnx = (x - g_extentMin.X) * g_magification + 0.5f;
	float scny = (y - g_extentMax.Y) * -g_magification + 0.5f;
	assert(INT_MIN <= scnx - g_hscrollPos && scnx - g_hscrollPos <= INT_MAX);
	assert(INT_MIN <= scny - g_vscrollPos && scny - g_vscrollPos <= INT_MAX);
	result.X = static_cast<int>(scnx) - g_hscrollPos;
	result.Y = static_cast<int>(scny) - g_vscrollPos;
	return result;
}


void FantomManager::DrawFantoms(HDC hdc)
{
	SetROP2(hdc, R2_XORPEN);
	for (list<CadObject *>::const_iterator i = m_fantoms.begin();
		i != m_fantoms.end(); i++)
	{
		(*i)->Draw(hdc, false);
	}
}


void FantomManager::RecalcFantoms()
{
	if (RecalcFantomsHandler)
		RecalcFantomsHandler();
}


void FantomManager::DeleteFantoms(bool update)
{
	if (m_fantoms.size() == 0)
		return;
	if (update)
	{
		ClientDC hdc(g_hclientWindow);
		DrawFantoms(hdc);
	}
	m_fantoms.clear();
}


void FantomManager::DeleteFantoms(HDC hdc)
{
	DrawFantoms(hdc);
	m_fantoms.clear();
}


GroupUndoItem::~GroupUndoItem()
{
	for (Items::iterator i = m_items.begin(); i != m_items.end(); i++)
		delete *i;
}

void GroupUndoItem::Do()
{
	for (Items::iterator i = m_items.begin(); i != m_items.end(); i++)
		(*i)->Do();
}

void GroupUndoItem::Undo()
{
	for (Items::iterator i = m_items.end(); i != m_items.begin();)
	{
		i--;
		(*i)->Undo();
	}
}


void AddObjectUndoItem::Do()
{
	g_doc.Objects.push_back(m_obj);
	m_ownedObj.release();
}

void AddObjectUndoItem::Undo()
{
	g_doc.Objects.remove(m_obj);
	m_ownedObj.reset(m_obj);
}


void AssignObjectUndoItem::Do()
{
	auto_ptr<CadObject> copy(m_toObject->Clone());
	m_toObject->Assign(m_fromObject.get());
	m_fromObject = copy;
}


void AssignObjectUndoItem::Undo()
{
	Do();
}


UndoManager::~UndoManager()
{
	DeleteItems(m_items.begin());
}

void UndoManager::AddWork(UndoItem * item)
{
	DeleteItems(m_pos);
	m_items.push_back(item);
	m_pos = m_items.end();
	item->Do();
}

bool UndoManager::CanUndo()
{
	return m_pos != m_items.begin();
}

void UndoManager::Undo()
{
	if (!CanUndo())
		return;
	ExitTool();
	m_pos--;
	(*m_pos)->Undo();
	InvalidateRect(g_hclientWindow, 0, true);
}

bool UndoManager::CanRedo()
{
	return m_pos != m_items.end();
}

void UndoManager::Redo()
{
	if (!CanRedo())
		return;
	ExitTool();
	(*m_pos)->Do();
	m_pos++;
	InvalidateRect(g_hclientWindow, 0, true);
}

void UndoManager::DeleteItems(Items::iterator pos)
{
	for (Items::iterator i = pos; i != m_items.end(); i++)
		delete *i;
	m_items.erase(pos, m_items.end());
}


void DeleteSelectedObjects()
{
	if (g_selected.size() != 0)
	{
		auto_ptr<GroupUndoItem> group(new GroupUndoItem);
		for (list<CadObject *>::iterator i = g_selected.begin();
			i != g_selected.end(); i++)
		{
			g_defaultTool.RemoveManipulators(*i);
			group->AddItem(new AddObjectUndoItem(*i));
		}
		g_selected.clear();
		g_undoManager.AddWork(new ReverseUndoItem(group.release()));
		InvalidateRect(g_hclientWindow, 0, true);
	}
}
