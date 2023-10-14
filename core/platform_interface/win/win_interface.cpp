#include "win_interface.h"
#include "os_platform.h"
#include "memory/memory.h"
#include <queue>

namespace os
{

struct WinOSInterface::WinOSContext
{
	struct EventListener
	{
		using Callback_t = std::function<void(void*, const core::OSEvent&)>;

		void*		argument;
		Callback_t	callback;
	};
	using EventListeners = ftl::Array<EventListener>;

	core::Point		m_lastMousePos;
	EventListeners	m_listeners;

	POINT get_last_mouse_pos() const
	{
		return POINT{ m_lastMousePos.x, m_lastMousePos.y };
	}

	void set_last_mouse_pos(const POINT& p)
	{
		m_lastMousePos.x = p.x;
		m_lastMousePos.y = p.y;
	}

	void iterate_event_listeners(const core::OSEvent& ev)
	{
		std::for_each(
			m_listeners.begin(), 
			m_listeners.end(), 
			[&ev](const EventListener& listener) -> void 
			{
				listener.callback(listener.argument, ev);
			}
		);
	}

} osContext{};

core::IOKey translate_key_code(size_t code)
{
	core::IOKey key{};
	switch (code)
	{
	case 0x41:
		key = core::IOKey::Io_Key_A;
		break;
	case 0x42:
		key = core::IOKey::Io_Key_B;
		break;
	case 0x43:
		key = core::IOKey::Io_Key_C;
		break;
	case 0x44:
		key = core::IOKey::Io_Key_D;
		break;
	case 0x45:
		key = core::IOKey::Io_Key_E;
		break;
	case 0x46:
		key = core::IOKey::Io_Key_F;
		break;
	case 0x47:
		key = core::IOKey::Io_Key_G;
		break;
	case 0x48:
		key = core::IOKey::Io_Key_H;
		break;
	case 0x49:
		key = core::IOKey::Io_Key_I;
		break;
	case 0x4A:
		key = core::IOKey::Io_Key_J;
		break;
	case 0x4B:
		key = core::IOKey::Io_Key_K;
		break;
	case 0x4C:
		key = core::IOKey::Io_Key_L;
		break;
	case 0x4D:
		key = core::IOKey::Io_Key_M;
		break;
	case 0x4E:
		key = core::IOKey::Io_Key_N;
		break;
	case 0x4F:
		key = core::IOKey::Io_Key_O;
		break;
	case 0x50:
		key = core::IOKey::Io_Key_P;
		break;
	case 0x51:
		key = core::IOKey::Io_Key_Q;
		break;
	case 0x52:
		key = core::IOKey::Io_Key_R;
		break;
	case 0x53:
		key = core::IOKey::Io_Key_S;
		break;
	case 0x54:
		key = core::IOKey::Io_Key_T;
		break;
	case 0x55:
		key = core::IOKey::Io_Key_U;
		break;
	case 0x56:
		key = core::IOKey::Io_Key_V;
		break;
	case 0x57:
		key = core::IOKey::Io_Key_W;
		break;
	case 0x58:
		key = core::IOKey::Io_Key_X;
		break;
	case 0x59:
		key = core::IOKey::Io_Key_Y;
		break;
	case 0x5A:
		key = core::IOKey::Io_Key_Z;
		break;
	case 0x30:
		key = core::IOKey::Io_Key_0;
		break;
	case 0x31:
		key = core::IOKey::Io_Key_1;
		break;
	case 0x32:
		key = core::IOKey::Io_Key_2;
		break;
	case 0x33:
		key = core::IOKey::Io_Key_3;
		break;
	case 0x34:
		key = core::IOKey::Io_Key_4;
		break;
	case 0x35:
		key = core::IOKey::Io_Key_5;
		break;
	case 0x36:
		key = core::IOKey::Io_Key_6;
		break;
	case 0x37:
		key = core::IOKey::Io_Key_7;
		break;
	case 0x38:
		key = core::IOKey::Io_Key_8;
		break;
	case 0x39:
		key = core::IOKey::Io_Key_9;
		break;
	case VK_F1:
		key = core::IOKey::Io_Key_F1;
		break;
	case VK_F2:
		key = core::IOKey::Io_Key_F2;
		break;
	case VK_F3:
		key = core::IOKey::Io_Key_F3;
		break;
	case VK_F4:
		key = core::IOKey::Io_Key_F4;
		break;
	case VK_F5:
		key = core::IOKey::Io_Key_F5;
		break;
	case VK_F6:
		key = core::IOKey::Io_Key_F6;
		break;
	case VK_F7:
		key = core::IOKey::Io_Key_F7;
		break;
	case VK_F8:
		key = core::IOKey::Io_Key_F8;
		break;
	case VK_F9:
		key = core::IOKey::Io_Key_F9;
		break;
	case VK_F10:
		key = core::IOKey::Io_Key_F10;
		break;
	case VK_F11:
		key = core::IOKey::Io_Key_F11;
		break;
	case VK_F12:
		key = core::IOKey::Io_Key_F12;
		break;
	case VK_CAPITAL:
		key = core::IOKey::Io_Key_Caps_Lock;
		break;
	case VK_TAB:
		key = core::IOKey::Io_Key_Tab;
		break;
	case VK_SHIFT:
		key = core::IOKey::Io_Key_Shift;
		break;
	case VK_CONTROL:
		key = core::IOKey::Io_Key_Ctrl;
		break;
	case VK_MENU:
		key = core::IOKey::Io_Key_Alt;
		break;
	case VK_RWIN:
		key = core::IOKey::Io_Key_Right_Window;
		break;
	case VK_LWIN:
		key = core::IOKey::Io_Key_Left_Window;
		break;
	case VK_BACK:
		key = core::IOKey::Io_Key_Backspace;
		break;
	case VK_RETURN:
		key = core::IOKey::Io_Key_Enter;
		break;
	case VK_OEM_5:
		key = core::IOKey::Io_Key_Pipe;
		break;
	case VK_ESCAPE:
		key = core::IOKey::Io_Key_Escape;
		break;
	case VK_OEM_3:
		key = core::IOKey::Io_Key_Tilde;
		break;
	case VK_OEM_PLUS:
		key = core::IOKey::Io_Key_Plus;
		break;
	case VK_OEM_MINUS:
		key = core::IOKey::Io_Key_Minus;
		break;
	case VK_OEM_4:
		key = core::IOKey::Io_Key_Open_Brace;
		break;
	case VK_OEM_6:
		key = core::IOKey::Io_Key_Close_Brace;
		break;
	case VK_OEM_1:
		key = core::IOKey::Io_Key_Colon;
		break;
	case VK_OEM_7:
		key = core::IOKey::Io_Key_Quote;
		break;
	case VK_OEM_COMMA:
		key = core::IOKey::Io_Key_Comma;
		break;
	case VK_OEM_PERIOD:
		key = core::IOKey::Io_Key_Period;
		break;
	case VK_OEM_2:
		key = core::IOKey::Io_Key_Slash;
		break;
	case VK_UP:
		key = core::IOKey::Io_Key_Up;
		break;
	case VK_DOWN:
		key = core::IOKey::Io_Key_Down;
		break;
	case VK_LEFT:
		key = core::IOKey::Io_Key_Left;
		break;
	case VK_RIGHT:
		key = core::IOKey::Io_Key_Right;
		break;
	case VK_SNAPSHOT:
		key = core::IOKey::Io_Key_Print_Screen;
		break;
	case VK_SCROLL:
		key = core::IOKey::Io_Key_Scroll_Lock;
		break;
	case VK_PAUSE:
		key = core::IOKey::Io_Key_Pause;
		break;
	case VK_INSERT:
		key = core::IOKey::Io_Key_Insert;
		break;
	case VK_HOME:
		key = core::IOKey::Io_Key_Home;
		break;
	case VK_PRIOR:
		key = core::IOKey::Io_Key_Page_Up;
		break;
	case VK_DELETE:
		key = core::IOKey::Io_Key_Delete;
		break;
	case VK_END:
		key = core::IOKey::Io_Key_End;
		break;
	case VK_NEXT:
		key = core::IOKey::Io_Key_Page_Down;
		break;
	case VK_SPACE:
		key = core::IOKey::Io_Key_Space;
		break;
	default:
		key = core::IOKey::Io_Key_Max;
		break;
	}
	return key;
}

LRESULT app_window_process(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	core::OSEvent e{};
	e.windowHandle = hWnd;
	switch (msg)
	{
	case WM_MOVE:
		e.event = core::OSEvent::Type::Window_Move; 
		e.winMove.x = static_cast<int32>(LOWORD(lParam));
		e.winMove.y = static_cast<int32>(HIWORD(lParam));
		break;
	case WM_SIZE:
		e.event = core::OSEvent::Type::Window_Resize;
		e.winSize.width = static_cast<int32>(LOWORD(lParam));
		e.winSize.height = static_cast<int32>(HIWORD(lParam));
		break;
	case WM_CLOSE:
		e.event = core::OSEvent::Type::Window_Close;
		break;
	case WM_ACTIVATE:
		if (wParam == WA_INACTIVE) {
			while (ShowCursor(true) < 0);
			ClipCursor(nullptr);
		}
		e.event = core::OSEvent::Type::Focus;
		e.focus.gained = wParam != WA_INACTIVE;
		break;
	case WM_QUIT:
		e.event = core::OSEvent::Type::Quit;
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		e.event = core::OSEvent::Type::Key;
		e.key.down = true;
		e.key.code = translate_key_code(static_cast<size_t>(wParam));
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		e.event = core::OSEvent::Type::Key;
		e.key.down = false;
		e.key.code = translate_key_code(static_cast<size_t>(wParam));
		break;
	/*case WM_CHAR:
		e.event = core::OSEvent::Type::Char_Input;
		e.textInput.utf8 = 0;
		UTF32ToUTF8((uint32)msg.wParam, (int8*)&e.TextInput.Utf8);
		break;*/
	case WM_INPUT:
		e = handle_mouse_input(hWnd, lParam);
		break;
	}

	if (e.event != core::OSEvent::Type::None)
	{
		osContext.iterate_event_listeners(e);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


WinOSInterface::WinOSInterface() :
	m_ctx{ &osContext }
{}

WinOSInterface::~WinOSInterface() {}

void WinOSInterface::sleep(size_t milliseconds) const
{
	Sleep(static_cast<DWORD>(milliseconds));
}

size_t WinOSInterface::get_current_thread_id() const
{
	return static_cast<size_t>(GetCurrentThreadId());
}

bool WinOSInterface::copy_to_clipboard(WndHandle handle, literal_t text, size_t len) const
{
	if (!OpenClipboard(reinterpret_cast<HWND>(handle)))
	{
		return false;
	}

	HGLOBAL memHandle = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(char));

	if (!memHandle)
	{
		return false;
	}

	char* mem = reinterpret_cast<char*>(GlobalLock(memHandle));

	if (mem != nullptr)
	{
		ftl::memcopy(mem, text, len);
		mem[len] = '\0';
	}

	GlobalUnlock(memHandle);

	EmptyClipboard();
	SetClipboardData(CF_TEXT, memHandle);
	CloseClipboard();

	return true;
}

bool WinOSInterface::copy_to_clipboard(WndHandle handle, wide_literal_t text, size_t len) const
{
	if (!OpenClipboard(reinterpret_cast<HWND>(handle)))
	{
		return false;
	}

	HGLOBAL memHandle = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));

	if (!memHandle)
	{
		return false;
	}

	wchar_t* mem = reinterpret_cast<wchar_t*>(GlobalLock(memHandle));

	if (mem != nullptr)
	{
		ftl::memcopy(mem, text, len);
		mem[len] = L'\0';
	}

	GlobalUnlock(memHandle);

	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, memHandle);
	CloseClipboard();

	return true;
}

void WinOSInterface::set_cursor(core::IOCursorType type) const
{
	static HCURSOR cursors[core::IOCursorType::Io_Cursor_Type_Max] = {
		LoadCursor(NULL, IDC_ARROW),
		LoadCursor(NULL, IDC_SIZENS),
		LoadCursor(NULL, IDC_SIZEWE),
		LoadCursor(NULL, IDC_SIZENWSE),
		LoadCursor(NULL, IDC_WAIT),
		LoadCursor(NULL, IDC_IBEAM)
	};
	SetCursor(cursors[type]);
}

void WinOSInterface::clip_cursor(int32 x, int32 y, int32 width, int32 height)
{
	RECT rect;
	rect.left = x;
	rect.right = x + width;
	rect.top = y;
	rect.bottom = y + height;
	ClipCursor(&rect);
}

void WinOSInterface::unclip_cursor() const
{
	ClipCursor(nullptr);
}

core::Point WinOSInterface::get_mouse_screen_pos()
{
	POINT p;
	if (!GetCursorPos(&p))
	{
		p = m_ctx->get_last_mouse_pos();
	}
	m_ctx->set_last_mouse_pos(p);

	return core::Point{ p.x, p.y };
}

void WinOSInterface::set_mouse_pos(WndHandle handle, int32 x, int32 y) const
{
	POINT point{ x, y };
	ClientToScreen(reinterpret_cast<HWND>(handle), &point);
	SetCursorPos(point.x, point.y);
}

void WinOSInterface::display_cursor(bool show) const
{
	if (show)
	{
		while (ShowCursor(show) < 0);
	}
	else
	{
		while (ShowCursor(show) >= 0);
	}
}

Handle WinOSInterface::get_app_instance_handle() const
{
	return reinterpret_cast<Handle>(GetModuleHandle(NULL));
}

WndHandle WinOSInterface::create_window(const core::PlatformWindowCreateInfo& info)
{
	const wchar_t windowClassName[] = L"AngkasawanApp";

	static WNDCLASSW wc = [&]() -> WNDCLASSW
	{
		WNDCLASSW windowClass{};

		windowClass.style = 0;
		windowClass.lpfnWndProc = app_window_process;
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
		info.title,
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
		return nullptr;
	}

	if (info.allowFileDrop)
	{
		// Accept drag and dropped files.
		DragAcceptFiles(hwnd, TRUE);
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	if (info.catchInput)
	{
		// Register raw input devide to receive WM_INPUT events.
		RAWINPUTDEVICE device;
		device.usUsagePage = 0x01;
		device.usUsage = 0x02;
		device.dwFlags = RIDEV_INPUTSINK;
		device.hwndTarget = hwnd;

		RegisterRawInputDevices(&device, 1, sizeof(device));
	}
	
	return hwnd;
}

void WinOSInterface::destroy_window(WndHandle handle) const
{
	DestroyWindow(reinterpret_cast<HWND>(handle));
}

core::Rect WinOSInterface::get_window_screen_rect(WndHandle handle) const
{
	RECT rect;
	GetWindowRect(reinterpret_cast<HWND>(handle), &rect);
	return { 
		rect.left, 
		rect.top, 
		rect.right - rect.left, 
		rect.bottom - rect.top 
	};
}

core::Rect WinOSInterface::get_client_screen_rect(WndHandle handle) const
{
	RECT rect;
	GetClientRect(reinterpret_cast<HWND>(handle), &rect);
	return { 
		rect.left, 
		rect.top, 
		rect.right - rect.left, 
		rect.bottom - rect.top 
	};
}

void WinOSInterface::set_window_screen_rect(WndHandle handle, const core::Rect& rect) const
{
	SetWindowPos(
		reinterpret_cast<HWND>(handle), 
		HWND_TOPMOST, 
		rect.left, 
		rect.top, 
		rect.width, 
		rect.height, 
		SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER
	);
}

void WinOSInterface::set_window_client_rect(WndHandle handle, const core::Rect& rect) const
{
	MoveWindow(
		reinterpret_cast<HWND>(handle),
		rect.left,
		rect.top,
		rect.width,
		rect.height,
		TRUE
	);
}

void WinOSInterface::set_window_title(WndHandle handle, literal_t title) const
{
	SetWindowTextA(reinterpret_cast<HWND>(handle), title);
}

void WinOSInterface::set_window_title(WndHandle handle, wide_literal_t title) const
{
	SetWindowTextW(reinterpret_cast<HWND>(handle), title);
}

void WinOSInterface::maximize_window(WndHandle handle) const
{
	ShowWindow(reinterpret_cast<HWND>(handle), SW_SHOWMAXIMIZED);
}

void WinOSInterface::set_window_position(WndHandle handle, int32 x, int32 y, int32 width, int32 height) const
{
	SetWindowPos(reinterpret_cast<HWND>(handle), HWND_TOP, x, y, width, height, SWP_FRAMECHANGED);
}

void WinOSInterface::set_full_screen(WndHandle handle) const
{
	int32 w = GetSystemMetrics(SM_CXSCREEN);
	int32 h = GetSystemMetrics(SM_CYSCREEN);
	SetWindowPos(reinterpret_cast<HWND>(handle), HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);
}

bool WinOSInterface::is_window_maximized(WndHandle handle) const
{
	WINDOWPLACEMENT placement{};
	GetWindowPlacement(reinterpret_cast<HWND>(handle), &placement);
	return placement.showCmd == SW_SHOWMAXIMIZED;
}

WndHandle WinOSInterface::get_focused_window() const
{
	return reinterpret_cast<WndHandle>(GetActiveWindow());
}

core::Dimension WinOSInterface::get_monitor_dimension(WndHandle handle) const
{
	HMONITOR monitorHandle = MonitorFromWindow(static_cast<HWND>(handle), MONITOR_DEFAULTTONEAREST);

	MONITORINFO info{};
	info.cbSize = sizeof(MONITORINFO);

	if (!GetMonitorInfoA(monitorHandle, &info))
	{
		return core::Dimension{ 0, 0 };
	}

	const RECT workingSize = info.rcWork;

	return core::Dimension{ 
		workingSize.right - workingSize.left, 
		workingSize.bottom - workingSize.top
	};
}

core::Dimension WinOSInterface::get_primary_monitor_dimension() const
{
	const int32 width = GetSystemMetrics(SM_CXFULLSCREEN);
	const int32 height = GetSystemMetrics(SM_CYFULLSCREEN);
	return core::Dimension{ width, height };
}

void WinOSInterface::peek_events() const
{
	MSG msg{};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void WinOSInterface::register_os_event_callback(void* argument, std::function<void(void*, const core::OSEvent&)>&& callback)
{
	osContext.m_listeners.emplace(argument, std::move(callback));
}

Dll::Dll(literal_t path) :
	m_path{ path }, m_addressMap{}, m_stringPool{}, m_handle {}
{}

Dll::Dll(Dll&& rhs) { *this = std::move(rhs); }

Dll& Dll::operator=(Dll&& rhs)
{
	if (this != &rhs)
	{
		m_path = std::move(rhs.m_path);
		m_handle = rhs.m_handle;

		m_stringPool.reserve(rhs.m_stringPool.capacity());

		if (rhs.m_addressMap.size())
		{
			m_addressMap.reserve(rhs.m_addressMap.size());
		}

		for (const auto [func, address] : m_addressMap)
		{
			std::string_view str = append_to_string_pool(func);
			m_addressMap.emplace(func, address);
		}

		rhs.m_stringPool.clear();
		rhs.m_addressMap.release();

		new (&rhs) Dll{};
	}
	return *this;
}

bool Dll::load()
{
	if (!valid() && m_path.length())
	{
		m_handle = LoadLibraryA(m_path.c_str());
	}
	return m_handle != nullptr;
}

void Dll::unload()
{
	if (valid())
	{
		FreeLibrary(static_cast<HMODULE>(m_handle));
		m_handle = nullptr;
	}
}

std::string_view Dll::append_to_string_pool(std::string_view str)
{
	if (!m_stringPool.size())
	{
		m_stringPool.reserve(64);
	}
	size_t const offset = m_stringPool.size();
	m_stringPool.append(str);
	return std::string_view{ m_stringPool.begin() + offset, m_stringPool.end() };
}

void* Dll::load_fn(std::string_view name)
{
	if (!m_addressMap.contains(name))
	{
		std::string_view fnSignature = append_to_string_pool(name);
		HMODULE module = static_cast<HMODULE>(m_handle);
		void* pAddress = reinterpret_cast<void*>(GetProcAddress(module, fnSignature.data()));

		m_addressMap.emplace(fnSignature, pAddress);
	}

	return m_addressMap[name];
}

std::string_view Dll::path() const
{
	return std::string_view{ m_path.c_str(), m_path.length() };
}

os::DllHandle Dll::handle() const
{
	return m_handle;
}

bool Dll::valid() const
{
	return m_handle != nullptr;
}

}