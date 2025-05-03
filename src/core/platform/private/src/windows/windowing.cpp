#include "platform_header.hpp"
#include "lib/bit_mask.hpp"
#include "windowing.hpp"

namespace core
{
namespace platform
{
extern auto app_wnd_proc(HWND, UINT, WPARAM, LPARAM) -> LRESULT;

auto WindowEvent::value_of(Type ev) -> value_type
{
	return static_cast<value_type>(ev);
}

auto Window::from(WindowContext& ctx, WindowCreateInfo&& info) -> std::expected<Ref<Window>, std::string_view>
{
	static wchar_t const windowClassName[] = L"AngkasawanCoreWindowClass";

	static WNDCLASSW wc = [&]() -> WNDCLASSW
	{
		WNDCLASSW windowClass{};

		windowClass.style = 0;
		windowClass.lpfnWndProc = app_wnd_proc;
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
		return std::unexpected{ "Failed to create window" };
	}

	if ((info.config & WindowConfig::Allow_File_Drop) != WindowConfig::None)
	{
		// Accept drag and dropped files.
		DragAcceptFiles(hwnd, TRUE);
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	if ((info.config & WindowConfig::Catch_Input) != WindowConfig::None)
	{
		// Register raw input device to receive WM_INPUT events.
		RAWINPUTDEVICE device;
		device.usUsagePage = 0x01;
		device.usUsage = 0x02;
		device.dwFlags = RIDEV_INPUTSINK;
		device.hwndTarget = hwnd;

		RegisterRawInputDevices(&device, 1, sizeof(device));
	}

	// auto&& [id, window] = ctx.m_windows.emplace();
	auto it = ctx.m_windows.emplace();
	
	auto&& window = *it;

	window.m_pContext = &ctx;
	window.m_info.title = lib::wstring{ info.title.data(), info.title.size() };
	window.m_info.nativeHandle = hwnd;
	window.m_info.pos = info.position;
	window.m_info.dim = info.dimension;
	window.m_info.state = WindowState::Ok;
	window.m_info.config = info.config;
	window.m_info.focused = true;

	window.reference();

	auto const id = reinterpret_cast<uintptr_t>(&window);

	PostMessage(hwnd, WM_USER + 0, std::bit_cast<WPARAM>(id), std::bit_cast<LPARAM>(&window));

	return Ref<Window>{ id, window };
}

auto Window::info() const -> WindowInfo const&
{
	return m_info;
}

//auto Window::set_title(std::wstring_view title) -> bool
//{
//	if (m_info.nativeHandle)
//	{
//		return PostMessage(std::bit_cast<HWND>(m_info.nativeHandle), WM_SETTEXT, 0, std::bit_cast<LPARAM>(title.data()));
//	}
//	return false;
//	//m_info.title = title;
//	//HWND hwnd = static_cast<HWND>(m_info.nativeHandle);
//	//if (!SetWindowTextW(hwnd, m_info.title.c_str()))
//	//{
//	//	return false;
//	//}
//	//return true;
//}

auto Window::set_position(Point p) -> bool
{
	if (m_info.nativeHandle)
	{
		return PostMessage(std::bit_cast<HWND>(m_info.nativeHandle), WM_USER + 2, 0, std::bit_cast<LPARAM>(p));
	}
	return false;
}

auto Window::set_dimension(Dimension d) -> bool
{
	if (m_info.nativeHandle)
	{
		return PostMessage(std::bit_cast<HWND>(m_info.nativeHandle), WM_USER + 1, SIZE_RESTORED, std::bit_cast<LPARAM>(d));
	}
	return false;
}

//auto Window::set_fullscreen() -> bool
//{
//	int32 w = GetSystemMetrics(SM_CXFULLSCREEN);
//	int32 h = GetSystemMetrics(SM_CYFULLSCREEN);
//
//	Dimension d{ .width = w, .height = h };
//
//	if (m_info.nativeHandle)
//	{
//		return PostMessage(std::bit_cast<HWND>(m_info.nativeHandle), WM_USER + 1, SIZE_RESTORED, std::bit_cast<LPARAM>(d));
//	}
//	return false;
//}

auto Window::maximize() -> bool
{
	int32 w = GetSystemMetrics(SM_CXSCREEN);
	int32 h = GetSystemMetrics(SM_CYSCREEN);

	Dimension d{ .width = w, .height = h };

	if (m_info.nativeHandle)
	{
		return PostMessage(std::bit_cast<HWND>(m_info.nativeHandle), WM_USER + 1, SIZE_MAXIMIZED, std::bit_cast<LPARAM>(d));
	}
	return false;
}

auto Window::is_minimized() const -> bool
{
	BOOL const iconic = IsIconic(static_cast<HWND>(m_info.nativeHandle));
	return iconic != 0;
}

auto Window::is_focused() const -> bool
{
	return m_info.focused;
}

auto Window::push_listener(WindowEvent::Type ev, lib::function<void(WindowEvent)>&& fn) -> void
{
	m_callbackTable[WindowEvent::value_of(ev)].push_back(std::move(fn));
}

auto Window::pop_listener(WindowEvent::Type ev) -> void
{
	m_callbackTable[WindowEvent::value_of(ev)].pop_back();
}

auto Window::valid() const -> bool
{
	return m_info.nativeHandle != nullptr;
}

auto Window::process_handle() const -> void*
{
	return reinterpret_cast<void*>(GetModuleHandle(nullptr));
}

auto Window::on(WindowEvent windowEvent) -> void
{
	for (auto&& fn : m_callbackTable[WindowEvent::value_of(windowEvent.ev)])
	{
		fn(windowEvent);
	}
}

auto Window::destroy(Window& window, uint64 id) -> void
{
	window.m_info.state = WindowState::Queued_For_Destroy;
	window.m_pContext->m_zombies.push_back(id);
}

auto WindowContext::destroy_windows() -> void
{
	if (m_zombies.size())
	{
		for (uint64 zombie : m_zombies)
		{
			auto const it = m_windows.get_iterator(reinterpret_cast<Window*>(zombie));
			if (it < m_windows.begin() || it >= m_windows.end())
			{
				continue;
			}
			DestroyWindow(static_cast<HWND>(it->info().nativeHandle));
			m_windows.erase(it);
		}
		m_zombies.clear();
	}
}

auto WindowContext::set_io_context(IOContext& ioContext) -> void
{
	m_pIoContext = &ioContext;
}
}
}