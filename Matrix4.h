#pragma once

#include <array>
#include <cassert>
#include "Types.h"
#include "Vector.h"

class Matrix4
{
public:
	Matrix4()
		: m_values()
	{ }

	Matrix4(const std::array<std::array<Real, 4>, 4> & values)
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
				m_values[i][j] = values[i][j];
		}
	}

	Matrix4 operator*(const Matrix4 & rhs) const
	{
		Matrix4 m;

		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				for (int k = 0; k < 4; k++)
				{
					m.m_values[i][j] += m_values[i][k] * rhs.m_values[k][j];
				}
			}
		}

		return m;
	}

	Vector4 operator*(const Vector4 & rhs) const
	{
		return {
			m_values[0][0] * rhs.x + m_values[0][1] * rhs.y + m_values[0][2] * rhs.z + m_values[0][3] * rhs.w,
			m_values[1][0] * rhs.x + m_values[1][1] * rhs.y + m_values[1][2] * rhs.z + m_values[1][3] * rhs.w,
			m_values[2][0] * rhs.x + m_values[2][1] * rhs.y + m_values[2][2] * rhs.z + m_values[2][3] * rhs.w,
			m_values[3][0] * rhs.x + m_values[3][1] * rhs.y + m_values[3][2] * rhs.z + m_values[3][3] * rhs.w,
		};
	}

	Vector3 operator*(const Vector3 & rhs) const
	{
		Vector4 v4 = (*this) * Vector4(rhs.x, rhs.y, rhs.z, 1.0);

		return { v4.x / v4.w, v4.y / v4.w, v4.z / v4.w };
	}

	static Matrix4 Translation(const Vector3 & rhs)
	{
		return {{{
			{ 1.0, 0.0, 0.0, rhs.x },
			{ 0.0, 1.0, 0.0, rhs.y },
			{ 0.0, 0.0, 1.0, rhs.z },
			{ 0.0, 0.0, 0.0, 1.0 }
		}}};
	}

	static Matrix4 RotationAboutY(Units::Radians_t, Real theta)
	{
		const Real c = cos(theta);
		const Real s = sin(theta);

		return {{{
			{ c,   0.0, s,   0.0 },
			{ 0.0, 1.0, 0.0, 0.0 },
			{ -s,  0.0, c,   0.0 },
			{ 0.0, 0.0, 0.0, 1.0 }
		}}};
	}

	static Matrix4 RotationAboutY(Units::Degrees_t, Real theta)
	{
		return RotationAboutY(Units::Radians, theta * DEG_TO_RAD);
	}

private:
	Real m_values[4][4];
};
