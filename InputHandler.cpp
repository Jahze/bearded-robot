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

	ResetCursor();
}
void InputHandler::SetWindowArea(const RECT & rect)
{
	unsigned width = rect.right - rect.left;
	unsigned height = rect.bottom - rect.top;

	m_halfScreenX = rect.left + width / 2;
	m_halfScreenY = rect.top + height / 2;

	::ShowCursor(FALSE);
	ResetCursor();
}

void InputHandler::ResetCursor()
{
	m_nextMouseMoveIsThis = true;
	::SetCursorPos(m_halfScreenX, m_halfScreenY);
}
