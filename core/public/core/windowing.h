#pragma once
#ifndef CORE_WINDOWING_WINDOWING_H
#define CORE_WINDOWING_WINDOWING_H

#include <string_view>
#include "lib/handle.h"
#include "lib/string.h"
#include "lib/paged_array.h"
#include "lib/map.h"
#include "core_minimal.h"

namespace core
{

namespace wnd
{

enum class WindowType
{
	None,
	Root,
	Child
};

enum class WindowState
{
	None = 0,
	Ok,
	Closing,
	Minimized,
	Queued_For_Destroy,
};

enum class WindowEvent
{
	Focus,
	Resize,
	Move,
	Close,
	Max
};

constexpr auto MAX_WINDOW_EVENT = static_cast<std::underlying_type_t<WindowEvent>>(WindowEvent::Max);
constexpr size_t MAX_EVENT_CALLBACK_COUNT = 8;

struct Window
{
	struct EventCallbackInfo
	{
		//using CallbackStaticArray = std::array<std::function<void()>, MAX_EVENT_CALLBACK_COUNT>;
		lib::wstring_128 name;
		std::function<void()> fn;
	};

	struct EventCallbackStore
	{
		using CallbackStaticArray = std::array<EventCallbackInfo, MAX_EVENT_CALLBACK_COUNT>;
		CallbackStaticArray callbacks;
		size_t callbackCount;
	};

	using EventCallbackTable = std::array<EventCallbackStore, MAX_WINDOW_EVENT>;

	EventCallbackTable callbackTable;
	lib::wstring title;
	void* nativeHandle;
	Point pos;
	Dimension dim;
	WindowState	state;
	bool fullscreen;
	bool allowFileDrop;
	bool focused;
	bool catchInputEvent;

	void invoke_callbacks_for_event(WindowEvent ev);
};

using window_handle = lib::opaque_handle<Window, uint32, std::numeric_limits<uint32>::max(), struct WindowingContext, class WindowInterface>;

struct WindowCreateInfo
{
	std::wstring_view title;
	std::wstring_view iconPath;
	Point position;
	Dimension dimension;
	struct
	{
		bool allowFileDrop	: 1;
		bool catchInput		: 1;
		bool borderless		: 1;
		bool fullscreen		: 1;
	} config;
};

struct WindowingContext
{
	using WindowLookUpTable = lib::map<std::uintptr_t, window_handle>;

	lib::paged_array<Window, 4> windows;
	lib::array<window_handle> zombies;
	WindowLookUpTable windowHandleTable;
	uint32 count;

	void on_tick();
	void destroy_windows();
	void listen_to_event(OSEvent const& ev);
	Window* window_from_handle(window_handle hnd) const;
	std::pair<window_handle, Window*> window_from_native_handle(void* native) const;
};

}

}

#endif // !CORE_WINDOWING_WINDOWING_H
