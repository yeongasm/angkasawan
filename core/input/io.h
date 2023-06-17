#pragma once
#ifndef CORE_INPUT_IO_H
#define CORE_INPUT_IO_H

#include "containers/bitset.h"
#include "platform_interface/platform_interface.h"

COREBEGIN

class WindowingManager;
class NativeWindow;
class SystemClock;

struct InputIOConfiguration
{
	float32 keyDoubleTapTime;
	float32 keyMinDurationForHold;
	float32 mouseDoubleClickTime;
	float32 mouseDoubleClickDistance;
	float32 mouseMinDurationForHold;
};

class ENGINE_API InputIOManager
{
public:

	using MouseMoveDt = Point;

	InputIOManager(PlatformOSInterface& platform, WindowingManager& windowManager);
	~InputIOManager();

	void listen_to_event(const OSEvent& ev, WindowingManager& windowManager);
	void on_tick(const SystemClock& clock);

	// Configuration helpers ...

	void update_input_io_configuration				(const InputIOConfiguration& config);
	void update_config_key_double_tap_time			(float32 seconds);
	void update_config_key_min_duration_for_hold	(float32 seconds);
	void update_config_mouse_double_click_time		(float32 seconds);
	void update_config_mouse_double_click_distance	(float32 pixels);
	void update_config_mouse_min_duration_for_hold	(float32 seconds);

	// Key state helpers ...

	bool key_pressed	(IOKey key) const;
	bool key_double_tap	(IOKey key) const;
	bool key_held		(IOKey key) const;
	bool key_released	(IOKey key) const;

	// Mouse state helpers ...

	bool		mouse_clicked		(IOMouseButton button) const;
	bool		mouse_double_clicked(IOMouseButton button) const;
	bool		mouse_held			(IOMouseButton button) const;
	bool		mouse_released		(IOMouseButton button) const;
	bool		mouse_dragging		(IOMouseButton button) const;
	MouseMoveDt	mouse_drag_delta	(IOMouseButton button) const;

	bool ctrl	() const;
	bool alt	() const;
	bool shift	() const;

private:
	//SystemClock&		m_clock;
	//WindowingManager&	m_windowManager;	// This is here just in case we need to handle an event for a specific window.
	NativeWindow*		m_currentWindow;

	// Mouse events ...

	ftl::Bitset<uint32, Io_Mouse_Button_Max> m_mouseButton;
	IOState m_mouseState[Io_Mouse_Button_Max];
	float32 m_mouseClickTime[Io_Mouse_Button_Max];
	float32 m_mousePrevClickTime[Io_Mouse_Button_Max];
	float32 m_mouseHoldDuration[Io_Mouse_Button_Max];
	float32 m_mouseWheel;
	float32 m_mouseWheelH;
	Point	m_mousePos;
	Point	m_mouseClickPos[Io_Mouse_Button_Max];
	//bool	m_mouseButton[Io_Mouse_Button_Max];

	// Keyboard events ...

	ftl::Bitset<uint32, Io_Key_Max> m_keys;
	IOState m_keyState[Io_Key_Max];
	float32 m_keyPressTime[Io_Key_Max];
	float32 m_keyPrevPressTime[Io_Key_Max];
	float32 m_keyHoldDuration[Io_Key_Max];
	//bool	m_keys[Io_Key_Max];

	//bool	m_flushState;

	InputIOConfiguration m_configuration;

	InputIOManager(const InputIOManager&)	= delete;
	InputIOManager(InputIOManager&&)		= delete;
	InputIOManager& operator=(const InputIOManager&) = delete;
	InputIOManager& operator=(InputIOManager&&)		 = delete;

	void update_mouse_state		(const SystemClock& clock);
	void update_keyboard_state	(const SystemClock& clock);
};

COREEND

#endif // !CORE_INPUT_IO_H
