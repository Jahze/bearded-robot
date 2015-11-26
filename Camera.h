#pragma once

#include "InputHandler.h"
#include "Matrix.h"
#include "Vector.h"

class Camera
	: public IMouseListener
{
public:
	Matrix4 GetTransform() const;

	void SetPosition(const Vector3 & position)
	{
		m_position = position;
	}

	void Move(const Vector3 & relative);

	void Yaw(Units::Radians_t, Real theta)
	{
		m_yaw += theta;

		//if (m_yaw > 360.0 * DEG_TO_RAD)
		//	m_yaw -= 360.0 * DEG_TO_RAD;
		//
		//if (m_yaw < 0.0)
		//	m_yaw += 360.0 * DEG_TO_RAD;
	}

	void Pitch(Units::Radians_t, Real theta)
	{
		m_pitch += theta;

		if (m_pitch > 90.0 * DEG_TO_RAD)
			m_pitch = 90.0 * DEG_TO_RAD;
		else if (m_pitch < -90.0 * DEG_TO_RAD)
			m_pitch = -90.0 * DEG_TO_RAD;

	}

	void Reset()
	{
		m_position = Vector3::Zero;
		m_pitch = 0.0;
		m_yaw = 0.0;
	}

	void MouseMoved(int xdelta, int ydelta);

private:
	Vector3 m_position = Vector3::Zero;
	Real m_pitch = 0.0; // pitch = rot around x
	Real m_yaw = 0.0; // yaw = rot around y
};
