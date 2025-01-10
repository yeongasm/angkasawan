#pragma once
#ifndef PLATFORM_INPUT_IO_H
#define PLATFORM_INPUT_IO_H

#include <array>
#include "lib/bitset.hpp"
#include "lib/set_once.hpp"
#include "platform_minimal.hpp"

namespace core
{
namespace platform
{

/**
* NOTE(afiq):
* 
* This method of handling IO is pretty much outdated.
* 
* What we should do instead is to store the state of each key / mouse input as it's own struct.
* We then introduce a get_[key | mouse]_state() function that returns the said key / mouse input's state.
* This way, users can have more control over the handling logic for each input.
*/

constexpr auto MAX_IO_MOUSE_BUTTONS		= std::to_underlying(IOMouseButton::Max);
constexpr auto MAX_IO_STATES			= std::to_underlying(IOState::Max);
constexpr auto MAX_IO_KEYS				= std::to_underlying(IOKey::Max);
constexpr auto MAX_IO_CURSOR_TYPES		= std::to_underlying(IOConfiguration::Max);
constexpr auto MAX_IO_CONFIGURATIONS	= std::to_underlying(IOConfiguration::Max);
/**
* @brief All values are in seconds.
*/
struct IOConfigurationInfo
{
	lib::set_once<float32> keyDoubleTapTime;
	lib::set_once<float32> keyMinDurationForHold;
	lib::set_once<float32> mouseDoubleClickTime;
	lib::set_once<float32> mouseDoubleClickDistance;
	lib::set_once<float32> mouseMinDurationForHold;
};

class IOContext
{
private:
	friend struct EventQueueHandler;

	struct Keyboard
	{
		lib::bitset<IOKey, MAX_IO_KEYS> key = {};
		IOState state[MAX_IO_KEYS] = {};
		float32 pressTime[MAX_IO_KEYS] = {};
		float32 prevPressTime[MAX_IO_KEYS] = {};
		float32 holdDuration[MAX_IO_KEYS] = {};
	} m_keyboard = {};

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
	} m_mouse = {};

	struct Configuration
	{
		float32 keyDoubleTapTime;
		float32 keyMinDurationForHold;
		float32 mouseDoubleClickTime;
		float32 mouseDoubleClickDistance;
		float32 mouseMinDurationForHold;
	} m_config = {};

	bool m_enableIOStateUpdate = true;

public:

	auto update() -> void;
	auto reset_mouse_wheel_state() -> void;
	auto update_configuration(IOConfigurationInfo const& info) -> void;
	auto configuration() const -> Configuration;
	auto key_pressed(IOKey key) -> bool;
	auto key_double_tap(IOKey key) -> bool;
	auto key_held(IOKey key) -> bool;
	auto key_released(IOKey key) -> bool;
	auto mouse_clicked(IOMouseButton button) -> bool;
	auto mouse_double_clicked(IOMouseButton button) -> bool;
	auto mouse_held(IOMouseButton button) -> bool;
	auto mouse_released(IOMouseButton button) -> bool;
	auto mouse_dragging(IOMouseButton button) -> bool;
	auto mouse_drag_delta(IOMouseButton button) -> Point;
	auto mouse_position() -> Point;
	auto mouse_click_position(IOMouseButton button) -> Point;
	auto mouse_wheel_v() -> float32;
	auto mouse_wheel_h() -> float32;
	auto ctrl() -> bool;
	auto alt() -> bool;
	auto shift() -> bool;
	auto enable_io_state_update(bool v = true) -> void;
};
}
}

#endif // !PLATFORM_INPUT_IO_H
