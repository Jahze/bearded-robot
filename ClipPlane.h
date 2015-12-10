#pragma once

#include <cassert>
#include <vector>

#include "Point.h"

class VertexShaderOutput;

class ClipPlane
{
public:
	ClipPlane(Point point, Point direction)
		: m_point(point)
		, m_direction(direction)
	{ }

	std::vector<VertexShaderOutput> Clip(const std::vector<VertexShaderOutput> & points) const;

private:
	void ProcessEdge(
		std::vector<VertexShaderOutput> & output,
		const VertexShaderOutput & p1,
		const VertexShaderOutput & p2) const;

	VertexShaderOutput Intersect(const VertexShaderOutput & p1, const VertexShaderOutput & p2) const;

	bool Inside(Point p) const;

private:
	Point m_point;
	Point m_direction;
};
