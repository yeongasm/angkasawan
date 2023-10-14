#pragma once
#ifndef CORE_INPUT_IO_H
#define CORE_INPUT_IO_H

#include <array>
#include "lib/bitset.h"
#include "core_minimal.h"

namespace core
{

namespace io
{

constexpr auto MAX_IO_MOUSE_BUTTONS	= static_cast<std::underlying_type_t<IOMouseButton>>(IOMouseButton::Max);
constexpr auto MAX_IO_STATES = static_cast<std::underlying_type_t<IOState>>(IOState::Max);
constexpr auto MAX_IO_KEYS = static_cast<std::underlying_type_t<IOKey>>(IOKey::Max);
constexpr auto MAX_IO_CURSOR_TYPES = static_cast<std::underlying_type_t<IOCursorType>>(IOConfiguration::Max);
constexpr auto MAX_IO_CONFIGURATIONS = static_cast<std::underlying_type_t<IOConfiguration>>(IOConfiguration::Max);

struct IOConfigurationInfo
{
	float32 keyDoubleTapTime;
	float32 keyMinDurationForHold;
	float32 mouseDoubleClickTime;
	float32 mouseDoubleClickDistance;
	float32 mouseMinDurationForHold;
};

struct IOContext
{
	struct Mouse
	{
		lib::bitset<IOMouseButton, MAX_IO_MOUSE_BUTTONS> button = {};
		IOState state[MAX_IO_MOUSE_BUTTONS] = {};
		float32 clickTime[MAX_IO_MOUSE_BUTTONS] = {};
		float32 prevClickTime[MAX_IO_MOUSE_BUTTONS] = {};
		float32 holdDuration[MAX_IO_MOUSE_BUTTONS] = {};
		float32 wheelV = {};
		float32 wheelH = {};
		Point pos = {};
		Point clickPos[MAX_IO_MOUSE_BUTTONS] = {};
	} mouse = {};

	struct Keyboard
	{
		lib::bitset<IOKey, MAX_IO_KEYS> key = {};
		IOState state[MAX_IO_KEYS] = {};
		float32 pressTime[MAX_IO_KEYS] = {};
		float32 prevPressTime[MAX_IO_KEYS] = {};
		float32 holdDuration[MAX_IO_KEYS] = {};
	} keyboard = {};

	std::array<float32, MAX_IO_CONFIGURATIONS> configuration = {};
	bool enableStateUpdate = true;

	void listen_to_event(OSEvent const& ev);
	void update_state();
};

CORE_API void update_configuration(IOConfiguration config, float32 val);
CORE_API IOConfigurationInfo get_configuration();
CORE_API bool key_pressed(IOKey key);
CORE_API bool key_double_tap(IOKey key);
CORE_API bool key_held(IOKey key);
CORE_API bool key_released(IOKey key);
CORE_API bool mouse_clicked(IOMouseButton button);
CORE_API bool mouse_double_clicked(IOMouseButton button);
CORE_API bool mouse_held(IOMouseButton button);
CORE_API bool mouse_released(IOMouseButton button);
CORE_API bool mouse_dragging(IOMouseButton button);
CORE_API Point mouse_drag_delta(IOMouseButton button);
CORE_API bool ctrl();
CORE_API bool alt();
CORE_API bool shift();

}

}

#endif // !CORE_INPUT_IO_H
