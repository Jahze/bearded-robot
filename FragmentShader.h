#pragma once

#include <array>
#include "Colour.h"
#include "VertexShader.h"

class FragmentShader
{
public:
	Colour Execute(int x, int y) const;

	void SetLightPosition(const Vector3 & position)
	{
		m_lightPosition = position;
	}

	void SetTriangleContext(const std::array<VertexShaderOutput, 3> * triangle)
	{
		m_triangleContext = triangle;
	}

private:
	VertexShaderOutput InterpolateForContext(int x, int y) const;

private:
	const std::array<VertexShaderOutput, 3> * m_triangleContext = nullptr;
	Vector3 m_lightPosition;
};