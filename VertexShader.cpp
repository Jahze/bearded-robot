#include "Projection.h"
#include "ShadyObject.h"
#include "VertexShader.h"


VertexShader::VertexShader(const Projection & projection, ShadyObject * shader)
	: m_projection(projection)
{
	SetShader(shader);
}

void VertexShader::SetShader(ShadyObject * shader)
{
	m_shader = shader;
	m_g_position = shader->GetGlobalLocation("g_position");
	m_g_normal = shader->GetGlobalLocation("g_normal");
	m_g_model = shader->GetGlobalLocation("g_model");
	m_g_view = shader->GetGlobalLocation("g_view");
	m_g_projection = shader->GetGlobalLocation("g_projection");
	m_g_projected_position = shader->GetGlobalReader("g_projected_position");
	m_g_world_position = shader->GetGlobalReader("g_world_position");
	m_g_world_normal = shader->GetGlobalReader("g_world_normal");
}

VertexShaderOutput VertexShader::Execute(const Vector3 & vertex) const
{
	return Execute(vertex, Vector3::Zero);
}

VertexShaderOutput VertexShader::Execute(const Vector3 & vertex, const Vector3 & normal) const
{
	VertexShaderOutput output;

	Vector4 position4 = { vertex.x, vertex.y, vertex.z, 1.0 };

	// set w to 0.0 so that translations in transformation matrices have no affect (it's a ray)
	Vector4 normal4 = { normal.x, normal.y, normal.z, 0.0 };

	if (m_shader)
	{
		m_g_position.Write(position4);
		m_g_normal.Write(normal4);
		m_g_model.Write(m_modelTransform);
		m_g_view.Write(m_viewTransform);
		m_g_projection.Write(m_projection.GetProjectionMatrix());

		m_shader->Execute();

		// g_world_position is actually relative to the camera, i.e. the camera
		// is at the origin. this means that any other "world" positions have to
		// be in view space too. for example a the light position is transformed
		// before it goes into the fragment shader.

		m_g_world_position.Read(output.m_position);
		m_g_projected_position.Read(output.m_projected);
		m_g_world_normal.Read(output.m_normal);
	}

	output.m_screen.x = m_projection.ToScreenX(output.m_projected.x);
	output.m_screen.y = m_projection.ToScreenY(output.m_projected.y);

	return output;
}
