#include <cstring>

#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(HWND hWnd)
	: m_hWnd(hWnd)
{
	HDC hdc = GetDC(hWnd);
	m_hDc = CreateCompatibleDC(hdc);

	RECT rect;
	GetClientRect(hWnd, &rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;
	m_hBitmap = CreateCompatibleBitmap(hdc, m_width, m_height);

	SelectObject(m_hDc, m_hBitmap);

	m_bytesPerPixel = GetDeviceCaps(m_hDc, BITSPIXEL) / 8;

	m_pixels = m_width*m_height;
	m_pBytes = new unsigned char [m_pixels * m_bytesPerPixel];
}

FrameBuffer::~FrameBuffer()
{
	DeleteObject(m_hDc);
	delete[] m_pBytes;
}

void FrameBuffer::SetFillColour(const Colour &fill)
{
}

void FrameBuffer::Clear()
{
	std::memset(m_pBytes, 0, m_pixels * m_bytesPerPixel);
}

void FrameBuffer::CopyToWindow()
{
	SetBitmapBits(m_hBitmap, m_pixels * m_bytesPerPixel, m_pBytes);

	HDC hdc = GetDC(m_hWnd);
	BOOL res = BitBlt(hdc, 0, 0, m_width, m_height, m_hDc, 0, 0, SRCCOPY);
}

void FrameBuffer::SetPixel(unsigned x, unsigned y, const Colour &colour)
{
	unsigned char r = (colour.r / 1.0f) * 255;
	unsigned char g = (colour.g / 1.0f) * 255;
	unsigned char b = (colour.b / 1.0f) * 255;
	unsigned start = (x * m_bytesPerPixel)  + (y * m_width * m_bytesPerPixel);

	// TODO/FIXME: assuming 4 bytes per pixel
	m_pBytes[start] = r;
	m_pBytes[start+1] = g;
	m_pBytes[start+2] = b;
	m_pBytes[start+3] = 0xff;
}
