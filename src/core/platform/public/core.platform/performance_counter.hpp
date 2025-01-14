#pragma once
#ifndef PLATFORM_TIME_SYSTEM_CLOCK_H
#define PLATFORM_TIME_SYSTEM_CLOCK_H

#include "platform_api.h"
#include "lib/common.hpp"

namespace core
{
namespace platform
{
struct PerformanceCounter
{
	PLATFORM_API static auto update() -> void;
	/** 
	* Milliseconds.
	*/
	PLATFORM_API static auto delta_time() -> float64;
	/**
	* Milliseconds.
	*/
	PLATFORM_API static auto elapsed_time() -> float64;
	/**
	* Milliseconds.
	*/
	PLATFORM_API static auto frametime() -> float64;
	/**
	* Frames per second.
	*/
	PLATFORM_API static auto framerate() -> float64;
};
}
}

#endif // !PLATFORM_TIME_SYSTEM_CLOCK_H

