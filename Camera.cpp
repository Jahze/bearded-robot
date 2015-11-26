#include "Camera.h"

Matrix4 Camera::GetTransform() const
{
	// http://stackoverflow.com/questions/11994819/how-can-i-move-the-camera-correctly-in-3d-space?rq=1
	// http://www.3dgep.com/understanding-the-view-matrix/
	// see comments about rotation order

	return Matrix4::RotationAboutX(Units::Radians, -m_pitch)
		* Matrix4::RotationAboutY(Units::Radians, -m_yaw)
		* Matrix4::Translation(-m_position);
}

void Camera::Move(const Vector3 & relative)
{
	m_position += Matrix4::RotationAboutY(Units::Radians, m_yaw)
		* Matrix4::RotationAboutX(Units::Radians, m_pitch)
		* relative;
}

void Camera::MouseMoved(int xdelta, int ydelta)
{
	if (xdelta != 0)
		Yaw(Units::Radians, -xdelta * 0.2 * DEG_TO_RAD);

	if (ydelta != 0)
		Pitch(Units::Radians, -ydelta * 0.2 * DEG_TO_RAD);
}
