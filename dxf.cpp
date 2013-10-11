/*
 * dxf.cpp
 *
 *  Created on: 25.02.2009
 *      Author: misha
 */
#include "dxf.h"
#include "globals.h" // for LocalAllocStr and GetWinErrorStr
#include <string>
#include <limits>


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


double DxfStrToDouble(const char * str);


using namespace std;


string DxfReader::ReadLine()
{
	string line;
	while (true)
	{
		if (m_lastPos == m_bufSize)
		{
			DWORD read;
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
		throw wstring(L"Error reading file: ") + ex.GetStr().c_str();
	}
	if (codeStr.size() == 0)
		return false;
	int code;
	int ret = sscanf(codeStr.c_str(), "%d", &code);
	if (ret != 1)
		throw wstring(L"File has invalid format");
	result = make_pair(code, valStr);
	return true;
}


long DxfStrToLong(const char * str)
{
	char * endp;
	long result = strtol(str, &endp, 10);
	if (result == 0 && str == endp)
		throw wstring(L"File has invalid format");
	return result;
}


double DxfStrToDouble(const char * str)
{
	char * endp;
	double result = strtod(str, &endp);
	if (result == 0 && str == endp)
		throw wstring(L"File has invalid format");
	return result;
}


void ImportDxf(HWND hwnd)
{
	wchar_t fileBuf[MAX_PATH] = {0};
	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = L"AutoCad Dxf Files\0*.dxf\0All files\0*.*\0\0";
	ofn.lpstrFile = fileBuf;
	ofn.nMaxFile = sizeof(fileBuf)/sizeof(fileBuf[0]);
	ofn.lpstrTitle = L"Import DXF";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (!GetOpenFileNameW(&ofn))
	{
		if (CommDlgExtendedError() != 0)
			assert(0);
		return;
	}
	try
	{
		WinHandle hfile = CreateFileW(fileBuf, FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
		if (!hfile)
		{
			throw wstring(L"Error opening file: ") + fileBuf +
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
			MessageBoxW(hwnd, L"File does not contain drawing", 0, MB_ICONEXCLAMATION);
			return;
		}
		if (!rdr.ReadItem(item) || item.first != 0)
			throw wstring(L"File has invalid format");
		bool done = false;
		auto_ptr<GroupUndoItem> groupItem(new GroupUndoItem);
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
					throw wstring(L"File has invalid format");
				auto_ptr<CadLine> line(new CadLine);
				line->Point1 = p1;
				line->Point2 = p2;
				groupItem->AddItem(new AddObjectUndoItem(line.release()));
			}
			else if (item.second == "CIRCLE")
			{
				Point<double> center;
				double radius;
				enum Flags
				{
					CX = 1,
					CY = 2,
					RADIUS = 4,
				};
				int flags = 0;
				bool done = false;
				while (!done && rdr.ReadItem(item))
				{
					switch (item.first)
					{
					case 0:
						done = true;
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
					}
				}
				if (flags != (CX | CY | RADIUS))
					throw wstring(L"File has invalid format");
				auto_ptr<CadCircle> circle(new CadCircle);
				circle->Center = center;
				circle->Radius = radius;
				groupItem->AddItem(new AddObjectUndoItem(circle.release()));
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
					throw wstring(L"File has invalid format");
				auto_ptr<CadArc> arc(new CadArc);
				arc->Center = center;
				arc->Radius = radius;
				arc->Ccw = true;
				arc->Start.X = cos(ang1*M_PI/180)*radius + center.X;
				arc->Start.Y = sin(ang1*M_PI/180)*radius + center.Y;
				arc->End.X = cos(ang2*M_PI/180)*radius + center.X;
				arc->End.Y = sin(ang2*M_PI/180)*radius + center.Y;
				groupItem->AddItem(new AddObjectUndoItem(arc.release()));
			}
			else if (item.second == "LWPOLYLINE")
			{
				long numVerts = -1;
				auto_ptr<CadPolyline> result(new CadPolyline);
				bool done = false;
				enum Flags
				{
					GOTX = 1,
					GOTY = 2,
				};
				int flags;
				while (!done && rdr.ReadItem(item))
				{
					switch (item.first)
					{
					case 0:
						done = true;
						break;
					case 90:
						numVerts = DxfStrToLong(item.second.c_str());
						break;
					case 70:
						result->Closed = (DxfStrToLong(item.second.c_str()) & 0x1) != 0;
						break;
					case 10:
						if (result->Nodes.size() > 0)
						{
							if ((flags & GOTX) == 0 || (flags & GOTY) == 0)
								throw wstring(L"File has invalid format");
						}
						flags = 0;
						assert(result->Nodes.size() <= static_cast<size_t>(numeric_limits<long>::max()));
						if (numVerts <= static_cast<long>(result->Nodes.size()))
							throw wstring(L"File has invalid format");
						result->Nodes.push_back(CadPolyline::Node());
						result->Nodes.back().Bulge = 0;
						result->Nodes.back().Point.X = DxfStrToDouble(item.second.c_str());
						flags |= GOTX;
						break;
					case 20:
						result->Nodes.back().Point.Y = DxfStrToDouble(item.second.c_str());
						flags |= GOTY;
						break;
					case 42:
						result->Nodes.back().Bulge = DxfStrToDouble(item.second.c_str());
						break;
					}
				}
				assert(result->Nodes.size() <= static_cast<size_t>(numeric_limits<long>::max()));
				if (static_cast<long>(result->Nodes.size()) != numVerts)
					throw wstring(L"File has invalid format");
				groupItem->AddItem(new AddObjectUndoItem(result.release()));
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
		g_undoManager.AddWork(groupItem.release());
		InvalidateRect(g_hclientWindow, 0, true);
	}
	catch (wstring & err)
	{
		if (MessageBoxW(hwnd, err.c_str(), 0, MB_ICONERROR) == 0)
			assert(0);
	}
}
