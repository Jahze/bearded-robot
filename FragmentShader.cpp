#include <cassert>
#include "FragmentShader.h"
#include "FrameBuffer.h"
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
}

FragmentShader::FragmentShader(ShadyObject * shader)
{
	SetShader(shader);
}

bool FragmentShader::Execute(int x, int y, FrameBuffer * buffer, Colour & colour) const
{
	InterpolatedValues interpolated = InterpolateForContext(x, y);

	if (buffer->GetDepth(x, y) < interpolated.z)
		return false;

	buffer->SetDepth(x, y, interpolated.z);

	if (m_shader)
	{
		m_g_light0_position.Write(m_lightPosition);
		m_g_world_position.Write(interpolated.position);
		m_g_world_normal.Write(interpolated.normal);

		m_shader->Execute();

		Vector4 c;

		m_g_colour.Read(c);

		colour = { c.x, c.y, c.z };
		return true;
	}

	colour = Colour::White;
	return true;
}

void FragmentShader::SetLightPosition(const Vector3 & position)
{
	m_lightPosition = position;
	m_g_light0_position.Write(m_lightPosition);
}

void FragmentShader::SetTriangleContext(const std::array<VertexShaderOutput, 3> * triangle)
{
	m_triangleContext = triangle;

	m_totalArea = AreaOfTriangle(
		(*m_triangleContext)[0].m_screen.x, (*m_triangleContext)[0].m_screen.y,
		(*m_triangleContext)[1].m_screen.x, (*m_triangleContext)[1].m_screen.y,
		(*m_triangleContext)[2].m_screen.x, (*m_triangleContext)[2].m_screen.y
	);
}

void FragmentShader::SetShader(ShadyObject * shader)
{
	m_shader = shader;
	m_g_light0_position = shader->GetGlobalLocation("g_light0_position");
	m_g_world_position = shader->GetGlobalLocation("g_world_position");
	m_g_world_normal = shader->GetGlobalLocation("g_world_normal");
	m_g_colour = shader->GetGlobalReader("g_colour");
}

namespace
{
	Vector3 InterpolateVector(const Vector3 & vector0, const Vector3 & vector1, const Vector3 & vector2,
		Real a1, Real a2, Real a3, Real total)
	{
		return{
			(vector0.x * a1 + vector1.x * a2 + vector2.x * a3) / total,
			(vector0.y * a1 + vector1.y * a2 + vector2.y * a3) / total,
			(vector0.z * a1 + vector1.z * a2 + vector2.z * a3) / total,
		};
	}
}

InterpolatedValues FragmentShader::InterpolateForContext(int x, int y) const
{
	// https://en.wikibooks.org/wiki/GLSL_Programming/Rasterization
	assert(m_triangleContext);

	const VertexShaderOutput & triangle0 = (*m_triangleContext)[0];
	const VertexShaderOutput & triangle1 = (*m_triangleContext)[1];
	const VertexShaderOutput & triangle2 = (*m_triangleContext)[2];

	Real x_ = x + 0.5;
	Real y_ = y + 0.5;

	Real subArea1 = AreaOfTriangle(
		triangle1.m_screen.x, triangle1.m_screen.y,
		triangle2.m_screen.x, triangle2.m_screen.y,
		x_, y_
	);

	Real subArea2 = AreaOfTriangle(
		triangle0.m_screen.x, triangle0.m_screen.y,
		triangle2.m_screen.x, triangle2.m_screen.y,
		x_, y_
	);

	Real subArea3 = AreaOfTriangle(
		triangle0.m_screen.x, triangle0.m_screen.y,
		triangle1.m_screen.x, triangle1.m_screen.y,
		x_, y_
	);

	Real a1 = subArea1 / (m_totalArea *  triangle0.m_projected.w);
	Real a2 = subArea2 / (m_totalArea *  triangle1.m_projected.w);
	Real a3 = subArea3 / (m_totalArea *  triangle2.m_projected.w);

	Real total = a1 + a2 + a3;

	InterpolatedValues output;

	output.position = InterpolateVector(
		triangle0.m_position,
		triangle1.m_position,
		triangle2.m_position,
		a1, a2, a3, total);
		

	output.normal = InterpolateVector(
		triangle0.m_normal,
		triangle1.m_normal,
		triangle2.m_normal,
		a1, a2, a3, total);

	output.z = (triangle0.m_projected.z * a1 + triangle1.m_projected.z * a2 + triangle2.m_projected.z * a3) / total;

	return output;
}
