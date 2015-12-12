#pragma once

#include <vector>
#include "Matrix.h"
#include "Vector.h"

namespace geometry
{

class Triangle
{
public:
	Vector3 points[3];

	Triangle(const Vector3 & p1, const Vector3 & p2, const Vector3 & p3)
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
	}

	Vector3 Normal() const
	{
		Vector3 u = points[1] - points[0];
		Vector3 v = points[2] - points[0];

		Vector3 normal = {
			u.y * v.z - u.z * v.y,
			u.z * v.x - u.x * v.z,
			u.x * v.y - u.y * v.x
		};

		return normal.NormalizedCopy();
	}

	Vector3 Centre() const
	{
		return {
			(points[0].x + points[1].x + points[2].x) / 3.0f,
			(points[0].y + points[1].y + points[2].y) / 3.0f,
			(points[0].z + points[1].z + points[2].z) / 3.0f,
		};
	}

	bool IsAntiClockwise() const
	{
		// http://myweb.lmu.edu/dondi/share/cg/backface-culling-and-vectors.pdf

		// Assumes the camera is looking in direction (0, 0, -1)

		return Normal().z <= 0.0f;
	}
};

class Object
{
public:
	const Matrix4 & GetModelMatrix() const
	{
		return m_model;
	}

	void SetModelMatrix(const Matrix4 & model)
	{
		m_model = model;
	}

	const std::vector<Triangle> & GetTriangles() const
	{
		return m_triangles;
	}

protected:
	Matrix4 m_model = Matrix4::Identity;
	std::vector<geometry::Triangle> m_triangles;
};

class Cube
	: public Object
{
public:
	Cube(Real size = 1.0)
	{
		Triangle faces[] =
		{
			// Front face
			{	-Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				-Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ,
				Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ },

			{	Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ,
				Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				-Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ },

			// Left face
			{	-Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				-Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ),
				-Vector3::UnitX + -Vector3::UnitY + Vector3::UnitZ},

			{	-Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ),
				-Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ),
				-Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ },

			// Right face
			{	Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ,
				Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ) },

			{	Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ),
				Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ) },

			// Top face
			{	-Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				-Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ) },

			{	Vector3::UnitX + Vector3::UnitY + Vector3::UnitZ,
				Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ),
				-Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ) },

			// Bottom face
			{	-Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ,
				-Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ),
				Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ, },

			{	Vector3::UnitX + (-Vector3::UnitY) + Vector3::UnitZ,
				-Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ),
				Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ) },

			// Back face
			{	-Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ),
				Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ),
				-Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ) },
			
			{	-Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ),
				Vector3::UnitX + Vector3::UnitY + (-Vector3::UnitZ),
				Vector3::UnitX + (-Vector3::UnitY) + (-Vector3::UnitZ) },
		};

		Real half = size / 2.0;

		for (auto & face : faces)
		{
			for (int i = 0; i < 3; ++i)
				face.points[i] = face.points[i] * half;
		}

		m_triangles.insert(m_triangles.begin(), std::begin(faces), std::end(faces));
	}
};

}
