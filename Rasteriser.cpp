#include <cassert>
#include "Colour.h"
#include "FrameBuffer.h"
#include "Rasteriser.h"
#include "Types.h"

namespace
{
	// https://en.wikibooks.org/wiki/GLSL_Programming/Rasterization
	// explains how to interpolate values
	// e.g. normals

	// i guess you do position like this too and the areas are based on the projected triangle?
	// yes -> vertex shaders to projection and it mentions V1,V2,V3 and output of vertex shader
	Colour CalculateColour(const Colour & ambient, const Colour & diffuse,
		const Vector3 & normal, const Vector3 & toLight)
	{
		assert(normal.Length() == 1.0);

		Vector3 directionToLight = toLight.NormalizedCopy();

		Real dot =
			normal.x * directionToLight.x +
			normal.y * directionToLight.y +
			normal.z * directionToLight.z;

		dot = std::max<Real>(0.0, dot);

		Real attenuation = 1.0 / toLight.Length();
		Vector3 colour = { diffuse.r, diffuse.g, diffuse.b };
		colour = colour * dot * attenuation;

		colour.x += ambient.r;
		colour.y += ambient.g;
		colour.z += ambient.b;

		// clamp colour

		return { colour.x, colour.y, colour.z };
	}
}

Rasteriser::Rasteriser(FrameBuffer *pFrame, RenderMode mode)
	: m_pFrame(pFrame)
	, m_mode(mode)
{
}

void Rasteriser::DrawTriangle(const geometry::Triangle & triangle, const geometry::Triangle & projected)
{
	if (m_mode == RenderMode::WireFrame)
	{
		DrawWireFrameTriangle(projected);
		return;
	}

	struct Point { int x; int y; };

	std::array<Point, 3> points = { {
		{ projected.points[0].x, projected.points[0].y },
		{ projected.points[1].x, projected.points[1].y },
		{ projected.points[2].x, projected.points[2].y }
	} };

	std::sort(std::begin(points), std::end(points),
		[](const Point & p1, const Point & p2) { return p1.y < p2.y; });

	if (points[0].y == points[1].y)
	{
		DrawFlatTopTriangle(
			points[0].x, points[0].y,
			points[1].x, points[1].y,
			points[2].x, points[2].y);
	}
	else if (points[1].y == points[2].y)
	{
		DrawFlatBottomTriangle(
			points[0].x, points[0].y,
			points[1].x, points[1].y,
			points[2].x, points[2].y);
	}
	else
	{
		int x_distance = points[2].x - points[0].x;
		int y_distance = points[2].y - points[0].y;
		Real x_delta = static_cast<Real>(x_distance) / static_cast<Real>(y_distance);
		int split_distance = points[1].y - points[0].y;

		Point split =
		{
			points[0].x + (x_delta * split_distance),
			points[1].y
		};

		DrawFlatBottomTriangle(
			points[0].x, points[0].y,
			split.x, split.y,
			points[1].x, points[1].y);

		DrawFlatTopTriangle(
			split.x, split.y,
			points[1].x, points[1].y,
			points[2].x, points[2].y);
	}
}

void Rasteriser::DrawWireFrameTriangle(const geometry::Triangle & triangle)
{
	DrawLine(
		triangle.points[0].x, triangle.points[0].y,
		triangle.points[1].x, triangle.points[1].y,
		Colour::White);

	DrawLine(
		triangle.points[1].x, triangle.points[1].y,
		triangle.points[2].x, triangle.points[2].y,
		Colour::White);

	DrawLine(
		triangle.points[2].x, triangle.points[2].y,
		triangle.points[0].x, triangle.points[0].y,
		Colour::White);
}

void Rasteriser::DrawFlatTopTriangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
	assert(y1 == y2);

	if (m_mode == RenderMode::ShowRasterTriSplits)
	{
		DrawLine(x1, y1, x2, y2, Colour::White);
		DrawLine(x2, y2, x3, y3, Colour::White);
		DrawLine(x3, y3, x1, y1, Colour::White);
		return;
	}

	if (x2 < x1)
		std::swap(x1, x2);

	int y_distance = y3 - y1;
	Real dx1 = static_cast<Real>(x3 - x1) / static_cast<Real>(y_distance);
	Real dx2 = static_cast<Real>(x3 - x2) / static_cast<Real>(y_distance);

	Real x_start = x1;
	Real x_end = x2;

	for (int y = y1; y <= y3; ++y)
	{
		const int end = round(x_end);

		for (int x = round(x_start); x <= end; ++x)
			m_pFrame->SetPixel(x, y, Colour::White);

		x_start += dx1;
		x_end += dx2;
	}
}

void Rasteriser::DrawFlatBottomTriangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
	assert(y2 == y3);

	if (m_mode == RenderMode::ShowRasterTriSplits)
	{
		DrawLine(x1, y1, x2, y2, Colour::White);
		DrawLine(x2, y2, x3, y3, Colour::White);
		DrawLine(x3, y3, x1, y1, Colour::White);
		return;
	}

	if (x3 < x2)
		std::swap(x2, x3);

	int y_distance = y2 - y1;
	Real dx1 = static_cast<Real>(x1 - x2) / static_cast<Real>(y_distance);
	Real dx2 = static_cast<Real>(x1 - x3) / static_cast<Real>(y_distance);

	Real x_start = x2;
	Real x_end = x3;

	for (int y = y2; y >= y1; --y)
	{
		const int end = round(x_end);

		for (int x = round(x_start); x <= end; ++x)
			m_pFrame->SetPixel(x, y, Colour::White);

		x_start += dx1;
		x_end += dx2;
	}
}

void Rasteriser::DrawLine(int x1, int y1, int x2, int y2, const Colour & colour)
{
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;

	int e = dx - dy;

	int x = x1;
	int y = y1;

	while (true)
	{
		// TODO this dies if it's x<0||x>width etc
		m_pFrame->SetPixel(x, y, colour);

		if (x == x2 && y == y2)
			break;

		int e2 = 2 * e;

		if (e2 > -dy)
		{
			e -= dy;
			x += sx;
		}

		if (e2 < dx)
		{
			e += dx;
			y += sy;
		}
	}
}
