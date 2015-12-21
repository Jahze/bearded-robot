#include <array>
#include <ctime>
#include <string>
#include <Windows.h>

#include "Camera.h"
#include "Console.h"
#include "FrameBuffer.h"
#include "InputHandler.h"
#include "Projection.h"
#include "Rasteriser.h"
#include "ScopedHDC.h"
#include "Scene.h"
#include "ShaderCache.h"
#include "VertexShader.h"

namespace
{
	FrameBuffer *g_frame = nullptr;

	Camera g_camera;

	InputHandler g_inputHandler;

	SceneDriver * g_sceneDriver;

	int g_mx, g_my;
}

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

	std::string str = "FPS: " + std::to_string((unsigned long long)lastFps) +
		" x=" + std::to_string(g_mx) + ", y=" + std::to_string(g_my);

	ScopedHDC hdc(hwnd);
	TextOut(hdc, 5, 5, str.c_str(), str.length());
}

void RenderLoop(HWND hWnd, RenderMode mode, bool cull, bool drawNormals, bool paused)
{
	if (g_frame == nullptr)
	{
		g_frame = new FrameBuffer(hWnd);
	}

	FrameBuffer *pFrame = g_frame;
	pFrame->Clear();

	Rasteriser rasta(pFrame, mode, ShaderCache::Get().FragmentShader());

	Vector3 light { 0.0, 0.0, 0.0 };
	rasta.SetLightPosition(light);

	const unsigned width = pFrame->GetWidth();
	const unsigned height = pFrame->GetHeight();

	Projection projection(90.0f, 1.0f, 1000.0f, width, height);

	VertexShader vertexShader(projection, ShaderCache::Get().VertexShader());

	g_sceneDriver->Update(paused);

	vertexShader.SetViewTransform(g_camera.GetTransform());

	ObjectIterator iterator = g_sceneDriver->GetObjects();

	while (iterator.HasMore())
	{
		geometry::Object * object = iterator.Next();

		vertexShader.SetModelTransform(object->GetModelMatrix());

		const auto & triangles = object->GetTriangles();
		const auto end = triangles.end();

		for (auto iter = triangles.begin(); iter != end; ++iter)
		{
			std::array<VertexShaderOutput,3> vertexShaded;

			for (unsigned i = 0; i < 3; ++i)
				vertexShaded[i] = vertexShader.Execute(iter->points[i], iter->normals[i]);

			geometry::Triangle projected = {
				vertexShaded[0].m_projected.XYZ(),
				vertexShaded[1].m_projected.XYZ(),
				vertexShaded[2].m_projected.XYZ(),
			};

			if (cull && projected.IsAntiClockwise())
				continue;

			rasta.DrawTriangle(vertexShaded);

			if (drawNormals)
			{
				for (unsigned i = 0; i < 3; ++i)
				{
					Vector3 start = iter->points[i];
					Vector3 end = start + (iter->normals[i] * 5.0);

					VertexShaderOutput start_v = vertexShader.Execute(start);
					VertexShaderOutput end_v = vertexShader.Execute(end);

					rasta.DrawLine(
						start_v.m_screen.x, start_v.m_screen.y,
						end_v.m_screen.x, end_v.m_screen.y,
						Colour::Red);
				}

			}
		}
	}

	g_frame->CopyToWindow();
	FrameCount(hWnd);
}

int WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RenderMode mode = RenderMode::WireFrame;
	static bool paused = false;
	static bool cull = true;
	static bool drawNormals = false;

	switch (message)
	{
		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			// TODO: put this somewhere else and use the default WM_PAINT handler
			RenderLoop(hWnd, mode, cull, drawNormals, paused);
			break;

		case WM_LBUTTONDOWN:
			g_inputHandler.SetLockCursor(true);
			g_inputHandler.AddMouseListener(&g_camera);
			break;

		case WM_LBUTTONUP:
			g_inputHandler.SetLockCursor(false);
			g_inputHandler.RemoveMouseListener(&g_camera);
			break;

		case WM_MOUSEMOVE:
			g_inputHandler.DecodeMouseMove(lParam, wParam);
			g_mx = LOWORD(lParam);
			g_my = HIWORD(lParam);
			break;

		case WM_KEYDOWN:
			if (wParam == 'A')
			{
				g_camera.Move(-Vector3::UnitX);
			}
			else if (wParam == 'D')
			{
				g_camera.Move(Vector3::UnitX);
			}
			else if (wParam == 'W')
			{
				g_camera.Move(-Vector3::UnitZ);
			}
			else if (wParam == 'S')
			{
				g_camera.Move(Vector3::UnitZ);
			}
			else if (wParam == VK_RETURN)
			{
				Console console;
				console.ProcessCommands();
			}
			break;

		case WM_KEYUP:
			if (wParam == 'M')
			{
				mode = static_cast<RenderMode>(static_cast<std::underlying_type<RenderMode>::type>(mode) + 1);

				if (mode == RenderMode::End)
					mode = RenderMode::First;
			}
			else if (wParam == 'N')
			{
				drawNormals = !drawNormals;
			}
			else if (wParam == 'C')
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
			else if (wParam == 'V')
			{
				g_camera.Reset();
			}
			else if (wParam == 'U')
			{
				std::string error;

				ShaderCache::Get().LoadVertexShader("vertex.shader", error);
				ShaderCache::Get().LoadFragmentShader("fragment.shader", error);
			}
			else if (wParam == 'P')
			{
				g_sceneDriver->Next();
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCommandLine, int nCmdShow)
{
	g_sceneDriver = new SceneDriver();

	std::string error;

	ShaderCache::Get().LoadVertexShader("vertex.shader", error);
	ShaderCache::Get().LoadFragmentShader("fragment.shader", error);

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

	g_inputHandler.SetWindowArea(hwnd);

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

		InvalidateRect(hwnd, NULL, false);
	}
	return 0;
}
