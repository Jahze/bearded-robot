#pragma once

#include <array>
#include "Colour.h"
#include "ShadyObject.h"
#include "VertexShader.h"

class FragmentShader
{
public:
	FragmentShader(ShadyObject * shader);

	Colour Execute(int x, int y) const;

	void SetLightPosition(const Vector3 & position);

	void SetTriangleContext(const std::array<VertexShaderOutput, 3> * triangle);

private:
	VertexShaderOutput InterpolateForContext(int x, int y) const;

private:
	const std::array<VertexShaderOutput, 3> * m_triangleContext = nullptr;
	Real m_totalArea;

	Vector3 m_lightPosition;
	ShadyObject *m_shader;

	ShadyObject::GlobalWriter m_g_light0_position;
	ShadyObject::GlobalWriter m_g_world_position;
	ShadyObject::GlobalWriter m_g_world_normal;

	ShadyObject::GlobalReader m_g_colour;
};
