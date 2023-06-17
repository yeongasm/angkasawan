#pragma once
#ifndef CORE_TIME_WIN_WIN_CLOCK_H
#define CORE_TIME_WIN_WIN_CLOCK_H

#include "os_shims.h"
#include "platform_interface/platform_common.h"

OSBEGIN

class ENGINE_API WinOSClock
{
public:

	WinOSClock();
	~WinOSClock();

	void	on_tick			();

	float64 delta_time		() const;
	float64 elapsed_time	() const;
	float64 frametime		() const;
	float64 framerate		() const;

	float32 delta_time_f	() const;
	float32 elapsed_time_f	() const;
	float32	frametime_f		() const;
	float32 framerate_f		() const;

private:
	struct WinOSClockContext;
	WinOSClockContext& m_ctx;

	WinOSClock(const WinOSClock&)			 = delete;
	WinOSClock& operator=(const WinOSClock&) = delete;
	WinOSClock(WinOSClock&&)			= delete;
	WinOSClock& operator=(WinOSClock&&) = delete;

	uint64 get_elapsed_time_ms();
};

OSEND

#endif // !CORE_TIME_WIN_WIN_CLOCK_H
