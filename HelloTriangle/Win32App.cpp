#include "Win32App.h"
#include "stdafx.h"
#include "HelloTriangle.h"

//winmain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	HelloTriangle demo(1280, 720, L"HelloTriangle");
	return Win32App::Run(&demo, hInstance, nCmdShow);
}

HWND Win32App::m_hwnd = nullptr;

int Win32App::Run(DxBase* base, HINSTANCE hInstance, int nCmdShow)
{
	//初始化window
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hIcon = LoadCursor(nullptr, IDI_WINLOGO);
	wcex.hInstance = hInstance;
	wcex.lpszClassName = L"Dx12Demo";
	RegisterClassEx(&wcex);

	RECT rect = { 0,0,static_cast<LONG>(base->GetWidth()),static_cast<LONG>(base->GetHeight()) };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd = CreateWindow(
		wcex.lpszClassName,
		base->GetTitle(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		hInstance,
		base
	);

	if (!m_hwnd)
	{
		MessageBox(nullptr, L"create window failed!", MB_OK, 0);
		return 0;
	}

	//基类初始化
	base->Init();

	ShowWindow(m_hwnd, nCmdShow);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			base->Update();
			base->Render();
		}
	}

	base->Destroy();

	return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Win32App::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

