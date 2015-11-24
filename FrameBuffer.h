#pragma once

#include <Windows.h>

#include "Colour.h"

class FrameBuffer
{
public:
	FrameBuffer(HWND hWnd);
	~FrameBuffer();

	void SetFillColour(const Colour &fill);
	void Clear();
	void CopyToWindow();

	void SetPixel(unsigned x, unsigned y, const Colour &colour);

	unsigned GetWidth() const { return m_width; }
	unsigned GetHeight() const { return m_height; }

private:
	unsigned m_width;
	unsigned m_height;
	unsigned m_pixels;
	unsigned m_bytesPerPixel;
	unsigned char *m_pBytes;
	HWND m_hWnd;
	HDC m_hDc;
	HBITMAP m_hBitmap;
	Colour m_fillColour;
};
