#pragma once
#ifndef PLATFORM_EVENT_QUEUE_H
#define PLATFORM_EVENT_QUEUE_H

#include "platform_minimal.hpp"
#include "lib/array.hpp"
#include "lib/function.hpp"
#include "windowing.hpp"

namespace core
{
namespace platform
{
class EventQueue
{
public:
	auto peek_events() const -> void;
	auto post_quit_message(int32 exitCode = 0) const -> void;
private:
	friend struct EventQueueHandler;
};
}
}

#endif // !PLATFORM_EVENT_QUEUE_H
