/*
 * globals.h
 *
 *  Created on: 28.01.2009
 *      Author: misha
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_


#include "console.h"
#include "exmath.h"
#include "resource.h"
#include <windows.h> // for HDC
#include <list>
#include <map>
#include <vector>
#include <loki/functor.h>


#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x20e
#endif

#ifndef GET_KEYSTATE_WPARAM
#define GET_KEYSTATE_WPARAM(x) LOWORD(x)
#endif

#define REGISTER_TOOL(id, toolClass, needSelection) \
	namespace Private \
	{ \
		struct toolClass ## Registrar \
		{ \
			toolClass ## Registrar() \
			{ \
				static toolClass instance; \
				g_toolManager.RegisterTool(id, &instance, needSelection); \
			} \
		} toolClass ## registrar; \
	}


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
	virtual void UpdateManip(const Point<double> & pt, int id) = 0;
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const = 0;
	virtual void Move(Point<double> displacement) = 0;
	virtual CadObject * Clone() = 0;
	virtual void Assign(CadObject * rhs) = 0;
	virtual size_t Serialize(unsigned char * ptr) const = 0;
	virtual void Load(unsigned char const *& ptr, size_t & size) = 0;
protected:
	CadObject() {}
	CadObject(const CadObject & orig) {}
	CadObject & operator=(const CadObject & orig) { return *this; }
};


class CadLine : public CadObject
{
public:
	static const int ID = 1;
	Point<double> Point1;
	Point<double> Point2;

	virtual void Draw(HDC hdc, bool selected) const;
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const;
	virtual Rect<double> GetBoundingRect() const { return Rect<double>(Point1, Point2).Normalized(); }
	virtual std::vector<Point<double> > GetManipulators() const;
	virtual void UpdateManip(const Point<double> & pt, int id);
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const;
	virtual void Move(Point<double> displacement);
	virtual CadLine * Clone();
	virtual void Assign(CadObject * rhs);
	void Assign(CadLine * rhs);
	virtual size_t Serialize(unsigned char * ptr) const;
	virtual void Load(unsigned char const *& ptr, size_t & size);
};


class CadPolyline : public CadObject
{
public:
	static const int ID = 4;
	struct Node
	{
		Point<double> Point;
		double Bulge;
	};
	std::list<Node> Nodes;
	bool Closed;
	CadPolyline() : Closed(false) {}
	virtual void Draw(HDC hdc, bool selected) const;
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const;
	virtual Rect<double> GetBoundingRect() const;
	virtual std::vector<Point<double> > GetManipulators() const;
	virtual void UpdateManip(const Point<double> & pt, int id);
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const;
	virtual void Move(Point<double> displacement);
	virtual CadPolyline * Clone() { return new CadPolyline(*this); }
	void Assign(CadPolyline * rhs) { *this = *rhs; }
	virtual size_t Serialize(unsigned char * ptr) const;
	virtual void Load(unsigned char const *& ptr, size_t & size);
protected:
	virtual void Assign(CadObject * rhs)
	{
		assert(typeid(*rhs) == typeid(CadPolyline));
		Assign(static_cast<CadPolyline*>(rhs));
	}
};


class CadCircle : public CadObject
{
public:
	static const int ID = 2;
	Point<double> Center;
	double Radius;
	virtual void Draw(HDC hdc, bool selected) const;
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const;
	virtual Rect<double> GetBoundingRect() const;
	virtual std::vector<Point<double> > GetManipulators() const;
	virtual void UpdateManip(const Point<double> & pt, int id);
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const;
	virtual void Move(Point<double> displacement);
	virtual CadCircle * Clone();
	virtual void Assign(CadObject * rhs);
	virtual size_t Serialize(unsigned char * ptr) const;
	virtual void Load(unsigned char const *& ptr, size_t & size);
};


class CadArc : public CadObject, public CircleArc<double>
{
public:
	static const int ID = 3;

	virtual void Draw(HDC hdc, bool selected) const;
	virtual bool IntersectsRect(double x1, double y1, double x2, double y2) const;
	virtual Rect<double> GetBoundingRect() const;
	virtual std::vector<Point<double> > GetManipulators() const;
	virtual void UpdateManip(const Point<double> & pt, int id);
	virtual std::vector<std::pair<Point<double>, PointType> > GetPoints() const;
	virtual void Move(Point<double> displacement);
	virtual CadArc * Clone();
	virtual void Assign(CadObject * rhs);
	void Assign(CadArc * rhs);
	virtual size_t Serialize(unsigned char * ptr) const;
	virtual void Load(unsigned char const *& ptr, size_t & size);
	CadArc & operator=(const CircleArc<double> & rhs) { CircleArc<double> & ca = *this; ca = rhs; return *this; }
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


class Tool
{
public:
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam) { return false; }
	virtual void Start() {}
	virtual void Cancel() { Exiting(); }
	virtual void Exiting() {}
	virtual void Command(const std::wstring & cmd) {}
protected:
	Tool() {}
	virtual ~Tool() {}
private:
	Tool(const Tool & orig);
	Tool & operator=(const Tool & orig);
};


class Selector : public Tool
{
public:
	Selector() : m_lassoOn(false), m_lassoDrawn(false) {}
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Cancel();
	Loki::Functor<void, LOKI_TYPELIST_2(CadObject*, bool)> SelectHandler;
	Tool * NextTool;
private:
	bool m_lassoOn;
	Point<double> m_lassoPt1, m_lassoPt2;
	bool m_lassoDrawn;
};


class DefaultTool : public Tool
{
public:
	DefaultTool() : m_state(Selecting) {}
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
	void DrawManipulators(HDC hdc);
	virtual void Cancel();
	virtual void Exiting();
	virtual void Command(const std::wstring & cmd);
	void RemoveManipulators(CadObject * obj);
private:
	enum State {Selecting, MovingManip};
	static const int MANIP_SIZE = 10; // pixels
	State m_state;
	std::list<Manipulator> m_manipulators;
	std::list<Manipulator>::iterator m_selManip;
	struct Manipulated
	{
		CadObject * Original;
		CadObject * Copy;
		int ManipId;
	};
	std::list<Manipulated> m_manipulated;
	std::auto_ptr<CadLine> m_fantomLine;
	void AddManipulators(CadObject * obj);
	void SelectHandler(CadObject * obj, bool select);
	void RecalcFantomsHandler();
	void ClearManipulated();
};


class ZoomTool : public Tool
{
public:
	ZoomTool() : m_zooming(false) {}
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
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
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
private:
	bool m_panning;
	int m_prevX;
	int m_prevY;
};


class CopyTool : public Tool
{
public:
	virtual void Start();
protected:
	bool InternalStart();
};


class CutTool : public CopyTool
{
public:
	virtual void Start();
};


class PasteTool : public Tool
{
public:
	virtual bool ProcessInput(HWND hwnd, unsigned int msg, WPARAM wparam, LPARAM lparam);
	virtual void Start();
	virtual void Command(const std::wstring & cmd);
	virtual void Exiting();
private:
	Point<double> m_basePoint;
	std::vector<CadObject *> m_objects;
	void DeleteCopies();
	void RecalcFantomsHandler();
	void CalcPositions(const Point<double> & pt);
	void FeedInsertionPoint(const Point<double> & pt);
};


class UndoTool : public Tool
{
public:
	virtual void Start();
};


class RedoTool : public Tool
{
public:
	virtual void Start();
};


class EraseTool : public Tool
{
public:
	virtual void Start();
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
	void AddFantom(CadObject * fantom) { m_fantoms.push_back(fantom); }
	void DrawFantoms(HDC hdc);
	void RecalcFantoms();
	void DeleteFantoms(bool update);
	void DeleteFantoms(HDC hdc);
	Loki::Functor<void> RecalcFantomsHandler;
private:
	std::list<CadObject *> m_fantoms;
};


class UndoItem
{
public:
	virtual ~UndoItem() {}
	virtual void Do() = 0;
	virtual void Undo() = 0;
	bool IsDone() { return m_done; }
protected:
	bool m_done;
	UndoItem(bool done) : m_done(done) {}
};


class ReverseUndoItem : public UndoItem
{
public:
	ReverseUndoItem(UndoItem * base, bool done = false) : UndoItem(done), m_base(base) {}
	virtual void Do() { m_base->Undo(); }
	virtual void Undo() { m_base->Do(); }
private:
	std::auto_ptr<UndoItem> m_base;
};


class GroupUndoItem : public UndoItem
{
public:
	GroupUndoItem(bool done = false) : UndoItem(done) {}
	~GroupUndoItem();
	virtual void Do();
	virtual void Undo();
	void AddItem(UndoItem * item) { m_items.push_back(item); }
private:
	typedef std::vector<UndoItem *> Items;
	Items m_items;
	friend class UndoManager;
};


class AddObjectUndoItem : public UndoItem
{
public:
	AddObjectUndoItem(CadObject * obj, bool done = false) :
		UndoItem(done), m_obj(obj)
		{ if (!done) m_ownedObj.reset(obj); }
	virtual void Do();
	virtual void Undo();
private:
	CadObject * m_obj;
	std::auto_ptr<CadObject> m_ownedObj;
};


class AssignObjectUndoItem : public UndoItem
{
public:
	AssignObjectUndoItem(CadObject * toObject, CadObject * fromObject, bool done = false) :
		UndoItem(done), m_toObject(toObject), m_fromObject(fromObject) {}
	virtual void Do();
	virtual void Undo();
private:
	CadObject * m_toObject;
	std::auto_ptr<CadObject> m_fromObject;
};


class UndoManager
{
public:
	UndoManager() : m_pos(m_items.begin()) {}
	~UndoManager();
	bool CanUndo();
	void Undo();
	bool CanRedo();
	void Redo();
	void AddWork(UndoItem * item);
	void AddGroupItem(std::auto_ptr<UndoItem> item);
	void RemoveGroupItem();
	void EndGroup();
private:
	typedef std::list<UndoItem *> Items;
	Items m_items;
	Items::iterator m_pos;
	std::auto_ptr<GroupUndoItem> m_group;
	void DeleteItems(Items::iterator pos);
};


class ToolManager
{
public:
	void RegisterTool(const std::wstring & id, Tool * tool, bool needSelection);
	void DispatchTool(const std::wstring & id);
private:
	struct ToolInfo
	{
		Tool * m_tool;
		bool m_needSelection;
	};
	typedef std::map<std::wstring, ToolInfo> ToolsMapType;
	ToolsMapType m_toolsMap;
};


extern Selector g_selector;
extern DefaultTool g_defaultTool;
extern ToolManager g_toolManager;
extern Tool * g_curTool;

extern FantomManager g_fantomManager;

extern UndoManager g_undoManager;

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

extern std::list<CadObject *> g_selected;
extern unsigned int g_clipboardFormat;


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

void ExitTool();

void DrawObjectSnap(HDC hdc, Point<int> pos, PointType type);

bool IsSelected(const CadObject * obj);


template<class T>
size_t WritePtr(unsigned char * &ptr, T val)
{
	if (ptr != 0)
	{
		*reinterpret_cast<T*>(ptr) = val;
		ptr += sizeof(T);
	}
	return sizeof(T);
}


template<class T>
void ReadPtr(unsigned char const * &ptr, T & val, size_t & size)
{
	assert(ptr != 0);
	assert(size >= sizeof(T));
	val = *reinterpret_cast<T const *>(ptr);
	ptr += sizeof(T);
	size -= sizeof(T);
}

void DeleteSelectedObjects();
void ExecuteCommand(const std::wstring & cmd);
void Cancel();
inline bool IsKey(const std::wstring & cmd, const std::wstring & key) { return wcsnicmp(cmd.c_str(), key.c_str(), cmd.size()) == 0; }

inline std::wstring ToLower(const std::wstring & str)
{
	std::wstring result(str.size(), L'\0');
	std::transform(str.begin(), str.end(), result.begin(), std::tolower);
	return result;
}

inline std::wstring IntToWstr(int value)
{
	wchar_t buffer[16];
	int len = std::swprintf(buffer, L"%d", value);
	assert(len > 0);
	return std::wstring(buffer, len);
}


inline bool TryParsePoint2D(const std::wstring & str, Point<double> & point)
{
	return (std::swscanf(str.c_str(), L"%lf,%lf", &point.X, &point.Y) == 2);
}


inline bool ParsePoint2D(const std::wstring & str, Point<double> & point)
{
	if (TryParsePoint2D(str, point))
		return true;
	g_console.Log(L"Invalid 2D point");
	return false;
}

#endif /* GLOBALS_H_ */
