#pragma once
#ifndef CORE_TIME_SYSTEM_CLOCK_H
#define CORE_TIME_SYSTEM_CLOCK_H

#ifdef _WIN32
#include "win/win_clock.h"
using PlatformClock = os::WinOSClock;
#else
#endif

COREBEGIN

/**
* TODO(Afiq):
* Introduce more clock related features as we progress further.
*/
class ENGINE_API SystemClock : protected PlatformClock
{
private:
	using Super = PlatformClock;

	SystemClock(const SystemClock&)				= delete;
	SystemClock& operator=(const SystemClock&)	= delete;
	SystemClock(SystemClock&&)					= delete;
	SystemClock& operator=(SystemClock&&)		= delete;
public:

	SystemClock() : 
		Super{} 
	{}

	~SystemClock() {}

	void on_tick() { Super::on_tick(); }

	template <std::floating_point T>
	T delta_time() const
	{
		if constexpr (std::is_same_v<T, float64>)
		{
			return Super::delta_time();
		}
		else
		{
			return Super::delta_time_f();
		}
	}

	template <std::floating_point T>
	T elapsed_time() const
	{
		if constexpr (std::is_same_v<T, float64>)
		{
			return Super::elapsed_time();
		}
		else
		{
			return Super::elapsed_time_f();
		}
	}

	template <std::floating_point T>
	T frametime() const
	{
		if constexpr (std::is_same_v<T, float64>)
		{
			return Super::frametime();
		}
		else
		{
			return Super::frametime_f();
		}
	}

	template <std::floating_point T>
	T framerate() const
	{
		if constexpr (std::is_same_v<T, float64>)
		{
			return Super::framerate();
		}
		else
		{
			return Super::framerate_f();
		}
	}
};

COREEND

#endif // !CORE_TIME_SYSTEM_CLOCK_H

