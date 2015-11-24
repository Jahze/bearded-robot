#include "DoubleBuffer.h"
#include "FrameBuffer.h"

DoubleBuffer::DoubleBuffer(HWND hwnd)
	: m_idx(0)
{
	m_frames[0] = new FrameBuffer(hwnd);
	m_frames[1] = new FrameBuffer(hwnd);
}

void DoubleBuffer::Swap()
{
	m_frames[m_idx]->CopyToWindow();
	m_idx = !m_idx;
}

FrameBuffer* DoubleBuffer::GetFrame()
{
	return m_frames[m_idx];
}
