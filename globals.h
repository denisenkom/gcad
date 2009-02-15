/*
 * globals.h
 *
 *  Created on: 28.01.2009
 *      Author: misha
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_


#include "exmath.h"
#include "resource.h"
#include <windows.h> // for HDC
#include <list>
#include <vector>
#include <loki/functor.h>


#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x20e
#endif

#ifndef GET_KEYSTATE_WPARAM
#define GET_KEYSTATE_WPARAM(x) LOWORD(x)
#endif


class WindowsError
{
};


class ClientDC
{
public:
	ClientDC(HWND hwnd)
	{
		m_hwnd = hwnd;
		m_hdc = GetDC(hwnd);
		if (m_hdc == 0)
			throw WindowsError();
	}
	~ClientDC() { ReleaseDC(m_hwnd, m_hdc); }
	operator HDC () { return m_hdc; }
private:
	HWND m_hwnd;
	HDC m_hdc;
};


enum PointType
{
	PointTypeEndPoint,
	PointTypeMiddle,
	PointTypeCenter,
	PointTypeQuadrant,
};


class CadObject
{
public:
	virtual ~CadObject() {}
	virtual void Draw(HDC hdc, bool selected) const = 0;
	bool IntersectsRect(const Rect<double> & rect) { return IntersectsRect(rect.Pt1.X, rect.Pt1.Y, rect.Pt2.X, rect.Pt2.Y); }
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const = 0;
	virtual Rect<double> GetBoundingRect() const = 0; // returns normalized bounding rectangle
	virtual std::vector<Point<double> > GetManipulators() const = 0;
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const = 0;
	virtual class Fantom * CreateFantom(int param) = 0;
	virtual void Move(Point<double> displacement) = 0;
	virtual CadObject * Clone() = 0;
	virtual void Assign(CadObject * rhs) = 0;
protected:
	CadObject() {}
	CadObject(const CadObject & orig) {}
	CadObject & operator=(const CadObject & orig) { return *this; }
};


class Fantom
{
public:
	virtual ~Fantom() {}
	virtual void Draw(HDC hdc) const = 0;
	virtual void Recalc() const = 0;
	virtual class CadObject * CreateCad() const = 0;
	virtual void AssignToCad(CadObject * to) const = 0;
protected:
	Fantom() {}
private:
	Fantom(const Fantom & orig);
	Fantom & operator=(const Fantom & orig);
};


class CadLine : public CadObject
{
public:
	Point<double> Point1;
	Point<double> Point2;

	virtual void Draw(HDC hdc, bool selected) const;
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const;
	virtual Rect<double> GetBoundingRect() const { return Rect<double>(Point1, Point2).Normalized(); }
	virtual std::vector<Point<double> > GetManipulators() const;
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const;
	virtual class Fantom * CreateFantom(int param);
	virtual void Move(Point<double> displacement);
	virtual CadObject * Clone();
	virtual void Assign(CadObject * rhs);
	void Assign(CadLine * rhs);
};


class FantomLine : public Fantom
{
public:
	FantomLine(Point<double> fixedPt) : m_fixedPt(fixedPt), m_movingPtNo(1) {}
	FantomLine(int movingPtNo, Point<double> fixedPt) : m_fixedPt(fixedPt), m_movingPtNo(movingPtNo) {}
	virtual void Draw(HDC hdc) const;
	virtual void Recalc() const {}
	virtual class CadObject * CreateCad() const;
	virtual void AssignToCad(CadObject * to) const;
	void AssignToCad(CadLine * to) const;
private:
	Point<double> m_fixedPt;
	int m_movingPtNo;
};


class CadCircle : public CadObject
{
public:
	Point<double> Center;
	double Radius;
	virtual void Draw(HDC hdc, bool selected) const;
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const;
	virtual Rect<double> GetBoundingRect() const;
	virtual std::vector<Point<double> > GetManipulators() const;
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const;
	virtual class Fantom * CreateFantom(int param);
	virtual void Move(Point<double> displacement);
	virtual CadCircle * Clone();
	virtual void Assign(CadObject * rhs);
};


class CadArc : public CadObject
{
public:
	Point<double> Center;
	Point<double> Start;
	Point<double> End;
	double Radius;
	bool Ccw;
	bool Straight;

	virtual void Draw(HDC hdc, bool selected) const;
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const;
	virtual Rect<double> GetBoundingRect() const;
	virtual std::vector<Point<double> > GetManipulators() const;
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const;
	virtual class Fantom * CreateFantom(int param);
	virtual void Move(Point<double> displacement);
	virtual CadObject * Clone();
	virtual void Assign(CadObject * rhs);
	void Assign(CadArc * rhs);
};


class FantomArc : public Fantom
{
public:
	FantomArc(Point<double> first, Point<double> second) : m_first(first), m_second(second), m_movingPtNo(2) {}
	FantomArc(int movingPtNo, Point<double> first, Point<double> second, Point<double> third) :
		m_first(first), m_second(second), m_third(third), m_movingPtNo(movingPtNo) {}
	virtual void Draw(HDC hdc) const;
	virtual void Recalc() const;
	virtual class CadObject * CreateCad() const;
	virtual void AssignToCad(CadObject * to) const;
private:
	// source points of arc
	mutable Point<double> m_first;
	Point<double> m_second;
	mutable Point<double> m_third;
	int m_movingPtNo;

	// calculated parameters of arc
	mutable Point<double> m_center;
	mutable double m_radius;
	mutable bool m_ccw;
	mutable bool m_straight;
};


class Document
{
public:
	std::list<CadObject *> Objects;
	~Document()
	{
		for (std::list<CadObject *>::iterator i = Objects.begin(); i != Objects.end(); i++)
			delete *i;
	}
};


class Manipulator
{
public:
	Point<double> Position;
	std::list<std::pair<CadObject *, int> > Links;
};


class Selector
{
public:
	Selector() : m_lassoOn(false), m_lassoDrawn(false) {}
	void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	void Cancel();
	Loki::Functor<void, LOKI_TYPELIST_2(CadObject*, bool)> SelectHandler;
private:
	bool m_lassoOn;
	Point<double> m_lassoPt1, m_lassoPt2;
	bool m_lassoDrawn;
};


class Tool
{
public:
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam) = 0;
	virtual void Start(const std::list<CadObject *> & selected) = 0;
	virtual void Cancel() {}
	virtual void Exiting() {}
protected:
	Tool() {}
	virtual ~Tool() {}
private:
	Tool(const Tool & orig);
	Tool & operator=(const Tool & orig);
};


class DefaultTool : public Tool
{
public:
	DefaultTool() : m_state(Selecting) {}
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start(const std::list<CadObject *> & selected);
	void DrawManipulators(HDC hdc);
	virtual void Cancel();
	virtual void Exiting();
private:
	enum State {Selecting, MovingManip};
	static const int MANIP_SIZE = 10; // pixels
	State m_state;
	std::list<Manipulator> m_manipulators;
	std::list<Manipulator>::iterator m_selManip;
	std::list<CadObject *> m_manipulated;
	void RemoveManipulators(CadObject * obj);
	void AddManipulators(CadObject * obj);
	void SelectHandler(CadObject * obj, bool select);
};


class ZoomTool : public Tool
{
public:
	ZoomTool() : m_zooming(false) {}
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start(const std::list<CadObject *> & selected);
	virtual void Cancel() {}
	virtual void Exiting() {}
private:
	bool m_zooming;
	int m_startX;
	int m_startY;
	float m_startMag;
	int m_startHscroll;
	int m_startVscroll;
	int m_vectorX;
	int m_vectorY;
};


class PanTool : public Tool
{
public:
	PanTool() : m_panning(false) {}
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start(const std::list<CadObject *> & selected);
	virtual void Cancel() {}
	virtual void Exiting() {}
private:
	bool m_panning;
	int m_prevX;
	int m_prevY;
};


class DrawLinesTool : public Tool
{
public:
	DrawLinesTool() : m_selectingSecondPoint(false) {}
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start(const std::list<CadObject *> & selected);
private:
	bool m_selectingSecondPoint;
	std::auto_ptr<CadLine> m_fantomLine;
	void RecalcFantomsHandler();
};


class DrawCircleTool : public Tool
{
public:
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start(const std::list<CadObject *> & selected);
private:
	enum State
	{
		StateSelectingCenter,
		StateSelectingRadius,
	};
	State m_state;
	std::auto_ptr<CadCircle> m_cadCircle;
	std::auto_ptr<CadLine> m_radiusLine;
	void RecalcFantomsHandler();
};


class DrawArcsTool : public Tool
{
public:
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start(const std::list<CadObject *> & selected);
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
};


class MoveTool : public Tool
{
public:
	virtual void ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start(const std::list<CadObject *> & selected);
	virtual void Cancel();
	virtual void Exiting();
private:
	enum State
	{
		StateSelecting,
		StateChoosingBasePoint,
		StateChoosingDestPoint,
	};
	State m_state;
	Point<double> m_basePoint;
	std::vector<CadObject *> m_objects;
	std::auto_ptr<CadLine> m_fantomLine;
	void DeleteCopies();
	void RecalcFantomsHandler();
};

enum CursorType { CursorTypeManual, CursorTypeSystem };
enum CustomCursorType { CustomCursorTypeSelect, CustomCursorTypeCross, CustomCursorTypeBox };

struct ToolInfo
{
public:
	int Id;
	::Tool & Tool;
};

namespace Private
{
	struct LocalAllocStrRef
	{
		wchar_t * m_buf;
		explicit LocalAllocStrRef(wchar_t * buf) : m_buf(buf) {}
	};
}


class LocalAllocStr
{
public:
	LocalAllocStr(LocalAllocStr & orig) { m_buf = orig.Release(); }
	LocalAllocStr(Private::LocalAllocStrRef orig) : m_buf(orig.m_buf) {}
	~LocalAllocStr() { if (m_buf != 0) if (LocalFree(m_buf) != 0) assert(0); }
	const wchar_t * c_str() const { assert(m_buf != 0); return m_buf; }
	operator LocalAllocStr() { return LocalAllocStr(this->Release()); }
	operator Private::LocalAllocStrRef() { return Private::LocalAllocStrRef(this->Release()); }

	inline friend LocalAllocStr GetWinErrorStr(unsigned long err = GetLastError())
	{
		LocalAllocStr result;
		if (!FormatMessageW(
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				0, err, 0, reinterpret_cast<wchar_t*>(&result.m_buf), 0, 0))
		{
			assert(0);
		}
		return result;
	}
private:
	wchar_t * m_buf;

	LocalAllocStr() : m_buf(0) {}
	explicit LocalAllocStr(wchar_t * buf) : m_buf(buf) {}
	LocalAllocStr & operator=(LocalAllocStr & orig) { this->~LocalAllocStr(); m_buf = orig.Release(); return *this; }
	wchar_t * Release() { wchar_t * result = m_buf; m_buf = 0; return result; }
};
LocalAllocStr fn();


class WinHandle
{
public:
	WinHandle(HANDLE h) : m_h(h) {}
	~WinHandle() { if (!!*this) if (!CloseHandle(m_h)) assert(0); }
	bool operator ! () { return m_h == INVALID_HANDLE_VALUE || m_h == 0; }
	HANDLE get() { assert(!!*this); return m_h; }
private:
	HANDLE m_h;
};


class FantomManager
{
public:
	void AddFantom(Fantom * fantom) { g_fantoms.push_back(fantom); }
	void AddFantom(CadObject * fantom) { m_fantoms2.push_back(fantom); }
	void DrawFantoms(HDC hdc);
	void RecalcFantoms();
	void DeleteFantoms(bool update);
	void DeleteFantoms(HDC hdc);
	std::list<Fantom *> g_fantoms;
	Loki::Functor<void> RecalcFantomsHandler;
private:
	std::list<CadObject *> m_fantoms2;
};


extern Selector g_selector;
extern DefaultTool g_defaultTool;
extern PanTool g_panTool;
extern ZoomTool g_zoomTool;
extern DrawLinesTool g_drawLinesTool;
extern DrawCircleTool g_drawCircleTool;
extern DrawArcsTool g_drawArcsTool;
extern MoveTool g_moveTool;
extern Tool * g_curTool;

extern FantomManager g_fantomManager;

const ToolInfo TOOLS[] = {
	{ID_VIEW_SELECT, g_defaultTool},
	{ID_VIEW_PAN, g_panTool},
	{ID_VIEW_ZOOM, g_zoomTool},
	{ID_DRAW_LINES, g_drawLinesTool},
	{ID_DRAW_CIRCLE, g_drawCircleTool},
	{ID_DRAW_ARCS, g_drawArcsTool},
	{ID_MODIFY_MOVE, g_moveTool},
};
const int NUM_TOOLS = sizeof(TOOLS) / sizeof(TOOLS[0]);


const wchar_t MAINWNDCLASS[] = L"GCadMainWindow";
const wchar_t MAINCLIENTCLASS[] = L"GCadClientWindow";

extern HINSTANCE g_hInstance;
extern HWND g_hmainWindow;
extern HWND g_htoolbar;
extern HWND g_hclientWindow;
extern HPEN g_gridHPen;
extern HPEN g_objectSnapHPen;
extern HBRUSH g_manipHBrush;
extern HBRUSH g_selectedManipHBrush;

extern Point<double> g_extentMin;
extern Point<double> g_extentMax;
extern double g_gridStep;
extern bool g_snapEnable;
extern double g_snapStep;
extern bool g_objectSnapEnable;

extern float g_magification;
extern int g_viewHeight;
extern int g_viewWidth;
extern int g_vrangeMin;
extern int g_vrangeMax;
extern int g_vscrollMin;
extern int g_vscrollMax;
extern int g_vscrollPos;
extern int g_hrangeMin;
extern int g_hrangeMax;
extern int g_hscrollMin;
extern int g_hscrollMax;
extern int g_hscrollPos;

extern CursorType g_cursorType;
extern CustomCursorType g_customCursorType;
extern HPEN g_cursorHPen;
extern bool g_cursorDrawn;
extern bool g_mouseInsideClient;
extern Point<int> g_cursorScn;
extern Point<double> g_cursorWrld;
extern HCURSOR g_cursorHandle;

extern HPEN g_lineHPen;
extern HPEN g_selectedLineHPen;

extern Document g_doc;

extern bool g_objSnapDrawn;
extern PointType g_objSnapType;
extern Point<int> g_objSnapPos;
extern bool g_canSnap;


Point<int> WorldToScreen(float x, float y);

inline Point<int> WorldToScreen(double x, double y)
{
	return WorldToScreen(static_cast<float>(x), static_cast<float>(y));
}

inline Point<int> WorldToScreen(Point<double> pt)
{
	return WorldToScreen(pt.X, pt.Y);
}

inline Point<int> WorldToScreen(Point<float> pt)
{
	return WorldToScreen(pt.X, pt.Y);
}


inline Point<float> ScreenToWorld(int x, int y)
{
	Point<float> result;
	result.X = static_cast<float>(g_extentMin.X) + (x + g_hscrollPos) / g_magification;
	result.Y = static_cast<float>(g_extentMax.Y) - (y + g_vscrollPos) / g_magification;
	return result;
}

inline Point<float> ScreenToWorld(Point<int> pt)
{
	return ScreenToWorld(pt.X, pt.Y);
}

void DrawCursorRaw(HDC hdc, int x, int y);


inline void DrawCursor(HDC hdc)
{
	SelectObject(hdc, g_cursorHPen);
	SetROP2(hdc, R2_XORPEN);
	DrawCursorRaw(hdc, g_cursorScn.X, g_cursorScn.Y);
}


void Zoom(float prevMag, float deltaMag, int hscrollPos, int vscrollPos, int x, int y);
void ExtendHScrollLimits(SCROLLINFO & si);
void ExtendVScrollLimits(SCROLLINFO & si);

inline const ToolInfo & ToolById(int id)
{
	for (int i = 0; i < NUM_TOOLS; i++)
	{
		if (TOOLS[i].Id == id)
			return TOOLS[i];
	}
	assert(0);
}

void SelectTool(int toolId);
void ExitTool();

void DrawObjectSnap(HDC hdc, Point<int> pos, PointType type);

bool IsSelected(const CadObject * obj);

#endif /* GLOBALS_H_ */
