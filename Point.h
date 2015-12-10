#pragma once

#include "Types.h"

struct Point
{
	Real x;
	Real y;

	Point operator+(const Point & rhs) const
	{
		return { x + rhs.x, y + rhs.y };
	}

	Point & operator+=(const Point & rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	Point operator-(const Point & rhs) const
	{
		return { x - rhs.x, y - rhs.y };
	}

	Point & operator-=(const Point & rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	Point operator*(Real scalar) const
	{
		return { x * scalar, y * scalar };
	}

	Point & operator*=(Real scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}
};
