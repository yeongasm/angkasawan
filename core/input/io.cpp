#include "io.h"
#include "math.h"
#include "windowing/windowing.h"
#include "time/system_clock.h"

COREBEGIN

struct InputIOEventCallbackArg
{
	InputIOManager& ioManager;
	WindowingManager& windowingManager;
};

InputIOManager::InputIOManager(PlatformOSInterface& platform, WindowingManager& windowManager) :
	m_currentWindow{},
	m_mouseState{},
	m_mouseClickTime{},
	m_mousePrevClickTime{},
	m_mouseHoldDuration{},
	m_mouseWheel{},
	m_mouseWheelH{},
	m_mousePos{},
	m_mouseClickPos{},
	m_mouseButton{},
	m_keyState{},
	m_keyPressTime{},
	m_keyPrevPressTime{},
	m_keyHoldDuration{},
	m_keys{},
	m_configuration{}
{
	// Ugly but this is the easiest path to getting listeners work inside of win proc callback.
	static InputIOEventCallbackArg args{ *this, windowManager };
	platform.register_os_event_callback(
		&args,
		[](void* argument, const core::OSEvent& ev) -> void
		{
			InputIOEventCallbackArg* arg = reinterpret_cast<InputIOEventCallbackArg*>(argument);
			arg->ioManager.listen_to_event(ev, arg->windowingManager);
		}
	);
}

InputIOManager::~InputIOManager() {}

void InputIOManager::listen_to_event(const OSEvent& ev, WindowingManager& windowManager)
{
	if (!m_currentWindow)
	{
		WinResult<NativeWindow*> result = windowManager.get_root_window();
		m_currentWindow = result.payload;
	}

	if (m_currentWindow && m_currentWindow->get_raw_handle() != ev.windowHandle)
	{
		m_currentWindow = windowManager.get_window_from_raw_handle(ev.windowHandle);
	}

	switch (ev.event)
	{
	case OSEvent::Type::Mouse_Move:
		m_mousePos.x = ev.mouseMove.x;
		m_mousePos.y = ev.mouseMove.y;
		break;
	case OSEvent::Type::Mouse_Button:
		m_mouseButton.set(ev.mouseButton.button, ev.mouseButton.down);
		break;
	case OSEvent::Type::Mouse_Wheel:
		m_mouseWheel = ev.mouseWheel.offset;
		break;
	case OSEvent::Type::Key:
		m_keys.set(ev.key.code, ev.key.down);
		break;
	default:
		break;
	}
}

void InputIOManager::on_tick(const SystemClock& clock)
{
	if (m_currentWindow && m_currentWindow->is_focused())
	{
		update_mouse_state(clock);
		update_keyboard_state(clock);
	}
}

void InputIOManager::update_input_io_configuration(const InputIOConfiguration& config)
{
	m_configuration = config;
}

void InputIOManager::update_config_key_double_tap_time(float32 seconds)
{
	m_configuration.keyDoubleTapTime = seconds;
}

void InputIOManager::update_config_key_min_duration_for_hold(float32 seconds)
{
	m_configuration.keyMinDurationForHold = seconds;
}

void InputIOManager::update_config_mouse_double_click_time(float32 seconds)
{
	m_configuration.mouseDoubleClickTime = seconds;
}

void InputIOManager::update_config_mouse_double_click_distance(float32 pixels)
{
	m_configuration.mouseDoubleClickDistance = pixels;
}

void InputIOManager::update_config_mouse_min_duration_for_hold(float32 seconds)
{
	m_configuration.mouseMinDurationForHold = seconds;
}

bool InputIOManager::key_pressed(IOKey key) const
{
	return m_keyState[key] == Io_State_Pressed;
}

bool InputIOManager::key_double_tap(IOKey key) const
{
	return m_keyState[key] == Io_State_Repeat;
}

bool InputIOManager::key_held(IOKey key) const
{
	return m_keyState[key] == Io_State_Held;
}

bool InputIOManager::key_released(IOKey key) const
{
	return m_keyState[key] == Io_State_Released;
}

bool InputIOManager::mouse_clicked(IOMouseButton button) const
{
	return m_mouseState[button] == Io_State_Pressed;
}

bool InputIOManager::mouse_double_clicked(IOMouseButton button) const
{
	return m_mouseState[button] == Io_State_Repeat;
}

bool InputIOManager::mouse_held(IOMouseButton button) const
{
	return m_mouseState[button] == Io_State_Held;
}

bool InputIOManager::mouse_released(IOMouseButton button) const
{
	return m_mouseState[button] == Io_State_Released;
}

bool InputIOManager::mouse_dragging(IOMouseButton button) const
{
	InputIOManager::MouseMoveDt drag = mouse_drag_delta(button);
	if (!drag.x && !drag.y)
	{
		return false;
	}
	return	math::compare_float(static_cast<float32>(drag.x), 0.0f) && 
			math::compare_float(static_cast<float32>(drag.y), 0.0f);
}

InputIOManager::MouseMoveDt InputIOManager::mouse_drag_delta(IOMouseButton button) const
{
	if (!mouse_held(button))
	{
		return InputIOManager::MouseMoveDt{ 0, 0 };
	}

	return InputIOManager::MouseMoveDt{
		m_mousePos.x - m_mouseClickPos[button].x,
		m_mousePos.y - m_mouseClickPos[button].y
	};
}

bool InputIOManager::ctrl() const
{
	return key_held(Io_Key_Ctrl);
}

bool InputIOManager::alt() const
{
	return key_held(Io_Key_Alt);
}

bool InputIOManager::shift() const
{
	return key_held(Io_Key_Shift);
}

void InputIOManager::update_mouse_state(const SystemClock& clock)
{
	const float32 elapsedTime = clock.elapsed_time<float32>();

	for (int32 i = 0; i < Io_Mouse_Button_Max; i++)
	{
		if (!m_mouseButton.has(i))
		{
			m_mouseClickPos[i]	= Point{};
			m_mouseState[i]		= Io_State_Released;
			continue;
		}

		if (m_mouseState[i] == Io_State_Pressed)
		{
			if (elapsedTime - m_mouseClickTime[i] >= m_configuration.mouseMinDurationForHold)
			{
				m_mouseState[i] = Io_State_Held;
			}

			if (m_mouseClickTime[i] - m_mousePrevClickTime[i] < m_configuration.mouseDoubleClickTime)
			{
				MouseMoveDt clickPosDelta	= { m_mousePos.x - m_mouseClickPos[i].x, m_mousePos.y - m_mouseClickPos[i].y };
				float32 lengthSquared		= static_cast<float32>(clickPosDelta.x * clickPosDelta.x + clickPosDelta.y * clickPosDelta.y);

				if (lengthSquared < m_configuration.mouseDoubleClickDistance * m_configuration.mouseDoubleClickDistance)
				{
					m_mouseState[i] = Io_State_Repeat;
				}
			}
		}

		if (m_mouseState[i] == Io_State_Held)
		{
			m_mouseHoldDuration[i] = elapsedTime - m_mouseClickTime[i];
		}

		if (m_mouseState[i] == Io_State_Released)
		{
			m_mouseClickPos[i]		= m_mousePos;
			m_mousePrevClickTime[i] = m_mouseClickTime[i];
			m_mouseClickTime[i]		= elapsedTime;
			m_mouseState[i]			= Io_State_Pressed;
		}
	}
}

void InputIOManager::update_keyboard_state(const SystemClock& clock)
{
	const float32 elapsedTime = clock.elapsed_time<float32>();

	for (int32 i = 0; i < Io_Key_Max; i++)
	{
		if (!m_keys.has(i))
		{
			m_keyState[i] = Io_State_Released;
			continue;
		}

		if (m_keyState[i] == Io_State_Pressed)
		{
			if (elapsedTime - m_keyPressTime[i] >= m_configuration.keyMinDurationForHold)
			{
				m_keyState[i] = Io_State_Held;
			}

			if (m_keyPressTime[i] - m_keyPrevPressTime[i] < m_configuration.keyDoubleTapTime)
			{
				m_keyState[i] = Io_State_Repeat;
			}
		}

		if (m_keyState[i] == Io_State_Held)
		{
			m_keyHoldDuration[i] = elapsedTime - m_keyPressTime[i];
		}

		if (m_keyState[i] == Io_State_Released)
		{
			m_keyPrevPressTime[i]	= m_keyPressTime[i];
			m_keyPressTime[i]		= elapsedTime;
			m_keyState[i]			= Io_State_Pressed;
		}
	}
}

COREEND