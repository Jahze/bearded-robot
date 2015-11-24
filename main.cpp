#include <array>
#include <chrono>
#include <ctime>
#include <string>
#include <Windows.h>

#include "DoubleBuffer.h"
#include "FrameBuffer.h"
#include "Geometry.h"
#include "Matrix4.h"
#include "Projection.h"
#include "Rasteriser.h"
#include "Vector.h"

DoubleBuffer *pFrames = 0;

static Real g_rotation = 0.0;
static std::chrono::steady_clock::time_point g_lastTime = std::chrono::steady_clock::now();
const static Real kRotationPerSec = 50.0;

void FrameCount(HWND hwnd)
{
	static time_t t = time(NULL);
	static unsigned count = 0;
	static unsigned lastFps = 0;

	time_t newTime = time(NULL);
	if (t != newTime)
	{
		lastFps = count;
		count = 0;
		t = newTime;
	}
	else
	{
		++count;
	}

	std::string str = "FPS: " + std::to_string((unsigned long long)lastFps)
		+ " Rotation: " + std::to_string((int)std::round(g_rotation));

	TextOut(GetDC(hwnd), 5, 5, str.c_str(), str.length());
}

int WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RenderMode mode = RenderMode::WireFrame;
	static bool paused = false;
	static bool cull = false;
	static bool drawNormals = false;

	switch (message)
	{
		case WM_PAINT:
		{
			if (pFrames == 0)
			{
				pFrames = new DoubleBuffer(hWnd);
			}

			FrameBuffer *pFrame = pFrames->GetFrame();
			pFrame->Clear();

			std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
			long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - g_lastTime).count();
			Real toRotate = (kRotationPerSec / 1000.0)* ms;

			if (!paused)
				g_rotation += toRotate;

			if (g_rotation > 360.0)
				g_rotation = 0.0;

			g_lastTime = time;
			Rasteriser rasta(pFrame, mode);

			const unsigned width = pFrame->GetWidth();
			const unsigned height = pFrame->GetHeight();

			// cube corners look a bit strange when centred
			Projection projection(45.0f, 90.0f, 10.0f, 1000.0f);

			Matrix4 modelTransform =
				Matrix4::Translation({ 0.0, 0.0, -50.0 }) *
				Matrix4::RotationAboutY(Units::Degrees, g_rotation);

			Matrix4 projectionMatrix = projection.GetProjectionMatrix();

			geometry::Cube cube(25.0);
			const auto end = cube.triangles.end();

			for (auto iter = cube.triangles.begin(); iter != end; ++iter)
			{
				// TODO: culling isn't correct i guess
				// would like to test it after projection

				geometry::Triangle justModel = *iter;

				for (unsigned i = 0; i < 3; ++i)
				{
					justModel.points[i] = modelTransform * iter->points[i];
				}

				if (cull && !justModel.IsAntiClockwise())
					continue;

				geometry::Triangle projected = *iter;

				for (unsigned i = 0; i < 3; ++i)
				{
					Vector4 p(projected.points[i].x, projected.points[i].y, projected.points[i].z, 1.0);
					p = projectionMatrix * modelTransform * p;

					projected.points[i].x = p.x / p.w;
					projected.points[i].y = p.y / p.w;
					projected.points[i].z = p.z / p.w;

					projection.ToScreen(width, height, projected.points[i]);
				}

				rasta.DrawTriangle(*iter, projected);

				if (drawNormals)
				{
					Vector3 centre = iter->Centre();
					Vector3 normalExtent = centre + (iter->Normal() * 5.0);

					centre = projectionMatrix * modelTransform * centre;
					normalExtent = projectionMatrix * modelTransform * normalExtent;

					projection.ToScreen(width, height, centre);
					projection.ToScreen(width, height, normalExtent);

					rasta.DrawLine(centre.x, centre.y, normalExtent.x, normalExtent.y, Colour::Red);
				}
			}

			pFrames->Swap();
			FrameCount(hWnd);

			break;
		}

		case WM_KEYUP:
			if (wParam == 0x4D) // m
			{
				mode = static_cast<RenderMode>(static_cast<std::underlying_type<RenderMode>::type>(mode) + 1);

				if (mode == RenderMode::End)
					mode = RenderMode::First;
			}
			else if (wParam == 0x4e) // n
			{
				drawNormals = !drawNormals;
			}
			else if (wParam == 0x43) // c
			{
				cull = !cull;
			}
			else if (wParam == VK_ESCAPE)
			{
				exit(0);
			}
			else if (wParam == VK_SPACE)
			{
				paused = !paused;
			}
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCommandLine, int nCmdShow)
{
	HBRUSH hPen = (HBRUSH)CreatePen(PS_INSIDEFRAME, 0, RGB(0,0,0));

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.hbrBackground = hPen; // OPT/FIXME/TODO: setting this to NULL will produce a WM_BCKGRND msg instead maybe faster?
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "BRWIN";

	RegisterClass(&wc);

	HWND hwnd = CreateWindow("BRWIN", "Bearded Robot", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, NULL, NULL, hInstance, NULL);
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	BOOL bRet;
	MSG msg;

	while( (bRet = GetMessage( &msg, hwnd, 0, 0 )) != 0)
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// HMM: Setting to true makes it flicker
		InvalidateRect(hwnd, NULL, false);
	}
	return 0;
}
