#pragma once
#ifndef CORE_EVENT_QUEUE_H
#define CORE_EVENT_QUEUE_H

#include "core_minimal.h"

namespace core
{

namespace os
{

struct EventQueue
{
	void peek_events();
	void register_event_listener_callback(std::function<void(OSEvent const&)>&& callback);
};

}

}

#endif // !CORE_EVENT_QUEUE_H
