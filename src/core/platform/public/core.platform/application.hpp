#pragma once
#ifndef CORE_PLATFORM_APPLICATION_HPP
#define CORE_PLATFORM_APPLICATION_HPP

#include "windowing.hpp"
#include "io.hpp"
#include "cursor.hpp"
#include "event_queue.hpp"
#include "cursor.hpp"
#include "performance_counter.hpp"

namespace core
{
namespace platform
{
class Application : public IOContext, protected WindowContext, public CursorContext, public EventQueue
{
public:
	Application();
	
	auto update() -> void;
};
}
}

#endif // !CORE_PLATFORM_APPLICATION_HPP
