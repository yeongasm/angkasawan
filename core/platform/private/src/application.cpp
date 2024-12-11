#include "application.hpp"
#include <functional>

namespace core
{
namespace platform
{
Application::Application()
{
	set_io_context(*this);
}

auto Application::update() -> void
{
	PerformanceCounter::update();
	EventQueue::peek_events();
	IOContext::update();
}
}
}