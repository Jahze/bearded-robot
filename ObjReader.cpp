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

bool ObjReader::Read(const std::string & filename)
{
	std::ifstream file(filename);

	if (! file.good())
		return false;

	std::vector<Vector3> points;
	std::vector<Vector3> normals;
	std::vector<std::tuple<std::size_t, std::size_t, std::size_t>> faces;

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

			//normals.emplace_back(std::stof(parts[1]), std::stof(parts[2]), std::stof(parts[3]));
		}
		else if (parts[0] == "f")
		{
			if (parts.size() != 4)
				return false;

			// 1-based -> 0-based
			std::size_t indices[3] = { std::stoull(parts[1]) - 1, std::stoull(parts[2]) - 1, std::stoull(parts[3]) - 1 };

			const std::size_t size = points.size();

			if (indices[0] >= size || indices[1] >= size || indices[2] >= size)
				return false;

			faces.emplace_back(indices[0], indices[1], indices[2]);
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

			for (auto && face : faces)
			{
				if (std::get<0>(face) == i || std::get<1>(face) == i || std::get<2>(face) == i)
				{
					geometry::Triangle triangle(points[std::get<0>(face)], points[std::get<1>(face)], points[std::get<2>(face)]);
					normal += triangle.Normal();
					++number;
				}
			}

			normal /= number;
			normals.push_back(normal);
		}
	}

	if (normals.size() != points.size())
		return false;

	std::vector<geometry::Triangle> triangles;

	for (auto && face : faces)
	{
		std::size_t v1 = std::get<0>(face);
		std::size_t v2 = std::get<1>(face);
		std::size_t v3 = std::get<2>(face);

		triangles.emplace_back(points[v1], points[v2], points[v3], normals[v1], normals[v2], normals[v3]);
	}

	m_model.reset(new ObjModel(std::move(triangles)));

	return true;
}

std::unique_ptr<ObjModel> ObjReader::GetModel()
{
	return std::move(m_model);
}
