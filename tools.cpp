/*
 * tools.cpp
 *
 *  Created on: 26.02.2009
 *      Author: misha
 */

#include "tools.h"
#include "console.h"
#include <loki/functor.h>
#include <loki/multimethods.h>
#include <loki/typelistmacros.h>
#include <algorithm>
#include <functional>


using namespace std;
using namespace Loki;


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


REGISTER_TOOL(L"line", DrawLinesTool);


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


REGISTER_TOOL(L"pline", DrawPLineTool);


void DrawPLineTool::Start()
{
	g_cursorType = CursorTypeManual;
	g_cursorHandle = 0;
	g_customCursorType = CustomCursorTypeCross;
	g_canSnap = true;
	g_console.SetPrompt(L"Specify first point:");
	g_fantomManager.RecalcFantomsHandler = Functor<void>(this, &DrawPLineTool::RecalcFantomsHandler);
	m_state = StateSelFirstPt;
}


bool DrawPLineTool::ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch (m_state)
	{
	case StateSelFirstPt:
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			g_console.LogCommand();
			FeedFirstPoint(g_cursorWrld);
			return true;
		default:
			return false;
		}
	case StateSelLineSecondPt:
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			g_console.LogCommand();
			FeedLineSecondPoint(g_cursorWrld);
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
	case StateSelArcEndPt:
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			g_console.LogCommand();
			FeedArcEndPoint(g_cursorWrld);
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
	case StateSelArcDirection:
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			g_console.LogCommand();
			FeedArcDirection(g_cursorWrld);
			return true;
		default:
			return false;
		}
	default:
		assert(0);
		return false;
	}
}


void DrawPLineTool::Command(const wstring & cmd)
{
	Point<double> pt;
	switch (m_state)
	{
	case StateSelFirstPt:
		if (ParsePoint2D(cmd, pt))
			FeedFirstPoint(pt);
		break;
	case StateSelLineSecondPt:
		if (IsKey(cmd, L"arc"))
		{
			g_fantomManager.DeleteFantoms(false);
			m_fantomArc.reset(new CadArc);
			*m_fantomArc = ArcFrom2PtAndNormTangent(m_fantomLine->Point1, m_arcDir, g_cursorWrld);
			g_fantomManager.AddFantom(m_fantomArc.get());
			g_fantomManager.AddFantom(m_fantomLine.get());
			InvalidateRect(g_hclientWindow, 0, true);
			m_state = StateSelArcEndPt;
			SetPrompt();
		}
		else if (m_result && m_result->Nodes.size() >= 2 &&
				IsKey(cmd, L"close"))
		{
			m_result->Closed = true;
			m_result->Nodes.back().Bulge = 0;
			InvalidateRect(g_hclientWindow, 0, true);
			ExitTool();
		}
		else if (ParsePoint2D(cmd, pt))
		{
			FeedLineSecondPoint(pt);
		}
		break;
	case StateSelArcEndPt:
		if (IsKey(cmd, L"line"))
		{
			g_fantomManager.DeleteFantoms(false);
			g_fantomManager.AddFantom(m_fantomLine.get());
			InvalidateRect(g_hclientWindow, 0, true);
			m_state = StateSelLineSecondPt;
			SetPrompt();
		}
		else if (m_result && m_result->Nodes.size() >= 2 &&
				IsKey(cmd, L"close"))
		{
			m_result->Closed = true;
			*m_fantomArc = ArcFrom2PtAndNormTangent(m_fantomArc->Start, m_arcDir, m_result->Nodes.front().Point);
			m_result->Nodes.back().Bulge = m_fantomArc->CalcBulge();
			InvalidateRect(g_hclientWindow, 0, true);
			ExitTool();
		}
		else if (IsKey(cmd, L"direction"))
		{
			m_state = StateSelArcDirection;
			g_fantomManager.DeleteFantoms(false);
			g_fantomManager.AddFantom(m_fantomLine.get());
			InvalidateRect(g_hclientWindow, 0, true);
			SetPrompt();
		}
		else if (ParsePoint2D(cmd, pt))
		{
			FeedArcEndPoint(pt);
		}
		break;
	case StateSelArcDirection:
		if (ParsePoint2D(cmd, pt))
			FeedArcDirection(pt);
		break;
	}
}


void DrawPLineTool::Exiting()
{
	if (m_result == 0)
		return;
	if (m_result->Nodes.size() <= 1)
	{
		g_doc.Objects.remove(m_result);
		delete m_result;
	}
	else
	{
		g_undoManager.AddWork(new AddObjectUndoItem(m_result, true));
	}
	m_result = 0;
	g_fantomManager.RecalcFantomsHandler = Functor<void>();
	m_fantomLine.reset(0);
	m_fantomArc.reset(0);
}


void DrawPLineTool::SetPrompt()
{
	switch (m_state)
	{
	case StateSelLineSecondPt:
		if (m_result && m_result->Nodes.size() >= 2)
			g_console.SetPrompt(L"Specify next point or [Arc/Close]:");
		else
			g_console.SetPrompt(L"Specify next point or [Arc]:");
		break;
	case StateSelArcEndPt:
		if (m_result && m_result->Nodes.size() >= 2)
			g_console.SetPrompt(L"Specify end point of arc or [Line/Direction/Close]:");
		else
			g_console.SetPrompt(L"Specify end point of arc or [Line/Direction]:");
		break;
	case StateSelArcDirection:
		g_console.SetPrompt(L"Specify tangent for arc's start point:");
		break;
	default:
		assert(0);
		break;
	}
}


void DrawPLineTool::RecalcFantomsHandler()
{
	switch (m_state)
	{
	case StateSelFirstPt:
		break;
	case StateSelLineSecondPt:
		m_fantomLine->Point2 = g_cursorWrld;
		break;
	case StateSelArcEndPt:
		*m_fantomArc = ArcFrom2PtAndNormTangent(m_fantomArc->Start, m_arcDir, g_cursorWrld);
		m_fantomLine->Point2 = g_cursorWrld;
		break;
	case StateSelArcDirection:
		m_fantomLine->Point2 = g_cursorWrld;
		break;
	}
}


void DrawPLineTool::FeedFirstPoint(const Point<double> & pt)
{
	m_fantomLine.reset(new CadLine);
	m_fantomLine->Point1 = pt;
	m_fantomLine->Point2 = g_cursorWrld;
	g_fantomManager.AddFantom(m_fantomLine.get());
	m_state = StateSelLineSecondPt;
	SetPrompt();
	m_result = new CadPolyline;
	g_doc.Objects.push_back(m_result);
	CadPolyline::Node node;
	node.Bulge = 0;
	node.Point = pt;
	m_result->Nodes.push_back(node);
	InvalidateRect(g_hclientWindow, 0, true);
}


void DrawPLineTool::FeedLineSecondPoint(const Point<double> & pt)
{
	if (pt == m_fantomLine->Point1)
	{
		g_console.Log(L"Points must be different");
		return;
	}
	CadPolyline::Node node;
	node.Bulge = 0;
	node.Point = pt;
	m_arcDir = (pt - m_fantomLine->Point1).Normalize();
	m_result->Nodes.push_back(node);
	m_fantomLine->Point1 = pt;
	m_fantomLine->Point2 = g_cursorWrld;
	InvalidateRect(g_hclientWindow, 0, true);
	SetPrompt();
}


void DrawPLineTool::FeedArcEndPoint(const Point<double> & pt)
{
	if (pt == m_fantomArc->Start)
	{
		g_console.Log(L"Points must be different");
		return;
	}
	*m_fantomArc = ArcFrom2PtAndNormTangent(m_fantomArc->Start, m_arcDir, pt);
	if (m_fantomArc->Radius == 0)
	{
		g_console.Log(L"Invalid arc");
		return;
	}
	m_result->Nodes.back().Bulge = m_fantomArc->CalcBulge();
	CadPolyline::Node node;
	node.Bulge = 0;
	node.Point = pt;
	m_result->Nodes.push_back(node);
	m_arcDir = DirVector((m_fantomArc->End - m_fantomArc->Center).Angle() + (m_fantomArc->Ccw ? M_PI/2 : -M_PI/2));
	*m_fantomArc = ArcFrom2PtAndNormTangent(pt, m_arcDir, g_cursorWrld);
	m_fantomLine->Point1 = pt;
	m_fantomLine->Point2 = g_cursorWrld;
	InvalidateRect(g_hclientWindow, 0, true);
	SetPrompt();
}


void DrawPLineTool::FeedArcDirection(const Point<double> & endpt)
{
	if (endpt == m_fantomLine->Point1)
	{
		g_console.Log(L"Points must be different");
		return;
	}
	m_arcDir = (endpt - m_fantomLine->Point1).Normalize();
	m_state = StateSelArcEndPt;
	m_fantomArc->Start = m_fantomLine->Point1;
	m_fantomLine->Point2 = m_fantomArc->End = endpt;
	m_fantomArc->Radius = 0;
	g_fantomManager.AddFantom(m_fantomArc.get());
	InvalidateRect(g_hclientWindow, 0, true);
	SetPrompt();
}


REGISTER_TOOL(L"circle", DrawCircleTool);


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
	CircleArc arc = ArcFrom3Pt(m_firstPoint, m_secondPoint, thirdPoint);
	if (arc.Radius == 0)
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


REGISTER_TOOL(L"arc", DrawArcsTool);


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
	*m_fantomArc = ArcFrom3Pt(m_fantomArc->Start, m_secondPoint, thirdPoint);
}


typedef SelectWrapperTool<MoveTool> WrappedMoveTool;
REGISTER_TOOL(L"move", WrappedMoveTool);


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


REGISTER_TOOL(L"trim", TrimTool);


void TrimTool::Start()
{
	BeginSelecting(L"Select cutting edges:", Functor<void, LOKI_TYPELIST_2(CadObject*, size_t)>(this, &TrimTool::SelectedEdgesHandler), true);
}


void TrimTool::Exiting()
{
	g_selected.clear();
}


void TrimTool::SelectedEdgesHandler(CadObject*, size_t)
{
	g_console.LogCommand();
	if (g_selected.size() == 0)
	{
		ExitTool();
		return;
	}
	m_bounds.assign(g_selected.begin(), g_selected.end());
	BeginSelecting(L"Select object to trim:", Functor<void, LOKI_TYPELIST_2(CadObject*, size_t)>(this, &TrimTool::SelectedObjectToTrimHandler), false);
}


void TrimTool::SelectedObjectToTrimHandler(CadObject * obj, size_t num)
{
	assert(num == 1);
	g_console.LogCommand();
	MakeTrim(obj);
	BeginSelecting(L"Select object to trim:", Functor<void, LOKI_TYPELIST_2(CadObject*, size_t)>(this, &TrimTool::SelectedObjectToTrimHandler), false);
}


template <class T>
vector<Point<double> > Intersect(const T & lhs, const CadPolyline & polyline1)
{
	vector<Point<double> > res;
	CadPolyline2 polyline = polyline1;
	for (CadPolyline2Iterator i = polyline.Begin(); i != polyline.End(); i++)
	{
		vector<Point<double> > subres = Intersect2(lhs, **i);
		res.insert(res.end(), subres.begin(), subres.end());
	}
	return res;
}


struct Intersector
{
	template <class T1, class T2>
	vector<Point<double> > Fire(const T1 & lhs, const T2 & rhs)
	{
		return Intersect(lhs, rhs);
	}

	template<class T>
	vector<Point<double> > Fire(const CadPolyline & polyline, const T & rhs)
	{
		assert(0);
	}

	vector<Point<double> > Fire(const CadPolyline & lhs, const CadPolyline & rhs)
	{
		assert(0);
	}

	vector<Point<double> > OnError(const CadObject & lhs, const CadObject & rhs) { assert(0); }
};

vector<Point<double> > Intersect2(const CadObject & lhs, const CadObject & rhs)
{
	return Loki::StaticDispatcher<Intersector,
		const CadObject, LOKI_TYPELIST_4(const CadLine, const CadCircle, const CadArc, const CadPolyline),
		true,
		const CadObject, LOKI_TYPELIST_4(const CadLine, const CadCircle, const CadArc, const CadPolyline),
		vector<Point<double> > >::Go(lhs, rhs, Intersector());
}


template <class T>
vector<Point<double> > Intersect2(const T & lhs, const CadObject & rhs)
{
	struct RhsDispatch : IConstCadObjVisitor
	{
		RhsDispatch(const T & lhs) : m_lhs(lhs) {}
		const T & m_lhs;
		vector<Point<double> > m_result;

		virtual void Visit(const CadLine & rhs)
		{
			m_result = Intersect(m_lhs, rhs);
		}
		virtual void Visit(const CadCircle & rhs)
		{
			m_result = Intersect(m_lhs, rhs);
		}
		virtual void Visit(const CadArc & rhs)
		{
			m_result = Intersect(m_lhs, rhs);
		}
		virtual void Visit(const CadPolyline & rhs)
		{
			m_result = Intersect(m_lhs, rhs);
		}
	} dispatch(lhs);
	rhs.Accept(dispatch);
	return dispatch.m_result;
}


class TrimVisitor : public ICadObjVisitor
{
	const vector<CadObject *> & m_bounds;
public:
	TrimVisitor(const vector<CadObject *> & bounds) : m_bounds(bounds) {}
private:
	template <class T>
	struct Comp : binary_function<Point<double>, Point<double>, bool>
	{
		Comp(const T & line) : m_line(line) {}
		bool operator()(const Point<double> & lhs, const Point<double> & rhs) const
		{
			return m_line.PointDist(lhs) < m_line.PointDist(rhs);
		}
		const T & m_line;
	};

	template <class T>
	void GenericTrim(T & line)
	{
		vector<Point<double> > intersections;
		for (vector<CadObject*>::const_iterator i = m_bounds.begin();
			i != m_bounds.end(); i++)
		{
			vector<Point<double> > res = Intersect2(line, **i);
			// removing intersections with end points
			for (vector<Point<double> >::const_iterator i = res.begin();
				i != res.end(); i++)
			{
				if (EqualsEpsilon(line.GetStart(), *i) || EqualsEpsilon(line.GetEnd(), *i))
					continue;
				intersections.push_back(*i);
			}
		}
		if (intersections.size() == 0)
			return;
		// sorting intersection points by distance from first point of line
		sort(intersections.begin(), intersections.end(), Comp<T>(line));
		// searching between which consecutive intersections lays cursor point
		vector<Point<double> >::const_iterator pos = find_if(intersections.begin(),
				intersections.end(), bind1st(Comp<T>(line), g_cursorWrld));
		// making trimming
		if (pos == intersections.end())
		{
			// cursor is between last intersection and end point, trimming end of line
			line.SetEnd(intersections.back());
		}
		else if (pos == intersections.begin())
		{
			// cursor is between start point of line and first intersection,
			// trimming beginning of line
			line.SetStart(intersections.front());
		}
		else
		{
			// cursor in between two intersections, splitting line
			T * newline = new T(line);
			newline->SetStart(*pos);
			line.SetEnd(*(pos - 1));
			g_doc.Objects.push_back(newline);
		}
	}

	virtual void Visit(CadLine & line)
	{
		GenericTrim(line);
	}

	virtual void Visit(CadArc & arc)
	{
		GenericTrim(arc);
	}

	struct CircleComp : binary_function<Point<double>, Point<double>, bool>
	{
		CircleComp(const Point<double> & center) : m_center(center) {}
		bool operator()(const Point<double> & lhs, const Point<double> & rhs) const
		{
			return (lhs - m_center).Angle() < (rhs - m_center).Angle();
		}
		Point<double> m_center;
	};

	virtual void Visit(CadCircle & circle)
	{
		vector<Point<double> > intersections;
		for (vector<CadObject*>::const_iterator i = m_bounds.begin();
			i != m_bounds.end(); i++)
		{
			vector<Point<double> > res = Intersect2(circle, **i);
			intersections.insert(intersections.end(), res.begin(), res.end());
		}
		if (intersections.size() == 0)
			return;
		if (intersections.size() < 2)
		{
			g_console.Log(L"Circle must be intersected in, at least, two points");
			return;
		}
		// sorting intersections by angle
		sort(intersections.begin(), intersections.end(), CircleComp(circle.Center));
		// searching where cursor point is
		Point<double> pt1, pt2;
		vector<Point<double> >::const_iterator iint = find_if(intersections.begin(),
				intersections.end(), bind1st(CircleComp(circle.Center), g_cursorWrld));
		if (iint == intersections.begin() || iint == intersections.end())
		{
			pt1 = intersections.front();
			pt2 = intersections.back();
		}
		else
		{
			pt1 = *iint;
			pt2 = *(iint-1);
		}
		// replacing circle with arc
		list<CadObject*>::iterator pos = find(g_doc.Objects.begin(), g_doc.Objects.end(), &circle);
		assert(pos != g_doc.Objects.end());
		auto_ptr<CadArc> arc(new CadArc(circle, pt1, pt2, true));
		delete *pos;
		*pos = arc.release();
	}

	void AddBeginCutted(CadPolyline2 & polyline,
			pair<CadPolyline2::Iterator, Point<double> > intersection)
	{
		IPolylineSeg * cutted = (*intersection.first)->CutBegin(intersection.second);
		if (cutted)
			polyline.m_elements.push_back(cutted);
	}

	void AddEndCutted(CadPolyline2 & polyline,
			pair<CadPolyline2::Iterator, Point<double> > intersection)
	{
		IPolylineSeg * cutted = (*intersection.first)->CutEnd(intersection.second);
		if (cutted)
			polyline.m_elements.push_back(cutted);
	}

	virtual void Visit(CadPolyline & polyline1)
	{
		CadPolyline2 polyline = polyline1;
		vector<pair<CadPolyline2::Iterator, Point<double> > > intersections;
		for (vector<CadObject*>::const_iterator ibound = m_bounds.begin();
			ibound != m_bounds.end(); ibound++)
		{
			for (CadPolyline2::Iterator iseg = polyline.Begin();
				iseg != polyline.End(); iseg++)
			{
				vector<Point<double> > subres = Intersect2(static_cast<const CadObject&>(**iseg), **ibound);
				for (vector<Point<double> >::const_iterator ipoint = subres.begin();
					ipoint != subres.end(); ipoint++)
				{
					if (!polyline.Closed() &&
							(EqualsEpsilon(polyline.Front()->GetStart(), *ipoint) ||
									EqualsEpsilon(polyline.Back()->GetEnd(), *ipoint)))
					{
						continue;
					}
					intersections.push_back(make_pair(iseg, *ipoint));
				}
			}
		}
		if (intersections.size() == 0)
			return;
		if (polyline.Closed() && intersections.size() < 2)
		{
			g_console.Log(L"Closed polyline must be intersected in, at least, two points");
			return;
		}
		// sorting intersections by range
		struct Private1
		{
			static bool Lesser(pair<CadPolyline2::Iterator, Point<double> > lhs, pair<CadPolyline2::Iterator, Point<double> > rhs)
			{
				if (lhs.first == rhs.first)
					return (*lhs.first)->PointDist(lhs.second) < (*lhs.first)->PointDist(rhs.second);
				else
					return lhs.first < rhs.first;
			}
		};
		sort(intersections.begin(), intersections.end(), ptr_fun(Private1::Lesser));
		// finding where cursor
		// pos points to nearest intersection after cursor
		// pos-1 points to nearest intersection before cursor
		struct Private2
		{
			static bool Greater(pair<CadPolyline2::Iterator, Point<double> > lhs, Point<double> rhs)
			{
				return (*lhs.first)->PointDist(lhs.second) > (*lhs.first)->PointDist(rhs);
			}
		};
		vector<pair<CadPolyline2::Iterator, Point<double> > >::iterator pos =
			find_if(intersections.begin(), intersections.end(), bind2nd(ptr_fun(Private2::Greater), g_cursorWrld));
		if (polyline.Closed())
		{
			if (pos == intersections.begin() || pos == intersections.end())
			{
				CadPolyline2 newpl;
				newpl.m_closed = false;
				AddBeginCutted(newpl, intersections.front());
				for (CadPolyline2::Iterator i = intersections.front().first + 1; i != intersections.back().first; i++)
					newpl.m_elements.push_back((*i)->Clone());
				AddEndCutted(newpl, intersections.back());
				polyline1.Assign(newpl.ToCadPolyline());
			}
			else
			{
				CadPolyline2 newpl;
				newpl.m_closed = false;
				AddBeginCutted(newpl, *pos);
				for (CadPolyline2::Iterator i = pos->first + 1; i != polyline.End(); i++)
					newpl.m_elements.push_back((*i)->Clone());
				for (CadPolyline2::Iterator i = polyline.Begin(); i != (pos-1)->first; i++)
					newpl.m_elements.push_back((*i)->Clone());
				AddEndCutted(newpl, *(pos-1));
				polyline1.Assign(newpl.ToCadPolyline());
			}
		}
		else
		{
			if (pos == intersections.begin())
			{
				// cutting beginning
				CadPolyline2 newpl;
				newpl.m_closed = false;
				AddBeginCutted(newpl, intersections.front());
				for (CadPolyline2::Iterator i = intersections.front().first + 1; i != polyline.End(); i++)
					newpl.m_elements.push_back((*i)->Clone());
				polyline1.Assign(newpl.ToCadPolyline());
			}
			else if (pos == intersections.end())
			{
				// cutting ending
				CadPolyline2 newpl;
				newpl.m_closed = false;
				for (CadPolyline2::Iterator i = polyline.Begin(); i != intersections.back().first; i++)
					newpl.m_elements.push_back((*i)->Clone());
				AddEndCutted(newpl, intersections.back());
				polyline1.Assign(newpl.ToCadPolyline());
			}
			else
			{
				// cutting middle
				CadPolyline2 newpl1;
				newpl1.m_closed = false;
				for (CadPolyline2::Iterator i = polyline.Begin(); i != (pos-1)->first; i++)
					newpl1.m_elements.push_back((*i)->Clone());
				AddEndCutted(newpl1, *(pos-1));
				polyline1.Assign(newpl1.ToCadPolyline());

				CadPolyline2 newpl2;
				newpl2.m_closed = false;
				AddBeginCutted(newpl2, *pos);
				for (CadPolyline2::Iterator i = pos->first + 1; i != polyline.End(); i++)
					newpl2.m_elements.push_back((*i)->Clone());
				g_doc.Objects.push_back(new CadPolyline(newpl2.ToCadPolyline()));
			}
		}
	}
};


void TrimTool::MakeTrim(CadObject * obj)
{
	TrimVisitor trimmer(m_bounds);
	obj->Accept(trimmer);
	InvalidateRect(g_hclientWindow, 0, true);
}
