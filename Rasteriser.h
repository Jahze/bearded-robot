#pragma once

#include <array>
#include <Windows.h>
#include "Colour.h"
#include "FragmentShader.h"
#include "Geometry.h"
#include "VertexShader.h"

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

	void SetLightPosition(const Vector3 & position)
	{
		m_lightPosition = position;
	}

	void DrawTriangle(const std::array<VertexShaderOutput, 3> & triangle);
	void DrawLine(int x1, int y1, int x2, int y2, const Colour & colour);

private:
	void DrawWireFrameTriangle(const std::array<VertexShaderOutput, 3> & triangle);
	void DrawFlatTopTriangle(const FragmentShader & fragmentShader,
		int x1, int y1, int x2, int y2, int x3, int y3);
	void DrawFlatBottomTriangle(const FragmentShader & fragmentShader,
		int x1, int y1, int x2, int y2, int x3, int y3);

	FrameBuffer *m_pFrame;
	RenderMode m_mode;
	Vector3 m_lightPosition;
};
