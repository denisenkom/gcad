/*
 * globals.cpp
 *
 *  Created on: 28.01.2009
 *      Author: misha
 */
#include "globals.h"
#include "console.h"
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
int g_vscrollPos;
int g_hrangeMin = static_cast<int>(g_extentMin.X * g_magification);
int g_hrangeMax = static_cast<int>(g_extentMax.X * g_magification);
int g_hscrollMin = g_hrangeMin;
int g_hscrollMax = g_hrangeMax;
int g_hscrollPos;

CursorType g_cursorType = CursorTypeManual;
CustomCursorType g_customCursorType = CustomCursorTypeSelect;
bool g_cursorDrawn = false;
bool g_mouseInsideClient = false;
Point<int> g_cursorScn;
Point<double> g_cursorWrld;

Document g_doc;

ToolManager g_toolManager;
Selector g_selector;
DefaultTool g_defaultTool;
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
	if (Radius != 0)
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


void ExitTool()
{
	g_fantomManager.DeleteFantoms(false);
	g_fantomManager.RecalcFantomsHandler = Functor<void>();
	g_curTool->Exiting();
	g_curTool = &g_defaultTool;
	g_defaultTool.Start();
	InvalidateRect(g_hclientWindow, 0, true);
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


bool Selector::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		m_lassoPt2 = m_lassoPt1 = ScreenToWorld(g_cursorScn.X, g_cursorScn.Y);
		m_lassoOn = true;
		return true;
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
			return true;
		}
		return false;
	case WM_LBUTTONUP:
	{
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
		return true;
	}
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_RETURN:
			if (NextTool == 0)
				return false;
			g_console.LogCommand();
			if (g_selected.size() == 0)
			{
				ExitTool();
				return true;
			}
			g_curTool = NextTool;
			NextTool->Start();
			InvalidateRect(hwnd, 0, true);
			return true;
		default:
			return false;
		}
	default:
		return false;
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

bool DefaultTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
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
			return true;
		case WM_KEYDOWN:
			switch (wparam)
			{
			case VK_DELETE:
				DeleteSelectedObjects();
				return true;
			default:
				return false;
			}
		default:
			return g_selector.ProcessInput(hwnd, msg, wparam, lparam);
		}
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
			return true;
		default:
			return false;
		}
	default:
		assert(0);
		return false;
	}
}


void DefaultTool::Start()
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeSelect;
	Functor<void, LOKI_TYPELIST_2(CadObject*,bool)> handler(this, &DefaultTool::SelectHandler);
	g_selector.SelectHandler = handler;
	g_canSnap = false;
	g_console.SetPrompt(L"command:");
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
		m_manipulators.clear();
		break;
	}
}


void DefaultTool::Command(const wstring & cmd)
{
	switch (m_state)
	{
	case Selecting:
		g_toolManager.DispatchTool(cmd);
		break;
	case MovingManip:
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


REGISTER_TOOL(L"zoom", ZoomTool, false);


bool ZoomTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_MOUSEMOVE:
		if (!m_zooming)
			return false;
		Zoom(m_startMag, -0.005f * (GET_Y_LPARAM(lparam) - m_startY),
			m_startHscroll, m_startVscroll, m_vectorX, m_vectorY);
		return true;
	case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        m_startY = GET_Y_LPARAM(lparam);
		m_startMag = g_magification;
		m_startHscroll = g_hscrollPos;
		m_startVscroll = g_vscrollPos;
        m_vectorX = GET_X_LPARAM(lparam);
		m_vectorY = GET_Y_LPARAM(lparam);
        m_zooming = true;
		return true;
	case WM_LBUTTONUP:
		SetCapture(0);
		m_zooming = false;
		return true;
	default:
		return false;
	}
}


void ZoomTool::Start()
{
	g_cursorType = CursorTypeSystem;
	g_cursorHandle = LoadCursorW(g_hInstance, MAKEINTRESOURCEW(IDC_ZOOM));
	g_canSnap = false;
}


REGISTER_TOOL(L"pan", PanTool, false);


bool PanTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		m_prevX = GET_X_LPARAM(lparam);
		m_prevY = GET_Y_LPARAM(lparam);
		m_panning = true;
		return true;
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
			return true;
		}
		return false;
	case WM_LBUTTONUP:
		SetCapture(0);
		m_panning = false;
		return true;
	default:
		return false;
	}
}


void PanTool::Start()
{
	g_cursorType = CursorTypeSystem;
	g_cursorHandle = LoadCursorW(g_hInstance, MAKEINTRESOURCEW(IDC_PAN));
	g_canSnap = false;
}


bool DrawLinesTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		g_console.LogCommand();
		FeedPoint(Point<double>(g_cursorWrld));
		return true;
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_RETURN:
			ExitTool();
			return true;
		default:
			return false;
		}
	default:
		return false;
	}
}


void DrawLinesTool::RecalcFantomsHandler()
{
	if (m_fantomLine.get() != 0)
		m_fantomLine->Point2 = g_cursorWrld;
}


void DrawLinesTool::FeedPoint(const Point<double> & pt)
{
	if (m_points.size() > 0)
	{
		g_fantomManager.DeleteFantoms(false);
		m_fantomLine->Point2 = pt;
		g_undoManager.AddGroupItem(auto_ptr<UndoItem>(new AddObjectUndoItem(m_fantomLine.release())));
	}
	m_points.push_back(pt);
	UpdatePrompt();
	m_fantomLine.reset(new CadLine());
	m_fantomLine->Point1 = pt;
	m_fantomLine->Point2 = g_cursorWrld;
	g_fantomManager.AddFantom(m_fantomLine.get());
	g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawLinesTool::RecalcFantomsHandler);
	InvalidateRect(g_hclientWindow, 0, true);
}



void DrawLinesTool::Start()
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	g_canSnap = true;
	UpdatePrompt();
}


bool TryParsePoint2D(const std::wstring & str, Point<double> & point)
{
	return (swscanf(str.c_str(), L"%lf,%lf", &point.X, &point.Y) == 2);
}


bool ParsePoint2D(const std::wstring & str, Point<double> & point)
{
	if (TryParsePoint2D(str, point))
		return true;
	g_console.Log(L"Invalid 2D point");
	return false;
}


REGISTER_TOOL(L"line", DrawLinesTool, false);


void DrawLinesTool::Command(const std::wstring & cmd)
{
	if (m_points.size() >= 3 && IsKey(cmd, L"close"))
	{
		FeedPoint(m_points.front());
		ExitTool();
		return;
	}
	if (m_points.size() >= 1 && IsKey(cmd, L"undo"))
	{
		m_points.pop_back();
		UpdatePrompt();
		if (m_points.size() > 0)
		{
			g_undoManager.RemoveGroupItem();
			m_fantomLine->Point1 = m_points.back();
		}
		else
		{
			g_fantomManager.DeleteFantoms(false);
			m_fantomLine.reset(0);
		}
		InvalidateRect(g_hclientWindow, 0, true);
		return;
	}
	Point<double> pt;
	if (ParsePoint2D(cmd, pt))
		FeedPoint(pt);
}


void DrawLinesTool::Exiting()
{
	if (m_points.size() >= 2)
		g_undoManager.EndGroup();
	m_points.clear();
}


void DrawLinesTool::UpdatePrompt()
{
	if (m_points.size() >= 3)
		g_console.SetPrompt(L"specify next point or [Close/Undo]:");
	else if (m_points.size() >= 1)
		g_console.SetPrompt(L"specify next point or [Undo]:");
	else
		g_console.SetPrompt(L"specify first point:");
}


REGISTER_TOOL(L"circle", DrawCircleTool, false);


bool DrawCircleTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (m_state)
	{
	case StateSelectingCenter:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			FeedCenter(g_cursorWrld);
			return true;
		default:
			return false;
		}
	case StateSelectingRadius:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			FeedRadius((g_cursorWrld - m_cadCircle->Center).Length());
			return true;
		default:
			return false;
		}
	case StateSelectingDiameter:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			FeedRadius((g_cursorWrld - m_cadCircle->Center).Length() / 2);
			return true;
		default:
			return false;
		}
	case State2PtSelectingFirstPoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			Feed2PtFirstPoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	case State2PtSelectingSecondPoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			Feed2PtSecondPoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	case State3PtSelectingFirstPoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			Feed3PtFirstPoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	case State3PtSelectingSecondPoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			Feed3PtSecondPoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	case State3PtSelectingThirdPoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			Feed3PtThirdPoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	default:
		assert(0);
		return false;
	}
}


void DrawCircleTool::RecalcFantomsHandler()
{
	switch (m_state)
	{
	case StateSelectingCenter:
		break;
	case StateSelectingRadius:
		m_cadCircle->Radius = (g_cursorWrld - m_cadCircle->Center).Length();
		m_line->Point2 = g_cursorWrld;
		break;
	case StateSelectingDiameter:
		m_cadCircle->Radius = (g_cursorWrld - m_cadCircle->Center).Length() / 2;
		m_line->Point2 = g_cursorWrld;
		break;
	case State2PtSelectingFirstPoint:
		break;
	case State2PtSelectingSecondPoint:
		m_cadCircle->Center = (g_cursorWrld + m_firstPoint)/2;
		m_cadCircle->Radius = (g_cursorWrld - m_firstPoint).Length() / 2;
		break;
	case State3PtSelectingFirstPoint:
		break;
	case State3PtSelectingSecondPoint:
		break;
	case State3PtSelectingThirdPoint:
		CalcCircleFrom3Pt(g_cursorWrld);
		break;
	}
}


void DrawCircleTool::Start()
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	m_state = StateSelectingCenter;
	g_canSnap = true;
	g_console.SetPrompt(L"specify center point for circle [2P/3P]:");
	g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawCircleTool::RecalcFantomsHandler);
}


void DrawCircleTool::Command(const wstring & cmd)
{
	Point<double> pt;
	double radius, diameter;
	switch (m_state)
	{
	case StateSelectingCenter:
		if (IsKey(cmd, L"2p"))
		{
			m_state = State2PtSelectingFirstPoint;
			g_console.SetPrompt(L"specify first point on circle's diameter:");
		}
		else if (IsKey(cmd, L"3p"))
		{
			m_state = State3PtSelectingFirstPoint;
			g_console.SetPrompt(L"specify first point on circle:");
		}
		else if (ParsePoint2D(cmd, pt))
		{
			FeedCenter(pt);
		}
		break;
	case StateSelectingRadius:
		if (TryParsePoint2D(cmd, pt))
		{
			FeedRadius((pt - m_cadCircle->Center).Length());
		}
		else if (swscanf(cmd.c_str(), L"%lf", &radius) == 1)
		{
			FeedRadius(radius);
		}
		else if (IsKey(cmd, L"diameter"))
		{
			m_state = StateSelectingDiameter;
			m_cadCircle->Radius = (g_cursorWrld - m_cadCircle->Center).Length() / 2;
			g_console.SetPrompt(L"specify diameter for circle or [Radius]:");
			InvalidateRect(g_hclientWindow, 0, true);
		}
		else
		{
			g_console.Log(L"expected radius value or point coordinates or character D");
		}
		break;
	case StateSelectingDiameter:
		if (TryParsePoint2D(cmd, pt))
		{
			FeedRadius((pt - m_cadCircle->Center).Length() / 2);
		}
		else if (swscanf(cmd.c_str(), L"%lf", &diameter) == 1)
		{
			FeedRadius(diameter);
		}
		else if (IsKey(cmd, L"radius"))
		{
			m_state = StateSelectingRadius;
			m_cadCircle->Radius = (g_cursorWrld - m_cadCircle->Center).Length();
			g_console.SetPrompt(L"specify radius for circle or [Diameter]:");
			InvalidateRect(g_hclientWindow, 0, true);
		}
		else
		{
			g_console.Log(L"expected diameter value or point coordinates or character R");
		}
		break;
	case State2PtSelectingFirstPoint:
		if (ParsePoint2D(cmd, pt))
			Feed2PtFirstPoint(pt);
		break;
	case State2PtSelectingSecondPoint:
		if (ParsePoint2D(cmd, pt))
			Feed2PtSecondPoint(pt);
		break;
	case State3PtSelectingFirstPoint:
		if (ParsePoint2D(cmd, pt))
			Feed3PtFirstPoint(pt);
		break;
	case State3PtSelectingSecondPoint:
		if (ParsePoint2D(cmd, pt))
			Feed3PtSecondPoint(pt);
		break;
	case State3PtSelectingThirdPoint:
		if (ParsePoint2D(cmd, pt))
			Feed3PtThirdPoint(pt);
		break;
	}
}


void DrawCircleTool::FeedCenter(const Point<double> & center)
{
	m_cadCircle.reset(new CadCircle());
	m_cadCircle->Center = center;
	m_cadCircle->Radius = (g_cursorWrld - center).Length();
	m_line.reset(new CadLine());
	m_line->Point1 = center;
	m_line->Point2 = g_cursorWrld;
	m_state = StateSelectingRadius;
	g_console.SetPrompt(L"specify radius for circle or [Diameter]:");
	g_fantomManager.AddFantom(m_cadCircle.get());
	g_fantomManager.AddFantom(m_line.get());
	InvalidateRect(g_hclientWindow, 0, true);
}


void DrawCircleTool::FeedRadius(double radius)
{
	if (radius <= 0)
	{
		g_console.Log(L"radius must be positive");
		return;
	}
	m_line.reset(0);
	m_cadCircle->Radius = radius;
	g_undoManager.AddWork(new AddObjectUndoItem(m_cadCircle.release()));
	ExitTool();
}


void DrawCircleTool::Feed2PtFirstPoint(const Point<double> & pt)
{
	m_firstPoint = pt;
	m_cadCircle.reset(new CadCircle());
	m_cadCircle->Center = (g_cursorWrld + pt)/2;
	m_cadCircle->Radius = (g_cursorWrld - pt).Length() / 2;
	m_state = State2PtSelectingSecondPoint;
	g_console.SetPrompt(L"specify second point of circle's diameter:");
	g_fantomManager.AddFantom(m_cadCircle.get());
	InvalidateRect(g_hclientWindow, 0, true);
}


void DrawCircleTool::Feed2PtSecondPoint(const Point<double> & pt)
{
	if (m_firstPoint == pt)
	{
		g_console.Log(L"Points must be different");
		return;
	}
	m_cadCircle->Center = (pt + m_firstPoint)/2;
	m_cadCircle->Radius = (pt - m_firstPoint).Length() / 2;
	g_undoManager.AddWork(new AddObjectUndoItem(m_cadCircle.release()));
	ExitTool();
}


void DrawCircleTool::Feed3PtFirstPoint(const Point<double> & pt)
{
	m_firstPoint = pt;
	m_state = State3PtSelectingSecondPoint;
	g_console.SetPrompt(L"specify second point on circle:");
}


void DrawCircleTool::Feed3PtSecondPoint(const Point<double> & pt)
{
	if (m_firstPoint == pt)
	{
		g_console.Log(L"Points must be different");
		return;
	}
	m_secondPoint = pt;
	CalcCircleFrom3Pt(g_cursorWrld);
	m_state = State3PtSelectingThirdPoint;
	g_console.SetPrompt(L"specify third point on circle:");
	InvalidateRect(g_hclientWindow, 0, true);
}


void DrawCircleTool::Feed3PtThirdPoint(const Point<double> & pt)
{
	CalcCircleFrom3Pt(pt);
	if (m_cadCircle.get() == 0)
	{
		g_console.Log(L"Invalid circle");
		return;
	}
	g_undoManager.AddWork(new AddObjectUndoItem(m_cadCircle.release()));
	ExitTool();
}


void DrawCircleTool::CalcCircleFrom3Pt(const Point<double> & thirdPoint)
{
	CircleArc<double> arc = ArcFrom3Pt(m_firstPoint, m_secondPoint, thirdPoint);
	if (arc.Straight)
	{
		if (m_cadCircle.get() != 0)
		{
			m_cadCircle.reset(0);
			g_fantomManager.DeleteFantoms(false);
			InvalidateRect(g_hclientWindow, 0, true);
		}
	}
	else
	{
		if (m_cadCircle.get() == 0)
		{
			m_cadCircle.reset(new CadCircle());
			g_fantomManager.AddFantom(m_cadCircle.get());
		}
		m_cadCircle->Center = arc.Center;
		m_cadCircle->Radius = arc.Radius;
	}
}


REGISTER_TOOL(L"arc", DrawArcsTool, false);


bool DrawArcsTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		switch (m_state)
		{
		case StateSelectingFirstPoint:
			g_console.LogCommand();
			FeedFirstPoint(g_cursorWrld);
			return true;
		case StateSelectingSecondPoint:
			g_console.LogCommand();
			FeedSecondPoint(g_cursorWrld);
			return true;
		case StateSelectingThirdPoint:
			g_console.LogCommand();
			FeedThirdPoint(g_cursorWrld);
			return true;
		}
	default:
		return false;
	}
}


void DrawArcsTool::Start()
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	m_state = StateSelectingFirstPoint;
	g_canSnap = true;
	g_console.SetPrompt(L"Specify first point of arc:");
	g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawArcsTool::RecalcFantomsHandler);
}


void DrawArcsTool::Command(const wstring & cmd)
{
	Point<double> pt;
	switch (m_state)
	{
	case StateSelectingFirstPoint:
		if (ParsePoint2D(cmd, pt))
			FeedFirstPoint(pt);
		break;
	case StateSelectingSecondPoint:
		if (ParsePoint2D(cmd, pt))
			FeedSecondPoint(pt);
		break;
	case StateSelectingThirdPoint:
		if (ParsePoint2D(cmd, pt))
			FeedThirdPoint(pt);
		break;
	}
}


void DrawArcsTool::RecalcFantomsHandler()
{
	if (m_fantomLine.get())
		m_fantomLine->Point2 = g_cursorWrld;
	if (m_fantomArc.get())
		CalcArcFrom3Pt(g_cursorWrld);
}


void DrawArcsTool::FeedFirstPoint(const Point<double> & pt)
{
	m_fantomLine.reset(new CadLine());
	m_fantomLine->Point1 = pt;
	m_fantomLine->Point2 = g_cursorWrld;
	g_fantomManager.AddFantom(m_fantomLine.get());
	m_state = StateSelectingSecondPoint;
	g_console.SetPrompt(L"Specify second point of arc:");
	InvalidateRect(g_hclientWindow, 0, true);
}


void DrawArcsTool::FeedSecondPoint(const Point<double> & pt)
{
	if (m_fantomLine->Point1 == pt)
	{
		g_console.Log(L"Points must be different");
		return;
	}
	g_fantomManager.DeleteFantoms(false);
	m_fantomArc.reset(new CadArc());
	m_fantomArc->Start = m_fantomLine->Point1;
	m_fantomArc->End = m_secondPoint = pt;
	m_fantomArc->Radius = 0;
	m_fantomLine.reset(0);
	g_fantomManager.AddFantom(m_fantomArc.get());
	m_state = StateSelectingThirdPoint;
	g_console.SetPrompt(L"Specify third point of arc:");
	InvalidateRect(g_hclientWindow, 0, true);
}


void DrawArcsTool::FeedThirdPoint(const Point<double> & pt)
{
	CalcArcFrom3Pt(pt);
	if (m_fantomArc->Radius == 0)
	{
		g_console.Log(L"Invalid arc");
		return;
	}
	g_undoManager.AddWork(new AddObjectUndoItem(m_fantomArc.release()));
	ExitTool();
}


void DrawArcsTool::CalcArcFrom3Pt(const Point<double> & thirdPoint)
{
	CircleArc<double> arc = ArcFrom3Pt(m_fantomArc->Start, m_secondPoint, thirdPoint);
	m_fantomArc->Center = arc.Center;
	m_fantomArc->End = thirdPoint;
	m_fantomArc->Ccw = arc.Ccw;
	m_fantomArc->Radius = arc.Straight ? 0 : arc.Radius;
}


REGISTER_TOOL(L"move", MoveTool, true);


bool MoveTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (m_state)
	{
	case StateChoosingBasePoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			FeedBasePoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	case StateChoosingDestPoint:
		switch (msg)
		{
		case WM_LBUTTONUP:
			g_console.LogCommand();
			FeedDestPoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	default:
		assert(0);
		return false;
	}
}


void MoveTool::Start()
{
	assert(g_selected.size() != 0);
	m_state = StateChoosingBasePoint;
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	g_canSnap = true;
	g_console.SetPrompt(L"Specify base point:");
}


void MoveTool::Command(const wstring & cmd)
{
	Point<double> pt;
	switch (m_state)
	{
	case StateChoosingBasePoint:
		if (ParsePoint2D(cmd, pt))
			FeedBasePoint(pt);
		break;
	case StateChoosingDestPoint:
		if (ParsePoint2D(cmd, pt))
			FeedDestPoint(pt);
		break;
	}
}


void MoveTool::Exiting()
{
	m_fantomLine.reset(0);
	DeleteCopies();
	g_selected.clear();
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
	CalcPositions(g_cursorWrld);
}


void MoveTool::CalcPositions(const Point<double> & pt)
{
	for (vector<CadObject*>::iterator i = m_objects.begin();
		i != m_objects.end(); i++)
	{
		(*i)->Move(pt - m_basePoint);
	}
	m_basePoint = pt;
}


void MoveTool::FeedBasePoint(const Point<double> & pt)
{
	m_basePoint = pt;
	g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &MoveTool::RecalcFantomsHandler);
	m_fantomLine.reset(new CadLine());
	m_fantomLine->Point1 = m_fantomLine->Point2 = m_basePoint;
	g_fantomManager.AddFantom(m_fantomLine.get());
	m_state = StateChoosingDestPoint;
	DeleteCopies();
	m_objects.resize(g_selected.size());
	int no = 0;
	for (list<CadObject*>::iterator i = g_selected.begin();
		i != g_selected.end(); i++, no++)
	{
		m_objects[no] = (*i)->Clone();
		g_fantomManager.AddFantom(m_objects[no]);
	}
	g_console.SetPrompt(L"Specify insertion point:");
}


void MoveTool::FeedDestPoint(const Point<double> & pt)
{
	CalcPositions(pt);
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


REGISTER_TOOL(L"pasteclip", PasteTool, false);


bool PasteTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		g_console.LogCommand();
		FeedInsertionPoint(g_cursorWrld);
		return true;
	default:
		return false;
	}
}


void PasteTool::Start()
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
		CalcPositions(g_cursorWrld);
		g_console.SetPrompt(L"Specify insertion point:");
	}
	else
	{
		ExitTool();
	}
	if (!CloseClipboard())
		assert(0);
}


void PasteTool::Command(const wstring & cmd)
{
	Point<double> pt;
	if (ParsePoint2D(cmd, pt))
		FeedInsertionPoint(pt);
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
	CalcPositions(g_cursorWrld);
}


void PasteTool::CalcPositions(const Point<double> & pt)
{
	for (vector<CadObject*>::iterator i = m_objects.begin();
		i != m_objects.end(); i++)
	{
		(*i)->Move(pt - m_basePoint);
	}
	m_basePoint = pt;
}


void PasteTool::FeedInsertionPoint(const Point<double> & pt)
{
	CalcPositions(pt);
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


REGISTER_TOOL(L"undo", UndoTool, false);


void UndoTool::Start()
{
	g_undoManager.Undo();
	ExitTool();
}


REGISTER_TOOL(L"mredo", RedoTool, false);


void RedoTool::Start()
{
	g_undoManager.Redo();
	ExitTool();
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

void UndoManager::AddGroupItem(auto_ptr<UndoItem> item)
{
	if (m_group.get() == 0)
		m_group.reset(new GroupUndoItem);
	item->Do();
	m_group->AddItem(item.release());
}

void UndoManager::RemoveGroupItem()
{
	assert(m_group.get() != 0);
	assert(m_group->m_items.size() > 0);
	m_group->m_items.back()->Undo();
	delete m_group->m_items.back();
	m_group->m_items.pop_back();
}

void UndoManager::EndGroup()
{
	assert(m_group.get() != 0);
	assert(m_group->m_items.size() > 0);
	DeleteItems(m_pos);
	m_items.push_back(m_group.release());
	m_pos = m_items.end();
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


void ToolManager::RegisterTool(const wstring & id, Tool * tool, bool needSelection)
{
	ToolInfo toolInfo = {tool, needSelection};
	m_toolsMap[ToLower(id)] = toolInfo;
}


void ToolManager::DispatchTool(const wstring & id)
{
	g_defaultTool.Exiting();
	static UndoTool s_undoTool;
	static RedoTool s_redoTool;
	size_t start = id.find_first_not_of(L"_.");
	if (start == wstring::npos)
		start = id.size();
	wstring idreal(id, start);
	idreal = ToLower(idreal);
	ToolsMapType::iterator pos = m_toolsMap.find(idreal);
	if (pos == m_toolsMap.end())
	{
		g_console.Log(L"unknown command: '" + idreal + L"'");
		return;
	}
	if (pos->second.m_needSelection)
	{
		if (g_selected.size() == 0)
		{
			g_selector.NextTool = pos->second.m_tool;
			g_curTool = &g_selector;
			g_cursorType = CursorTypeManual;
			g_cursorHandle = 0;
			g_customCursorType = CustomCursorTypeBox;
			g_selector.SelectHandler = Functor<void, LOKI_TYPELIST_2(CadObject*,bool)>();
			g_canSnap = false;
			g_console.SetPrompt(L"Select objects:");
		}
		else
		{
			g_console.Log(L"Found " + IntToWstr(g_selected.size()) + L" objects");
			g_curTool = pos->second.m_tool;
			g_curTool->Start();
		}
	}
	else
	{
		g_selected.clear();
		g_curTool = pos->second.m_tool;
		g_curTool->Start();
	}
	InvalidateRect(g_hclientWindow, 0, true);
}


void ExecuteCommand(const wstring & cmd)
{
	if (g_curTool != &g_defaultTool)
		Cancel();
	else
		g_console.ClearInput();
	g_console.LogCommand(cmd);
	g_toolManager.DispatchTool(cmd);
}


void Cancel()
{
	g_console.LogCommand(g_console.GetInput() + L"*cancel*");
	g_console.ClearInput();
	g_fantomManager.DeleteFantoms(false);
	g_curTool->Cancel();
	if (g_curTool != &g_defaultTool)
	{
		g_curTool = &g_defaultTool;
		g_defaultTool.Start();
	}
	InvalidateRect(g_hclientWindow, 0, true);
}
