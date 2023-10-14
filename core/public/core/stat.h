#pragma once
#ifndef CORE_TIME_SYSTEM_CLOCK_H
#define CORE_TIME_SYSTEM_CLOCK_H

#include "core_minimal.h"

namespace core
{

namespace stat
{

CORE_API inline float64 delta_time();
CORE_API inline float64 elapsed_time();
CORE_API inline float64 frame_time();
CORE_API inline float64 frame_rate();
CORE_API inline float32 delta_time_f();
CORE_API inline float32 elapsed_time_f();
CORE_API inline float32 frame_time_f();
CORE_API inline float32 frame_rate_f();

}

}

#endif // !CORE_TIME_SYSTEM_CLOCK_H

