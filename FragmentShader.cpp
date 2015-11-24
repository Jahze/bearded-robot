
#include <cassert>
#include "FragmentShader.h"

namespace
{
	Real AreaOfTriangle(Real x1, Real y1, Real x2, Real y2, Real x3, Real y3)
	{
		return std::abs((x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2)) / 2.0);
	}

	Real Clamp(Real value, Real min, Real max)
	{
		return std::min<Real>(std::max<Real>(value, min), max);
	}
}

Colour FragmentShader::Execute(int x, int y) const
{
	VertexShaderOutput interpolated = InterpolateForContext(x, y);
	Vector3 directionToLight = m_lightPosition - interpolated.m_position;
	Real lightDistance = directionToLight.Length();

	directionToLight.Normalize();

	const Vector3 ambient(0.2, 0.2, 0.2);
	const Vector3 diffuse(1.0, 1.0, 1.0);
	const Vector3 specular(1.0, 1.0, 1.0);

	Real dot = interpolated.m_normal.Dot(directionToLight);

	Real clamped = std::max<Real>(0.0, dot);

	const Real k = 0.1;
	const int shininess = 200;
	Real attenuation = 1.0 / (lightDistance * k);
	Vector3 colour = ambient + (diffuse * clamped * attenuation);

	//if (dot > 0.0)
	//{
	//	Vector3 fromLight = -directionToLight;
	//	Vector3 reflected = fromLight - interpolated.m_normal * (2.0 * fromLight.Dot(interpolated.m_normal));
	//
	//	Vector3 toCamera = Vector3::UnitZ - interpolated.m_position;
	//	toCamera.Normalize();
	//
	//	Real cosa = reflected.Dot(toCamera);
	//	cosa = std::max<Real>(0.0, cosa);
	//
	//	colour = colour + (specular * std::pow(cosa, shininess) * attenuation);
	//}

	return { Clamp(colour.x, 0.0, 1.0), Clamp(colour.y, 0.0, 1.0), Clamp(colour.z, 0.0, 1.0) };
}

VertexShaderOutput FragmentShader::InterpolateForContext(int x, int y) const
{
	// https://en.wikibooks.org/wiki/GLSL_Programming/Rasterization
	assert(m_triangleContext);

	Real totalArea = AreaOfTriangle(
		(*m_triangleContext)[0].m_screenX, (*m_triangleContext)[0].m_screenY,
		(*m_triangleContext)[1].m_screenX, (*m_triangleContext)[1].m_screenY,
		(*m_triangleContext)[2].m_screenX, (*m_triangleContext)[2].m_screenY
	);

	Real subArea1 = AreaOfTriangle(
		(*m_triangleContext)[1].m_screenX, (*m_triangleContext)[1].m_screenY,
		(*m_triangleContext)[2].m_screenX, (*m_triangleContext)[2].m_screenY,
		x, y
	);

	Real subArea2 = AreaOfTriangle(
		(*m_triangleContext)[0].m_screenX, (*m_triangleContext)[0].m_screenY,
		(*m_triangleContext)[2].m_screenX, (*m_triangleContext)[2].m_screenY,
		x, y
	);

	Real subArea3 = AreaOfTriangle(
		(*m_triangleContext)[0].m_screenX, (*m_triangleContext)[0].m_screenY,
		(*m_triangleContext)[1].m_screenX, (*m_triangleContext)[1].m_screenY,
		x, y
	);

	Real a1 = subArea1 / totalArea;
	Real a2 = subArea2 / totalArea;
	Real a3 = subArea3 / totalArea;

	a1 /= (*m_triangleContext)[0].m_projected.w;
	a2 /= (*m_triangleContext)[1].m_projected.w;
	a3 /= (*m_triangleContext)[2].m_projected.w;

	Real total = a1 + a2 + a3;

	VertexShaderOutput output;

	output.m_position.x =
		(*m_triangleContext)[0].m_position.x * a1 +
		(*m_triangleContext)[1].m_position.x * a2 +
		(*m_triangleContext)[2].m_position.x * a3;

	output.m_position.y =
		(*m_triangleContext)[0].m_position.y * a1 +
		(*m_triangleContext)[1].m_position.y * a2 +
		(*m_triangleContext)[2].m_position.y * a3;

	output.m_position.z =
		(*m_triangleContext)[0].m_position.z * a1 +
		(*m_triangleContext)[1].m_position.z * a2 +
		(*m_triangleContext)[2].m_position.z * a3;

	output.m_position = output.m_position / total;

	//

	output.m_normal.x =
		(*m_triangleContext)[0].m_normal.x * a1 +
		(*m_triangleContext)[1].m_normal.x * a2 +
		(*m_triangleContext)[2].m_normal.x * a3;

	output.m_normal.y =
		(*m_triangleContext)[0].m_normal.y * a1 +
		(*m_triangleContext)[1].m_normal.y * a2 +
		(*m_triangleContext)[2].m_normal.y * a3;

	output.m_normal.z =
		(*m_triangleContext)[0].m_normal.z * a1 +
		(*m_triangleContext)[1].m_normal.z * a2 +
		(*m_triangleContext)[2].m_normal.z * a3;

	output.m_normal = output.m_normal / total;

	return output;
}
