#pragma once

#include "Geometry.h"
#include "Matrix.h"
#include "Scene.h"

#include "ObjReader.h"

namespace scene
{
	class Teapot
		: public IScene
	{
	public:
		Teapot()
		{
			ObjReader reader;

			if (reader.Read("models\\teapot.wfobj"))
			{
				m_teapot = reader.GetModel();
				m_teapot->SetModelMatrix(Matrix4::Translation({ 0.0, -5.0, -30.0 }) * Matrix4::Scale({ 10.0, 10.0, 10.0 }));

				// Pass 1 backface rendering for outlines
				ShadyObject * outlineVertexShader = ShaderCache::Get().GetVertexShader("outline_vertex.shader");
				m_teapot->SetVertexShader(0, outlineVertexShader);

				ShadyObject * outlineFragmentShader = ShaderCache::Get().GetFragmentShader("outline_fragment.shader");
				m_teapot->SetFragmentShader(0, outlineFragmentShader);

				m_teapot->SetReverseCull(0, true);

				// Pass 2 cel shaded
				m_teapot->AddPass();

				m_teapot->SetVertexShader(1, ShaderCache::Get().DefaultVertexShader());

				ShadyObject * celFragmentShader = ShaderCache::Get().GetFragmentShader("fragment_cel.shader");
				m_teapot->SetFragmentShader(1, celFragmentShader);
			}
		}

		void Update(long long ms)
		{
			const Real kRotationPerSec = 25.0;

			Real toRotate = (kRotationPerSec / 1000.0) * ms;

			m_rotation += toRotate;

			if (m_rotation > 360.0)
				m_rotation = 0.0;

			Matrix4 modelTransform =
				Matrix4::Translation({ 0.0, -5.0, -30.0 }) *
				Matrix4::RotationAboutY(Units::Degrees, m_rotation) *
				Matrix4::Scale({ 10.0, 10.0, 10.0 });

			m_teapot->SetModelMatrix(modelTransform);
		}

		ObjectIterator GetObjects()
		{
			if (m_teapot)
				return ObjectIterator({ m_teapot.get() });

			return ObjectIterator({});
		}

	private:
		Real m_rotation = 0.0;
		std::unique_ptr<geometry::Object> m_teapot;
	};

}
