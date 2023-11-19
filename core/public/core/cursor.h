#pragma once
#ifndef CORE_CURSOR_H
#define CORE_CURSOR_H

#include <array>
#include <type_traits>
#include "lib/handle.h"
#include "lib/paged_array.h"

namespace core
{
namespace cursor
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

struct Cursor
{
	std::array<void*, CursorState::MAX> states;
};

using cursor_handle = lib::opaque_handle<Cursor, uint32, std::numeric_limits<uint32>::max(), struct CursorContext>;

struct CursorContext
{
	lib::paged_array<Cursor, 4> cursors;
	lib::array<cursor_handle> zombies;
	cursor_handle defaultCursor;

	uint32 count;

	auto initialize_default_cursor() -> void;
	auto get_default_cursor() -> Cursor&;
	auto cursor_from_handle(cursor_handle hnd) -> Cursor*;
};
}
}

#endif // !CORE_CURSOR_H
