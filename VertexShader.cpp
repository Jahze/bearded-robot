#include "Projection.h"
#include "ShadyObject.h"
#include "VertexShader.h"


VertexShader::VertexShader(const Projection & projection, ShadyObject * shader)
	: m_projection(projection)
	, m_shader(shader)
{ }

VertexShaderOutput VertexShader::Execute(const Vector3 & vertex) const
{
	return Execute(vertex, Vector3::Zero);
}

VertexShaderOutput VertexShader::Execute(const Vector3 & vertex, const Vector3 & normal) const
{
	VertexShaderOutput output;

	Vector4 posisiton4 = { vertex.x, vertex.y, vertex.z, 1.0 };

	// set w to 0.0 so that translations in transformation matrices have no affect (it's a ray)
	Vector4 normal4 = { normal.x, normal.y, normal.z, 0.0 };

	if (m_shader)
	{
		m_shader->WriteGlobal("g_position", posisiton4);
		m_shader->WriteGlobal("g_normal", normal4);
		m_shader->WriteGlobal("g_model", m_modelTransform);
		m_shader->WriteGlobal("g_view", m_viewTransform);
		m_shader->WriteGlobal("g_projection", m_projection.GetProjectionMatrix());

		m_shader->Execute();

		m_shader->LoadGlobal("g_world_position", &output.m_position);
		m_shader->LoadGlobal("g_projected_position", &output.m_projected);
		m_shader->LoadGlobal("g_world_normal", &output.m_normal);
	}

	output.m_screenX = m_projection.ToScreenX(output.m_projected.x);
	output.m_screenY = m_projection.ToScreenY(output.m_projected.y);

	return output;
}
