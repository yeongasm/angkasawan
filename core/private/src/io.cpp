#include "io.h"
#include "stat.h"

namespace core
{

namespace io
{

constinit IOContext gIOCtx{};

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
		mouse.wheelV = ev.mouseWheel.offset;
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

	if (enableStateUpdate)
	{
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
}

void update_configuration(IOConfiguration config, float32 val)
{
	auto cfgIndex = static_cast<std::underlying_type_t<IOConfiguration>>(config);
	gIOCtx.configuration[cfgIndex] = val;
}

IOConfigurationInfo get_configuration()
{
	IOConfigurationInfo config{};
	lib::memcopy(&config, &gIOCtx.configuration, sizeof(IOConfigurationInfo));
	return config;
}

bool key_pressed(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return gIOCtx.keyboard.state[index] == IOState::Pressed;
}

bool key_double_tap(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return gIOCtx.keyboard.state[index] == IOState::Repeat;
}

bool key_held(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return gIOCtx.keyboard.state[index] == IOState::Held;
}

bool key_released(IOKey key)
{
	auto index = static_cast<std::underlying_type_t<IOKey>>(key);
	return gIOCtx.keyboard.state[index] == IOState::Released;
}

bool mouse_clicked(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return gIOCtx.mouse.state[index] == IOState::Pressed;
}

bool mouse_double_clicked(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return gIOCtx.mouse.state[index] == IOState::Repeat;
}

bool mouse_held(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return gIOCtx.mouse.state[index] == IOState::Held;
}

bool mouse_released(IOMouseButton button)
{
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return gIOCtx.mouse.state[index] == IOState::Released;
}

bool mouse_dragging(IOMouseButton button)
{
	Point drag = mouse_drag_delta(button);
	if (!drag.x && !drag.y)
	{
		return false;
	}
	return ((static_cast<float32>(drag.x) - 0.0f) <= 0.0001f) 
		&& ((static_cast<float32>(drag.y) - 0.0f) <= 0.0001f);
}

Point mouse_drag_delta(IOMouseButton button)
{
	if (!mouse_held(button))
	{
		return Point{ 0, 0 };
	}
	auto index = static_cast<std::underlying_type_t<IOMouseButton>>(button);
	return Point{
		gIOCtx.mouse.pos.x - gIOCtx.mouse.clickPos[index].x,
		gIOCtx.mouse.pos.y - gIOCtx.mouse.clickPos[index].y
	};
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