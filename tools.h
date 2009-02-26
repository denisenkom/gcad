/*
 * tools.h
 *
 *  Created on: 26.02.2009
 *      Author: misha
 */

#ifndef TOOLS_H_
#define TOOLS_H_


#include "exmath.h"
#include "globals.h"


class DrawLinesTool : public Tool
{
public:
	DrawLinesTool() {}
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
	virtual void Command(const std::wstring & cmd);
	virtual void Exiting();
private:
	std::list<Point<double> > m_points;
	std::auto_ptr<CadLine> m_fantomLine;
	void RecalcFantomsHandler();
	void FeedPoint(const Point<double> & pt);
	void UpdatePrompt();
};


class DrawPLineTool : public Tool
{
public:
	DrawPLineTool() : m_arcDir(1, 0) {}
	virtual void Start();
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Command(const std::wstring & cmd);
	virtual void Exiting();
private:
	enum State
	{
		StateSelFirstPt,
		StateSelLineSecondPt,
		StateSelArcEndPt,
		StateSelArcDirection,
	};
	State m_state;
	std::auto_ptr<CadLine> m_fantomLine;
	std::auto_ptr<CadArc> m_fantomArc;
	CadPolyline * m_result;
	Point<double> m_arcDir;
	void SetPrompt();
	void RecalcFantomsHandler();
	void FeedFirstPoint(const Point<double> & pt);
	void FeedLineSecondPoint(const Point<double> & pt);
	void FeedArcEndPoint(const Point<double> & pt);
	void FeedArcDirection(const Point<double> & endpt);
};


class DrawCircleTool : public Tool
{
public:
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
	virtual void Command(const std::wstring & cmd);
private:
	enum State
	{
		StateSelectingCenter,
		StateSelectingRadius,
		StateSelectingDiameter,
		State2PtSelectingFirstPoint,
		State2PtSelectingSecondPoint,
		State3PtSelectingFirstPoint,
		State3PtSelectingSecondPoint,
		State3PtSelectingThirdPoint,
	};
	State m_state;
	std::auto_ptr<CadCircle> m_cadCircle;
	std::auto_ptr<CadLine> m_line;
	Point<double> m_firstPoint, m_secondPoint;
	void RecalcFantomsHandler();
	void FeedCenter(const Point<double> & center);
	void FeedRadius(double radius);
	void Feed2PtFirstPoint(const Point<double> & pt);
	void Feed2PtSecondPoint(const Point<double> & pt);
	void Feed3PtFirstPoint(const Point<double> & pt);
	void Feed3PtSecondPoint(const Point<double> & pt);
	void Feed3PtThirdPoint(const Point<double> & pt);
	void CalcCircleFrom3Pt(const Point<double> & thirdPoint);
};


class DrawArcsTool : public Tool
{
public:
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
	virtual void Command(const std::wstring & cmd);
private:
	enum State
	{
		StateSelectingFirstPoint,
		StateSelectingSecondPoint,
		StateSelectingThirdPoint
	};
	State m_state;
	std::auto_ptr<CadLine> m_fantomLine;
	Point<double> m_secondPoint;
	std::auto_ptr<CadArc> m_fantomArc;
	void RecalcFantomsHandler();
	void FeedFirstPoint(const Point<double> & pt);
	void FeedSecondPoint(const Point<double> & pt);
	void FeedThirdPoint(const Point<double> & pt);
	void CalcArcFrom3Pt(const Point<double> & thirdPoint);
};


class MoveTool : public Tool
{
public:
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
	virtual void Command(const std::wstring & cmd);
	virtual void Exiting();
private:
	enum State
	{
		StateChoosingBasePoint,
		StateChoosingDestPoint,
	};
	State m_state;
	Point<double> m_basePoint;
	std::vector<CadObject *> m_objects;
	std::auto_ptr<CadLine> m_fantomLine;
	void DeleteCopies();
	void RecalcFantomsHandler();
	void CalcPositions(const Point<double> & pt);
	void FeedBasePoint(const Point<double> & pt);
	void FeedDestPoint(const Point<double> & pt);
};


#endif /* TOOLS_H_ */
