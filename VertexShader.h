#pragma once

#include "Colour.h"
#include "Matrix4.h"

class Projection;

struct VertexShaderOutput
{
	Vector4 m_projected;
	Vector3 m_position;
	Vector3 m_normal;
	Real m_screenX;
	Real m_screenY;
};

class VertexShader
{
public:
	VertexShader(const Projection & projection);

	VertexShaderOutput Execute(const Vector3 & vertex) const;
	VertexShaderOutput Execute(const Vector3 & vertex, const Vector3 & normal) const;

	void SetModelTransform(const Matrix4 & model)
	{
		m_modelTransform = model;
	}

private:
	const Projection & m_projection;
	Matrix4 m_modelTransform;
};
