#pragma once
#ifndef PLATFORM_WINDOWING_WINDOWING_H
#define PLATFORM_WINDOWING_WINDOWING_H

#include <string_view>
#include <expected>
#include "lib/string.hpp"
#include "lib/paged_array.hpp"
#include "lib/map.hpp"
#include "lib/resource.hpp"
#include "lib/function.hpp"
#include "platform_minimal.hpp"
#include "io.hpp"

namespace core
{
namespace platform
{
enum class WindowState
{
	None = 0,
	Ok,
	Closing,
	Queued_For_Destroy,
};

class WindowContext;
class Window;

enum class WindowConfig : uint8
{
	None = 0,
	Allow_File_Drop = 0x01,
	Catch_Input		= 0x02,
	Borderless		= 0x04,
	Fullscreen		= 0x08
};

struct WindowCreateInfo
{
	std::wstring_view title;
	std::wstring_view iconPath;
	Point position;
	Dimension dimension;
	WindowConfig config;
};

struct WindowInfo
{
	lib::wstring title;
	void* nativeHandle;
	Point pos;
	Dimension dim;
	WindowState state;
	WindowConfig config;
	bool focused;
};

struct WindowEvent
{
	enum class Type : uint8
	{
		Focus,
		Resize,
		Move,
		Close
	};

	using value_type = std::underlying_type_t<Type>;

	static auto value_of(Type) -> value_type;

	union Value
	{
		struct
		{
			int32 x, y;
		} pos = {};
		struct
		{
			int32 width, height;
		} dim;
		bool focused;

		template <Type ev>
		auto value() -> auto
		{
			if constexpr (ev == Type::Focus)
			{
				return focused;
			}
			else if constexpr (ev == Type::Resize)
			{
				return dim;
			}
			else if constexpr (ev == Type::Move)
			{
				return pos;
			}
			else
			{
				return;
			}
		}
	};

	Window& window;
	Type ev;
	Value previous;
	Value current;
};

class Window : public lib::ref_counted
{
public:

	static auto from(WindowContext& ctx, WindowCreateInfo&& info) -> std::expected<Ref<Window>, std::string_view>;

	auto info() const -> WindowInfo const&;

	//auto set_title(std::wstring_view title) -> bool;
	auto set_position(Point p) -> bool;
	auto set_dimension(Dimension d) -> bool;
	//auto set_fullscreen() -> bool;
	auto maximize() -> bool;
	auto is_minimized() const -> bool;
	auto is_focused() const -> bool;
	auto push_listener(WindowEvent::Type ev, lib::function<void(WindowEvent)>&& fn) -> void;
	auto pop_listener(WindowEvent::Type ev) -> void;

	auto valid() const -> bool;
	auto process_handle() const -> void*;

private:
	friend class Ref<Window>;
	friend class WindowContext;
	friend struct EventQueueHandler;

	static auto destroy(Window& window, uint64 id) -> void;

	static constexpr std::string_view _windowEventNames[] = { "focus", "resize", "move", "close" };

	auto on(WindowEvent) -> void;

	using EventCallbackTable = std::array<lib::array<lib::function<void(WindowEvent const)>>, 4>;

	WindowContext* m_pContext			= {};
	EventCallbackTable m_callbackTable	= {};
	WindowInfo m_info					= {};
};

class WindowContext
{
public:
	auto destroy_windows() -> void;
	auto set_io_context(IOContext& ioContext) -> void;
private:
	friend class Window;
	friend struct EventQueueHandler;

	using index = lib::paged_array<Window, 4>::index;

	lib::paged_array<Window, 4> m_windows = {};
	lib::array<uint64>			m_zombies = {};
	IOContext*					m_pIoContext = {};
};
}
}

#endif // !PLATFORM_WINDOWING_WINDOWING_H
