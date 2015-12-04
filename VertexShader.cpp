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

	Vector4 v4 = { vertex.x, vertex.y, vertex.z, 1.0 };

	// TODO w == 0?

	//output.m_projected = m_projection.GetProjectionMatrix() * m_precomputedModelView * v4;
	//output.m_projected.x /= output.m_projected.w;
	//output.m_projected.y /= output.m_projected.w;
	//output.m_projected.z /= output.m_projected.w;
	//
	//Vector4 worldPos = (m_modelTransform * v4);
	//output.m_position = { worldPos.x, worldPos.y, worldPos.z };
	//assert(worldPos.z == 1.0);

	if (m_shader)
	{
		Vector4 n = { normal.x, normal.y, normal.z, 1.0 };

		m_shader->WriteGlobal("g_position", v4);
		m_shader->WriteGlobal("g_normal", n);
		m_shader->WriteGlobal("g_model", m_modelTransform);
		m_shader->WriteGlobal("g_view", m_viewTransform);
		m_shader->WriteGlobal("g_projection", m_projection.GetProjectionMatrix());

		m_shader->Execute();

		m_shader->LoadGlobal("g_world_position", &output.m_position);
		m_shader->LoadGlobal("g_projected_position", &output.m_projected);
	}

	// TODO: this is removing the translation component -> need a better way to express it
	output.m_normal = (Matrix3)m_modelTransform * normal;

	output.m_normal.Normalize();

	output.m_screenX = m_projection.ToScreenX(output.m_projected.x);
	output.m_screenY = m_projection.ToScreenY(output.m_projected.y);

	return output;
}
