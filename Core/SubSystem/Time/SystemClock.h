#pragma once
#ifndef LEARNVK_SUBSYTEM_TIME_CLOCK
#define LEARNVK_SUBSYTEM_TIME_CLOCK

#include <chrono>
#include "Platform/Minimal.h"
#include "Library/Templates/Templates.h"


class SystemClock
{
private:

	using Clock			= std::chrono::high_resolution_clock;
	using TimePoint		= std::chrono::time_point<std::chrono::steady_clock>;
	using TimeDuration	= std::chrono::duration<float64, std::milli>;

	TimePoint		Start;
	TimeDuration	LastFrameTime;
	TimeDuration	CurrentTime;
	float64			DeltaTime;

public:

	SystemClock() :
		Start{}, LastFrameTime{}, CurrentTime{}, DeltaTime(0) {}

	~SystemClock() {}

	DELETE_COPY_AND_MOVE(SystemClock)

	void StartSystemClock()
	{
		Start = Clock::now();
	}

	/**
	* Should be called every frame.
	*/
	void Tick()
	{
		CurrentTime		= Clock::now() - Start;
		DeltaTime		= Min(CurrentTime.count() - LastFrameTime.count(), 0.1);
		LastFrameTime	= CurrentTime;
	}

	float64 Timestep()		const { return DeltaTime; }
	float64 ElapsedTime()	const { return CurrentTime.count(); }
	float64 PrevFrameTime()	const { return LastFrameTime.count(); }
	float32 FTimestep()		const { return static_cast<float32>(DeltaTime); }
	float32 FElapsedTime()	const { return static_cast<float32>(CurrentTime.count()); }
	float32 FPrevFrameTime()const { return static_cast<float32>(LastFrameTime.count()); }
};


#endif // !LEARNVK_SUBSYTEM_TIME_CLOCK