#include "platform_header.h"
#include "stat.h"

namespace core
{

namespace stat
{

// nspf - nanoseconds per frame.

struct Win64StatContext
{
	LARGE_INTEGER hz;	// This is the frequency of the performance counter, in counts per second.
	LARGE_INTEGER count;
	LARGE_INTEGER lastFrameCount;

	float64	dt;
	float64	elapsed;
	float64	totalMspf;
	float64	nspf[120];
	size_t	i;
} g_stat_context;

struct Win64StatContextInitializer
{
	Win64StatContextInitializer()
	{
		QueryPerformanceFrequency(&g_stat_context.hz);

		QueryPerformanceCounter(&g_stat_context.count);
		g_stat_context.lastFrameCount = g_stat_context.count;
	}
} static init{};

void update_stat_timings()
{
	QueryPerformanceCounter(&g_stat_context.count);

	uint64 delta = static_cast<uint64>(g_stat_context.count.QuadPart - g_stat_context.lastFrameCount.QuadPart);

	// Promote to nanoseconds.
	delta *= 1000000000;
	delta /= g_stat_context.hz.QuadPart;

	// Store data as milliseconds.
	g_stat_context.dt = static_cast<float64>(delta);
	g_stat_context.elapsed += g_stat_context.dt;
	g_stat_context.totalMspf += g_stat_context.dt - g_stat_context.nspf[g_stat_context.i];
	g_stat_context.nspf[g_stat_context.i++] = g_stat_context.dt;
	g_stat_context.i %= 120;

	g_stat_context.lastFrameCount = g_stat_context.count;
}

float64 delta_time()
{
	return g_stat_context.dt / 1000000000.0;
}

float64 elapsed_time()
{
	return g_stat_context.elapsed / 1000000000.0;
}

float64 frame_time()
{
	return 1.0 / frame_rate();
}

float64 frame_rate()
{
	return 1000000000.0 / (g_stat_context.totalMspf / 120.0);
}

float32 delta_time_f()
{
	return static_cast<float32>(g_stat_context.dt) / 1000000000.f;
}

float32 elapsed_time_f()
{
	return static_cast<float32>(g_stat_context.elapsed) / 1000000000.f;
}

float32 frame_time_f()
{
	return 1.0f / frame_rate_f();
}

float32 frame_rate_f()
{
	return 1000000000.0f / (static_cast<float32>(g_stat_context.totalMspf) / 120.0f);
}

}

}