#pragma once

#include <Windows.h>

class ScopedHDC
{
public:
	ScopedHDC(HWND hwnd)
		: m_hwnd(hwnd)
		, m_hdc(::GetDC(hwnd))
	{ }

	~ScopedHDC()
	{
		::ReleaseDC(m_hwnd, m_hdc);
	}

	operator HDC() const
	{
		return m_hdc;
	}

private:
	HWND m_hwnd;
	HDC m_hdc;
};
