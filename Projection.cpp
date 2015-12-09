#include <cmath>
#include "Projection.h"

Projection::Projection(Real fov, Real znear, Real zfar, unsigned width, unsigned height)
	: m_fov(fov)
	, m_znear(znear)
	, m_zfar(zfar)
	, m_width(width)
	, m_height(height)
{ }

Matrix4 Projection::GetProjectionMatrix() const
{
	Real aspect = (Real)m_width / (Real)m_height;
	Real scalex = 1 / tan(m_fov * DEG_TO_RAD * 0.5f);
	Real scaley = scalex * aspect;
	Real zClip1 = -((m_zfar + m_znear) / (m_zfar - m_znear));
	Real zClip2 = -((2 * m_znear * m_zfar) / (m_zfar - m_znear));

	return {{{
		{ scalex, 0.0,    0.0,    0.0    },
		{ 0.0,    scaley, 0.0,    0.0    },
		{ 0.0,    0.0,    zClip1, zClip2 },
		{ 0.0,    0.0,    -1.0,   0.0    }
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
