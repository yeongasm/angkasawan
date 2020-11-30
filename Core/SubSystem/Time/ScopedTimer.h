#pragma once
#ifndef LEARNVK_SUBSYSTEM_SCOPED_TIMER
#define LEARNVK_SUBSYSTEM_SCOPED_TIMER

#include <chrono>
#include "Platform/Minimal.h"


template <typename TimerPrecision>
class BaseScopedTimer
{
public:

	using Clock			= std::chrono::high_resolution_clock;
	using TimePoint		= std::chrono::time_point<std::chrono::steady_clock>;
	using TimeDuration	= std::chrono::duration<TimerPrecision, std::milli>;

private:

	TimePoint Start;
	TimePoint End;
	TimeDuration Duration;

public:

	BaseScopedTimer() :
		Start{}, End{}, Duration{}
	{
		Start = Clock::now();
	}

	~BaseScopedTimer()
	{}

	BaseScopedTimer(const BaseScopedTimer& Rhs) = delete;
	BaseScopedTimer(BaseScopedTimer&& Rhs)		= delete;

	BaseScopedTimer& operator= (const BaseScopedTimer& Rhs) = delete;
	BaseScopedTimer& operator= (BaseScopedTimer&& Rhs)		= delete;

	TimerPrecision Now()
	{
		End = Clock::now();
		Duration = Start - End;
		return Duration.count();
	}

};


template <typename TimerPrecision>
class ScopedTimer : public BaseScopedTimer<TimerPrecision>
{
public:

	using CallbackFuncPtr = void(*)(TimerPrecision);

private:

	using Super = BaseScopedTimer<TimerPrecision>;

	CallbackFuncPtr Callback;

public:

	ScopedTimer(CallbackFuncPtr InCallback) :
		Super(), Callback(InCallback)
	{}

	~ScopedTimer()
	{
		if (Callback)
		{
			Callback(Super::Now());
		}
	}

	ScopedTimer(const ScopedTimer& Rhs) = delete;
	ScopedTimer(ScopedTimer&& Rhs)		= delete;

	ScopedTimer& operator= (const ScopedTimer& Rhs) = delete;
	ScopedTimer& operator= (ScopedTimer&& Rhs)		= delete;

};


template <typename TimerPrecision>
class PrintfScopedTimer : public ScopedTimer<TimerPrecision>
{
private:

	using Super = ScopedTimer<TimerPrecision>;

	void PrintTime(TimerPrecision Count)
	{
		printf("Time: %.5f ms\n", Count);
	}

public:

	PrintfScopedTimer() : 
		Super(PrintTime)
	{}

	~PrintfScopedTimer()
	{}

	PrintfScopedTimer(const PrintfScopedTimer& Rhs) = delete;
	PrintfScopedTimer(PrintfScopedTimer&& Rhs)		= delete;

	PrintfScopedTimer& operator= (const PrintfScopedTimer& Rhs) = delete;
	PrintfScopedTimer& operator= (PrintfScopedTimer&& Rhs)		= delete;

};


using FScopedTimer = ScopedTimer<float32>;
using DScopedTimer = ScopedTimer<float64>;

using FPrintfScopedTimer = PrintfScopedTimer<float32>;
using DPrintfScopedTimer = PrintfScopedTimer<float64>;


#endif // !LEARNVK_SUBSYSTEM_SCOPED_TIMER