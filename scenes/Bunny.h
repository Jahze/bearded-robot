#pragma once

#include "Geometry.h"
#include "Matrix.h"
#include "Scene.h"

#include "ObjReader.h"

namespace scene
{

class Bunny
	: public IScene
{
public:
	Bunny()
	{
		ObjReader reader;

		if (reader.Read("models\\bunny.wfobj"))
		{
			m_bunny = reader.GetModel();
			m_bunny->SetModelMatrix(Matrix4::Translation({ 0.0, -5.0, -50.0 }) * Matrix4::Scale({ 80.0, 80.0, 80.0 }));
		}
	}

	void Update(long long ms)
	{
		const Real kRotationPerSec = 50.0;

		Real toRotate = (kRotationPerSec / 1000.0) * ms;

		m_rotation += toRotate;

		if (m_rotation > 360.0)
			m_rotation = 0.0;

		Matrix4 modelTransform =
			Matrix4::Translation({ 0.0, -5.0, -50.0 }) *
			Matrix4::RotationAboutY(Units::Degrees, m_rotation) *
			Matrix4::Scale({ 80.0, 80.0, 80.0 });

		m_bunny->SetModelMatrix(modelTransform);
	}

	ObjectIterator GetObjects()
	{
		if (m_bunny)
			return ObjectIterator({ m_bunny.get() });

		return ObjectIterator({});
	}

private:
	Real m_rotation = 0.0;
	std::unique_ptr<geometry::Object> m_bunny;
};

}
