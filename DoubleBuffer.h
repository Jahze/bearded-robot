#pragma once

#include <Windows.h>

class FrameBuffer;

class DoubleBuffer
{
public:
	DoubleBuffer(HWND hwnd);

	void Swap();
	FrameBuffer *GetFrame();

private:
	unsigned m_idx;
	FrameBuffer *m_frames[2];
};
