#include <cassert>
#include <memory>
#include "Colour.h"
#include "FrameBuffer.h"
#include "Rasteriser.h"
#include "Types.h"

Rasteriser::Rasteriser(FrameBuffer *pFrame, RenderMode mode)
	: m_pFrame(pFrame)
	, m_mode(mode)
{
}

void Rasteriser::DrawTriangle(const std::array<VertexShaderOutput, 3> & triangle)
{
	if (ShouldCull(triangle))
		return;

	if (m_mode == RenderMode::WireFrame)
	{
		DrawWireFrameTriangle(triangle);
		return;
	}

	struct Point { int x; int y; };

	std::array<Point, 3> points = { {
		{ triangle[0].m_screenX, triangle[0].m_screenY },
		{ triangle[1].m_screenX, triangle[1].m_screenY },
		{ triangle[2].m_screenX, triangle[2].m_screenY }
	} };

	std::sort(std::begin(points), std::end(points),
		[](const Point & p1, const Point & p2) { return p1.y < p2.y; });

	extern std::unique_ptr<ShadyObject> g_fragmentShader;

	FragmentShader shader(g_fragmentShader.get());

	shader.SetTriangleContext(&triangle);
	shader.SetLightPosition(m_lightPosition);

	if (points[0].y == points[1].y)
	{
		DrawFlatTopTriangle(
			shader,
			points[0].x, points[0].y,
			points[1].x, points[1].y,
			points[2].x, points[2].y);
	}
	else if (points[1].y == points[2].y)
	{
		DrawFlatBottomTriangle(
			shader,
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
			shader,
			points[0].x, points[0].y,
			split.x, split.y,
			points[1].x, points[1].y);

		DrawFlatTopTriangle(
			shader,
			split.x, split.y,
			points[1].x, points[1].y,
			points[2].x, points[2].y);
	}
}

void Rasteriser::DrawWireFrameTriangle(const std::array<VertexShaderOutput, 3> & triangle)
{
	DrawLine(
		triangle[0].m_screenX, triangle[0].m_screenY,
		triangle[1].m_screenX, triangle[1].m_screenY,
		Colour::White);

	DrawLine(
		triangle[1].m_screenX, triangle[1].m_screenY,
		triangle[2].m_screenX, triangle[2].m_screenY,
		Colour::White);

	DrawLine(
		triangle[2].m_screenX, triangle[2].m_screenY,
		triangle[0].m_screenX, triangle[0].m_screenY,
		Colour::White);
}

void Rasteriser::DrawFlatTopTriangle(const FragmentShader & fragmentShader,
	int x1, int y1, int x2, int y2, int x3, int y3)
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
			m_pFrame->SetPixel(x, y, fragmentShader.Execute(x, y));

		x_start += dx1;
		x_end += dx2;
	}
}

void Rasteriser::DrawFlatBottomTriangle(const FragmentShader & fragmentShader,
	int x1, int y1, int x2, int y2, int x3, int y3)
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
			m_pFrame->SetPixel(x, y, fragmentShader.Execute(x, y));

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

bool Rasteriser::ShouldCull(const std::array<VertexShaderOutput, 3> & triangle)
{
	const unsigned width = m_pFrame->GetWidth();
	const unsigned height = m_pFrame->GetHeight();

	// TODO: generate new triangle (clipping) when the part of it is in the view space
	for (auto && v : triangle)
	{
		if (v.m_screenX < 0 || v.m_screenX > width)
			return true;

		if (v.m_screenY < 0 || v.m_screenY > height)
			return true;

		if (v.m_projected.z < -1.0 || v.m_projected.z > 1.0)
			return true;
	}

	return false;
}
