#pragma once

#include <cmath>
#include <vector>
#include "Matrix.h"
#include "Vector.h"

namespace geometry
{

class Triangle
{
public:
	Vector3 points[3];
	Vector3 normals[3];

	Triangle(const Vector3 & p1, const Vector3 & p2, const Vector3 & p3)
		: normals()
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;
	}

	Triangle(const Vector3 & p1, const Vector3 & p2, const Vector3 & p3,
		const Vector3 & n1, const Vector3 & n2, const Vector3 & n3)
	{
		points[0] = p1;
		points[1] = p2;
		points[2] = p3;

		normals[0] = n1;
		normals[1] = n2;
		normals[2] = n3;
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
			Vector3 normal = face.Normal();

			for (int i = 0; i < 3; ++i)
			{
				face.points[i] = face.points[i] * half;
				face.normals[i] = normal;
			}
		}

		m_triangles.insert(m_triangles.begin(), std::begin(faces), std::end(faces));
	}
};


class Sphere
	: public Object
{
public:
	Sphere(Real size = 1.0, uint32_t subdivision = 2)
	{
		Real t = (1.0 + std::sqrtf(5.0)) * 0.5;

		std::array<Vector3, 12> points =
		{{
			{ -1.0,  t,    0.0 },
			{  1.0,  t,    0.0 },
			{ -1.0, -t,    0.0 },
			{  1.0, -t,    0.0 },
			{  0.0, -1.0,  t   },
			{  0.0,  1.0,  t   },
			{  0.0, -1.0, -t   },
			{  0.0,  1.0, -t   },
			{    t,  0.0, -1.0 },
			{    t,  0.0,  1.0 },
			{   -t,  0.0, -1.0 },
			{   -t,  0.0,  1.0 },
		}};

		std::vector<Triangle> faces =
		{
			{ points[0], points[11], points[5] },
			{ points[0], points[5], points[1] },
			{ points[0], points[1], points[7] },
			{ points[0], points[7], points[10] },
			{ points[0], points[10], points[11] },
			{ points[1], points[5], points[9] },
			{ points[5], points[11], points[4] },
			{ points[11], points[10], points[2] },
			{ points[10], points[7], points[6] },
			{ points[7], points[1], points[8] },
			{ points[3], points[9], points[4] },
			{ points[3], points[4], points[2] },
			{ points[3], points[2], points[6] },
			{ points[3], points[6], points[8] },
			{ points[3], points[8], points[9] },
			{ points[4], points[9], points[5] },
			{ points[2], points[4], points[11] },
			{ points[6], points[2], points[10] },
			{ points[8], points[6], points[7] },
			{ points[9], points[8], points[1] },
		};

		for (uint32_t i = 0; i < subdivision; ++i)
		{
			std::vector<Triangle> newFaces;

			for (auto && face : faces)
			{
				Vector3 mid1pos = face.points[0] + ((face.points[1] - face.points[0]) * 0.5);
				Vector3 mid2pos = face.points[1] + ((face.points[2] - face.points[1]) * 0.5);
				Vector3 mid3pos = face.points[2] + ((face.points[0] - face.points[2]) * 0.5);

				newFaces.emplace_back(face.points[0], mid1pos, mid3pos);
				newFaces.emplace_back(face.points[1], mid2pos, mid1pos);
				newFaces.emplace_back(face.points[2], mid3pos, mid2pos);
				newFaces.emplace_back(mid1pos, mid2pos, mid3pos);
			}

			faces = std::move(newFaces);
		}

		Real half = size / 2.0;

		for (auto && face : faces)
		{
			for (int i = 0; i < 3; ++i)
			{
				face.normals[i] = face.points[i].NormalizedCopy();

				Real l = face.points[i].Length();
				face.points[i] = face.points[i] * (half / l);
			}
		}

		m_triangles.insert(m_triangles.begin(), faces.begin(), faces.end());
	}
};

}
