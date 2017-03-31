#pragma once
#include <Windows.h>

class Input
{
public:
	~Input();

public:
	void Init();
	void Listen(UINT msg, float x, float y);	//	Listen to the user input
	static Input* GetInstance()
	{
		static Input instance;

		return &instance;
	}

public:
	float GetMouseX() const;
	float GetMouseY() const;
	float GetLastMouseX() const;
	float GetLastMouseY() const;
	bool IsLMouseDown() const;
	bool IsRMouseDown() const;
	bool IsMouseUp() const;
	bool IsMouseMove() const;
	int IsKeyDown(int key) const;

private:
	POINT m_lastMousePos;
	POINT m_mousePos;
	bool m_lMouseDown;
	bool m_rMouseDown;
	bool m_mouseUp;
	bool m_mouseMove;

private:
	Input();
	Input(const Input&);
	void operator=(const Input&);
};
