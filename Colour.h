#pragma once

#include "Types.h"

class Colour
{
public:
	const static Colour White;
	const static Colour Red;
	const static Colour Blue;
	const static Colour Green;

	Colour(Real r_ = 0.0f, Real g_ = 0.0f, Real b_ = 0.0f) : r(r_), g(g_), b(b_) { }

	Real r;
	Real g;
	Real b;
};
