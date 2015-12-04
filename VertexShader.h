#pragma once

#include "Colour.h"
#include "Matrix.h"

class Camera;
class Projection;
class ShadyObject;

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
	VertexShader(const Projection & projection, ShadyObject * shader);

	VertexShaderOutput Execute(const Vector3 & vertex) const;
	VertexShaderOutput Execute(const Vector3 & vertex, const Vector3 & normal) const;

	void SetModelTransform(const Matrix4 & model)
	{
		m_modelTransform = model;
		m_precomputedModelView = m_viewTransform * m_modelTransform;
	}

	void SetViewTransform(const Matrix4 & view)
	{
		m_viewTransform = view;
		m_precomputedModelView = m_viewTransform * m_modelTransform;
	}

private:
	const Projection & m_projection;
	Matrix4 m_modelTransform;
	Matrix4 m_viewTransform;
	Matrix4 m_precomputedModelView;
	ShadyObject *m_shader;
};
