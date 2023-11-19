#include "io.h"
#include "stat.h"
#include <fmt/format.h>

namespace core
{

namespace io
{

constinit IOContext g_io_context{};

void IOContext::listen_to_event(OSEvent const& ev)
{
	switch (ev.event)
	{
	case OSEvent::Type::Mouse_Move:
		mouse.pos.x = ev.mouseMove.x;
		mouse.pos.y = ev.mouseMove.y;
		break;
	case OSEvent::Type::Mouse_Button:
		mouse.button.set(static_cast<IOMouseButton>(ev.mouseButton.button), ev.mouseButton.down);
		break;
	case OSEvent::Type::Mouse_Wheel:
		mouse.wheelV = ev.mouseWheelDelta.v;
		mouse.wheelH = ev.mouseWheelDelta.h;
		break;
	case OSEvent::Type::Key:
		keyboard.key.set(ev.key.code, ev.key.down);
		break;
	default:
		break;
	}
}

void IOContext::update_state()
{
	// Update mouse state.
	const float32 elapsedTime = stat::elapsed_time_f();
	IOConfigurationInfo config = get_configuration();

	if (!enableStateUpdate)
	{
		return;
	}

	for (int32 i = 0; i < MAX_IO_MOUSE_BUTTONS; i++)
	{
		if (!mouse.button.has(static_cast<IOMouseButton>(i)))
		{
			mouse.clickPos[i] = Point{};
			mouse.state[i] = IOState::Released;
			continue;
		}

		if (mouse.state[i] == IOState::Pressed)
		{
			if (elapsedTime - mouse.clickTime[i] >= config.mouseMinDurationForHold)
			{
				mouse.state[i] = IOState::Held;
			}

			if (mouse.clickTime[i] - mouse.prevClickTime[i] < config.mouseDoubleClickTime)
			{
				Point clickPosDelta = { mouse.pos.x - mouse.clickPos[i].x, mouse.pos.y - mouse.clickPos[i].y };
				float32 lengthSquared = static_cast<float32>(clickPosDelta.x * clickPosDelta.x + clickPosDelta.y * clickPosDelta.y);

				if (lengthSquared < config.mouseDoubleClickDistance * config.mouseDoubleClickDistance)
				{
					mouse.state[i] = IOState::Repeat;
				}
			}
		}

		if (mouse.state[i] == IOState::Held)
		{
			mouse.holdDuration[i] = elapsedTime - mouse.clickTime[i];
		}

		if (mouse.state[i] == IOState::Released)
		{
			mouse.clickPos[i] = mouse.pos;
			mouse.prevClickTime[i] = mouse.clickTime[i];
			mouse.clickTime[i] = elapsedTime;
			mouse.state[i] = IOState::Pressed;
		}
	}

	// Update keyboard state.
	for (int32 i = 0; i < MAX_IO_KEYS; i++)
	{
		if (!keyboard.key.has(static_cast<IOKey>(i)))
		{
			keyboard.state[i] = IOState::Released;
			continue;
		}

		if (keyboard.state[i] == IOState::Pressed)
		{
			if (elapsedTime - keyboard.pressTime[i] >= config.keyMinDurationForHold)
			{
				keyboard.state[i] = IOState::Held;
			}

			if (keyboard.pressTime[i] - keyboard.prevPressTime[i] < config.keyDoubleTapTime)
			{
				keyboard.state[i] = IOState::Repeat;
			}
		}

		if (keyboard.state[i] == IOState::Held)
		{
			keyboard.holdDuration[i] = elapsedTime - keyboard.pressTime[i];
		}

		if (keyboard.state[i] == IOState::Released)
		{
			keyboard.prevPressTime[i] = keyboard.pressTime[i];
			keyboard.pressTime[i] = elapsedTime;
			keyboard.state[i] = IOState::Pressed;
		}
	}
}

void IOContext::reset_mouse_wheel_state()
{
	mouse.wheelH = 0.f;
	mouse.wheelV = 0.f;
}

void update_configuration(IOConfiguration config, float32 val)
{
	auto cfgIndex = static_cast<std::underlying_type_t<IOConfiguration>>(config);
	g_io_context.configuration[cfgIndex] = val;
}

IOConfigurationInfo get_configuration()
{
	IOConfigurationInfo config{};
	lib::memcopy(&config, &g_io_context.configuration, sizeof(IOConfigurationInfo));
	return config;
}

bool key_pressed(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return g_io_context.keyboard.state[index] == IOState::Pressed;
}

bool key_double_tap(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return g_io_context.keyboard.state[index] == IOState::Repeat;
}

bool key_held(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return g_io_context.keyboard.state[index] == IOState::Held;
}

bool key_released(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return g_io_context.keyboard.state[index] == IOState::Released;
}

bool mouse_clicked(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return g_io_context.mouse.state[index] == IOState::Pressed;
}

bool mouse_double_clicked(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return g_io_context.mouse.state[index] == IOState::Repeat;
}

bool mouse_held(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return g_io_context.mouse.state[index] == IOState::Held;
}

bool mouse_released(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return g_io_context.mouse.state[index] == IOState::Released;
}

bool mouse_dragging(IOMouseButton button)
{
	Point drag = mouse_drag_delta(button);
	return drag.x && drag.y;
}

Point mouse_drag_delta(IOMouseButton button)
{
	if (!mouse_held(button))
	{
		return Point{ 0, 0 };
	}
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return Point{
		g_io_context.mouse.pos.x - g_io_context.mouse.clickPos[index].x,
		g_io_context.mouse.pos.y - g_io_context.mouse.clickPos[index].y
	};
}

auto mouse_position() -> Point
{
	return g_io_context.mouse.pos;
}

auto mouse_click_position(IOMouseButton button) -> Point
{
	auto button_index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return g_io_context.mouse.clickPos[button_index];
}

auto mouse_wheel_v() -> float32
{
	return g_io_context.mouse.wheelV;
}

auto mouse_wheel_h() -> float32
{
	return g_io_context.mouse.wheelH;
}

bool ctrl()
{
	return key_held(IOKey::Ctrl);
}

bool alt()
{
	return key_held(IOKey::Alt);
}

bool shift()
{
	return key_held(IOKey::Shift);
}

}

}