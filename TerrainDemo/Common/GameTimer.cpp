#include "GameTimer.h"
#include <windows.h>

GameTimer::GameTimer() : m_secondsPerCount(0.0), m_deltaTime(-1.0), m_baseTime(0), m_pausedTime(0),
m_prevTime(0), m_currTime(0), m_isStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_secondsPerCount = 1.0 / countsPerSec;
}


float GameTimer::TotalTime() const
{
	if (m_isStopped)
	{
		return static_cast<float>((m_stopTime - m_baseTime - m_pausedTime)*m_secondsPerCount);
	}
	else
	{
		return static_cast<float>((m_currTime - m_baseTime - m_pausedTime)*m_secondsPerCount);
	}
}

float GameTimer::DeltaTime() const
{
	return static_cast<float>(m_deltaTime);
}

void GameTimer::Tick()
{
	if (m_isStopped)
	{
		m_deltaTime = 0.0;
		return;
	}
	//得到这一帧的时间
	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
	m_currTime = currentTime;

	//两次计时之间的时间
	m_deltaTime = (m_currTime - m_prevTime) * m_secondsPerCount;

	m_prevTime = m_currTime;

	if (m_deltaTime < 0.0)
		m_deltaTime = 0.0;
}

void GameTimer::Reset()
{
	__int64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER*)(&currentTime));

	m_baseTime = currentTime;
	m_prevTime = currentTime;
	m_stopTime = 0;
	m_isStopped = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (m_isStopped)
	{
		//更新暂停的总时间
		m_pausedTime += (startTime - m_stopTime);

		m_prevTime = startTime;

		//不再暂停
		m_stopTime = 0;
		m_isStopped = false;
	}
}

void GameTimer::Stop()
{
	if (!m_isStopped)
	{
		__int64 currentTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);

		m_stopTime = currentTime;
		m_isStopped = true;
	}
}




