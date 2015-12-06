#include <cassert>
#include "FragmentShader.h"
#include "ShadyObject.h"

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

	const std::string g_light0_position("g_light0_position");
	const std::string g_world_position("g_world_position");
	const std::string g_world_normal("g_world_normal");
	const std::string g_colour("g_colour");
}

FragmentShader::FragmentShader(ShadyObject * shader)
	: m_shader(shader)
	, m_g_light0_position(shader->GetGlobalLocation("g_light0_position"))
	, m_g_world_position(shader->GetGlobalLocation("g_world_position"))
	, m_g_world_normal(shader->GetGlobalLocation("g_world_normal"))
	, m_g_colour(shader->GetGlobalReader("g_colour"))
{ }

Colour FragmentShader::Execute(int x, int y) const
{
	VertexShaderOutput interpolated = InterpolateForContext(x, y);

	if (m_shader)
	{
		m_g_light0_position.Write(m_lightPosition);
		m_g_world_position.Write(interpolated.m_position);
		m_g_world_normal.Write(interpolated.m_normal);

		m_shader->Execute();

		Vector4 c;

		m_g_colour.Read(c);

		return { c.x, c.y, c.z };
	}

	return Colour::White;
}

void FragmentShader::SetTriangleContext(const std::array<VertexShaderOutput, 3> * triangle)
{
	m_triangleContext = triangle;

	m_totalArea = AreaOfTriangle(
		(*m_triangleContext)[0].m_screenX, (*m_triangleContext)[0].m_screenY,
		(*m_triangleContext)[1].m_screenX, (*m_triangleContext)[1].m_screenY,
		(*m_triangleContext)[2].m_screenX, (*m_triangleContext)[2].m_screenY
	);
}

namespace
{
	Vector3 InterpolateVector(const Vector3 & vector0, const Vector3 & vector1, const Vector3 & vector2,
		Real a1, Real a2, Real a3, Real total)
	{
		Vector3 out;

		out.x = vector0.x * a1 +
			vector1.x * a2 +
			vector2.x * a3;

		out.y =
			vector0.y * a1 +
			vector1.y * a2 +
			vector2.y * a3;

		out.z =
			vector0.z * a1 +
			vector1.z * a2 +
			vector2.z * a3;

		return out / total;
	}
}

VertexShaderOutput FragmentShader::InterpolateForContext(int x, int y) const
{
	// https://en.wikibooks.org/wiki/GLSL_Programming/Rasterization
	assert(m_triangleContext);

	const VertexShaderOutput & triangle0 = (*m_triangleContext)[0];
	const VertexShaderOutput & triangle1 = (*m_triangleContext)[1];
	const VertexShaderOutput & triangle2 = (*m_triangleContext)[2];

	Real subArea1 = AreaOfTriangle(
		triangle1.m_screenX, triangle1.m_screenY,
		triangle2.m_screenX, triangle2.m_screenY,
		x, y
	);

	Real subArea2 = AreaOfTriangle(
		triangle0.m_screenX, triangle0.m_screenY,
		triangle2.m_screenX, triangle2.m_screenY,
		x, y
	);

	Real subArea3 = AreaOfTriangle(
		triangle0.m_screenX, triangle0.m_screenY,
		triangle1.m_screenX, triangle1.m_screenY,
		x, y
	);

	Real a1 = subArea1 / (m_totalArea *  triangle0.m_projected.w);
	Real a2 = subArea2 / (m_totalArea *  triangle1.m_projected.w);
	Real a3 = subArea3 / (m_totalArea *  triangle2.m_projected.w);

	Real total = a1 + a2 + a3;

	VertexShaderOutput output;

	output.m_position = InterpolateVector(
		triangle0.m_position,
		triangle1.m_position,
		triangle2.m_position,
		a1, a2, a3, total);
		

	output.m_normal = InterpolateVector(
		triangle0.m_normal,
		triangle1.m_normal,
		triangle2.m_normal,
		a1, a2, a3, total);

	return output;
}
