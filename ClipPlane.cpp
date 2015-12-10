#include "ClipPlane.h"
#include "VertexShader.h"

namespace
{
	Vector3 LinearInterpolate(Vector3 start, Vector3 end, Real distance)
	{
		return {
			start.x + (end.x - start.x) * distance,
			start.y + (end.y - start.y) * distance,
			start.z + (end.z - start.z) * distance,
		};
	}

	Vector4 LinearInterpolate(Vector4 start, Vector4 end, Real distance)
	{
		return{
			start.x + (end.x - start.x) * distance,
			start.y + (end.y - start.y) * distance,
			start.z + (end.z - start.z) * distance,
			start.w + (end.w - start.w) * distance,
		};
	}
}

std::vector<VertexShaderOutput> ClipPlane::Clip(const std::vector<VertexShaderOutput> & points) const
{
	std::vector<VertexShaderOutput> output;

	const std::size_t size = points.size();

	if (size == 0)
		return output;

	const std::size_t limit = size - 1;

	for (std::size_t i = 0; i < limit; ++i)
		ProcessEdge(output, points[i], points[i+1]);

	ProcessEdge(output, points[limit], points[0]);

	return output;
}

void ClipPlane::ProcessEdge(std::vector<VertexShaderOutput> & output, const VertexShaderOutput & p1,
	const VertexShaderOutput & p2) const
{
	if (Inside(p1.m_screen))
	{
		// both inside
		if (Inside(p2.m_screen))
		{
			output.push_back(p2);
		}
		// inside -> outside
		else
		{
			output.push_back(Intersect(p1, p2));
		}
	}
	else
	{
		// outside -> inside
		if (Inside(p2.m_screen))
		{
			output.push_back(Intersect(p1, p2));
			output.push_back(p2);
		}
		// both outside
		else
		{
			// do nothing
		}
	}
}

VertexShaderOutput ClipPlane::Intersect(const VertexShaderOutput & p1, const VertexShaderOutput & p2) const
{
	Point direction = p2.m_screen - p1.m_screen;
	Point w = m_point - p1.m_screen;

	Real distance =
		(m_direction.x * w.y - m_direction.y * w.x) / (m_direction.x * direction.y - m_direction.y * direction.x);

	assert(distance >= 0.0 && distance <= 1.0);

	VertexShaderOutput out;
	out.m_normal = LinearInterpolate(p1.m_normal, p2.m_normal, distance);
	out.m_position = LinearInterpolate(p1.m_position, p2.m_position, distance);
	out.m_projected = LinearInterpolate(p1.m_projected, p2.m_projected, distance);

	out.m_screen = p1.m_screen + direction * distance;

	return out;
}

bool ClipPlane::Inside(Point p) const
{
	Point other = m_point + m_direction;

	return (p.x - m_point.x) * (other.y - m_point.y) - (p.y - m_point.y) * (other.x - m_point.x) < 0;
}
