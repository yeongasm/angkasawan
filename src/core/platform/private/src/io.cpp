#include "io.hpp"
#include "performance_counter.hpp"

namespace core
{
namespace platform
{
auto IOContext::update() -> void
{
	// Update mouse state.
	const float32 elapsedTime = static_cast<float32>(PerformanceCounter::elapsed_time());

	if (!m_enableIOStateUpdate)
	{
		return;
	}

	for (int32 i = 0; i < MAX_IO_MOUSE_BUTTONS; i++)
	{
		if (!m_mouse.button.has(static_cast<IOMouseButton>(i)))
		{
			m_mouse.clickPos[i] = Point{};
			m_mouse.state[i] = IOState::Released;
			continue;
		}

		if (m_mouse.state[i] == IOState::Pressed)
		{
			if (elapsedTime - m_mouse.clickTime[i] >= m_config.mouseMinDurationForHold)
			{
				m_mouse.state[i] = IOState::Held;
			}

			if (m_mouse.clickTime[i] - m_mouse.prevClickTime[i] < m_config.mouseDoubleClickTime)
			{
				Point clickPosDelta = { m_mouse.pos.x - m_mouse.clickPos[i].x, m_mouse.pos.y - m_mouse.clickPos[i].y };
				float32 lengthSquared = static_cast<float32>(clickPosDelta.x * clickPosDelta.x + clickPosDelta.y * clickPosDelta.y);

				if (lengthSquared < m_config.mouseDoubleClickDistance * m_config.mouseDoubleClickDistance)
				{
					m_mouse.state[i] = IOState::Repeat;
				}
			}
		}

		if (m_mouse.state[i] == IOState::Repeat &&
			elapsedTime - m_mouse.clickTime[i] >= m_config.mouseMinDurationForHold)
		{
			m_mouse.state[i] = IOState::Held;
		}

		if (m_mouse.state[i] == IOState::Held)
		{
			m_mouse.holdDuration[i] = elapsedTime - m_mouse.clickTime[i];
		}

		if (m_mouse.state[i] == IOState::Released)
		{
			m_mouse.clickPos[i] = m_mouse.pos;
			m_mouse.prevClickTime[i] = m_mouse.clickTime[i];
			m_mouse.clickTime[i] = elapsedTime;
			m_mouse.state[i] = IOState::Pressed;
		}
	}

	// Update keyboard state.
	for (int32 i = 0; i < MAX_IO_KEYS; i++)
	{
		if (!m_keyboard.key.has(static_cast<IOKey>(i)))
		{
			m_keyboard.state[i] = IOState::Released;
			continue;
		}

		if (m_keyboard.state[i] == IOState::Pressed)
		{
			if (m_keyboard.pressTime[i] - m_keyboard.prevPressTime[i] < m_config.keyDoubleTapTime)
			{
				m_keyboard.state[i] = IOState::Repeat;
			} 
			
			if (elapsedTime - m_keyboard.pressTime[i] >= m_config.keyMinDurationForHold)
			{
				m_keyboard.state[i] = IOState::Held;
			}
		}

		if (m_keyboard.state[i] == IOState::Repeat &&
			elapsedTime - m_keyboard.pressTime[i] >= m_config.keyMinDurationForHold)
		{
			m_keyboard.state[i] = IOState::Held;
		}

		if (m_keyboard.state[i] == IOState::Held)
		{
			m_keyboard.holdDuration[i] = elapsedTime - m_keyboard.pressTime[i];
		}

		if (m_keyboard.state[i] == IOState::Released)
		{
			m_keyboard.prevPressTime[i] = m_keyboard.pressTime[i];
			m_keyboard.pressTime[i] = elapsedTime;
			m_keyboard.state[i] = IOState::Pressed;
		}
	}
}

auto IOContext::reset_mouse_wheel_state() -> void
{
	m_mouse.wheelH = 0.f;
	m_mouse.wheelV = 0.f;
}

auto IOContext::update_configuration(IOConfigurationInfo const& info) -> void
{
	if (info.keyDoubleTapTime)
	{
		m_config.keyDoubleTapTime = *info.keyDoubleTapTime;
	}
	if (info.keyMinDurationForHold)
	{
		m_config.keyMinDurationForHold = *info.keyMinDurationForHold;
	}
	if (info.mouseDoubleClickDistance)
	{
		m_config.mouseDoubleClickDistance = *info.mouseDoubleClickDistance;
	}
	if (info.mouseDoubleClickTime)
	{
		m_config.mouseDoubleClickTime = *info.mouseDoubleClickTime;
	}
	if (info.mouseMinDurationForHold)
	{
		m_config.mouseMinDurationForHold = *info.mouseMinDurationForHold;
	}
}

auto IOContext::configuration() const -> Configuration
{
	return m_config;
}

auto IOContext::key_pressed(IOKey key) -> bool
{
	auto index = std::to_underlying(key);
	return m_keyboard.state[index] == IOState::Pressed;
}

auto IOContext::key_double_tap(IOKey key) -> bool
{
	auto index = std::to_underlying(key);
	return m_keyboard.state[index] == IOState::Repeat;
}

auto IOContext::key_held(IOKey key) -> bool
{
	auto index = std::to_underlying(key);
	return m_keyboard.state[index] == IOState::Held;
}

auto IOContext::key_released(IOKey key) -> bool
{
	auto index = std::to_underlying(key);
	return m_keyboard.state[index] == IOState::Released;
}

auto IOContext::mouse_clicked(IOMouseButton button) -> bool
{
	auto index = std::to_underlying(button);
	return m_mouse.state[index] == IOState::Pressed;
}

auto IOContext::mouse_double_clicked(IOMouseButton button) -> bool
{
	auto index = std::to_underlying(button);
	return m_mouse.state[index] == IOState::Repeat;
}

auto IOContext::mouse_held(IOMouseButton button) -> bool
{
	auto index = std::to_underlying(button);
	return m_mouse.state[index] == IOState::Held;
}

auto IOContext::mouse_released(IOMouseButton button) -> bool
{
	auto index = std::to_underlying(button);
	return m_mouse.state[index] == IOState::Released;
}

auto IOContext::mouse_dragging(IOMouseButton button) -> bool
{
	Point drag = mouse_drag_delta(button);
	return drag.x && drag.y;
}

auto IOContext::mouse_drag_delta(IOMouseButton button) -> Point
{
	if (!mouse_held(button))
	{
		return Point{ 0, 0 };
	}
	auto index = std::to_underlying(button);
	return Point{
		m_mouse.pos.x - m_mouse.clickPos[index].x,
		m_mouse.pos.y - m_mouse.clickPos[index].y
	};
}

auto IOContext::mouse_position() -> Point
{
	return m_mouse.pos;
}

auto IOContext::mouse_click_position(IOMouseButton button) -> Point
{
	auto button_index = std::to_underlying(button);
	return m_mouse.clickPos[button_index];
}

auto IOContext::mouse_wheel_v() -> float32
{
	return m_mouse.wheelV;
}

auto IOContext::mouse_wheel_h() -> float32
{
	return m_mouse.wheelH;
}

auto IOContext::ctrl() -> bool
{
	return key_held(IOKey::Ctrl);
}

auto IOContext::alt() -> bool
{
	return key_held(IOKey::Alt);
}

auto IOContext::shift() -> bool
{
	return key_held(IOKey::Shift);
}

auto IOContext::enable_io_state_update(bool v) -> void
{
	m_enableIOStateUpdate = v;
}
}
}