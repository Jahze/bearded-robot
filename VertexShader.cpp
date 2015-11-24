#include "Projection.h"
#include "VertexShader.h"


VertexShader::VertexShader(const Projection & projection)
	: m_projection(projection)
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

	output.m_projected = m_projection.GetProjectionMatrix() * m_modelTransform * v4;
	output.m_projected.x /= output.m_projected.w;
	output.m_projected.y /= output.m_projected.w;
	output.m_projected.z /= output.m_projected.w;

	output.m_position = m_modelTransform * vertex;

	// TODO: this is removing the translation component -> need a better way to express it
	output.m_normal = (Matrix3)m_modelTransform * normal;

	output.m_normal.Normalize();

	output.m_screenX = m_projection.ToScreenX(output.m_projected.x);
	output.m_screenY = m_projection.ToScreenY(output.m_projected.y);

	return output;
}
