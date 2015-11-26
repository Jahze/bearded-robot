#pragma once

#include <vector>
#include <Windows.h>

class IMouseListener
{
public:
	virtual void MouseMoved(int xdelta, int ydelta) = 0;
};

class InputHandler
{
public:
	void DecodeMouseMove(LPARAM lParam, WPARAM wParam);

	void SetWindowArea(const RECT & rect);

	void AddMouseListener(IMouseListener *pListener)
	{
		m_mouseListeners.push_back(pListener);
	}

private:
	void ResetCursor();

private:
	unsigned m_halfScreenX = 0;
	unsigned m_halfScreenY = 0;
	int m_lastX = 0;
	int m_lastY = 0;
	bool m_nextMouseMoveIsThis = false;

	std::vector<IMouseListener*> m_mouseListeners;
};
