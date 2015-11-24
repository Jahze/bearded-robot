#pragma once

#include <array>
#include "Colour.h"
#include "Geometry.h"
#include <Windows.h>

class FrameBuffer;

enum class RenderMode
{
	First = 0,

	WireFrame = 0,
	Fill,
	ShowRasterTriSplits,

	End,
};

class Rasteriser
{
public:
	Rasteriser(FrameBuffer *pFrame, RenderMode mode);
	void DrawTriangle(const geometry::Triangle & triangle, const geometry::Triangle & projected);
	void DrawLine(int x1, int y1, int x2, int y2, const Colour & colour);

private:
	void DrawWireFrameTriangle(const geometry::Triangle & triangle);
	void DrawFlatTopTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
	void DrawFlatBottomTriangle(int x1, int y1, int x2, int y2, int x3, int y3);

	FrameBuffer *m_pFrame;
	RenderMode m_mode;
};
