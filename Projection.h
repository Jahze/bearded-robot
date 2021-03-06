#pragma once

#include "Types.h"
#include "Matrix.h"
#include "Vector.h"

class Projection
{
public:
	Projection(Real fov, Real znear, Real zfar, unsigned width, unsigned height);

	Matrix4 GetProjectionMatrix() const;

	Real ToScreenX(Real x) const;
	Real ToScreenY(Real y) const;

private:
	Real m_fov;
	Real m_znear;
	Real m_zfar;
	unsigned m_width;
	unsigned m_height;
};
