#include "win_clock.h"
#include "os_platform.h"

OSBEGIN

// Mspf - milliseconds per frame.

struct WinOSClock::WinOSClockContext
{
	uint64	start;
	uint64	lastFrameTime;

	float64	dt;
	float64	elapsed;

	float64	totalMspf;
	float64	mspf[120];
	size_t	i;
} clockContext;

WinOSClock::WinOSClock() :
	m_ctx{ clockContext }
{
	m_ctx.start = get_elapsed_time_ms();
	m_ctx.lastFrameTime = m_ctx.start;
}

WinOSClock::~WinOSClock() {}

void WinOSClock::on_tick()
{
	uint64 now = get_elapsed_time_ms();
	uint64 timestep = now - m_ctx.lastFrameTime;

	m_ctx.dt = static_cast<float64>(timestep);

	m_ctx.elapsed += m_ctx.dt;
	m_ctx.lastFrameTime = now;

	m_ctx.totalMspf += m_ctx.dt - m_ctx.mspf[m_ctx.i];
	m_ctx.mspf[m_ctx.i++] = m_ctx.dt;

	m_ctx.i %= 120;
}

float64 WinOSClock::delta_time() const
{
	return m_ctx.dt;
}

float64 WinOSClock::elapsed_time() const
{
	return m_ctx.elapsed;
}

float64 WinOSClock::frametime() const
{
	return 1000.0 / framerate();
}

float64 WinOSClock::framerate() const
{
	return 1.0 / (m_ctx.totalMspf / 120.0);
}

float32 WinOSClock::delta_time_f() const
{
	return static_cast<float32>(m_ctx.dt);
}

float32 WinOSClock::elapsed_time_f() const
{
	return static_cast<float32>(m_ctx.elapsed);
}

float32 WinOSClock::frametime_f() const
{
	return 1000.0f / framerate_f();
}

float32 WinOSClock::framerate_f() const
{
	return 1.0f / (static_cast<float32>(m_ctx.totalMspf) / 120.0f);
}

uint64 WinOSClock::get_elapsed_time_ms()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	LARGE_INTEGER hz;
	QueryPerformanceFrequency(&hz);

	return static_cast<uint64>((counter.QuadPart * 1000) / hz.QuadPart);
}

OSEND