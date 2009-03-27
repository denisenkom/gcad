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
#include <limits>
#include <vector>


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


CadLine * CadLine::Clone() const
{
	return new CadLine(*this);
}


void CadLine::Assign(const CadObject & rhs)
{
	const CadLine * line = dynamic_cast<const CadLine *>(&rhs);
	assert(line != 0);
	Assign(*line);
}


void CadLine::Assign(const CadLine & line)
{
	*this = line;
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


static void DrawArcInternal(HDC hdc, const CircleArc & arc, void (*drawer)(HDC, int, int, int, int, int, int, int, int))
{
	if (arc.Radius != 0 & arc.Radius < 10E+10)
	{
		Point<int> center = WorldToScreen(arc.Center);
		Point<int> start = WorldToScreen(arc.Start);
		Point<int> end = WorldToScreen(arc.End);
		int r = static_cast<int>(floor(arc.Radius * g_magification + 0.5));
		if (!SetArcDirection(hdc, arc.Ccw ? AD_COUNTERCLOCKWISE : AD_CLOCKWISE))
			assert(0);
		drawer(hdc, center.X - r, center.Y - r, center.X + r + 1, center.Y + r + 1, start.X, start.Y, end.X, end.Y);
	}
}


static void ArcDrawer(HDC hdc, int rcl, int rct, int rcr, int rcb, int sx, int sy, int ex, int ey)
{
	if (!Arc(hdc, rcl, rct, rcr, rcb, sx, sy, ex, ey))
		assert(0);
}

static void ArcToDrawer(HDC hdc, int rcl, int rct, int rcr, int rcb, int sx, int sy, int ex, int ey)
{
	if (!LineTo(hdc, sx, sy))
		assert(0);
	if (!ArcTo(hdc, rcl, rct, rcr, rcb, sx, sy, ex, ey))
		assert(0);
	if (!LineTo(hdc, ex, ey))
		assert(0);
}


inline void DrawArc(HDC hdc, const CircleArc & arc)
{
	DrawArcInternal(hdc, arc, &ArcDrawer);
}

inline void DrawArcTo(HDC hdc, const CircleArc & arc)
{
	DrawArcInternal(hdc, arc, &ArcToDrawer);
}


// draws from current raster position
static void DrawPolylineSeg(HDC hdc, const CadPolyline::Node & from, const CadPolyline::Node & to)
{
	if (from.Bulge == 0)
	{
		Point<int> scnPt = WorldToScreen(to.Point.X, to.Point.Y);
		LineTo(hdc, scnPt.X, scnPt.Y);
	}
	else
	{
		DrawArcTo(hdc, ArcFrom2PtAndBulge(from.Point, to.Point, from.Bulge));
	}
}


void CadPolyline::Draw(HDC hdc, bool selected) const
{
	HPEN hpen = selected ? g_selectedLineHPen : g_lineHPen;
	if (SelectObject(hdc, hpen) == NULL)
		assert(0);
	if (SetBkColor(hdc, RGB(0, 0, 0)) == CLR_INVALID)
		assert(0);
	vector<Node>::const_iterator i = Nodes.begin();
	Node prev;
	assert(i != Nodes.end());
	Point<int> scnPt = WorldToScreen(i->Point);
	MoveToEx(hdc, scnPt.X, scnPt.Y, 0);
	prev = *i;
	i++;
	for (; i != Nodes.end(); prev = *i, i++)
		DrawPolylineSeg(hdc, prev, *i);
	if (Closed)
		DrawPolylineSeg(hdc, Nodes.back(), Nodes.front());
}


static bool PolylineSegIntersectsRect(const CadPolyline::Node & from, const CadPolyline::Node & to, const Rect<double> & rect)
{
	if (from.Bulge == 0)
	{
		return LineIntersectsRect(from.Point, to.Point, rect);
	}
	else
	{
		CircleArc arc = ArcFrom2PtAndBulge(from.Point, to.Point, from.Bulge);
		return IsIntersects(arc, rect);
	}
}


bool CadPolyline::IntersectsRect(double x1, double y1, double x2, double y2) const
{
	Node prev;
	Rect<double> rect(x1, y1, x2, y2);
	for (vector<Node>::const_iterator i = Nodes.begin(); i != Nodes.end(); prev = *i, i++)
	{
		if (i == Nodes.begin())
			continue;
		if (PolylineSegIntersectsRect(prev, *i, rect))
			return true;
	}
	if (Closed)
		return PolylineSegIntersectsRect(Nodes.back(), Nodes.front(), rect);
	return false;
}


static Rect<double> PolylineSegBoundingRect(const CadPolyline::Node & from,
		const CadPolyline::Node & to)
{
	if (from.Bulge == 0)
		return Rect<double>(from.Point, to.Point).Normalized();
	else
		return ArcFrom2PtAndBulge(from.Point, to.Point, from.Bulge).CalcBoundingRect();

}


Rect<double> CadPolyline::GetBoundingRect() const
{
	Rect<double> result;
	bool first = true;
	Node prev;
	for (vector<Node>::const_iterator i = Nodes.begin(); i != Nodes.end(); prev = *i, i++)
	{
		if (i == Nodes.begin())
			continue;
		Rect<double> rect = PolylineSegBoundingRect(prev, *i);
		if (first)
		{
			result = rect;
			first = false;
		}
		else
		{
			result = ::GetBoundingRect(result, rect);
		}
	}
	if (Closed)
	{
		result = ::GetBoundingRect(result,
				PolylineSegBoundingRect(Nodes.back(), Nodes.front()));
	}
	return result;
}


std::vector<Point<double> > CadPolyline::GetManipulators() const
{
	vector<Point<double> > result;
	Node prev;
	for (vector<Node>::const_iterator i = Nodes.begin(); i != Nodes.end(); prev = *i, i++)
	{
		if (i != Nodes.begin())
		{
			if (prev.Bulge != 0)
				result.push_back(ArcMiddleFrom2PtAndBulge(prev.Point, i->Point, prev.Bulge));
		}
		result.push_back(i->Point);
	}
	if (Closed)
	{
		if (Nodes.back().Bulge != 0)
		{
			result.push_back(ArcMiddleFrom2PtAndBulge(Nodes.back().Point,
					Nodes.front().Point, Nodes.back().Bulge));
		}
	}
	return result;
}


void CadPolyline::UpdateManip(const Point<double> & pt, int id)
{
	int counter = 0;
	vector<Node>::iterator prev;
	for (vector<Node>::iterator i = Nodes.begin(); i != Nodes.end(); i++)
	{
		if (i != Nodes.begin())
		{
			if (prev->Bulge != 0)
			{
				if (counter == id)
				{
					CircleArc arc = ArcFrom3Pt(prev->Point, pt,
						i->Point);
					prev->Bulge = arc.CalcBulge();
					return;
				}
				counter++;
			}
		}
		if (counter == id)
		{
			i->Point = pt;
			return;
		}
		counter++;
		prev = i;
	}
	if (Closed)
	{
		assert(counter == id);
		CircleArc arc = ArcFrom3Pt(Nodes.back().Point, pt,
				Nodes.front().Point);
		prev->Bulge = arc.CalcBulge();
		return;
	}
	assert(0);
}


// adds magnet points for given segment not including endpoints
static void AddPolylineSegPoints(const CadPolyline::Node & from,
		const CadPolyline::Node & to,
		vector<pair<Point<double>, PointType> > & result)
{
	if (from.Bulge == 0)
	{
		result.push_back(make_pair((from.Point + to.Point)/2, PointTypeMiddle));
	}
	else
	{
		Point<double> middle = ArcMiddleFrom2PtAndBulge(from.Point, to.Point, from.Bulge);
		CircleArc arc = ArcFrom3Pt(from.Point, middle, to.Point);
		result.push_back(make_pair(middle, PointTypeMiddle));
		result.push_back(make_pair(arc.Center, PointTypeCenter));
	}

}


std::vector<std::pair<Point<double>, PointType> > CadPolyline::GetPoints() const
{
	vector<pair<Point<double>, PointType> > result;
	Node prev;
	for (vector<Node>::const_iterator i = Nodes.begin(); i != Nodes.end();
		prev = *i, i++)
	{
		if (i != Nodes.begin())
			AddPolylineSegPoints(prev, *i, result);
		result.push_back(make_pair(i->Point, PointTypeEndPoint));
	}
	if (Closed)
		AddPolylineSegPoints(Nodes.back(), Nodes.front(), result);
	return result;
}


void CadPolyline::Move(Point<double> displacement)
{
	for (vector<Node>::iterator i = Nodes.begin(); i != Nodes.end(); i++)
		i->Point += displacement;
}


size_t CadPolyline::Serialize(unsigned char * ptr) const
{
	size_t result = 0;
	result += WritePtr(ptr, ID);
	result += WritePtr(ptr, static_cast<int>(Closed));
	result += WritePtr(ptr, Nodes.size());
	for (vector<Node>::const_iterator i = Nodes.begin(); i != Nodes.end(); i++)
		result += WritePtr(ptr, *i);
	return result;
}


void CadPolyline::Load(unsigned char const *& ptr, size_t & size)
{
	size_t numNodes;
	int closed;
	ReadPtr(ptr, closed, size);
	Closed = closed;
	ReadPtr(ptr, numNodes, size);
	for (size_t i = 0; i < numNodes; i++)
	{
		Node node;
		ReadPtr(ptr, node, size);
		Nodes.push_back(node);
	}
}


CadPolyline2::CadPolyline2(const std::vector<CadPolyline::Node> & nodes, bool closed)
{
	InitFromNodes(nodes, closed);
}

CadPolyline2::CadPolyline2(const CadPolyline & rhs)
{
	InitFromNodes(rhs.Nodes, rhs.Closed);
}

CadPolyline2::~CadPolyline2()
{
	for_each(m_elements.begin(), m_elements.end(), ptr_fun(operator delete));
}

void CadPolyline2::InitFromNodes(const std::vector<CadPolyline::Node> & nodes, bool closed)
{
	m_closed = closed;
	CadPolyline::Node prev;
	for (vector<CadPolyline::Node>::const_iterator i = nodes.begin(); i != nodes.end(); prev = *i, i++)
	{
		if (i == nodes.begin())
			continue;
		AddSeg(prev, *i);
	}
	if (closed)
		AddSeg(nodes.back(), nodes.front());
}

void CadPolyline2::AddSeg(CadPolyline::Node node1, CadPolyline::Node node2)
{
	if (node1.Bulge == 0)
	{
		m_elements.push_back(new CadLine(node1.Point, node2.Point));
	}
	else
	{
		m_elements.push_back(new CadArc(ArcFrom2PtAndBulge(node1.Point,
				node2.Point, node1.Bulge)));
	}
}

CadPolyline CadPolyline2::ToCadPolyline() const
{
	CadPolyline result;
	result.Closed = m_closed;
	for (vector<IPolylineSeg*>::const_iterator i = m_elements.begin(); i != m_elements.end(); i++)
	{
		CadPolyline::Node node;
		node.Point = (*i)->GetStart();
		node.Bulge = (*i)->GetBulge();
		result.Nodes.push_back(node);
	}
	if (!m_closed)
	{
		CadPolyline::Node node;
		node.Point = m_elements.back()->GetEnd();
		node.Bulge = 0;
		result.Nodes.push_back(node);
	}
	return result;
}

void CadCircle::Draw(HDC hdc, bool selected) const
{
	HPEN hpen = selected ? g_selectedLineHPen : g_lineHPen;
	if (SelectObject(hdc, hpen) == NULL)
		assert(0);
	if (SetBkColor(hdc, RGB(0, 0, 0)) == CLR_INVALID)
		assert(0);
	Point<int> center = WorldToScreen(Center);
	int r = static_cast<int>(Radius * g_magification + 0.5);
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
	res = VertLineIntersectsCircle(x1, *this);
	if (res.first)
	{
		if (y1 <= Center.Y + res.second && Center.Y + res.second <= y2)
			return true;
		else if (res.second != 0 && y1 <= Center.Y - res.second && Center.Y - res.second <= y2)
			return true;
	}
	res = VertLineIntersectsCircle(x2, *this);
	if (res.first)
	{
		if (y1 <= Center.Y + res.second && Center.Y + res.second <= y2)
			return true;
		else if (res.second != 0 && y1 <= Center.Y - res.second && Center.Y - res.second <= y2)
			return true;
	}
	res = HorzLineIntersectsCircle(y1, *this);
	if (res.first)
	{
		if (x1 <= Center.X + res.second && Center.X + res.second <= x2)
			return true;
		else if (res.second != 0 && x1 <= Center.X - res.second && Center.X - res.second <= x2)
			return true;
	}
	res = HorzLineIntersectsCircle(y2, *this);
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


CadCircle * CadCircle::Clone() const
{
	return new CadCircle(*this);
}


void CadCircle::Assign(const CadObject & rhs)
{
	const CadCircle * rhsCircle = dynamic_cast<const CadCircle *>(&rhs);
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
	DrawArc(hdc, *this);
}


bool CadArc::IntersectsRect(double x1, double y1, double x2, double y2) const
{
	return IsIntersects(*this, Rect<double>(x1, y1, x2, y2));
}


Rect<double> CadArc::GetBoundingRect() const
{
	return CalcBoundingRect();
}


vector<Point<double> > CadArc::GetManipulators() const
{
	Point<double> middle = CalcMiddlePoint();
	vector<Point<double> > result(3);
	result[0] = Start;
	result[1] = middle;
	result[2] = End;
	return result;
}


void CadArc::UpdateManip(const Point<double> & pt, int id)
{
	Point<double> middle = CalcMiddlePoint();
	switch (id)
	{
	case 0: Start = pt; break;
	case 1: middle = pt; break;
	case 2: End = pt; break;
	default: assert(0); break;
	}
	CircleArc arc = ArcFrom3Pt(Start, middle, End);
	Center = arc.Center;
	Radius = arc.Radius;
}


vector<pair<Point<double>, PointType> > CadArc::GetPoints() const
{
	Point<double> middle = CalcMiddlePoint();
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


CadArc * CadArc::Clone() const
{
	return new CadArc(*this);
}


void CadArc::Assign(const CadObject & rhs)
{
	const CadArc * arc = dynamic_cast<const CadArc *>(&rhs);
	assert(arc != 0);
	Assign(*arc);
}


void CadArc::Assign(const CadArc & arc)
{
	*this = arc;
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
		if (m_multiselect)
		{
			m_lassoPt2 = m_lassoPt1 = ScreenToWorld(g_cursorScn);
			m_lassoOn = true;
		}
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
			if (m_lassoDrawn)
				MyRectangle(hdc, WorldToScreen(m_lassoPt1), m_lassoScnPt2);
			m_lassoPt2 = ScreenToWorld(g_cursorScn);
			if (m_lassoPt2.X < m_lassoPt1.X)
				SelectObject(hdc, g_selectedLineHPen);
			else
				SelectObject(hdc, g_lineHPen);
			MyRectangle(hdc, WorldToScreen(m_lassoPt1), g_cursorScn);
			m_lassoScnPt2 = g_cursorScn;
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
		if (m_multiselect && (GetKeyState(VK_SHIFT) & 0x8000))
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
				if (m_multiselect && IsSelected(*i))
					continue;
				bool good;
				if (intersect)
					good = (*i)->IntersectsRect(testRect);
				else
					good = IsLeftContainsRight(testRect, (*i)->GetBoundingRect());
				if (good)
				{
					if (!m_multiselect)
					{
						g_curTool = m_prevTool;
						if (DoneCallback)
							DoneCallback(*i, 1);
						return true;
					}
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
			if (!m_multiselect)
				return false;
			g_curTool = m_prevTool;
			{
				vector<CadObject*> sel(g_selected.begin(), g_selected.end());
				if (DoneCallback)
					DoneCallback(sel.front(), sel.size());
			}
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


void Selector::DrawLasso(HDC hdc)
{
	if (m_lassoDrawn)
	{
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		if (m_lassoPt2.X < m_lassoPt1.X)
			SelectObject(hdc, g_selectedLineHPen);
		else
			SelectObject(hdc, g_lineHPen);
		MyRectangle(hdc, WorldToScreen(m_lassoPt1), m_lassoScnPt2);
	}
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
				ExecuteCommand(L"erase");
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
	g_selector.DoneCallback = Functor<void, LOKI_TYPELIST_2(CadObject*, size_t)>();
	g_selector.m_multiselect = true;
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
	assert(ulManipsNum <= static_cast<size_t>(numeric_limits<int>::max()));
	int manipsNum = static_cast<int>(ulManipsNum);
	// rounding manipulators to epsilon
	for (int i = 0; i < manipsNum; i++)
	{
		manips[i].X = RoundEpsilon(manips[i].X);
		manips[i].Y = RoundEpsilon(manips[i].Y);
	}
	// joining manipulators
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


REGISTER_TOOL(L"zoom", ZoomTool);


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


REGISTER_TOOL(L"pan", PanTool);


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


typedef SelectWrapperTool<CopyTool> WrappedCopyTool;
REGISTER_TOOL(L"copyclip", WrappedCopyTool);


void CopyTool::Start()
{
	InternalStart();
	g_selected.clear();
	ExitTool();
}


bool CopyTool::InternalStart()
{
	if (!OpenClipboard(g_hmainWindow))
	{
		g_console.Log(L"Unable to open clipboard");
		return false;
	}
	if (!EmptyClipboard())
		assert(0);
	size_t totalSize = 0;
	for (list<CadObject *>::const_iterator i = g_selected.begin();
		i != g_selected.end(); i++)
	{
		totalSize += (*i)->Serialize(0);
	}
	HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, totalSize);
	assert(hglob);
	unsigned char * pmem = reinterpret_cast<unsigned char *>(GlobalLock(hglob));
	unsigned char * ptr = pmem;
	assert(pmem);
	for (list<CadObject *>::const_iterator i = g_selected.begin();
		i != g_selected.end(); i++)
	{
		ptr += (*i)->Serialize(ptr);
	}
	if (!GlobalUnlock(hglob))
		assert(GetLastError() == NO_ERROR);
	if (!SetClipboardData(g_clipboardFormat, hglob))
		assert(0);
	if (!CloseClipboard())
		assert(0);
	return true;
}


typedef SelectWrapperTool<CutTool> WrappedCutTool;
REGISTER_TOOL(L"cutclip", WrappedCutTool);


void CutTool::Start()
{
	if (InternalStart())
		DeleteSelectedObjects();
	g_selected.clear();
	ExitTool();
}


REGISTER_TOOL(L"pasteclip", PasteTool);


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
		m_basePoint = Point<double>(numeric_limits<double>::max(),
				numeric_limits<double>::max());
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
			case CadPolyline::ID: obj = new CadPolyline(); break;
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


REGISTER_TOOL(L"u", UndoTool);


void UndoTool::Start()
{
	g_undoManager.Undo();
	ExitTool();
}


REGISTER_TOOL(L"mredo", RedoTool);


void RedoTool::Start()
{
	g_undoManager.Redo();
	ExitTool();
}


typedef SelectWrapperTool<EraseTool> WrappedEraseTool;
REGISTER_TOOL(L"erase", WrappedEraseTool);


void EraseTool::Start()
{
	DeleteSelectedObjects();
	ExitTool();
}


Point<int> WorldToScreen(float x, float y)
{
	Point<int> result;
	float scnx = (x - g_extentMin.X) * g_magification + 0.5f;
	float scny = (y - g_extentMax.Y) * -g_magification + 0.5f;
	assert(numeric_limits<int>::min() <= scnx - g_hscrollPos && scnx - g_hscrollPos <= numeric_limits<int>::max());
	assert(numeric_limits<int>::min() <= scny - g_vscrollPos && scny - g_vscrollPos <= numeric_limits<int>::max());
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
	m_toObject->Assign(*m_fromObject.get());
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
	if (!item->IsDone())
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


void ToolManager::RegisterTool(const wstring & id, Tool * tool)
{
	m_toolsMap[ToLower(id)] = tool;
}


void BeginSelecting(const wchar_t * prompt, const Functor<void, LOKI_TYPELIST_2(CadObject*, size_t)> & doneCallback, bool multiselect)
{
	g_selector.m_multiselect = multiselect;
	g_selector.DoneCallback = doneCallback;
	g_selector.m_prevTool = g_curTool;
	g_curTool = &g_selector;
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeBox;
	g_selector.SelectHandler = Functor<void, LOKI_TYPELIST_2(CadObject*,bool)>();
	g_canSnap = false;
	g_console.SetPrompt(prompt);
}


void ToolManager::DispatchTool(const wstring & id)
{
	g_defaultTool.Exiting();
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
	if (dynamic_cast<SelectBaseTool*>(pos->second) == 0)
		g_selected.clear();
	g_curTool = pos->second;
	g_curTool->Start();
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
