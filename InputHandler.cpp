#include "InputHandler.h"

void InputHandler::DecodeMouseMove(LPARAM lParam, WPARAM wParam)
{
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);

	if (m_nextMouseMoveIsThis)
	{
		m_lastX = x;
		m_lastY = y;
		m_nextMouseMoveIsThis = false;
		return;
	}

	for (auto && listener : m_mouseListeners)
		listener->MouseMoved(x - m_lastX, y - m_lastY);

	m_lastX = x;
	m_lastY = y;

	if (m_lockCursor)
	{
		ResetCursor();
	}

}
void InputHandler::SetWindowArea(HWND hwnd)
{
	m_hwnd = hwnd;

	RECT rect;
	::GetWindowRect(hwnd, &rect);

	unsigned width = rect.right - rect.left;
	unsigned height = rect.bottom - rect.top;

	m_halfScreenX = rect.left + width / 2;
	m_halfScreenY = rect.top + height / 2;

	POINT pos;
	::GetCursorPos(&pos);
	::ScreenToClient(m_hwnd, &pos);

	m_lastX = pos.x;
	m_lastY = pos.y;

	if (m_lockCursor)
		ResetCursor();
}

void InputHandler::SetLockCursor(bool lock)
{
	m_lockCursor = lock;
	::ShowCursor(!lock);
}

void InputHandler::ResetCursor()
{
	m_nextMouseMoveIsThis = true;
	::SetCursorPos(m_halfScreenX, m_halfScreenY);
}
