#include "platform_header.h"
#include "engine.h"

namespace core
{
namespace cursor
{
auto CursorContext::initialize_default_cursor() -> void
{
	auto&& [index, cursor] = cursors.emplace();

	cursor.states[CursorState::ARROW]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_ARROW));
	cursor.states[CursorState::TEXT]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_IBEAM));
	cursor.states[CursorState::WAIT]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_WAIT));
	cursor.states[CursorState::CROSS]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_CROSS));
	cursor.states[CursorState::SIZE_NWSE]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZENWSE));
	cursor.states[CursorState::SIZE_NESW]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZENESW));
	cursor.states[CursorState::SIZE_WE]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZEWE));
	cursor.states[CursorState::SIZE_NS]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZENS));
	cursor.states[CursorState::MOVE]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZEALL));
	cursor.states[CursorState::POINTER]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_HAND));
	cursor.states[CursorState::PROGRESS]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_APPSTARTING));

	defaultCursor = cursor_handle{ index.to_uint64() };
}

auto CursorContext::get_default_cursor() -> Cursor&
{
	return *cursor_from_handle(defaultCursor);
}

auto CursorContext::cursor_from_handle(cursor_handle hnd) -> Cursor*
{
	return cursors.at(cursor_index::from(hnd.access(*this)));
}
}

auto Engine::show_cursor(bool show) -> void
{
	cursor::Cursor& defaultCursor = mCursorContext.get_default_cursor();

	if (show)
	{
		HCURSOR hcursor = reinterpret_cast<HCURSOR>(defaultCursor.states[cursor::CursorState::ARROW]);
		SetCursor(hcursor);
	}
	else
	{
		SetCursor(nullptr);
	}
}

auto Engine::set_cursor_position(wnd::window_handle hnd, Point point) -> void
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (pWindow)
	{
		HWND hwnd = static_cast<HWND>(pWindow->nativeHandle);
		POINT p{ point.x, point.y };
		ClientToScreen(hwnd, &p);
		SetCursorPos(p.x, p.y);
	}
}
}
