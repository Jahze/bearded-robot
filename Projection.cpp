#include <cmath>
#include "Projection.h"

Projection::Projection(Real fovx, Real fovy, Real znear, Real zfar, unsigned width, unsigned height)
	: m_fovx(fovx)
	, m_fovy(fovy)
	, m_znear(znear)
	, m_zfar(zfar)
	, m_width(width)
	, m_height(height)
{ }

Matrix4 Projection::GetProjectionMatrix() const
{
	Real fovxCoeff = atan((m_fovx * DEG_TO_RAD) / 2.0f);
	Real fovyCoeff = atan((m_fovy * DEG_TO_RAD) / 2.0f);
	Real zClip1 = -((m_zfar + m_znear) / (m_zfar - m_znear));
	Real zClip2 = -((2 * m_znear * m_zfar) / (m_zfar - m_znear));

	return {{{
		{ fovxCoeff, 0.0, 0.0, 0.0 },
		{ 0.0, fovyCoeff, 0.0, 0.0 },
		{ 0.0, 0.0, zClip1, zClip2 },
		{ 0.0, 0.0, -1.0, 0.0 }
	}}};
}

Real Projection::ToScreenX(Real x) const
{
	return ((x + 1.0f) / 2.0f) * m_width;
}

Real Projection::ToScreenY(Real y) const
{
	return (1.0f - ((y + 1.0f) / 2.0f)) * m_height;
}
