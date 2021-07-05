#pragma once
#ifndef LEARNVK_SUBSYTEM_TIME_CLOCK
#define LEARNVK_SUBSYTEM_TIME_CLOCK

#include <chrono>
#include "Platform/Minimal.h"
#include "Library/Templates/Templates.h"


class SystemClock
{
private:

	using Clock		= std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::steady_clock::time_point;
	using TimeDuration = std::chrono::duration<float64>;

	TimePoint	Start;
	TimePoint	CurrentTime;
	TimePoint	LastFrameTime;
	float64		DeltaTime;
	TimeDuration elapsed;

public:

	SystemClock() :
		Start{}, LastFrameTime{}, CurrentTime{}, DeltaTime(0) {}

	~SystemClock() {}

	DELETE_COPY_AND_MOVE(SystemClock)

	void StartSystemClock()
	{
		Start = CurrentTime = LastFrameTime = Clock::now();
		//LastFrameTime = Start;
		//LastFrameTime = Clock::now();
	}

	/**
	* Should be called every frame.
	*/
	void Tick()
	{
		//auto startMs	= std::chrono::time_point_cast<std::chrono::seconds>(Start);
		CurrentTime		= Clock::now();
		TimeDuration dt = CurrentTime - LastFrameTime;
		elapsed += dt;
		DeltaTime		= dt.count();
		DeltaTime = astl::Min(DeltaTime, 0.1);
		LastFrameTime	= CurrentTime;
	}

	float64 Timestep() const 
	{ 
		return DeltaTime; 
	}

	float64 ElapsedTime() const 
	{
		return elapsed.count();
	}

	//float64 PrevFrameTime()	const { return LastFrameTime.count(); }

	float32 FTimestep()	const 
	{ 
		return static_cast<float32>(DeltaTime); 
	}

	float32 FElapsedTime() const 
	{ 
		return static_cast<float32>(elapsed.count());
	}
	//float32 FPrevFrameTime()const { return static_cast<float32>(LastFrameTime.count()); }
};


#endif // !LEARNVK_SUBSYTEM_TIME_CLOCK
