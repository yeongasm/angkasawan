#include "platform_header.h"
#include "stat.h"

namespace core
{

namespace stat
{

// Mspf - milliseconds per frame.

struct Win64StatContext
{
	uint64 start;
	uint64 lastFrameTime;
	float64	dt;
	float64	elapsed;
	float64	totalMspf;
	float64	mspf[120];
	size_t	i;
} gStatCtx;

uint64 get_elapsed_time_ms()
{
	LARGE_INTEGER hz;
	LARGE_INTEGER counter;

	QueryPerformanceFrequency(&hz);
	QueryPerformanceCounter(&counter);

	return static_cast<uint64>((counter.QuadPart * 1000) / hz.QuadPart);
}

struct Win64StatContextInitializer
{
	Win64StatContextInitializer()
	{
		gStatCtx.start = get_elapsed_time_ms();
		gStatCtx.lastFrameTime = gStatCtx.start;
	}
} static init{};

void update_stat_timings()
{
	uint64 now = get_elapsed_time_ms();
	uint64 timestep = now - gStatCtx.lastFrameTime;

	gStatCtx.dt = static_cast<float64>(timestep);
	gStatCtx.elapsed += gStatCtx.dt;
	gStatCtx.lastFrameTime = now;
	gStatCtx.totalMspf += gStatCtx.dt - gStatCtx.mspf[gStatCtx.i];
	gStatCtx.mspf[gStatCtx.i++] = gStatCtx.dt;
	gStatCtx.i %= 120;
}

float64 delta_time()
{
	return gStatCtx.dt;
}

float64 elapsed_time()
{
	return gStatCtx.elapsed;
}

float64 frame_time()
{
	return 1000.0 / frame_rate();
}

float64 frame_rate()
{
	return 1.0 / (gStatCtx.totalMspf / 120.0);
}

float32 delta_time_f()
{
	return static_cast<float32>(gStatCtx.dt);
}

float32 elapsed_time_f()
{
	return static_cast<float32>(gStatCtx.elapsed);
}

float32 frame_time_f()
{
	return 1000.0f / frame_rate_f();
}

float32 frame_rate_f()
{
	return 1.0f / (static_cast<float32>(gStatCtx.totalMspf) / 120.0f);
}

}

}