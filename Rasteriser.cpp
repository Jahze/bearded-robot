#include <cassert>
#include <memory>
#include "ClipPlane.h"
#include "Colour.h"
#include "FrameBuffer.h"
#include "Rasteriser.h"
#include "Types.h"

Rasteriser::Rasteriser(FrameBuffer *pFrame, RenderMode mode, ShadyObject * shader)
	: m_pFrame(pFrame)
	, m_mode(mode)
	, m_fragmentShader(shader)
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
		{ triangle[0].m_screen.x, triangle[0].m_screen.y },
		{ triangle[1].m_screen.x, triangle[1].m_screen.y },
		{ triangle[2].m_screen.x, triangle[2].m_screen.y }
	} };

	std::sort(std::begin(points), std::end(points),
		[](const Point & p1, const Point & p2) { return p1.y < p2.y; });

	FragmentShader shader(m_fragmentShader);

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
		triangle[0].m_screen.x, triangle[0].m_screen.y,
		triangle[1].m_screen.x, triangle[1].m_screen.y,
		Colour::White);

	DrawLine(
		triangle[1].m_screen.x, triangle[1].m_screen.y,
		triangle[2].m_screen.x, triangle[2].m_screen.y,
		Colour::White);

	DrawLine(
		triangle[2].m_screen.x, triangle[2].m_screen.y,
		triangle[0].m_screen.x, triangle[0].m_screen.y,
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
	const Real width = m_pFrame->GetWidth();
	const Real height = m_pFrame->GetHeight();

	bool shouldClip = false;

	for (auto && v : triangle)
	{
		if (v.m_screen.x < 0 || v.m_screen.x > width)
			shouldClip = true;

		if (v.m_screen.y < 0 || v.m_screen.y > height)
			shouldClip = true;

		if (v.m_projected.z < -1.0 || v.m_projected.z > 1.0)
			return true;
	}

	if (! shouldClip)
		return false;

	std::vector<VertexShaderOutput> points(triangle.begin(), triangle.end());

	ClipPlane top({ 0.0, 0.0 }, { 1.0, 0.0 });
	ClipPlane bottom({ 0.0, height-0.1f }, { -1.0, 0.0 });
	ClipPlane left({ 0.0, 0.0 }, { 0.0, -1.0 });
	ClipPlane right({ width-0.1f, 0.0 }, { 0.0, 1.0 });

	points = top.Clip(points);
	points = bottom.Clip(points);
	points = left.Clip(points);
	points = right.Clip(points);

	if (points.size() < 3)
	{
		assert(points.empty());
		return true;
	}

	const std::size_t limit = points.size();

	for (std::size_t i = 2; i < limit; ++i)
	{
		std::array<VertexShaderOutput, 3> clipped;

		clipped[0] = points[0];
		clipped[1] = points[i-1];
		clipped[2] = points[i];

		// TODO: how can we avoid this?
		// they go to tiny non-zero values sometimes
		for (auto && v : clipped)
		{
			if (v.m_screen.x < 0)
				v.m_screen.x = 0;

			if (v.m_screen.y < 0)
				v.m_screen.y = 0;
		}

		DrawTriangle(clipped);
	}

	return true;
}
