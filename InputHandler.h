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

	void SetWindowArea(HWND hwnd);

	void AddMouseListener(IMouseListener * listener)
	{
		m_mouseListeners.push_back(listener);
	}

	void RemoveMouseListener(IMouseListener * listener)
	{
		auto iter = std::find(m_mouseListeners.begin(), m_mouseListeners.end(), listener);

		if (iter != m_mouseListeners.end())
			m_mouseListeners.erase(iter);
	}

	void SetLockCursor(bool lock);

private:
	void ResetCursor();

private:
	HWND m_hwnd;
	unsigned m_halfScreenX = 0;
	unsigned m_halfScreenY = 0;
	int m_lastX = 0;
	int m_lastY = 0;
	bool m_nextMouseMoveIsThis = false;
	bool m_lockCursor = false;

	std::vector<IMouseListener*> m_mouseListeners;
};
