#pragma once

#include <array>
#include <Windows.h>
#include "Colour.h"
#include "FragmentShader.h"
#include "Geometry.h"
#include "VertexShader.h"

class FrameBuffer;
class ShadyObject;

enum class RenderMode
{
	First = 0,

	WireFrame = 0,
	Fill,
	Both,

	End,
};

class Rasteriser
{
public:
	Rasteriser(FrameBuffer *pFrame, RenderMode mode, ShadyObject * shader);

	void SetLightPosition(const Vector3 & position)
	{
		m_lightPosition = position;
	}

	void SetShader(ShadyObject * shader)
	{
		m_fragmentShader = shader;
	}

	void DrawTriangle(const std::array<VertexShaderOutput, 3> & triangle);
	void DrawLine(int x1, int y1, int x2, int y2, const Colour & colour);

private:
	void DrawTriangle(const FragmentShader & fragmentShader,
		Real x1, Real y1, Real x2, Real y2, Real x3, Real y3);
	void DrawWireFrameTriangle(const std::array<VertexShaderOutput, 3> & triangle);

	bool ShouldCull(const std::array<VertexShaderOutput, 3> & triangle);

	FrameBuffer *m_pFrame;
	RenderMode m_mode;
	Vector3 m_lightPosition;
	ShadyObject * m_fragmentShader;
};
