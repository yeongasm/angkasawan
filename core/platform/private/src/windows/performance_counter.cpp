#include "platform_header.h"
#include "performance_counter.hpp"

namespace core
{
namespace platform
{
struct QPCContext
{
	LARGE_INTEGER hz;	// This is the frequency of the performance counter, in counts per second.
	LARGE_INTEGER count;
	LARGE_INTEGER lastFrameCount;

	float64	dt;
	float64	elapsed;
	float64	totalMspf;
	float64	nspf[120];
	size_t	i;

	QPCContext() :
		hz{},
		count{},
		lastFrameCount{},
		dt{},
		elapsed{},
		totalMspf{},
		nspf{},
		i{}
	{
		QueryPerformanceFrequency(&hz);

		QueryPerformanceCounter(&count);
		lastFrameCount = count;
	}
} static gQPCContext = {};

auto PerformanceCounter::update() -> void
{
	QueryPerformanceCounter(&gQPCContext.count);

	uint64 delta = static_cast<uint64>(gQPCContext.count.QuadPart - gQPCContext.lastFrameCount.QuadPart);

	// Promote to nanoseconds.
	delta *= 1000000000;
	delta /= gQPCContext.hz.QuadPart;

	// Store data as milliseconds.
	gQPCContext.dt = static_cast<float64>(delta);
	gQPCContext.elapsed += gQPCContext.dt;
	gQPCContext.totalMspf += gQPCContext.dt - gQPCContext.nspf[gQPCContext.i];
	gQPCContext.nspf[gQPCContext.i++] = gQPCContext.dt;
	gQPCContext.i %= 120;

	gQPCContext.lastFrameCount = gQPCContext.count;
}

auto PerformanceCounter::delta_time() -> float64
{
	return gQPCContext.dt / 1000000000.0;
}

auto PerformanceCounter::elapsed_time() -> float64
{
	return gQPCContext.elapsed / 1000000000.0;
}

auto PerformanceCounter::frametime() -> float64
{
	return 1.0 / framerate();
}

auto PerformanceCounter::framerate() -> float64
{
	return 1000000000.0 / (gQPCContext.totalMspf / 120.0);
}
}
}