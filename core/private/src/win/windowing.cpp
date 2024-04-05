#include "platform_header.h"
#include "engine.h"

namespace core
{

namespace os
{
	extern LRESULT app_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
}

namespace wnd
{

void WindowingContext::on_tick()
{
	destroy_windows();
}

void WindowingContext::destroy_windows()
{
	if (zombies.size())
	{
		for (window_handle zombieHandle : zombies)
		{
			window_index idx{ window_index::from(zombieHandle.access(*this)) };
			Window* pWindow = windows.at(idx);
			if (!pWindow)
			{
				continue;
			}
			windowHandleTable.erase(reinterpret_cast<std::uintptr_t>((*pWindow).nativeHandle));
			DestroyWindow(static_cast<HWND>((*pWindow).nativeHandle));
			windows.erase(idx);
		}
		zombies.clear();
	}
}

}

wnd::window_handle Engine::create_window(wnd::WindowCreateInfo const& info)
{
	const wchar_t windowClassName[] = L"AngkasawanCoreWindowClass";

	static WNDCLASSW wc = [&]() -> WNDCLASSW
	{
		WNDCLASSW windowClass{};

		windowClass.style = 0;
		windowClass.lpfnWndProc = os::app_wnd_proc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = GetModuleHandle(NULL);
		windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		windowClass.hCursor = NULL;
		windowClass.hbrBackground = NULL;
		windowClass.lpszClassName = windowClassName;

		if (!RegisterClass(&windowClass))
		{
			ASSERTION(false && "Failed to register window class!");
		}

		return windowClass;
	}();

	HWND hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		windowClassName,
		info.title.data(),
		WS_OVERLAPPEDWINDOW,
		info.position.x,
		info.position.y,
		info.dimension.width,
		info.dimension.height,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);

	if (hwnd == NULL)
	{
		return wnd::window_handle::invalid_handle();
	}

	if (info.config.allowFileDrop)
	{
		// Accept drag and dropped files.
		DragAcceptFiles(hwnd, TRUE);
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	if (info.config.catchInput)
	{
		// Register raw input device to receive WM_INPUT events.
		RAWINPUTDEVICE device;
		device.usUsagePage = 0x01;
		device.usUsage = 0x02;
		device.dwFlags = RIDEV_INPUTSINK;
		device.hwndTarget = hwnd;

		RegisterRawInputDevices(&device, 1, sizeof(device));
	}

	std::pair res = mWindowContext.windows.emplace(
		wnd::Window::EventCallbackTable{},
		info.title.data(),
		hwnd,
		info.position,
		info.dimension,
		wnd::WindowState::Ok,
		info.config.fullscreen,
		info.config.allowFileDrop,
		true,
		info.config.catchInput
	);
	mWindowContext.windowHandleTable.insert(reinterpret_cast<std::uintptr_t>(hwnd), wnd::window_handle{ res.first.to_uint32() });
	++mWindowContext.count;

	return wnd::window_handle{ res.first.to_uint32() };
}

void* Engine::get_application_handle() const
{
	return reinterpret_cast<void*>(GetModuleHandle(NULL));
}

bool Engine::set_window_dimension(wnd::window_handle hnd, Dimension dim)
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return false;
	}
	pWindow->dim = dim;
	HWND hwnd = static_cast<HWND>(pWindow->nativeHandle);
	if (!SetWindowPos(hwnd, HWND_TOP, pWindow->pos.x, pWindow->pos.y, pWindow->dim.width, pWindow->dim.height, SWP_FRAMECHANGED))
	{
		return false;
	}
	return true;
}

bool Engine::set_window_position(wnd::window_handle hnd, Point pos)
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return false;
	}
	pWindow->pos = pos;
	HWND hwnd = static_cast<HWND>(pWindow->nativeHandle);
	if (!SetWindowPos(hwnd, HWND_TOP, pWindow->pos.x, pWindow->pos.y, pWindow->dim.width, pWindow->dim.height, SWP_FRAMECHANGED))
	{
		return false;
	}
	return true;
}

bool Engine::set_window_title(wnd::window_handle hnd, std::wstring_view title)
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return false;
	}
	pWindow->title = title;
	HWND hwnd = static_cast<HWND>(pWindow->nativeHandle);
	if (!SetWindowTextW(hwnd, pWindow->title.c_str()))
	{
		return false;
	}
	return true;
}

void Engine::set_window_as_fullscreen(wnd::window_handle hnd)
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (pWindow)
	{
		int32 w = GetSystemMetrics(SM_CXSCREEN);
		int32 h = GetSystemMetrics(SM_CYSCREEN);

		pWindow->pos = { 0, 0 };
		pWindow->dim = { w, h };

		HWND hwnd = static_cast<HWND>(pWindow->nativeHandle);
		SetWindowPos(hwnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);
	}
}

bool Engine::is_window_minimized(wnd::window_handle hnd) const
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return false;
	}
	BOOL const iconic = IsIconic(static_cast<HWND>(pWindow->nativeHandle));
	return iconic > 0;
}

auto Engine::is_window_focused(wnd::window_handle hnd) const -> bool
{
	wnd::Window* pWindow = mWindowContext.window_from_handle(hnd);
	if (!pWindow)
	{
		return false;
	}
	return pWindow->focused;
}

}