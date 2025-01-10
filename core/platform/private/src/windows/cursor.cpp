#include "platform_header.hpp"
#include "cursor.hpp"

namespace core
{
namespace platform
{
CursorContext::CursorContext() :
	states{}
{
	states[CursorState::ARROW]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_ARROW));
	states[CursorState::TEXT]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_IBEAM));
	states[CursorState::WAIT]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_WAIT));
	states[CursorState::CROSS]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_CROSS));
	states[CursorState::SIZE_NWSE]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZENWSE));
	states[CursorState::SIZE_NESW]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZENESW));
	states[CursorState::SIZE_WE]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZEWE));
	states[CursorState::SIZE_NS]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZENS));
	states[CursorState::MOVE]		= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_SIZEALL));
	states[CursorState::POINTER]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_HAND));
	states[CursorState::PROGRESS]	= reinterpret_cast<void*>(LoadCursorW(nullptr, IDC_APPSTARTING));
}

auto CursorContext::show_cursor(bool show) -> void
{
	if (show)
	{
		HCURSOR hcursor = reinterpret_cast<HCURSOR>(states[CursorState::ARROW]);
		SetCursor(hcursor);
	}
	else
	{
		SetCursor(nullptr);
	}
}

auto CursorContext::set_cursor_position(Ref<Window> const& window, Point point) const -> void
{
	if (window.valid())
	{
		HWND hwnd = static_cast<HWND>(window->info().nativeHandle);
		POINT p{ point.x, point.y };
		ClientToScreen(hwnd, &p);
		SetCursorPos(p.x, p.y);
	}
}
}
}
