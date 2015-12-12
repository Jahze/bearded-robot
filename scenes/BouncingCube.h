#pragma once

#pragma once

#include "Geometry.h"
#include "Matrix.h"
#include "Scene.h"

namespace scene
{

class BouncingCube
	: public IScene
{
public:
	void Update(long long ms)
	{
		if (! m_stopped)
		{
			Real seconds = static_cast<Real>(ms) * 0.001;
			Real acceleration = m_acceleration;

			bool goingUp = false;

			// Going up
			if (m_speed > 0.0)
			{
				goingUp = true;
				acceleration *= 1.2;
				m_highest = m_lasty;
			}

			m_speed += acceleration * seconds;

			Real y = m_lasty + (m_speed * seconds);

			if (y < -30.0)
			{
				y = -30.0;
				m_speed = -m_speed * 0.8;

				if (! goingUp && m_highest < -29.0)
					m_stopped = true;
			}

			m_lasty = y;
		}
		else
		{
			m_speed = 0.0;
			m_lasty = 30.0;
			m_highest = 30.0;
			m_stopped = false;
		}

		Matrix4 modelTransform =
			Matrix4::Translation({ 0.0, 0.0, -70.0 });

		modelTransform = modelTransform * Matrix4::Translation({0.0, m_lasty, 0.0});

		m_cube.SetModelMatrix(modelTransform);
	}

	ObjectIterator GetObjects()
	{
		return ObjectIterator({ &m_cube });
	}

private:
	Real m_speed = 0.0;
	Real m_acceleration = -9.8;

	Real m_lasty = 30.0;
	Real m_highest = 30.0;
	bool m_stopped = false;

	geometry::Cube m_cube { 10.0 };
};

}
