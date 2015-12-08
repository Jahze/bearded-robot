#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <memory>
#include <string>
#include <Windows.h>

#include "Camera.h"
#include "DoubleBuffer.h"
#include "FrameBuffer.h"
#include "Geometry.h"
#include "InputHandler.h"
#include "Matrix.h"
#include "Projection.h"
#include "Rasteriser.h"
#include "ScopedHDC.h"
#include "ShaderCompiler.h"
#include "Vector.h"
#include "VertexShader.h"

DoubleBuffer *pFrames = 0;

namespace
{
	std::unique_ptr<ShadyObject> CreateVertexShader()
	{
		std::ifstream file("vertex.shader");

		std::string source{std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>()};

		std::string error;
		ShaderCompiler compiler;

		std::unique_ptr<ShadyObject> object = compiler.CompileVertexShader(source, error);

		assert(error.empty());

		return object;
	}

	std::unique_ptr<ShadyObject> CreateFragmentShader()
	{
		std::ifstream file("fragment.shader");

		std::string source{std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>()};

		std::string error;
		ShaderCompiler compiler;

		std::unique_ptr<ShadyObject> object = compiler.CompileFragmentShader(source, error);

		assert(error.empty());

		return object;
	}

	Real g_rotation = 0.0;

	std::chrono::steady_clock::time_point g_lastTime = std::chrono::steady_clock::now();

	const Real kRotationPerSec = 50.0;

	Camera g_camera;

	InputHandler g_inputHandler;

	std::unique_ptr<ShadyObject> g_vertexShader;
}

// TODO : pass this through correctly

std::unique_ptr<ShadyObject> g_fragmentShader;

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

	ScopedHDC hdc(hwnd);
	TextOut(hdc, 5, 5, str.c_str(), str.length());
}

int WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RenderMode mode = RenderMode::WireFrame;
	static bool paused = false;
	static bool cull = true;
	static bool drawNormals = false;
	static bool yrot = true;

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
			Projection projection(45.0f, 90.0f, 1.0f, 1000.0f, width, height);

			Matrix4 modelTransform =
				Matrix4::Translation({ 0.0, 0.0, -50.0 });
			
			if (yrot)
				modelTransform = modelTransform * Matrix4::RotationAboutY(Units::Degrees, g_rotation);
			else
				modelTransform = modelTransform * Matrix4::RotationAboutX(Units::Degrees, g_rotation);

			//Vector3 rotationCentre{ 0.0, 0.0, -50.0 };
			//Vector3 direction{ Real(sin(g_rotation * DEG_TO_RAD)), 0.0, Real(cos(g_rotation * DEG_TO_RAD)) };
			//g_camera.SetPosition(rotationCentre + direction * 50.0);

			VertexShader vertexShader(projection, g_vertexShader.get());
			vertexShader.SetModelTransform(modelTransform);
			vertexShader.SetViewTransform(g_camera.GetTransform());

			Vector3 light{ 0.0, 0.0, 0.0 };
			//light = g_camera.GetTransform() * light;
			rasta.SetLightPosition(light);

			geometry::Cube cube(25.0);
			const auto end = cube.triangles.end();

			for (auto iter = cube.triangles.begin(); iter != end; ++iter)
			{
				const Vector3 normal = iter->Normal();

				std::array<VertexShaderOutput,3> vertexShaded;

				for (unsigned i = 0; i < 3; ++i)
					vertexShaded[i] = vertexShader.Execute(iter->points[i], normal);

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
					Vector3 centre = iter->Centre();
					Vector3 normalExtent = centre + (iter->Normal() * 5.0);

					VertexShaderOutput centre_v = vertexShader.Execute(centre);
					VertexShaderOutput normalExtent_v = vertexShader.Execute(normalExtent);

					rasta.DrawLine(
						centre_v.m_screenX, centre_v.m_screenY,
						normalExtent_v.m_screenX, normalExtent_v.m_screenY,
						Colour::Red);
				}
			}

			pFrames->Swap();
			FrameCount(hWnd);

			break;
		}

		case WM_MOUSEMOVE:
			g_inputHandler.DecodeMouseMove(lParam, wParam);
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
			else if (wParam == 'R')
			{
				yrot = !yrot;
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
				g_vertexShader = CreateVertexShader();
				g_fragmentShader = CreateFragmentShader();
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
	g_vertexShader = CreateVertexShader();
	g_fragmentShader = CreateFragmentShader();

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

	RECT rect;
	::GetWindowRect(hwnd, &rect);
	g_inputHandler.SetWindowArea(rect);
	g_inputHandler.AddMouseListener(&g_camera);

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
