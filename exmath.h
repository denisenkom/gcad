#ifndef EXMATH_H_INCLUDED
#define EXMATH_H_INCLUDED


#include <algorithm>
#include <cmath>
#include <cassert>
#include <utility>


template <typename T>
struct Point
{
	T X;
	T Y;

	Point() {}
	Point(const T & x, const T & y) : X(x), Y(y) {}

	template <typename T2>
	Point(const Point<T2> & left) : X(left.X), Y(left.Y) {}

	Point<T> & operator = (const Point<T> & left)
	{
		X = left.X;
		Y = left.Y;
		return *this;
	}

	template <typename T2>
	Point<T> & operator = (const Point<T2> & left)
	{
		X = left.X;
		Y = left.Y;
		return *this;
	}

	Point<T> & operator += (const Point<T> & left)
	{
		*this = *this + left;
		return *this;
	}

	bool operator==(const Point<T> & rhs) const
	{
		return X == rhs.X && Y == rhs.Y;
	}

	bool operator!=(const Point<T> & rhs) const
	{
		return !(*this == rhs);
	}

	T Length() const
	{
		return sqrt(X * X + Y * Y);
	}

	friend Point<T> operator-(const Point<T> & lhs, const Point<T> & rhs)
	{
		return Point<T>(rhs.X - lhs.X, rhs.Y - lhs.Y);
	}

	friend Point<T> operator+(const Point<T> & lhs, const Point<T> & rhs)
	{
		return Point<T>(rhs.X + lhs.X, rhs.Y + lhs.Y);
	}
};


template<typename T>
struct Rect
{
	Rect() {}

	Rect(Point<T> pt1, Point<T> pt2)
		: Pt1(pt1), Pt2(pt2)
	{
	}

	Rect(T x1, T y1, T x2, T y2)
		: Pt1(x1, y1), Pt2(x2, y2)
	{
	}

	Rect<T> Normalized() const
	{
		return Rect<T>(std::min(Pt1.X, Pt2.X), std::min(Pt1.Y, Pt2.Y),
				std::max(Pt1.X, Pt2.X), std::max(Pt1.Y, Pt2.Y));
	}

	Point<T> Pt1;
	Point<T> Pt2;

	// rectangles must be normalized
	friend bool IsLeftContainsRight(const Rect<T> & left, const Rect<T> & right)
	{
		return left.Pt1.X <= right.Pt1.X && right.Pt2.X <= left.Pt2.X &&
			left.Pt1.Y <= right.Pt1.Y && right.Pt2.Y <= left.Pt2.Y;
	}

	friend bool IsRectsIntersects(const Rect<T> & a, const Rect<T> & b)
	{
		// rectangle a fully on left from rectangle b
		if (a.Pt2.X < b.Pt1.X)
			return false;
		// rectangle a fully on right from rectangle b
		if (b.Pt2.X < a.Pt1.X)
			return false;
		// rectangle a fully above from rectangle b
		if (a.Pt2.Y < b.Pt1.Y)
			return false;
		// rectangle a fully below from rectangle b
		if (b.Pt2.Y < a.Pt1.Y)
			return false;
		return true;
	}
};


template<class scalar>
inline scalar det3(scalar m[3][3])
{
	return m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] + m[1][0]*m[2][1]*m[0][2] -
		m[0][2]*m[1][1]*m[2][0] - m[1][0]*m[0][1]*m[2][2] - m[0][0]*m[1][2]*m[2][1];
}


template<typename scalar> // float or double
bool LineIntersectsRect(scalar p1x, scalar p1y, scalar p2x, scalar p2y, scalar x1, scalar y1, scalar x2, scalar y2)
{
	assert(x1 <= x2);
	assert(y1 <= y2);

	// is line entirely inside rectangle
	if (p1x >= x1 && p1x <= x2 && p2x >= x1 && p2x <= x2 && p1y >= y1 && p1y <= y2 && p2y >= y1 && p2y <= y2)
		return true;
	bool vertical = p1x == p2x;
	bool horizontal = p1y == p2y;
	// checking intersections with vertical sides of rectangle
	if (!vertical)
	{
		if (min(p1x, p2x) <= x1 && x1 <= max(p1x, p2x))
		{
			scalar inty1 = (x1 - p1x)*(p2y - p1y)/(p2x - p1x) + p1y;
			if (y1 <= inty1 && inty1 <= y2)
				return true;
		}
		if (min(p1x, p2x) <= x2 && x2 <= max(p1x, p2x))
		{
			scalar inty2 = (x2 - p1x)*(p2y - p1y)/(p2x - p1x) + p1y;
			if (y1 <= inty2 && inty2 <= y2)
				return true;
		}
	}
	// checking intersections with horizontal sides of rectangle
	if (!horizontal)
	{
		if (min(p1y, p2y) <= y1 && y1 <= max(p1y, p2y))
		{
			scalar intx1 = (y1 - p1y)*(p2x - p1x)/(p2y - p1y) + p1x;
			if (x1 <= intx1 && intx1 <= x2)
				return true;
		}
		if (min(p1y, p2y) <= y2 && y2 <= max(p1y, p2y))
		{
			scalar intx2 = (y2 - p1y)*(p2x - p1x)/(p2y - p1y) + p1x;
			if (x1 <= intx2 && intx2 <= x2)
				return true;
		}
	}
	return false;
}


template<typename scalar> // float or double
inline bool AngInArc(scalar angle, scalar startAngle, scalar endAngle)
{
	if (startAngle < endAngle)
		return startAngle <= angle && angle <= endAngle;
	else
		return angle <= endAngle || startAngle <= angle;
}


template<typename scalar> // float or double
inline bool PtInArc(scalar x, scalar y, scalar startAngle, scalar endAngle)
{
	assert(startAngle != endAngle);
	return AngInArc(atan2(y, x), startAngle, endAngle);
}


template<typename scalar> // float or double
inline std::pair<bool, scalar> VertLineIntersectsCircle(scalar x, Point<scalar> c, scalar r)
{
	// is strait line intersects circle
	if (c.X - r <= x && x <= c.X + r)
		return make_pair(true, sqrt(r*r - (x - c.X)*(x - c.X)));
	else
		return make_pair(false, static_cast<scalar>(0.0));
}


template<typename scalar> // float or double
inline std::pair<bool, scalar> HorzLineIntersectsCircle(scalar y, Point<scalar> c, scalar r)
{
	// is strait line intersects circle
	if (c.Y - r <= y && y <= c.Y + r)
		return std::make_pair(true, sqrt(r*r - (y - c.Y)*(y - c.Y)));
	else
		return std::make_pair(false, static_cast<scalar>(0.0));
}


template<typename scalar> // float or double
inline bool VertLineIntersectArc(scalar x, scalar y1, scalar y2, scalar cx, scalar cy, scalar r, scalar startAngle, scalar endAngle)
{
	std::pair<bool, double> res;
	res = VertLineIntersectsCircle(x, Point<scalar>(cx, cy), r);
	if (!res.first)
		return false;
	// is intersection inside line cut
	if (y1 <= cy + res.second && cy + res.second <= y2)
	{
		if (PtInArc(x - cx, res.second, startAngle, endAngle))
			return true;
	}
	// have 2nd root and intersection inside line cut
	if (res.second != 0 && y1 <= cy - res.second && cy - res.second <= y2)
	{
		if (PtInArc(x - cx, -res.second, startAngle, endAngle))
			return true;
	}
	return false;
}


template<typename scalar> // float or double
inline bool HorzLineIntersectArc(scalar y, scalar x1, scalar x2, scalar cx, scalar cy, scalar r, scalar startAngle, scalar endAngle)
{
	// is strait line intersects circle
	if (cy - r <= y && y <= cy + r)
	{
		scalar root = sqrt(r*r - (y - cy)*(y - cy));
		scalar intx = cx + root;
		// is intersection inside line cut
		if (x1 <= intx && intx <= x2)
		{
			if (PtInArc(root, y - cy, startAngle, endAngle))
				return true;
		}
		// have 2nd root
		if (root != 0)
		{
			// is intersection inside line cut
			scalar intx = cx - root;
			if (x1 <= intx && intx <= x2)
			{
				if (PtInArc(-root, y - cy, startAngle, endAngle))
					return true;
			}
		}
	}
	return false;
}


template<typename scalar> // float or double
inline Rect<scalar> ArcsBoundingRect(scalar cx, scalar cy, scalar r, scalar p1x, scalar p1y, scalar p2x, scalar p2y, bool ccw)
{
	Rect<scalar> result = Rect<double>(p1x, p1y, p2x, p2y).Normalized();
	scalar startAngle = atan2(p1y - cy, p1x - cx);
	scalar endAngle = atan2(p2y - cy, p2x - cx);
	if (!ccw)
		swap(startAngle, endAngle);
	if (AngInArc(0.0, startAngle, endAngle))
		result.Pt2.X = cx + r;
	if (AngInArc(M_PI / 2, startAngle, endAngle))
		result.Pt2.Y = cy + r;
	if (AngInArc(M_PI, startAngle, endAngle))
		result.Pt1.X = cx - r;
	if (AngInArc(-M_PI / 2, startAngle, endAngle))
		result.Pt1.Y = cy - r;
	return result;
}


template<typename scalar> // float or double
inline bool ArcIntersectsRect(scalar cx, scalar cy, scalar r, scalar p1x, scalar p1y, scalar p2x, scalar p2y, bool ccw, scalar x1, scalar y1, scalar x2, scalar y2)
{
	assert(x1 <= x2);
	assert(y1 <= y2);

	Rect<scalar> brect = ArcsBoundingRect(cx, cy, r, p1x, p1y, p2x, p2y, ccw);
	if (!IsRectsIntersects(brect, Rect<scalar>(x1, y1, x2, y2)))
		return false;

	// checking is arc fits in rectangle
	if (IsLeftContainsRight(Rect<scalar>(x1, y1, x2, y2), brect))
		return true;

	// determining angles of arc end points
	scalar angle1 = atan2(p1y - cy, p1x - cx);
	scalar angle2 = atan2(p2y - cy, p2x - cx);
	// ensuring CCW direction from angle1 to angle2
	if (!ccw)
	{
		scalar temp = angle1;
		angle1 = angle2;
		angle2 = temp;
	}

	// checking intersection with left side of rectangle
	if (VertLineIntersectArc(x1, y1, y2, cx, cy, r, angle1, angle2))
		return true;
	// checking intersection with right side of rectangle
	if (VertLineIntersectArc(x2, y1, y2, cx, cy, r, angle1, angle2))
		return true;
	// checking intersection with top side of rectangle
	if (HorzLineIntersectArc(y1, x1, x2, cx, cy, r, angle1, angle2))
		return true;
	// checking intersection with bottom side of rectangle
	if (HorzLineIntersectArc(y2, x1, x2, cx, cy, r, angle1, angle2))
		return true;
	return false;
}


template<typename scalar>
inline void CalcMiddlePointOfArc(scalar cx, scalar cy, scalar r, scalar p1x, scalar p1y, scalar p2x, scalar p2y, bool ccw, scalar & pmx, scalar & pmy)
{
	// determining angles of arc end points
	scalar angle1 = atan2(p1y - cy, p1x - cx);
	scalar angle2 = atan2(p2y - cy, p2x - cx);
	// ensuring CCW direction from angle1 to angle2
	if (!ccw)
	{
		scalar temp = angle1;
		angle1 = angle2;
		angle2 = temp;
	}
	scalar delta = angle2 - angle1;
	if (delta < 0)
		delta += 2 * M_PI;
	pmx = cos(angle1 + delta/2)*r + cx;
	pmy = sin(angle1 + delta/2)*r + cy;
}


#endif // EXMATH_H_INCLUDED
