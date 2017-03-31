#pragma once

#include "DxBase.h"
class DxBase;

class Win32App
{
public:
	static int Run(DxBase* base, HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	static HWND m_hwnd;
};

