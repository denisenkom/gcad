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

	Point<T> Normalize() const
	{
		return *this/Length();
	}

	T Angle() const
	{
		return std::atan2(Y, X);
	}

	Point<T> Rotate(T angle) const
	{
		return Point<T>(X * std::cos(angle) - Y * std::sin(angle),
				X * std::sin(angle) + Y * std::cos(angle));
	}

	friend Point<T> DirVector(T angle)
	{
		return Point<T>(std::cos(angle), std::sin(angle));
	}

	friend Point<T> operator-(const Point<T> & lhs, const Point<T> & rhs)
	{
		return Point<T>(lhs.X - rhs.X, lhs.Y - rhs.Y);
	}

	friend Point<T> operator+(const Point<T> & lhs, const Point<T> & rhs)
	{
		return Point<T>(lhs.X + rhs.X, lhs.Y + rhs.Y);
	}

	template <typename T2>
	friend Point<T> operator/(const Point<T> & lhs, T2 rhs)
	{
		return Point<T>(lhs.X / rhs, lhs.Y / rhs);
	}

	template <typename T2>
	friend Point<T> operator*(const Point<T> & lhs, T2 rhs)
	{
		return Point<T>(lhs.X * rhs, lhs.Y * rhs);
	}

	template <typename T2>
	friend Point<T> operator*(T2 lhs, const Point<T> & rhs)
	{
		return rhs * lhs;
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

	// rectangles must be normalized
	friend Rect GetBoundingRect(const Rect & lhs, const Rect & rhs)
	{
		return Rect(std::min(lhs.Pt1.X, rhs.Pt1.X),
				std::min(lhs.Pt1.Y, rhs.Pt1.Y),
				std::max(lhs.Pt2.X, rhs.Pt2.X),
				std::max(lhs.Pt2.Y, rhs.Pt2.Y));
	}
};


template<class scalar>
class Matrix2
{
private:
	scalar m[2][2];

public:
	Matrix2() {}
	Matrix2(scalar m11, scalar m12,
			scalar m21, scalar m22)
	{
		m[0][0] = m11; m[0][1] = m12;
		m[1][0] = m21; m[1][1] = m22;
	}

	scalar Determinant() const
	{
		return m[0][0]*m[1][1] - m[0][1]*m[1][0];
	}

	scalar * operator [](int row)
	{
		assert(0 <= row && row < 2);
		return m[row];
	}

	scalar const * operator [](int row) const
	{
		assert(0 <= row && row < 2);
		return m[row];
	}
};


template<class scalar>
class Matrix3
{
private:
	scalar m[3][3];

public:
	Matrix3() {}
	Matrix3(scalar m11, scalar m12, scalar m13,
			scalar m21, scalar m22, scalar m23,
			scalar m31, scalar m32, scalar m33)
	{
		m[0][0] = m11; m[0][1] = m12; m[0][2] = m13;
		m[1][0] = m21; m[1][1] = m22; m[1][2] = m23;
		m[2][0] = m31; m[2][1] = m32; m[2][2] = m33;
	}

	scalar Determinant() const
	{
		return m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] + m[1][0]*m[2][1]*m[0][2] -
			m[0][2]*m[1][1]*m[2][0] - m[1][0]*m[0][1]*m[2][2] - m[0][0]*m[1][2]*m[2][1];
	}

	Matrix3<scalar> Transpose() const
	{
		return Matrix3<scalar>(
				m[0][0], m[1][0], m[2][0],
				m[0][1], m[1][1], m[2][1],
				m[0][2], m[1][2], m[2][2]);
	}

	Matrix2<scalar> Minor(int row, int col) const
	{
		Matrix2<scalar> result;
		assert(0 <= row && row < 3);
		assert(0 <= col && col < 3);
		for (int r = 0, dr = 0; r < 3; r++)
		{
			if (r == row)
				continue;
			for (int c = 0, dc = 0; c < 3; c++)
			{
				if (c == col)
					continue;
				result[dr][dc] = m[r][c];
				dc++;
			}
			dr++;
		}
		return result;
	}

	Matrix3<scalar> Inverse() const
	{
		Matrix3<scalar> result;
		scalar det = Determinant();
		for (int r = 0; r < 3; r++)
		{
			for (int c = 0; c < 3; c++)
				result[r][c] = (((r + c) % 2) == 0 ? 1 : -1) * Minor(c, r).Determinant() / det; // transposed
		}
		return result;
	}

	scalar * operator [](int row)
	{
		assert(0 <= row && row < 3);
		return m[row];
	}

	scalar const * operator [](int row) const
	{
		assert(0 <= row && row < 3);
		return m[row];
	}

	friend Matrix3<scalar> operator *(const Matrix3<scalar> & lhs, const Matrix3<scalar> & rhs)
	{
		Matrix3<scalar> result;
		for (int r = 0; r < 3; r++)
		{
			for (int c = 0; c < 3; c++)
			{
				scalar val = 0;
				for (int i = 0; i < 3; i++)
					val += lhs[r][i]*rhs[i][c];
				result[r][c] = val;
			}
		}
		return result;
	}

	friend Point<scalar> operator *(const Matrix3<scalar> & lhs, const Point<scalar> & rhs)
	{
		return Point<scalar>(lhs[0][0]*rhs.X + lhs[0][1]*rhs.Y + lhs[0][2],
				lhs[1][0]*rhs.X + lhs[1][1]*rhs.Y + lhs[1][2]);
	}
};

template<typename scalar> // float or double
bool ArcIntersectsRect(scalar cx, scalar cy, scalar r, scalar p1x, scalar p1y, scalar p2x, scalar p2y, bool ccw, scalar x1, scalar y1, scalar x2, scalar y2);

template<typename scalar> // float or double
inline Rect<scalar> ArcsBoundingRect(scalar cx, scalar cy, scalar r, scalar p1x, scalar p1y, scalar p2x, scalar p2y, bool ccw);

template<class scalar>
struct CircleArc
{
	Point<scalar> Center;
	scalar Radius;
	Point<scalar> Start;
	Point<scalar> End;
	bool Ccw;

	Point<scalar> CalcMiddlePoint() const
	{
		// determining angles of arc end points
		scalar angle1 = (Start - Center).Angle();
		scalar angle2 = (End - Center).Angle();
		// ensuring CCW direction from angle1 to angle2
		if (!Ccw)
		{
			scalar temp = angle1;
			angle1 = angle2;
			angle2 = temp;
		}
		scalar delta = angle2 - angle1;
		if (delta < 0)
			delta += 2 * M_PI;
		return DirVector(angle1 + delta/2) * Radius + Center;
	}

	scalar CalcBulge() const
	{
		scalar rel = (CalcMiddlePoint() - Start).Length() / ((End - Start).Length() / 2);
		return (Ccw ? 1 : -1) * std::sqrt(rel*rel - 1);
	}

	Rect<scalar> CalcBoundingRect() const
	{
		return ArcsBoundingRect<scalar>(Center.X, Center.Y, Radius,
				Start.X, Start.Y, End.X, End.Y, Ccw);
	}

	friend CircleArc ArcFrom3Pt(const Point<scalar> & p1, const Point<scalar> & p2, const Point<scalar> & p3)
	{
		CircleArc result;
		scalar sx1sy1 = p1.X*p1.X + p1.Y*p1.Y;
		scalar sx2sy2 = p2.X*p2.X + p2.Y*p2.Y;
		scalar sx3sy3 = p3.X*p3.X + p3.Y*p3.Y;
		Matrix3<scalar> m11(
				p1.X, p1.Y, 1,
				p2.X, p2.Y, 1,
				p3.X, p3.Y, 1);
		Matrix3<scalar> m12(
				sx1sy1, p1.Y, 1,
				sx2sy2, p2.Y, 1,
				sx3sy3, p3.Y, 1);
		Matrix3<scalar> m13(
				sx1sy1, p1.X, 1,
				sx2sy2, p2.X, 1,
				sx3sy3, p3.X, 1);
		Matrix3<scalar> m14(
				sx1sy1, p1.X, p1.Y,
				sx2sy2, p2.X, p2.Y,
				sx3sy3, p3.X, p3.Y);
		scalar dm11 = m11.Determinant();
		if (dm11 == 0)
		{
			result.Start = p1;
			result.End = p3;
			result.Radius = 0;
		}
		else
		{
			scalar dm12 = m12.Determinant();
			scalar dm13 = m13.Determinant();
			scalar dm14 = m14.Determinant();
			scalar cx = +.5f * dm12/dm11;
			scalar cy = -.5f * dm13/dm11;
			result.Center = Point<scalar>(cx, cy);
			result.Radius = std::sqrt(cx*cx + cy*cy + dm14/dm11);
			result.Ccw = dm11 > 0;
			result.Start = p1;
			result.End = p3;
		}
		return result;
	}

	// tangent must be normalized
	friend CircleArc ArcFrom2PtAndNormTangent(const Point<scalar> & p1, const Point<scalar> & tangent, const Point<scalar> & p2)
	{
		CircleArc result;
		Matrix3<scalar> disp(
				1, 0, p1.X,
				0, 1, p1.Y,
				0, 0,    1);
		Matrix3<scalar> rot(
				tangent.X, -tangent.Y, 0,
				tangent.Y,  tangent.X, 0,
				0,          0,         1);
		Point<scalar> p2img = rot.Inverse()*disp.Inverse()*p2;
		if (p2img.Y == 0)
		{
			result.Start = p1;
			result.End = p2;
			result.Radius = 0;
		}
		else
		{
			scalar cy = (p2img.X*p2img.X + p2img.Y*p2img.Y)/2/p2img.Y;
			result.Radius = std::abs(cy);
			Point<scalar> centerimg(0, cy);
			result.Center = disp*rot*centerimg;
			result.Ccw = cy > 0;
			result.Start = p1;
			result.End = p2;
		}
		return result;
	}

	friend Point<scalar> ArcMiddleFrom2PtAndBulge(const Point<scalar> & p1, const Point<scalar> & p2, scalar bulge)
	{
		return (p2 + p1)/2 + ((p2 - p1)/2).Rotate(-M_PI/2) * bulge;
	}

	friend CircleArc ArcFrom2PtAndBulge(const Point<scalar> & p1, const Point<scalar> & p2, scalar bulge)
	{
		return ArcFrom3Pt(p1, ArcMiddleFrom2PtAndBulge(p1, p2, bulge), p2);
	}
};


template <class scalar>
inline bool Intersects(const CircleArc<scalar> & arc, const Rect<scalar> & rect)
{
	return ArcIntersectsRect(arc.Center.X, arc.Center.Y, arc.Radius, arc.Start.X,
			arc.Start.Y, arc.End.X, arc.End.Y, arc.Ccw, rect.Pt1.X, rect.Pt1.Y, rect.Pt2.X, rect.Pt2.Y);
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
bool LineIntersectsRect(const Point<scalar> & p1, const Point<scalar> p2,
		const Rect<scalar> & rect)
{
	return LineIntersectsRect(p1.X, p1.Y, p2.X, p2.Y, rect.Pt1.X, rect.Pt1.Y,
			rect.Pt2.X, rect.Pt2.Y);
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


#endif // EXMATH_H_INCLUDED
