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
	TimeDuration Elapsed;
  uint32 FrSecIndex;
  float64 TotalSecPerFrame;
  float64 SecPerFrame[120];

public:

	SystemClock() :
    Start{}, LastFrameTime{}, CurrentTime{}, DeltaTime(0), Elapsed{}, FrSecIndex{ 0 }, TotalSecPerFrame{}, SecPerFrame{ 0 }
  {}

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
		CurrentTime		= Clock::now();
		TimeDuration dt = CurrentTime - LastFrameTime;
		Elapsed += dt;
		DeltaTime		= dt.count();
		DeltaTime = astl::Min(DeltaTime, 0.1);
		LastFrameTime	= CurrentTime;

    TotalSecPerFrame += DeltaTime - SecPerFrame[FrSecIndex];
    SecPerFrame[FrSecIndex++] = DeltaTime;
    FrSecIndex %= 120;
	}

	float64 Timestep    ()  const { return DeltaTime; }
	float64 ElapsedTime ()  const { return Elapsed.count(); }
  float64 Framerate   ()  const { return 1.0 / (TotalSecPerFrame / 120.0); }
  float64 Frametime   ()  const { return 1000.0 / Framerate(); }
	float32 FTimestep   ()  const { return static_cast<float32>(DeltaTime); }
	float32 FElapsedTime()  const { return static_cast<float32>(Elapsed.count()); }
  float32 FFramerate  ()  const { return 1.0f / (static_cast<float32>(TotalSecPerFrame) / 120.0f); }
  float32 FFrametime  ()  const { return 1000.0f / static_cast<float32>(FFramerate()); }
};


#endif // !LEARNVK_SUBSYTEM_TIME_CLOCK
