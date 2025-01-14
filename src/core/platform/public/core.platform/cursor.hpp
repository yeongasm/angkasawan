#pragma once
#ifndef PLATFORM_CURSOR_H
#define PLATFORM_CURSOR_H

#include <array>
#include <type_traits>
#include "lib/paged_array.hpp"
#include "platform_minimal.hpp"
#include "windowing.hpp"

namespace core
{
namespace platform
{
struct CursorState
{
	inline static constexpr auto ARROW		= 0;
	inline static constexpr auto TEXT		= 1;
	inline static constexpr auto WAIT		= 2;
	inline static constexpr auto CROSS		= 3;
	inline static constexpr auto SIZE_NWSE	= 4;
	inline static constexpr auto SIZE_NESW	= 5;
	inline static constexpr auto SIZE_WE	= 6;
	inline static constexpr auto SIZE_NS	= 7;
	inline static constexpr auto MOVE		= 8;
	inline static constexpr auto POINTER	= 9;
	inline static constexpr auto PROGRESS	= 10;
	inline static constexpr auto MAX		= 11;
};

struct CursorContext
{
	std::array<void*, CursorState::MAX> states;

	CursorContext();

	auto show_cursor(bool show = true) -> void;
	auto set_cursor_position(Ref<Window> const& window, Point point) const -> void;
};
}
}

#endif // !PLATFORM_CURSOR_H
