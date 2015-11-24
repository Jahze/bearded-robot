#include <cmath>
#include "Projection.h"

Projection::Projection(Real fovx, Real fovy, Real znear, Real zfar)
{
	m_fovxCoeff = atan((fovx * DEG_TO_RAD) / 2.0f);
	m_fovyCoeff = atan((fovy * DEG_TO_RAD) / 2.0f);
	m_zClip1 = -((zfar + znear) / (zfar - znear));
	m_zClip2 = -((2 * znear * zfar) / (zfar - znear));
}

Matrix4 Projection::GetProjectionMatrix() const
{
	return {{{
		{ m_fovxCoeff, 0.0, 0.0, 0.0 },
		{ 0.0, m_fovyCoeff, 0.0, 0.0 },
		{ 0.0, 0.0, m_zClip1, m_zClip2 },
		{ 0.0, 0.0, -1.0, 0.0 }
	}}};
}

void Projection::Project(Vector3 & v)
{
	Real w = -v.z;

	// TODO w == 0?

	v.x *= m_fovxCoeff;
	v.x /= w;

	v.y *= m_fovyCoeff;
	v.y /= w;

	v.z = (m_zClip1 * v.z) + m_zClip2;
	v.z /= w;
}

void Projection::ToScreen(unsigned width, unsigned height, Vector3 & v)
{
	v.x = ((v.x + 1.0f) / 2.0f) * width;
	v.y = (1.0f - ((v.y + 1.0f) / 2.0f)) * height;
}

//bool Projection::ShouldClip(const Vector<4> & v)
//{
//	for (unsigned i = 0; i < 4; ++i)
//	{
//		if (v.elements[i] < -1.0f || v.elements[i] > 1.0f)
//			return false;
//	}
//
//	return true;
//}
