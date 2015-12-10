#pragma once

#include "Colour.h"
#include "Matrix.h"
#include "Point.h"
#include "ShadyObject.h"

class Camera;
class Projection;

struct VertexShaderOutput
{
	Vector4 m_projected;
	Vector3 m_position;
	Vector3 m_normal;
	Point m_screen;
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

	ShadyObject::GlobalWriter m_g_position;
	ShadyObject::GlobalWriter m_g_normal;
	ShadyObject::GlobalWriter m_g_model;
	ShadyObject::GlobalWriter m_g_view;
	ShadyObject::GlobalWriter m_g_projection;

	ShadyObject::GlobalReader m_g_projected_position;
	ShadyObject::GlobalReader m_g_world_position;
	ShadyObject::GlobalReader m_g_world_normal;
};
