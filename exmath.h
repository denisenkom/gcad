#ifndef EXMATH_H_INCLUDED
#define EXMATH_H_INCLUDED


#include <algorithm>
#include <cmath>
#include <cassert>
#include <limits>
#include <utility>
#include <vector>


const double EPSILON = 0.00000001;


inline double RoundEpsilon(double x)
{
	return floor(x / EPSILON + 0.5) * EPSILON;
}


inline bool EqualsEpsilon(double lhs, double rhs)
{
	return std::fabs(lhs - rhs) <= EPSILON;
}


class NormalAngle
{
	double m_ang;
	static double NormalizeAngle(double angle)
	{
		double norm = std::fmod(angle + M_PI, 2*M_PI);
		if (norm <= 0)
			norm = 2*M_PI + norm;
		return norm - M_PI;
	}
public:
	explicit NormalAngle(double ang) { m_ang = (-M_PI < ang && ang <= M_PI ? ang : NormalizeAngle(ang)); }
	operator double() { return m_ang; }
	NormalAngle operator-() { return NormalAngle(-m_ang); }
	friend NormalAngle operator+(NormalAngle lhs, NormalAngle rhs)
	{
		double res = lhs.m_ang + rhs.m_ang;
		if (res > M_PI)
			res -= 2*M_PI;
		else if (res <= -M_PI)
			res += 2*M_PI;
		return NormalAngle(res);
	}
	friend NormalAngle operator-(NormalAngle lhs, NormalAngle rhs)
	{
		return operator+(lhs, -rhs);
	}
	double To2PiAng() { return m_ang >= 0 ? m_ang : m_ang + 2*M_PI; }
};


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

	Point<T> operator - ()
	{
		return Point<T>(-X, -Y);
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

	NormalAngle Angle() const
	{
		return NormalAngle(std::atan2(Y, X));
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

	friend Point<T> NegateAngle(Point<T> dir)
	{
		return Point<T>(dir.X, -dir.Y);
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

	friend T DotProduct(const Point<T> & lhs, const Point<T> & rhs)
	{
		return lhs.X * rhs.X + lhs.Y * rhs.Y;
	}

	friend bool EqualsEpsilon(Point<T> lhs, Point<T> rhs)
	{
		return EqualsEpsilon(lhs.X, rhs.X) &&
			EqualsEpsilon(lhs.Y, rhs.Y);
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

	// must be normalized
	bool Contains(const Point<T> & pt)
	{
		return Pt1.X <= pt.X && pt.X <= Pt2.X &&
			Pt1.Y <= pt.Y && pt.Y <= Pt2.Y;
	}

	// must be normalized
	bool ContainsWithEpsilon(const Point<T> & pt)
	{
		return Pt1.X - EPSILON <= pt.X &&
			pt.X <= Pt2.X + EPSILON &&
			Pt1.Y - EPSILON <= pt.Y &&
			pt.Y <= Pt2.Y + EPSILON;
	}

	// must be normalized
	bool ContainsNonInclusive(const Point<T> & pt)
	{
		return Pt1.X < pt.X && pt.X < Pt2.X &&
			Pt1.Y < pt.Y && pt.Y < Pt2.Y;
	}

	bool IsNormalized()
	{
		return Pt1.X <= Pt2.X && Pt1.Y <= Pt2.Y;
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

	friend Matrix3<scalar> DisplaceMatrix(Point<scalar> vector)
	{
		return Matrix3<scalar>(
				1, 0, vector.X,
				0, 1, vector.Y,
				0, 0, 1);
	}

	// vector must be normalized
	friend Matrix3<scalar> RotationMatrix(Point<scalar> dir)
	{
		return Matrix3<double>(
				dir.X, -dir.Y, 0,
				dir.Y,  dir.X, 0,
				0, 0, 1);
	}

	friend Matrix3<scalar> RotationMatrix(scalar angle)
	{
		return RotationMatrix(DirVector(angle));
	}
};


// checks if angle inside arc
// range is started from startAndle and sweeps CCW to endAngle
inline bool AngInArc(NormalAngle angle, NormalAngle startAngle, NormalAngle endAngle)
{
	if (startAngle < endAngle)
		return startAngle <= angle && angle <= endAngle;
	else
		return angle <= endAngle || startAngle <= angle;
}


struct Circle
{
	Point<double> Center;
	double Radius;

	friend bool operator==(Circle lhs, Circle rhs)
	{
		return lhs.Center == rhs.Center && lhs.Radius == rhs.Radius;
	}
};


//template<class scalar>
struct CircleArc : public Circle
{
	Point<double> Start;
	Point<double> End;
	bool Ccw;

	CircleArc() {}
	CircleArc(Circle circle, Point<double> start, Point<double> end, bool ccw) :
		Circle(circle), Start(start), End(end), Ccw(ccw) {}

	Point<double> CalcMiddlePoint() const
	{
		// determining angles of arc end points
		double angle1 = (Start - Center).Angle();
		double angle2 = (End - Center).Angle();
		// ensuring CCW direction from angle1 to angle2
		if (!Ccw)
		{
			double temp = angle1;
			angle1 = angle2;
			angle2 = temp;
		}
		double delta = angle2 - angle1;
		if (delta < 0)
			delta += 2 * M_PI;
		return DirVector(angle1 + delta/2) * Radius + Center;
	}

	double CalcBulge() const
	{
		double rel = (CalcMiddlePoint() - Start).Length() / ((End - Start).Length() / 2);
		return (Ccw ? 1 : -1) * std::sqrt(rel*rel - 1);
	}

	bool ContainsAng(NormalAngle angle) const
	{
		NormalAngle startAng = (Start - Center).Angle();
		NormalAngle endAng = (End - Center).Angle();
		if (!Ccw)
			std::swap(startAng, endAng);
		return AngInArc(angle, startAng, endAng);
	}

	bool ContainsAngWithEpsilon(NormalAngle angle) const
	{
		NormalAngle startAng = (Start - Center).Angle();
		NormalAngle endAng = (End - Center).Angle();
		if (!Ccw)
			std::swap(startAng, endAng);
		return AngInArc(angle, startAng - NormalAngle(EPSILON), endAng + NormalAngle(EPSILON));
	}

	Rect<double> CalcBoundingRect() const
	{
		Rect<double> result = Rect<double>(Start, End).Normalized();
		NormalAngle startAngle = (Start - Center).Angle();
		NormalAngle endAngle = (End - Center).Angle();
		if (!Ccw)
			std::swap(startAngle, endAngle);
		if (AngInArc(NormalAngle(0.0), startAngle, endAngle))
			result.Pt2.X = Center.X + Radius;
		if (AngInArc(NormalAngle(M_PI / 2), startAngle, endAngle))
			result.Pt2.Y = Center.Y + Radius;
		if (AngInArc(NormalAngle(M_PI), startAngle, endAngle))
			result.Pt1.X = Center.X - Radius;
		if (AngInArc(NormalAngle(-M_PI / 2), startAngle, endAngle))
			result.Pt1.Y = Center.Y - Radius;
		return result;
	}

	friend CircleArc ArcFrom3Pt(const Point<double> & p1, const Point<double> & p2, const Point<double> & p3)
	{
		CircleArc result;
		double sx1sy1 = p1.X*p1.X + p1.Y*p1.Y;
		double sx2sy2 = p2.X*p2.X + p2.Y*p2.Y;
		double sx3sy3 = p3.X*p3.X + p3.Y*p3.Y;
		Matrix3<double> m11(
				p1.X, p1.Y, 1,
				p2.X, p2.Y, 1,
				p3.X, p3.Y, 1);
		Matrix3<double> m12(
				sx1sy1, p1.Y, 1,
				sx2sy2, p2.Y, 1,
				sx3sy3, p3.Y, 1);
		Matrix3<double> m13(
				sx1sy1, p1.X, 1,
				sx2sy2, p2.X, 1,
				sx3sy3, p3.X, 1);
		Matrix3<double> m14(
				sx1sy1, p1.X, p1.Y,
				sx2sy2, p2.X, p2.Y,
				sx3sy3, p3.X, p3.Y);
		double dm11 = m11.Determinant();
		if (dm11 == 0)
		{
			result.Start = p1;
			result.End = p3;
			result.Radius = 0;
		}
		else
		{
			double dm12 = m12.Determinant();
			double dm13 = m13.Determinant();
			double dm14 = m14.Determinant();
			double cx = +.5f * dm12/dm11;
			double cy = -.5f * dm13/dm11;
			result.Center = Point<double>(cx, cy);
			result.Radius = std::sqrt(cx*cx + cy*cy + dm14/dm11);
			result.Ccw = dm11 > 0;
			result.Start = p1;
			result.End = p3;
		}
		return result;
	}

	// tangent must be normalized
	friend CircleArc ArcFrom2PtAndNormTangent(const Point<double> & p1, const Point<double> & tangent, const Point<double> & p2)
	{
		CircleArc result;
		Matrix3<double> disp(
				1, 0, p1.X,
				0, 1, p1.Y,
				0, 0,    1);
		Matrix3<double> rot(
				tangent.X, -tangent.Y, 0,
				tangent.Y,  tangent.X, 0,
				0,          0,         1);
		Point<double> p2img = rot.Inverse()*disp.Inverse()*p2;
		if (p2img.Y == 0)
		{
			result.Start = p1;
			result.End = p2;
			result.Radius = 0;
		}
		else
		{
			double cy = (p2img.X*p2img.X + p2img.Y*p2img.Y)/2/p2img.Y;
			result.Radius = std::abs(cy);
			Point<double> centerimg(0, cy);
			result.Center = disp*rot*centerimg;
			result.Ccw = cy > 0;
			result.Start = p1;
			result.End = p2;
		}
		return result;
	}

	friend Point<double> ArcMiddleFrom2PtAndBulge(const Point<double> & p1, const Point<double> & p2, double bulge)
	{
		return (p2 + p1)/2 + ((p2 - p1)/2).Rotate(-M_PI/2) * bulge;
	}

	friend CircleArc ArcFrom2PtAndBulge(const Point<double> & p1, const Point<double> & p2, double bulge)
	{
		return ArcFrom3Pt(p1, ArcMiddleFrom2PtAndBulge(p1, p2, bulge), p2);
	}
};


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


inline bool PtInArc(Point<double> pt, NormalAngle startAngle, NormalAngle endAngle)
{
	assert(startAngle != endAngle);
	return AngInArc(pt.Angle(), startAngle, endAngle);
}


inline std::pair<bool, double> VertLineIntersectsCircle(double x, Circle circle)
{
	// is straight line intersects circle
	double cx = circle.Center.X;
	double r = circle.Radius;
	if (cx - r <= x && x <= cx + r)
		return std::make_pair(true, sqrt(r*r - (x - cx)*(x - cx)));
	else
		return std::make_pair(false, 0.0);
}


inline std::pair<bool, double> HorzLineIntersectsCircle(double y, Circle circle)
{
	// is straight line intersects circle
	double cy = circle.Center.Y;
	double r = circle.Radius;
	if (cy - r <= y && y <= cy + r)
		return std::make_pair(true, sqrt(r*r - (y - cy)*(y - cy)));
	else
		return std::make_pair(false, 0.0);
}


inline bool VertLineIntersectArc(double x, double y1, double y2, Circle circle, NormalAngle startAngle, NormalAngle endAngle)
{
	std::pair<bool, double> res;
	res = VertLineIntersectsCircle(x, circle);
	if (!res.first)
		return false;
	double cx = circle.Center.X, cy = circle.Center.Y;
	// is intersection inside line cut
	if (y1 <= cy + res.second && cy + res.second <= y2)
	{
		if (PtInArc(Point<double>(x - cx, res.second), startAngle, endAngle))
			return true;
	}
	// have 2nd root and intersection inside line cut
	if (res.second != 0 && y1 <= cy - res.second && cy - res.second <= y2)
	{
		if (PtInArc(Point<double>(x - cx, -res.second), startAngle, endAngle))
			return true;
	}
	return false;
}


inline bool HorzLineIntersectArc(double y, double x1, double x2, Circle circle, NormalAngle startAngle, NormalAngle endAngle)
{
	double cx = circle.Center.X, cy = circle.Center.Y;
	double r = circle.Radius;
	// is strait line intersects circle
	if (cy - r <= y && y <= cy + r)
	{
		double root = sqrt(r*r - (y - cy)*(y - cy));
		double intx = cx + root;
		// is intersection inside line cut
		if (x1 <= intx && intx <= x2)
		{
			if (PtInArc(Point<double>(root, y - cy), startAngle, endAngle))
				return true;
		}
		// have 2nd root
		if (root != 0)
		{
			// is intersection inside line cut
			double intx = cx - root;
			if (x1 <= intx && intx <= x2)
			{
				if (PtInArc(Point<double>(-root, y - cy), startAngle, endAngle))
					return true;
			}
		}
	}
	return false;
}


inline bool IsIntersects(CircleArc arc, Rect<double> rect)
{
	assert(rect.IsNormalized());

	Rect<double> brect = arc.CalcBoundingRect();
	if (!IsRectsIntersects(brect, rect))
		return false;

	// checking is arc fits in rectangle
	if (IsLeftContainsRight(rect, brect))
		return true;

	// determining angles of arc end points
	NormalAngle angle1 = (arc.Start - arc.Center).Angle();
	NormalAngle angle2 = (arc.End - arc.Center).Angle();
	// ensuring CCW direction from angle1 to angle2
	if (!arc.Ccw)
		std::swap(angle1, angle2);

	// checking intersection with left side of rectangle
	if (VertLineIntersectArc(rect.Pt1.X, rect.Pt1.Y, rect.Pt2.Y, arc, angle1, angle2))
		return true;
	// checking intersection with right side of rectangle
	if (VertLineIntersectArc(rect.Pt2.X, rect.Pt1.Y, rect.Pt2.Y, arc, angle1, angle2))
		return true;
	// checking intersection with top side of rectangle
	if (HorzLineIntersectArc(rect.Pt1.Y, rect.Pt1.X, rect.Pt2.X, arc, angle1, angle2))
		return true;
	// checking intersection with bottom side of rectangle
	if (HorzLineIntersectArc(rect.Pt2.Y, rect.Pt1.X, rect.Pt2.X, arc, angle1, angle2))
		return true;
	return false;
}


struct Straight
{
	double A, B, C;
};


struct Line
{
	Point<double> Point1;
	Point<double> Point2;

	Line() {}
	Line(Point<double> pt1, Point<double> pt2) : Point1(pt1), Point2(pt2) {}

	Rect<double> GetBoundingRect() const
	{
		return Rect<double>(Point1, Point2).Normalized();
	}

	Straight GetStraight() const
	{
		double x1 = Point1.X, x2 = Point2.X;
		double y1 = Point1.Y, y2 = Point2.Y;
		Straight result = {y2-y1, -(x2-x1), y1*(x2-x1) - x1*(y2-y1)};
		return result;
	}
};


inline std::vector<Point<double> > Intersect(const Line & l1,
		const Line & l2)
{
	Point<double> p1 = l1.Point1, p2 = l1.Point2;
	Point<double> p3 = l2.Point1, p4 = l2.Point2;
	// enforcing reflexivity of intersection function,
	// i.e. same result from intersect(a,b) and intersect(b,a)
	/*Order(p1, p2);
	Order(p3, p4);
	if (p1.X > p3.X || p1.X == p3.X && p1.Y > p3.Y)
	{
		swap(p1, p3);
		swap(p2, p4);
	}
	else if (p1 == p3 && (p2.X > p4.X || p2.X == p4.X && p2.Y > p4.Y))
	{
		swap(p2, p4);
	}*/
	// calculating intersection using Cramer matrix method
	// for solving system of linear equations
	Matrix2<double> m(p2.Y - p1.Y, p1.X - p2.X,
			p4.Y - p3.Y, p3.X - p4.X);
	double detm = m.Determinant();
	std::vector<Point<double> > res;
	if (detm == 0)
		return res;
	Matrix2<double> mx(p1.X*p2.Y - p2.X*p1.Y, p1.X - p2.X,
			p3.X*p4.Y - p4.X*p3.Y, p3.X - p4.X);
	Matrix2<double> my(p2.Y - p1.Y, p1.X*p2.Y - p2.X*p1.Y,
			p4.Y - p3.Y, p3.X*p4.Y - p4.X*p3.Y);
	Point<double> pt(mx.Determinant()/detm, my.Determinant()/detm);
	// checking that intersection is within lines segments
	bool bres = l1.GetBoundingRect().ContainsWithEpsilon(pt) &&
		l2.GetBoundingRect().ContainsWithEpsilon(pt);
	if (bres)
		res.push_back(pt);
	return res;
}


struct SquareEquation
{
	bool HasRoots;
	bool TwoRoots;
	double Root1;
	double Root2;
	SquareEquation(double a, double b, double c)
	{
		double descr = b*b - 4*a*c;
		if (descr < 0)
		{
			HasRoots = false;
		}
		else if (descr == 0)
		{
			HasRoots = true;
			TwoRoots = false;
			Root1 = -b/2/a;
		}
		else
		{
			HasRoots = true;
			TwoRoots = true;
			Root1 = (-b + sqrt(descr))/2/a;
			Root2 = (-b - sqrt(descr))/2/a;
		}
	}
};


struct CircLineIntersectRes
{
	bool HasIntersection;
	bool TwoPoints;
	Point<double> Point1;
	Point<double> Point2;
};


inline std::vector<Point<double> > Intersect(const Line & line, const Circle & circle)
{
	Straight str = line.GetStraight();
	double a = str.A, b = str.B, c = str.C;
	double cx = circle.Center.X, cy = circle.Center.Y, r = circle.Radius;
	Point<double> pt1, pt2;
	bool hasPoints;
	bool twoPoints;
	if (a == 0)
	{
		double y = line.Point1.Y;
		std::pair<bool, double> tuple = HorzLineIntersectsCircle(y, circle);
		if (hasPoints = tuple.first)
		{
			pt1 = Point<double>(cx + tuple.second, y);
			if (twoPoints = tuple.second != 0)
				pt2 = Point<double>(cx - tuple.second, y);
		}
	}
	else if (b == 0)
	{
		double x = line.Point1.X;
		std::pair<bool, double> tuple = VertLineIntersectsCircle(x, circle);
		if (hasPoints = tuple.first)
		{
			pt1 = Point<double>(x, cy + tuple.second);
			if (twoPoints = tuple.second != 0)
				pt2 = Point<double>(x, cy - tuple.second);
		}
	}
	else
	{
		SquareEquation eq(a/b*a/b + 1, 2*(a*c/b/b + a*cy/b - cx), (c/b + cy)*(c/b + cy) + cx*cx - r*r);
		if (hasPoints = eq.HasRoots)
		{
			pt1 = Point<double>(eq.Root1, -(c + a*eq.Root1)/b);
			if (twoPoints = eq.TwoRoots)
				pt2 = Point<double>(eq.Root2, -(c + a*eq.Root2)/b);
		}
	}
	std::vector<Point<double> > res;
	if (hasPoints)
	{
		Rect<double> brect = line.GetBoundingRect();
		if (brect.ContainsWithEpsilon(pt1))
			res.push_back(pt1);
		if (twoPoints && brect.ContainsWithEpsilon(pt2))
			res.push_back(pt2);
	}
	return res;
}

inline std::vector<Point<double> > Intersect(const Circle & circle, const Line & line)
{
	return Intersect(line, circle);
}


inline std::vector<Point<double> > Intersect(const Line & line, const CircleArc & arc)
{
	std::vector<Point<double> > points = Intersect(static_cast<const Circle&>(arc), line);
	std::vector<Point<double> > res;
	for (std::vector<Point<double> >::const_iterator i = points.begin();
		i != points.end(); i++)
	{
		if (arc.ContainsAngWithEpsilon((*i - arc.Center).Angle()))
			res.push_back(*i);
	}
	return res;
}

inline std::vector<Point<double> > Intersect(const CircleArc & arc, const Line & line)
{
	return Intersect(line, arc);
}


inline std::vector<Point<double> > Intersect(const Circle & lhs, const Circle & rhs)
{
	double r0 = rhs.Radius, r1 = lhs.Radius;
	Point<double> p0 = rhs.Center;
	Point<double> p1 = lhs.Center;
	std::vector<Point<double> > res;
	if (rhs == lhs)
		return res;
	double d = (p1 - p0).Length();
	// circles outside each other
	if (d > r0 + r1 + EPSILON)
		return res;
	// one circle inside other
	if (d < std::fabs(r0 - r1) - EPSILON)
		return res;
	// one point
	if (EqualsEpsilon(d, r0 + r1))
	{
		res.push_back(p0 + r0*(p1 - p0)/d);
		return res;
	}
	double a = (r0*r0 - r1*r1 + d*d)/2/d;
	double h = std::sqrt(r0*r0 - a*a);
	Point<double> p2 = p0 + a*(p1 - p0)/d;
	res.push_back(Point<double>(p2.X + h*(p1.Y - p0.Y)/d, p2.Y - h*(p1.X - p0.X)/d));
	res.push_back(Point<double>(p2.X - h*(p1.Y - p0.Y)/d, p2.Y + h*(p1.X - p0.X)/d));
	return res;
}


inline std::vector<Point<double> > Intersect(const Circle & circle, const CircleArc & arc)
{
	std::vector<Point<double> > points = Intersect(circle, static_cast<const Circle&>(arc));
	std::vector<Point<double> > res;
	for (std::vector<Point<double> >::const_iterator i = points.begin();
		i != points.end(); i++)
	{
		if (arc.ContainsAngWithEpsilon((*i - arc.Center).Angle()))
			res.push_back(*i);
	}
	return res;
}

inline std::vector<Point<double> > Intersect(const CircleArc & arc, const Circle & circle)
{
	return Intersect(circle, arc);
}


inline std::vector<Point<double> > Intersect(const CircleArc & lhs, const CircleArc & rhs)
{
	std::vector<Point<double> > points = Intersect(static_cast<const Circle&>(lhs),
			static_cast<const Circle&>(rhs));
	std::vector<Point<double> > res;
	for (std::vector<Point<double> >::const_iterator i = points.begin();
		i != points.end(); i++)
	{
		if (lhs.ContainsAngWithEpsilon((*i - lhs.Center).Angle()) &&
				rhs.ContainsAngWithEpsilon((*i - rhs.Center).Angle()))
		{
			res.push_back(*i);
		}
	}
	return res;
}


#endif // EXMATH_H_INCLUDED
