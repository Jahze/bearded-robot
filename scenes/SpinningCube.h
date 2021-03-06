#pragma once

#include "Geometry.h"
#include "Matrix.h"
#include "Scene.h"

namespace scene
{

class SpinningCube
	: public IScene
{
public:
	SpinningCube()
	{
		m_cube.SetModelMatrix(Matrix4::Translation({ 0.0, 0.0, -50.0 }));
	}

	void Update(long long ms)
	{
		const Real kRotationPerSec = 50.0;

		Real toRotate = (kRotationPerSec / 1000.0) * ms;

		m_rotation += toRotate;

		if (m_rotation > 360.0)
			m_rotation = 0.0;

		Matrix4 modelTransform =
			Matrix4::Translation({ 0.0, 0.0, -50.0 });

		modelTransform = modelTransform * Matrix4::RotationAboutY(Units::Degrees, m_rotation);

		m_cube.SetModelMatrix(modelTransform);
	}

	ObjectIterator GetObjects()
	{
		return ObjectIterator({ &m_cube });
	}

private:
	Real m_rotation = 0.0;
	geometry::Cube m_cube{10.0};
};

}
