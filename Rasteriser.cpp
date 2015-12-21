#include <cassert>
#include <memory>
#include "ClipPlane.h"
#include "Colour.h"
#include "FrameBuffer.h"
#include "Point.h"
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

	DrawTriangle(shader, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
}

void Rasteriser::DrawTriangle(const FragmentShader & fragmentShader,
	Real x1, Real y1, Real x2, Real y2, Real x3, Real y3)
{
	Real height = y3 - y1;

	if (height == 0.0)
		return;

	Real y1_floor = std::floor(y1);
	int y_start = (y1 - y1_floor <= 0.5) ? y1_floor : y1_floor + 1.0;

	Real y2_floor = std::floor(y2);
	int y_mid = (y2 - y2_floor <= 0.5) ? y2_floor - 1.0 : y2_floor;

	Real y3_floor = std::floor(y3);
	int y_end = (y3 - y3_floor <= 0.5) ? y3_floor - 1.0 : y3_floor;

	Real delta1 = (x3 - x1) / height;

	for (int y = y_start; y <= y_end; ++y)
	{
		Real ry = y;
		ry += 0.5;

		Real px1 = x1 + delta1 * (ry - y1);
		Real px2;

		if (y <= y_mid)
		{
			Real delta2 = (x2 - x1) / (y2 - y1);
			px2 = x1 + delta2 * (ry - y1);
		}
		else
		{
			Real delta2 = (x3 - x2) / (y3 - y2);
			px2 = x2 + delta2 * (ry - y2);
		}

		if (px1 > px2)
			std::swap(px1, px2);

		Real px1_floor = std::floor(px1);
		int px_start = (px1 - px1_floor <= 0.5) ? px1_floor : px1_floor + 1.0;

		Real px2_floor = std::floor(px2);
		int px_end = (px2 - px2_floor <= 0.5) ? px2_floor - 1.0 : px2_floor;

		for (int x = px_start; x <= px_end; ++x)
		{
			Colour colour;

			if (fragmentShader.Execute(x, y, m_pFrame, colour))
				m_pFrame->SetPixel(x, y, colour);
		}
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
		if (x >= 0 && y >= 0 && x < m_pFrame->GetWidth() && y < m_pFrame->GetHeight())
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
