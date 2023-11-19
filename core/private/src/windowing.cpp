#include "engine.h"

namespace core
{
namespace wnd
{

void Window::invoke_callbacks_for_event(WindowEvent ev)
{
	if (ev != WindowEvent::Max)
	{
		auto eventIndex = static_cast<std::underlying_type_t<WindowEvent>>(ev);
		Window::EventCallbackStore const& callbackStore = callbackTable[eventIndex];
		for (size_t i = 0; i < callbackStore.callbackCount; ++i)
		{
			Window::EventCallbackInfo const& info = callbackStore.callbacks[i];
			info.fn();
		}
	}
}

void WindowingContext::listen_to_event(OSEvent const& ev)
{
	auto [handle, pWindow] = window_from_native_handle(ev.window);

	if (pWindow)
	{
		Window& window = *pWindow;
		switch (ev.event)
		{
		case OSEvent::Type::Quit:
		case OSEvent::Type::Window_Close:
			window.invoke_callbacks_for_event(WindowEvent::Close);
			window.state = WindowState::Closing;
			--count;
			zombies.push_back(handle);
			break;
		case OSEvent::Type::Window_Resize:
			window.invoke_callbacks_for_event(WindowEvent::Resize);
			window.dim = { ev.winSize.width, ev.winSize.height };
			break;
		case OSEvent::Type::Window_Move:
			window.invoke_callbacks_for_event(WindowEvent::Move);
			window.pos = { ev.winMove.x, ev.winMove.y };
			break;
		case OSEvent::Type::Focus:
			window.invoke_callbacks_for_event(WindowEvent::Focus);
			window.focused = ev.focus.gained;
		default:
			break;
		}
	}
}

Window* WindowingContext::window_from_handle(window_handle hnd) const
{
	return windows.at(hnd.access(*this));
}

std::pair<window_handle, Window*> WindowingContext::window_from_native_handle(void* native) const
{
	std::uintptr_t address = reinterpret_cast<std::uintptr_t>(native);
	lib::ref ref = windowHandleTable.at(address);
	if (!ref.is_null())
	{
		window_handle handle = ref->second;
		decltype(windows)::index idx{ handle.access(*this) };
		return std::pair{ handle , &windows[idx] };
	}
	return std::pair{ window_handle::invalid_handle(), nullptr };
}

}

bool Engine::destroy_window(wnd::window_handle& hnd)
{
	bool destroyed = false;
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (pWindow &&
		pWindow->state == wnd::WindowState::Ok)
	{
		pWindow->state = wnd::WindowState::Queued_For_Destroy;
		--mWindowContext.count;
		mWindowContext.zombies.push(hnd);
		destroyed = true;;
	}
	hnd.invalidate();
	return destroyed;
}

void* Engine::get_window_native_handle(wnd::window_handle hnd) const
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return nullptr;
	}
	return pWindow->nativeHandle;
}

Point Engine::get_window_position(wnd::window_handle hnd) const
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return Point{};
	}
	return pWindow->pos;
}

Dimension Engine::get_window_dimension(wnd::window_handle hnd) const
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return Dimension{};
	}
	return pWindow->dim;
}

std::wstring_view Engine::get_window_title(wnd::window_handle hnd) const
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return std::wstring_view{};
	}
	return std::wstring_view{ pWindow->title.data(), pWindow->title.size() };
}

wnd::WindowState Engine::get_window_state(wnd::window_handle hnd) const
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return wnd::WindowState::None;
	}
	return pWindow->state;
}

void Engine::register_window_listener(wnd::window_handle hnd, wnd::WindowEvent ev, std::function<void()>&& callback, lib::wstring_128 identifier)
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (pWindow)
	{
		auto eventIndex = static_cast<std::underlying_type_t<wnd::WindowEvent>>(ev);
		wnd::Window::EventCallbackStore& callbackStore = pWindow->callbackTable[eventIndex];

		ASSERTION(callbackStore.callbackCount < wnd::MAX_EVENT_CALLBACK_COUNT && "Maximum number of registered callbacks allowed reached.");

		if (callbackStore.callbackCount < wnd::MAX_EVENT_CALLBACK_COUNT)
		{
			wnd::Window::EventCallbackInfo& callbackInfo = callbackStore.callbacks[callbackStore.callbackCount];

			callbackInfo.fn = std::move(callback);
			callbackInfo.name = std::move(identifier);

			++callbackStore.callbackCount;
		}
	}
}

uint32 Engine::num_windows() const
{
	return mWindowContext.count;
}

}