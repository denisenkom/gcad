#include <cmath>
#include <cassert>

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
inline bool PtInArc(scalar x, scalar y, scalar startAngle, scalar endAngle)
{
	assert(startAngle != endAngle);
	scalar angle = atan2(y, x);
	if (startAngle < endAngle)
		return startAngle <= angle && angle <= endAngle;
	else
		return angle <= endAngle || startAngle <= angle;
}


template<typename scalar> // float or double
inline bool VertLineIntersectArc(scalar x, scalar y1, scalar y2, scalar cx, scalar cy, scalar r, scalar startAngle, scalar endAngle)
{
	// is strait line intersects circle
	if (cx - r <= x && x <= cx + r)
	{
		scalar root = sqrt(r*r - (x - cx)*(x - cx));
		scalar inty = cy + root;
		// is intersection inside line cut
		if (y1 <= inty && inty <= y2)
		{
			if (PtInArc(x - cx, root, startAngle, endAngle))
				return true;
		}
		// have 2nd root
		if (root != 0)
		{
			scalar inty = cy - root;
			// is intersection inside line cut
			if (y1 <= inty && inty <= y2)
			{
				if (PtInArc(x - cx, -root, startAngle, endAngle))
					return true;
			}
		}
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
inline bool ArcIntersectsRect(scalar cx, scalar cy, scalar r, scalar p1x, scalar p1y, scalar p2x, scalar p2y, bool ccw, scalar x1, scalar y1, scalar x2, scalar y2)
{
	assert(x1 <= x2);
	assert(y1 <= y2);

	// checking is full circle fits in rectangle
	if (x1 <= cx - r && cx + r <= x2 && y1 <= cy - r && cy + r <= y2)
		return true;

	// rectangle fully on left from circle
	if (cx + r < x1)
		return false;
	// rectangle fully on right from circle
	if (x2 < cx - r)
		return false;
	// rectangle fully above circle
	if (cy + r < y1)
		return false;
	// rectangle fully below circle
	if (y2 < cx - r)
		return false;

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
