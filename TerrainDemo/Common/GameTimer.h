#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();
	float TotalTime() const;//返回定时器从计时开始到现在的总时间，不包括暂停时间
	float DeltaTime() const;//返回本次计时与上次计时之间的时间差

	void Reset();//重置
	void Start();//暂停后恢复计时
	void Stop();//暂停
	void Tick();//计时一次，每一帧调用

private:
	double m_secondsPerCount;//系统相关，系统两次计数之间的时间差，是定时器把时间转换为s的基准
	double m_deltaTime;//距离上次计时的时间差

	__int64 m_baseTime;//定时器开始工作的时间点
	__int64 m_pausedTime;//暂停的总时间
	__int64 m_stopTime;//暂停的开始时间
	__int64 m_prevTime;//上次计时的时间点
	__int64 m_currTime;//本次计时的时间点

	bool m_isStopped;//是否暂停
};

#endif	//GAMETIMER_H

