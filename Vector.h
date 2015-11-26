#pragma once

#include <cmath>
#include "Types.h"

class Vector3
{
public:
	const static Vector3 Zero;
	const static Vector3 UnitX;
	const static Vector3 UnitY;
	const static Vector3 UnitZ;

	Real x;
	Real y;
	Real z;

	Vector3()
		: x()
		, y()
		, z()
	{ }

	Vector3(Real x_, Real y_, Real z_)
		: x(x_)
		, y(y_)
		, z(z_)
	{ }

	Real Length() const
	{
		return sqrt(x * x + y * y + z * z);
	}

	Vector3 & Normalize()
	{
		x /= Length();
		y /= Length();
		z /= Length();
		return *this;
	}

	Real Dot(const Vector3 & rhs) const
	{
		return x * rhs.x + y * rhs.y + z * rhs.z;
	}

	Vector3 NormalizedCopy() const
	{
		return { x / Length(), y / Length(), z / Length() };
	}

	Vector3 operator-() const
	{
		return { -x, -y, -z };
	}

	Vector3 operator*(Real scalar) const
	{
		return { x * scalar, y * scalar, z * scalar };
	}

	Vector3 operator/(Real scalar) const
	{
		return { x / scalar, y / scalar, z / scalar };
	}

	Vector3 operator+(const Vector3 & rhs) const
	{
		return { x + rhs.x, y + rhs.y, z + rhs.z };
	}

	Vector3 & operator+=(const Vector3 & rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	Vector3 operator-(const Vector3 & rhs) const
	{
		return { x - rhs.x, y - rhs.y, z - rhs.z };
	}
};

class Vector4
{
public:
	Real x;
	Real y;
	Real z;
	Real w;

	Vector4()
		: x()
		, y()
		, z()
		, w()
	{ }

	Vector4(Real x_, Real y_, Real z_, Real w_)
		: x(x_)
		, y(y_)
		, z(z_)
		, w(w_)
	{ }

	Vector3 XYZ() const
	{
		return { x, y, z };
	}
};
