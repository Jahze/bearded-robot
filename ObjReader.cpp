#include <fstream>
#include <string>
#include "ObjReader.h"

namespace
{
	std::vector<std::string> Split(const std::string & string, char c)
	{
		std::vector<std::string> out;

		std::string::size_type cursor = 0;
		std::string::size_type pos = string.find(c);

		while (pos != std::string::npos)
		{
			out.push_back(string.substr(cursor, pos - cursor));

			cursor = pos;

			while (string[cursor] == c)
				cursor++;

			pos = string.find(c, cursor);
		}

		if (cursor < string.length())
			out.push_back(string.substr(cursor));

		return out;
	}
}

ObjModel::ObjModel(std::vector<geometry::Triangle> && faces)
{
	m_triangles = std::move(faces);
}

namespace
{
	struct Face
	{
		static const std::size_t None = -1;

		std::size_t point = None;
		std::size_t texture = None;
		std::size_t normal = None;

		Face(const std::string & face)
		{
			std::vector<std::string> parts = Split(face, '/');

			point = std::stoull(parts[0]) - 1;

			if (parts.size() > 1)
			{
				if (! parts[1].empty())
				{
					texture = std::stoull(parts[1]) - 1;
				}

				if (parts.size() > 2)
				{
					normal = std::stoull(parts[2]) - 1;
				}
			}
		}
	};
}

bool ObjReader::Read(const std::string & filename)
{
	std::ifstream file(filename);

	if (! file.good())
		return false;

	std::vector<Vector3> points;
	std::vector<Vector3> normals;
	std::vector<std::tuple<Face, Face, Face>> faces;

	while (file)
	{
		std::string line;
		std::getline(file, line);

		if (line[0] == '#')
			continue;

		if (line.empty())
			continue;

		std::vector<std::string> parts = Split(line, ' ');

		if (parts[0] == "v")
		{
			if (parts.size() != 4)
				return false;

			points.emplace_back(std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3]));
		}
		else if (parts[0] == "vn")
		{
			if (parts.size() != 4)
				return false;

			Vector3 normal(std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3]));
			normal.Normalize();

			normals.emplace_back(normal);
		}
		else if (parts[0] == "f")
		{
			if (parts.size() != 4)
				return false;

			faces.emplace_back(parts[1], parts[2], parts[3]);
		}
		// Ignore
		else if (parts[0] == "mtllib" || parts[0] == "usemtl" || parts[0] == "vt" ||
			parts[0] == "g" || parts[0] == "s" || parts[0] == "o")
		{
			continue;
		}
		else
		{
			return false;
		}
	}

	if (normals.empty())
	{
		const std::size_t size = points.size();

		for (std::size_t i = 0; i < size; ++i)
		{
			Vector3 normal;
			std::size_t number = 0;

			const std::size_t index = normals.size();

			for (auto && face : faces)
			{
				Face & f1 = std::get<0>(face);
				Face & f2 = std::get<1>(face);
				Face & f3 = std::get<2>(face);

				if (f1.point == i || f2.point == i ||f3.point == i)
				{
					geometry::Triangle triangle(points[f1.point], points[f2.point], points[f3.point]);
					normal += triangle.Normal();
					++number;

					if (f1.point == i)
						f1.normal = index;

					if (f2.point == i)
						f2.normal = index;

					if (f3.point == i)
						f3.normal = index;
				}
			}

			normal /= number;
			normals.push_back(normal);
		}
	}

	std::vector<geometry::Triangle> triangles;

	for (auto && face : faces)
	{
		Face f1 = std::get<0>(face);
		Face f2 = std::get<1>(face);
		Face f3 = std::get<2>(face);

		assert(f1.normal != Face::None);
		assert(f2.normal != Face::None);
		assert(f3.normal != Face::None);

		triangles.emplace_back(points[f1.point], points[f2.point], points[f3.point],
			normals[f1.normal], normals[f2.normal], normals[f3.normal]);
	}

	m_model.reset(new ObjModel(std::move(triangles)));

	return true;
}

std::unique_ptr<ObjModel> ObjReader::GetModel()
{
	return std::move(m_model);
}
