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
			{ points[0], points[11], points[5] },	// 0
			{ points[0], points[5], points[1] },	// 1
			{ points[0], points[1], points[7] },	// 2
			{ points[0], points[7], points[10] },	// 3
			{ points[0], points[10], points[11] },	// 4
			{ points[1], points[5], points[9] },	// 5
			{ points[5], points[11], points[4] },	// 6
			{ points[11], points[10], points[2] },	// 7
			{ points[10], points[7], points[6] },	// 8
			{ points[7], points[1], points[8] },	// 9
			{ points[3], points[9], points[4] },	// 10
			{ points[3], points[4], points[2] },	// 11
			{ points[3], points[2], points[6] },	// 12
			{ points[3], points[6], points[8] },	// 13
			{ points[3], points[8], points[9] },	// 14
			{ points[4], points[9], points[5] },	// 15
			{ points[2], points[4], points[11] },	// 16
			{ points[6], points[2], points[10] },	// 17
			{ points[8], points[6], points[7] },	// 18
			{ points[9], points[8], points[1] },	// 19
		};

		int normalContributorIndices[][5] =
		{
			{ 0, 1, 2, 3, 4 },
			{ 1, 2, 5, 9, 19 },
			{ 7, 11, 12, 16, 17 },
			{ 10, 11, 12, 13, 14 },
			{ 6, 10, 11, 15, 16 },
			{ 0, 1, 5, 6, 15 },
			{ 8, 12, 13, 17, 18 },
			{ 2, 3, 8, 9, 18 },
			{ 9, 13, 14, 18, 19 },
			{ 5, 10, 14, 15, 19 },
			{ 3, 4, 7, 8, 17 },
			{ 0, 4, 6, 7, 16 },
		};

		std::array<Vector3, 12> normals;

		for (std::size_t i = 0; i < normals.size(); ++i)
		{
			Vector3 normal;

			for (int j = 0; j < 5; ++j)
			{
				normal += faces[normalContributorIndices[i][j]].Normal();
			}

			normal /= 5.0;
			normal.Normalize();

			for (auto && face : faces)
			{
				for (int j = 0; j < 3; ++j)
				{
					if (face.points[j] == points[i])
						face.normals[j] = normal;
				}
			}
		}

		for (uint32_t i = 0; i < subdivision; ++i)
		{
			std::vector<Triangle> newFaces;

			for (auto && face : faces)
			{
				Vector3 mid1pos = face.points[0] + ((face.points[1] - face.points[0]) * 0.5);
				Vector3 mid2pos = face.points[1] + ((face.points[2] - face.points[1]) * 0.5);
				Vector3 mid3pos = face.points[2] + ((face.points[0] - face.points[2]) * 0.5);

				Vector3 mid1norm = (face.normals[0] + face.normals[1]) * 0.5;
				Vector3 mid2norm = (face.normals[1] + face.normals[2]) * 0.5;
				Vector3 mid3norm = (face.normals[2] + face.normals[0]) * 0.5;

				mid1norm.Normalize();
				mid2norm.Normalize();
				mid3norm.Normalize();

				newFaces.emplace_back(face.points[0], mid1pos, mid3pos, face.normals[0], mid1norm, mid3norm);
				newFaces.emplace_back(face.points[1], mid2pos, mid1pos, face.normals[1], mid2norm, mid1norm);
				newFaces.emplace_back(face.points[2], mid3pos, mid2pos, face.normals[2], mid3norm, mid2norm);
				newFaces.emplace_back(mid1pos, mid2pos, mid3pos, mid1norm, mid2norm, mid3norm);
			}

			faces = std::move(newFaces);
		}

		Real half = size / 2.0;

		for (auto && face : faces)
		{
			for (int i = 0; i < 3; ++i)
			{
				Real l = face.points[i].Length();
				face.points[i] = face.points[i] * (half / l);
			}
		}

		m_triangles.insert(m_triangles.begin(), faces.begin(), faces.end());
	}
};

}
