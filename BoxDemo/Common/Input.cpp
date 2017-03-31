#include "Input.h"

//******************
//½ûÖ¹¹¹Ôì¸´ÖÆ
//******************
Input::Input() {}
Input::Input(const Input&) {}
Input::~Input() {}
void Input::operator=(const Input&) {}

void Input::Init()
{
	m_lastMousePos = { 0, 0 };
	m_mousePos = { 0, 0 };
	m_lMouseDown = false;
	m_rMouseDown = false;
	m_mouseUp = false;
	m_mouseMove = false;
}

void Input::Listen(UINT msg, float x, float y)
{
	m_lastMousePos.x = m_mousePos.x;
	m_lastMousePos.y = m_mousePos.y;

	m_mousePos.x = x;
	m_mousePos.y = y;

	switch (msg)
	{
	case WM_LBUTTONDOWN:
		m_lMouseDown = true;
		break;
	case WM_RBUTTONDOWN:
		m_rMouseDown = true;
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		m_mouseUp = true;
		m_lMouseDown = false;
		m_rMouseDown = false;
		break;
	case WM_MOUSEMOVE:
		m_mouseMove = true;
		break;
	default:
		m_mouseMove = false;
		break;
	}
}

float  Input::GetMouseX() const
{
	return m_mousePos.x;
}

float  Input::GetMouseY() const
{
	return m_mousePos.y;
}

float Input::GetLastMouseX() const
{
	return m_lastMousePos.x;
}

float  Input::GetLastMouseY() const
{
	return m_lastMousePos.y;
}

bool Input::IsLMouseDown() const
{
	return m_lMouseDown;
}

bool Input::IsRMouseDown() const
{
	return m_rMouseDown;
}

bool Input::IsMouseUp() const
{
	return m_mouseUp;
}

bool Input::IsMouseMove() const
{
	return m_mouseMove;
}

int Input::IsKeyDown(int vKey) const
{
	return GetAsyncKeyState(vKey) & 0x8000;
}
