#pragma once

#define DEG_TO_RAD 0.0174533

namespace Units
{
	struct Radians_t {};
	const Radians_t Radians = Radians_t();

	struct Degrees_t {};
	const Degrees_t Degrees = Degrees_t();
}

typedef float Real;

inline Real operator "" _radians(long double r)
{
	return static_cast<Real>(r);
}

inline Real operator "" _degrees(long double r)
{
	return static_cast<Real>(r * DEG_TO_RAD);
}

