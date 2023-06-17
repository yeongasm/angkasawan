#include "windowing.h"
#include "containers/bitset.h"

COREBEGIN

NativeWindow::NativeWindow() :
	m_title{},
	m_manager{},
	m_handle{},
	m_position{},
	m_dimension{},
	m_state{ WindowState::Invalid },
	m_fullscreen{},
	m_allowDropFiles{},
	m_catchInputEvents{},
	m_focused{}
{}

NativeWindow::NativeWindow(WindowingManager& manager) :
	m_title{},
	m_manager{ &manager },
	m_handle{},
	m_position{},
	m_dimension{},
	m_state{ WindowState::Invalid },
	m_fullscreen{},
	m_allowDropFiles{},
	m_catchInputEvents{},
	m_focused{}
{}

NativeWindow::~NativeWindow() {}

NativeWindow::NativeWindow(NativeWindow&& rhs) { *this = std::move(rhs); }

NativeWindow& NativeWindow::operator=(NativeWindow&& rhs)
{
	if (this != &rhs)
	{
		m_title			= std::move(rhs.m_title);
		m_manager		= rhs.m_manager;
		m_handle		= rhs.m_handle;
		m_position		= rhs.m_position;
		m_dimension		= rhs.m_dimension;
		m_state			= rhs.m_state;
		m_fullscreen	= rhs.m_fullscreen;
		m_allowDropFiles = rhs.m_allowDropFiles;
		m_catchInputEvents = rhs.m_catchInputEvents;
		m_focused		= rhs.m_focused;

		new (&rhs) NativeWindow{};
	}
	return *this;
}

ErrorCode NativeWindow::destroy()
{
	return m_manager->destroy_window(this);
}

os::Handle NativeWindow::get_application_handle() const
{
	return m_manager->m_platform.get_app_instance_handle();
}

os::WndHandle NativeWindow::get_raw_handle() const
{
	return m_handle;
}

bool NativeWindow::is_fullscreen() const
{
	return m_fullscreen;
}

bool NativeWindow::catches_input() const
{
	return m_catchInputEvents;
}

bool NativeWindow::allow_drop_files() const
{
	return m_allowDropFiles;
}

bool NativeWindow::is_focused() const
{
	return m_focused;
}

void NativeWindow::set_dimension(int32 width, int32 height)
{
	m_dimension.width = width;
	m_dimension.height = height;

	m_state = WindowState::Queued_For_Update;

	m_manager->push_for_update(*this, WindowingManager::WindowUpdateContext::Type::Resize);
}

void NativeWindow::set_position(int32 x, int32 y)
{
	m_position.x = x;
	m_position.y = y;

	m_state = WindowState::Queued_For_Update;

	m_manager->push_for_update(*this, WindowingManager::WindowUpdateContext::Type::Move);
}

void NativeWindow::set_title(wide_literal_t title)
{
	m_title = title;
	m_state = WindowState::Queued_For_Update;

	m_manager->push_for_update(*this, WindowingManager::WindowUpdateContext::Type::Renamed);
}

Point NativeWindow::get_position() const
{
	return m_position;
}

Dimension NativeWindow::get_dimension() const
{
	return m_dimension;
}

wide_literal_t NativeWindow::get_title() const
{
	return m_title.c_str();
}

WindowState NativeWindow::get_state() const
{
	return m_state;
}

bool NativeWindow::on(NativeWindowEvent ev, std::function<void()>&& callback)
{
	using index_type = std::underlying_type_t<NativeWindowEvent>;
	index_type index = static_cast<index_type>(ev);

	if (m_eventCallbacks[index])
	{
		return false;
	}

	m_eventCallbacks[index] = std::move(callback);

	return true;
}

void NativeWindow::invoke_callback(NativeWindowEvent ev)
{
	using index_type = std::underlying_type_t<NativeWindowEvent>;
	index_type index = static_cast<index_type>(ev);

	if (m_eventCallbacks[index])
	{
		m_eventCallbacks[index]();
	}
}

WindowingManager::WindowingManager(PlatformOSInterface& platform) :
	m_windows{},
	m_freeIndices{},
	m_updateStack{},
	m_platform{ platform },
	m_rootWindow{},
	m_numWindows{}
{
	m_platform.register_os_event_callback(
		this, 
		[](void* argument, const core::OSEvent& ev) -> void 
		{
			WindowingManager* manager = reinterpret_cast<WindowingManager*>(argument);
			manager->listen_to_event(ev);
		}
	);
}


WindowingManager::~WindowingManager() {}

WinResult<NativeWndHnd> WindowingManager::create_window(const NativeWindowCreateInfo& info, bool immediate)
{
	if (m_rootWindow != nullptr && ((info.flags & Native_Window_Flags_Root_Window) != 0))
	{
		return WinResult<NativeWndHnd>{ ErrorCode::Window_Root_Exist, ftl::invalid_handle_v<NativeWindow> };
	}

	/*if (!m_allowMultiWindow && m_windows.length() == 1)
	{
		return WinResult<NativeWndHnd>{ WindowState::Multiple_Windows_Not_Allowed, invalid_handle_v<NativeWindow> };
	}*/

	size_t index = -1ull;

	if (m_freeIndices.size())
	{
		index = m_freeIndices.back();
		m_freeIndices.pop();
	}

	NativeWindow window{ *this };

	window.m_position = info.position;
	window.m_dimension = info.dimension;
	window.m_title = info.title;
	window.m_state = WindowState::Queued_For_Create;
	window.m_allowDropFiles = (info.flags & Native_Window_Flags_Allow_Drop);
	window.m_catchInputEvents = (info.flags & Native_Window_Flags_Catch_Input);

	ErrorCode status = create_window(window, index, immediate);

	if ((info.flags & Native_Window_Flags_Root_Window) != 0)
	{
		m_rootWindow = &m_windows[index];
	}

	return WinResult<NativeWndHnd>{ status, NativeWndHnd{ index } };
}

ErrorCode WindowingManager::destroy_window(NativeWndHnd handle)
{
	if (!is_handle_valid(handle))
	{
		return ErrorCode::Window_Invalid_Handle;
	}

	NativeWindow& window = m_windows[handle.get()];

	if (window.get_state() == WindowState::Invalid)
	{
		return ErrorCode::Window_Invalid_Handle;
	}

	destroy_window(&window);

	return ErrorCode::Ok;
}

WinResult<NativeWindow*> WindowingManager::get_window(NativeWndHnd handle)
{
	if (!is_handle_valid(handle))
	{
		return WinResult<NativeWindow*>{ ErrorCode::Window_Invalid_Handle, nullptr };
	}

	NativeWindow* window = &m_windows[handle.get()];

	if (window->get_state() == WindowState::Invalid)
	{
		return WinResult<NativeWindow*>{ ErrorCode::Window_Invalid_Window, nullptr };
	}

	return WinResult<NativeWindow*>{ ErrorCode::Ok, window };
}

WinResult<NativeWindow*> WindowingManager::get_root_window() const
{
	return WinResult<NativeWindow*>{ ErrorCode::Ok, m_rootWindow };
}

void WindowingManager::on_tick()
{
	force_build_windows();
}

size_t WindowingManager::get_num_windows() const
{
	return m_numWindows;
}


void WindowingManager::listen_to_event(const OSEvent& ev)
{
	using index_type = std::underlying_type_t<NativeWindowEvent>;
	NativeWindow* window = get_native_window_from_raw_handle(ev.windowHandle);

	if (window != nullptr)
	{
		ASSERTION(window != nullptr && "OS is returning an event from another window?");
		switch (ev.event)
		{
		case OSEvent::Type::Quit:
		case OSEvent::Type::Window_Close:
			window->invoke_callback(NativeWindowEvent::Close);
			handle_close_on_event(*window);
			break;
		case OSEvent::Type::Window_Resize:
			window->invoke_callback(NativeWindowEvent::Resize);
			update_window_properties(*window, ev);
			break;
		case OSEvent::Type::Window_Move:
			window->invoke_callback(NativeWindowEvent::Move);
			update_window_properties(*window, ev);
			break;
		case OSEvent::Type::Focus:
			window->invoke_callback(NativeWindowEvent::Focus);
			window->m_focused = ev.focus.gained;
		default:
			break;
		}
	}
}

bool WindowingManager::are_window_dimensions_valid(Dimension const& dimension)
{
	return (dimension.width != -1) && (dimension.height != -1);
}

ErrorCode WindowingManager::set_window_as_fullscreen(NativeWndHnd handle)
{
	WinResult<NativeWindow*> result = get_window(handle);

	if (result.status != ErrorCode::Ok)
	{
		return result.status;
	}

	push_for_update(*result.payload, WindowUpdateContext::Type::Fullscreen);

	return ErrorCode::Ok;
}

NativeWindow* WindowingManager::get_window_from_raw_handle(os::WndHandle hnd)
{
	return get_native_window_from_raw_handle(hnd);
}

bool WindowingManager::is_handle_valid(NativeWndHnd handle)
{
	return handle != ftl::invalid_handle_v<NativeWindow>;
}

ErrorCode WindowingManager::create_window(NativeWindow& window, size_t& at, bool immediate)
{
	if (at != -1)
	{
		m_windows.emplace_at(at, std::move(window));
	}
	else
	{
		at = m_windows.emplace(std::move(window));
	}

	NativeWindow* wnd = &m_windows[at];

	if (!immediate)
	{
		WindowUpdateContext ctx{ WindowUpdateContext::Type::Create, wnd };
		m_updateStack.emplace(std::move(ctx));
	}
	else
	{
		create_os_window(*wnd);
	}

	++m_numWindows;

	return ErrorCode::Ok;
}

ErrorCode WindowingManager::destroy_window(NativeWindow* window)
{
	window->m_state = WindowState::Queued_For_Destroy;

	push_for_update(*window, WindowUpdateContext::Type::Destroy);
	--m_numWindows;

	return ErrorCode::Ok;
}

void WindowingManager::push_for_update(NativeWindow& window, WindowUpdateContext::Type type)
{
	WindowUpdateContext ctx{ type, &window };
	m_updateStack.emplace(std::move(ctx));
}

bool WindowingManager::create_os_window(NativeWindow& window)
{
	PlatformWindowCreateInfo platformWindowInfo{};

	platformWindowInfo.isChildWindow = false;
	platformWindowInfo.title = window.get_title();
	platformWindowInfo.dimension.width = window.get_dimension().width;
	platformWindowInfo.dimension.height = window.get_dimension().height;
	platformWindowInfo.position.x = window.get_position().x;
	platformWindowInfo.position.y = window.get_position().y;
	platformWindowInfo.allowFileDrop = window.m_allowDropFiles;
	platformWindowInfo.catchInput = window.m_catchInputEvents;

	window.m_handle = m_platform.create_window(platformWindowInfo);

	if (!window.m_handle)
	{
		window.m_state = WindowState::Invalid;
		return false;
	}

	window.m_state = WindowState::Ok;

	return true;
}

void WindowingManager::destroy_os_window(NativeWindow& window)
{
	size_t index = m_windows.index_of(window);

	m_platform.destroy_window(window.get_raw_handle());

	m_windows.pop_at(index, false);
	m_freeIndices.push(index);
}

void WindowingManager::maximize_window(NativeWindow& window)
{
	const Dimension monitorWorkingSpace = m_platform.get_monitor_dimension(window.get_raw_handle());

	window.m_dimension.width = monitorWorkingSpace.width;
	window.m_dimension.height = monitorWorkingSpace.height;
	window.m_position.x = 0;
	window.m_position.y = 0;

	window.m_fullscreen = true;

	m_platform.maximize_window(window.get_raw_handle());
}

void WindowingManager::update_window_position(NativeWindow& window)
{
	const Point pos = window.get_position();
	const Dimension dim = window.get_dimension();

	m_platform.set_window_position(window.get_raw_handle(), pos.x, pos.y, dim.width, dim.height);
}

void WindowingManager::update_window_properties(NativeWindow& window, const OSEvent& ev)
{
	if (ev.event == OSEvent::Type::Window_Resize)
	{
		window.m_dimension.width = ev.winSize.width;
		window.m_dimension.height = ev.winSize.height;
	}
	else
	{
		window.m_position.x = ev.winMove.x;
		window.m_position.y = ev.winMove.y;
	}
}

void WindowingManager::handle_close_on_event(NativeWindow& window)
{
	if (m_rootWindow == &window)
	{
		for (NativeWindow& wnd : m_windows)
		{
			if (wnd.get_state() == WindowState::Invalid)
			{
				continue;
			}

			if (&wnd == &window)
			{
				continue;
			}

			destroy_window(&wnd);
		}
		m_rootWindow = nullptr;
	}
	destroy_window(&window);
}

void WindowingManager::force_build_windows()
{
	for (WindowUpdateContext& context : m_updateStack)
	{
		switch (context.type)
		{
		case WindowUpdateContext::Type::Create:
			create_os_window(*context.window);
			break;
		case WindowUpdateContext::Type::Destroy:
			destroy_os_window(*context.window);
			break;
		case WindowUpdateContext::Type::Move:
		case WindowUpdateContext::Type::Resize:
			update_window_position(*context.window);
			break;
		case WindowUpdateContext::Type::Fullscreen:
			maximize_window(*context.window);
			break;
		case WindowUpdateContext::Type::Renamed:
		default:
			m_platform.set_window_title(context.window->get_raw_handle(), context.window->get_title());
			break;
		}
	}
	m_updateStack.empty();
}

NativeWindow* WindowingManager::get_native_window_from_raw_handle(os::WndHandle handle)
{
	NativeWindow* result = nullptr;
	for (NativeWindow& window : m_windows)
	{
		if (window.get_raw_handle() == handle)
		{
			result = &window;
			break;
		}
	}
	return result;
}

COREEND