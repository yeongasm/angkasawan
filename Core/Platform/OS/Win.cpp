#include "Win.h"
#include <shellapi.h>
#include "Library/Containers/Map.h"

namespace OS
{
	uint32 TranslateKeyCode(uint32 KeyCode)
	{
		switch (KeyCode)
		{
			case 0x41: return 0;
			case 0x42: return 1;
			case 0x43: return 2;
			case 0x44: return 3;
			case 0x45: return 4;
			case 0x46: return 5;
			case 0x47: return 6;
			case 0x48: return 7;
			case 0x49: return 8;
			case 0x4A: return 9;
			case 0x4B: return 10;
			case 0x4C: return 11;
			case 0x4D: return 12;
			case 0x4E: return 13;
			case 0x4F: return 14;
			case 0x50: return 15;
			case 0x51: return 16;
			case 0x52: return 17;
			case 0x53: return 18;
			case 0x54: return 19;
			case 0x55: return 20;
			case 0x56: return 21;
			case 0x57: return 22;
			case 0x58: return 23;
			case 0x59: return 24;
			case 0x5A: return 25;
			case 0x30: return 26;
			case 0x31: return 27;
			case 0x32: return 28;
			case 0x33: return 29;
			case 0x34: return 30;
			case 0x35: return 31;
			case 0x36: return 32;
			case 0x37: return 33;
			case 0x38: return 34;
			case 0x39: return 35;
			case VK_F1: return 36;
			case VK_F2: return 37;
			case VK_F3: return 38;
			case VK_F4: return 39;
			case VK_F5: return 40;
			case VK_F6: return 41;
			case VK_F7: return 42;
			case VK_F8: return 43;
			case VK_F9: return 44;
			case VK_F10: return 45;
			case VK_F11: return 46;
			case VK_F12: return 47;
			case VK_CAPITAL: return 48;
			case VK_TAB: return 49;
			case VK_SHIFT: return 50;
			case VK_CONTROL: return 51;
			case VK_MENU: return 52;
			case VK_RWIN: return 53;
			case VK_LWIN: return 54;
			case VK_BACK: return 55;
			case VK_RETURN: return 56;
			case VK_OEM_5: return 57;
			case VK_ESCAPE: return 58;
			case VK_OEM_3: return 59;
			case VK_OEM_PLUS: return 60;
			case VK_OEM_MINUS: return 61;
			case VK_OEM_4: return 62;
			case VK_OEM_6: return 63;
			case VK_OEM_1: return 64;
			case VK_OEM_7: return 65;
			case VK_OEM_COMMA: return 66;
			case VK_OEM_PERIOD: return 67;
			case VK_OEM_2: return 68;
			case VK_UP: return 69;
			case VK_DOWN: return 70;
			case VK_LEFT: return 71;
			case VK_RIGHT: return 72;
			case VK_SNAPSHOT: return 73;
			case VK_SCROLL: return 74;
			case VK_PAUSE: return 75;
			case VK_INSERT: return 76;
			case VK_HOME: return 77;
			case VK_PRIOR: return 78;
			case VK_DELETE: return 79;
			case VK_END: return 80;
			case VK_NEXT: return 81;
			case VK_SPACE: return 82;
		}

		return -1;
	}

	struct
	{
		Interface* App			= nullptr;
		Point RelativeMousePos	= {};
		bool RelativeMouse		= false;
		bool RawInputRegistered = false;
		bool Finished			= false;
		struct
		{
			HCURSOR Load;
			HCURSOR SizeNS;
			HCURSOR SizeWE;
			HCURSOR SizeNWSE;
			HCURSOR Arrow;
			HCURSOR TextInput;
		} Cursors;
	} Ctx;

	void UTF32ToUTF8(uint32 Utf32, char* Utf8)
	{
		if (Utf32 <= 0x7F) {
			Utf8[0] = (char)Utf32;
		}
		else if (Utf32 <= 0x7FF) {
			Utf8[0] = 0xC0 | (char)((Utf32 >> 6) & 0x1F);
			Utf8[1] = 0x80 | (char)(Utf32 & 0x3F);
		}
		else if (Utf32 <= 0xFFFF) {
			Utf8[0] = 0xE0 | (char)((Utf32 >> 12) & 0x0F);
			Utf8[1] = 0x80 | (char)((Utf32 >> 6) & 0x3F);
			Utf8[2] = 0x80 | (char)(Utf32 & 0x3F);
		}
		else if (Utf32 <= 0x10FFFF) {
			Utf8[0] = 0xF0 | (char)((Utf32 >> 18) & 0x0F);
			Utf8[1] = 0x80 | (char)((Utf32 >> 12) & 0x3F);
			Utf8[2] = 0x80 | (char)((Utf32 >> 6) & 0x3F);
			Utf8[3] = 0x80 | (char)(Utf32 & 0x3F);
		}
		else {
			VKT_ASSERT(false);
		}
	}

	static auto WatchFileTime = [](const char* Filename) -> FILETIME
	{
		WIN32_FIND_DATAA find;
		HANDLE handle = ::FindFirstFileA(Filename, &find);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return FILETIME();
		}
		::FindClose(handle);
		return find.ftLastWriteTime;
	};

	bool DllModule::LoadDllModule()
	{
#if HOTRELOAD_ENABLED
		// NOTE(Ygsm):
		// I don't know why Sleep(1) solves the issue. Perhaps a thread context switch forces the file to no longer belong to the process?
		OS::Sleep(1);
		::CopyFileA(Filename.C_Str(), TempFilename.C_Str(), FALSE);
		Handle = OS::LoadDllLibrary(TempFilename.C_Str());
#else
		Handle = OS::LoadDllLibrary(Filename.C_Str());
#endif
		if (!Handle) { return false; }

		OnDllLoad	= reinterpret_cast<CallbackFunc>(OS::LoadProcAddress(Handle, "OnDllLoad"));
		OnDllUnload = reinterpret_cast<CallbackFunc>(OS::LoadProcAddress(Handle, "OnDllUnload"));

		LastModified = WatchFileTime(Filename.C_Str());

		return true;
	}

	void DllModule::UnloadDllModule()
	{
		OS::FreeDllLibrary(Handle);
		::DeleteFileA(TempFilename.C_Str());
	}

	bool DllModule::WasDllUpdated()
	{
		//static auto watchFileTime = [](const char* Filename) -> FILETIME
		//{
		//	WIN32_FIND_DATAA find;
		//	HANDLE handle = ::FindFirstFileA(Filename, &find);
		//	if (handle == INVALID_HANDLE_VALUE)
		//	{
		//		return FILETIME();
		//	}
		//	::FindClose(handle);
		//	return find.ftLastWriteTime;
		//};

		FILETIME updatedFileTime = WatchFileTime(Filename.C_Str());
		if (!::CompareFileTime(&updatedFileTime, &LastModified))
		{
			return false;
		}

		LastModified = updatedFileTime;

		return true;
	}

	void SetApplicationInstance(Interface& App)
	{
		Ctx.App = &App;
	}

	void InitializeOSContext()
	{
		Ctx.Cursors.Arrow		= LoadCursor(NULL, IDC_ARROW);
		Ctx.Cursors.TextInput	= LoadCursor(NULL, IDC_IBEAM);
		Ctx.Cursors.Load		= LoadCursor(NULL, IDC_WAIT);
		Ctx.Cursors.SizeNS		= LoadCursor(NULL, IDC_SIZENS);
		Ctx.Cursors.SizeWE		= LoadCursor(NULL, IDC_SIZEWE);
		Ctx.Cursors.SizeNWSE	= LoadCursor(NULL, IDC_SIZENWSE);
	}

	size_t GetCPUCount()
	{
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		size_t num = info.dwNumberOfProcessors;
		num = num > 0 ? num : 1;

		return num;
	}

	DllHandle GetHandleToModule(const char* ModuleName)
	{
		return ::GetModuleHandleA(ModuleName);
	}

	void Sleep(size_t Milliseconds)
	{
		::Sleep(static_cast<DWORD>(Milliseconds));
	}

	size_t GetCurrentThreadId()
	{
		return ::GetCurrentThreadId();
	}
	
	void CopyToClipboard(const char* Text)
	{
		if (!::OpenClipboard(NULL)) return;
		int32 len = static_cast<int32>(strlen(Text));
		HGLOBAL memHandle = ::GlobalAlloc(GMEM_MOVEABLE, (len * sizeof(uint8)) + 1);
		if (!memHandle) return;

		int8* mem = reinterpret_cast<int8*>(::GlobalLock(memHandle));
		if (mem)
		{
			for (int32 i = 0; i < len + 1; i++)
				mem[i] = Text[i];

			mem[len] = '\0';
		}
		::GlobalUnlock(memHandle);
		::EmptyClipboard();
		::SetClipboardData(CF_TEXT, memHandle);
		::CloseClipboard();
	}

	void SetCursor(CursorType Type)
	{
		switch (Type)
		{
		case CursorType::DEFAULT:
			::SetCursor(Ctx.Cursors.Arrow);
			break;
		case CursorType::LOAD:
			::SetCursor(Ctx.Cursors.Load);
			break;
		case CursorType::SIZE_NS:
			::SetCursor(Ctx.Cursors.SizeNS);
			break;
		case CursorType::SIZE_WE:
			::SetCursor(Ctx.Cursors.SizeWE);
			break;
		case CursorType::SIZE_NWSE:
			::SetCursor(Ctx.Cursors.SizeNWSE);
			break;
		case CursorType::TEXT_INPUT:
			::SetCursor(Ctx.Cursors.TextInput);
			break;
		case CursorType::UNDEFINED:
		default:
			VKT_ASSERT(false);
			break;
		}
	}

	void ClipCursor(int32 Screen_X, int32 Screen_Y, int32 Width, int32 Height)
	{
		RECT rect;
		rect.left = Screen_X;
		rect.right = Screen_X + Width;
		rect.top = Screen_Y;
		rect.bottom = Screen_Y + Height;
		::ClipCursor(&rect);
	}

	void UnclipCursor()
	{
		::ClipCursor(NULL);
	}

	Point GetMouseScreenPos()
	{
		POINT p;
		static POINT lastPoint = {};
		const BOOL b = ::GetCursorPos(&p);
		if (!b)
		{
			p = lastPoint;
		}
		lastPoint = p;
		return { p.x, p.y };
	}

	void SetMousePos(WindowHandle Wnd, int32 x, int32 y)
	{
		POINT point;
		point.x = x;
		point.y = y;
		::ClientToScreen(Wnd, &point);
		::SetCursorPos(point.x, point.y);
	}

	void ShowCursor(bool Show)
	{
		if (Show)
		{
			::SetCursor(Ctx.Cursors.Arrow);
			//while (::ShowCursor(Show) < 0);
		}
		else
		{
			::SetCursor(NULL);
			//while (::ShowCursor(Show) >= 0);
		}
	}

	WindowHandle CreateAppWindow(const WindowCreateInformation& Info)
	{
		VKT_ASSERT(Ctx.App);
		const CHAR clsName[MAX_PATH_LENGTH] = "AngkasawanEngineWin32";
		static WNDCLASS wc = [&]() -> WNDCLASS 
		{
			WNDCLASS wc = {};
			auto WndProc = [](HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				Event e;
				e.Window = hWnd;
				switch (Msg)
				{
				case WM_MOVE:
					e.EventType = Event::Type::WINDOW_MOVE;
					e.WinMove.x = (uint16)LOWORD(lParam);
					e.WinMove.y = (uint16)HIWORD(lParam);
					Ctx.App->OnEvent(e);
					break;
				case WM_SIZE:
					e.EventType = Event::Type::WINDOW_RESIZE;
					e.WinSize.Width = LOWORD(lParam);
					e.WinSize.Height = HIWORD(lParam);
					Ctx.App->OnEvent(e);
					break;
				case WM_CLOSE:
					e.EventType = Event::Type::WINDOW_CLOSE;
					Ctx.App->OnEvent(e);
					return 0;
				case WM_PAINT: {
					// Temporary solution I guess?
					// My guess why the Lumix engine don't do this is because it tells the Graphics API to do it instead.
					//PAINTSTRUCT ps;
					//HDC hdc = ::BeginPaint(hWnd, &ps);
					//::FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
					//::EndPaint(hWnd, &ps);
					break;
				}
				case WM_ACTIVATE:
					if (wParam == WA_INACTIVE) {
						OS::ShowCursor(true);
						OS::UnclipCursor();
					}
					e.EventType = Event::Type::FOCUS;
					e.Focus.Gained = wParam != WA_INACTIVE;
					Ctx.App->OnEvent(e);
					break;
				}
				return DefWindowProc(hWnd, Msg, wParam, lParam);
			};

			wc.style = 0;
			wc.lpfnWndProc = WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = GetModuleHandle(NULL);
			wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
			wc.hCursor = NULL;
			wc.hbrBackground = NULL;
			wc.lpszClassName = clsName;

			if (!RegisterClass(&wc))
			{
				VKT_ASSERT(false);
				return {};
			}
			return wc;
		}();

		//HWND window		= Info.Window;
		DWORD style		= Info.Flags & WindowCreateInformation::NO_DECORATION ? WS_POPUP : WS_OVERLAPPEDWINDOW;
		DWORD extStyle	= Info.Flags & WindowCreateInformation::NO_TASKBAR_ICON ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW;
		HWND hwnd = CreateWindowEx(
			extStyle,
			clsName,
			Info.Name,
			style,
			Info.PosX,
			Info.PosY,
			Info.Width,
			Info.Height,
			NULL,
			NULL,
			wc.hInstance,
			NULL
		);

		if (hwnd == NULL)
		{
			VKT_ASSERT(false);
		}

		if (Info.HandleFileDrop)
		{
			DragAcceptFiles(hwnd, TRUE);
		}

		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);

		if (!Ctx.RawInputRegistered)
		{
			RAWINPUTDEVICE device;
			device.usUsagePage = 0x01;
			device.usUsage = 0x02;
			device.dwFlags = RIDEV_INPUTSINK;
			device.hwndTarget = hwnd;
			BOOL status = RegisterRawInputDevices(&device, 1, sizeof(device));
			VKT_ASSERT(status);
			Ctx.RawInputRegistered = true;
		}

		return hwnd;
	}

	void DestroyAppWindow(WindowHandle Wnd)
	{
		DestroyWindow(Wnd);
	}

	Rectangle GetWindowScreenRect(WindowHandle Wnd)
	{
		RECT rect;
		GetWindowRect(Wnd, &rect);
		return { rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top };
	}

	Rectangle GetWindowClientRect(WindowHandle Wnd)
	{
		RECT rect;
		BOOL status = ::GetClientRect(Wnd, &rect);
		VKT_ASSERT(status);
		return { rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top };
	}

	void SetWindowClientRect(WindowHandle Wnd, const Rectangle& Rect)
	{
		BOOL status = ::SetWindowPos(Wnd, HWND_TOPMOST, Rect.Left, Rect.Top, Rect.Width, Rect.Height, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		VKT_ASSERT(status);
	}

	void SetWindowScreenRect(WindowHandle Wnd, const Rectangle& Rect)
	{
		::MoveWindow(Wnd, Rect.Left, Rect.Top, Rect.Width, Rect.Height, TRUE);
	}

	void SetWindowTitle(WindowHandle Wnd, const char* Title)
	{
		::SetWindowText(Wnd, Title);
	}

	void MaximizeWindow(WindowHandle Wnd)
	{
		::ShowWindow(Wnd, SW_SHOWMAXIMIZED);
	}

	WindowState SetFullScreen(WindowHandle Wnd)
	{
		WindowState res;
		res.Rect = OS::GetWindowScreenRect(Wnd);
		res.Style = ::SetWindowLongPtr(Wnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
		int32 w = GetSystemMetrics(SM_CXSCREEN);
		int32 h = GetSystemMetrics(SM_CYSCREEN);
		::SetWindowPos(Wnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);
		return res;
	}

	void Restore(WindowHandle Wnd, WindowState State)
	{
		::SetWindowLongPtr(Wnd, GWL_STYLE, State.Style);
		OS::SetWindowScreenRect(Wnd, State.Rect);
	}

	bool IsWindowMaximized(WindowHandle Wnd)
	{
		WINDOWPLACEMENT placement = {};
		BOOL res = ::GetWindowPlacement(Wnd, &placement);
		VKT_ASSERT(res);
		return placement.showCmd == SW_SHOWMAXIMIZED;
	}

	WindowHandle GetFocused()
	{
		return ::GetActiveWindow();
	}

	bool IsKeyDown(uint32 Key)
	{
		const SHORT res = GetAsyncKeyState(Key);
		return (res & 0x8000) != 0;
	}

	//int32 GetDPI()
	//{
	//	return int32();
	//}

	void ProcessEvent()
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Ctx.Finished) return;

			bool discard = false;
			Event e;
			e.Window = msg.hwnd;
			
			switch (msg.message)
			{
			case WM_DROPFILES:
				e.EventType = Event::Type::DROP_FILE;
				e.FileDrop.Handle = (HDROP)msg.wParam;
				Ctx.App->OnEvent(e);
				break;
			case WM_QUIT:
				e.EventType = Event::Type::QUIT;
				Ctx.App->OnEvent(e);
				break;
			case WM_CLOSE:
				e.EventType = Event::Type::WINDOW_CLOSE;
				Ctx.App->OnEvent(e);
				break;
			case WM_SYSKEYDOWN:
				discard = msg.wParam == VK_MENU;
				break;
			case WM_KEYDOWN:
				e.EventType = Event::Type::KEY;
				e.Key.Down = true;
				e.Key.Code = TranslateKeyCode((uint32)msg.wParam);
				Ctx.App->OnEvent(e);
				break;
			case WM_KEYUP:
				e.EventType = Event::Type::KEY;
				e.Key.Down = false;
				e.Key.Code = TranslateKeyCode((uint32)msg.wParam);
				Ctx.App->OnEvent(e);
				break;
			case WM_CHAR:
				e.EventType = Event::Type::CHAR;
				e.TextInput.Utf8 = 0;
				UTF32ToUTF8((uint32)msg.wParam, (int8*)&e.TextInput.Utf8);
				Ctx.App->OnEvent(e);
				break;
			case WM_INPUT: {
					HRAWINPUT hRawInput = (HRAWINPUT)msg.lParam;
					UINT dataSize = {};
					GetRawInputData(hRawInput, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
					alignas(RAWINPUT) char dataBuf[1024];
					if (dataSize == 0 || dataSize > sizeof(dataBuf)) break;

					GetRawInputData(hRawInput, RID_INPUT, dataBuf, &dataSize, sizeof(RAWINPUTHEADER));

					const RAWINPUT* raw = (const RAWINPUT*)dataBuf;
					if (raw->header.dwType != RIM_TYPEMOUSE) break;

					const RAWMOUSE& mouseData = raw->data.mouse;
					const USHORT flags = mouseData.usButtonFlags;
					const short wheel_delta = (short)mouseData.usButtonData;
					const LONG x = mouseData.lLastX, y = mouseData.lLastY;

					if (wheel_delta) 
					{
						e.MouseWheel.Offset = (float)wheel_delta / WHEEL_DELTA;
						e.EventType = Event::Type::MOUSE_WHEEL;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) 
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::LEFT;
						e.MouseButton.Down = true;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_LEFT_BUTTON_UP) 
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::LEFT;
						e.MouseButton.Down = false;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) 
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::RIGHT;
						e.MouseButton.Down = true;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_RIGHT_BUTTON_UP)
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::RIGHT;
						e.MouseButton.Down = false;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) 
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::MIDDLE;
						e.MouseButton.Down = true;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_MIDDLE_BUTTON_UP)
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::MIDDLE;
						e.MouseButton.Down = false;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_BUTTON_4_DOWN)
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::EXTENDED + 1;
						e.MouseButton.Down = true;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_BUTTON_4_UP)
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::EXTENDED + 1;
						e.MouseButton.Down = false;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_BUTTON_5_DOWN)
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::EXTENDED;
						e.MouseButton.Down = true;
						Ctx.App->OnEvent(e);
					}

					if (flags & RI_MOUSE_BUTTON_5_UP)
					{
						e.EventType = Event::Type::MOUSE_BUTTON;
						e.MouseButton.Button = MouseButton::EXTENDED;
						e.MouseButton.Down = false;
						Ctx.App->OnEvent(e);
					}

					if (x != 0 || y != 0) 
					{
						POINT p;
						::GetCursorPos(&p);
						::ScreenToClient(msg.hwnd, &p);
						e.EventType = Event::Type::MOUSE_MOVE;
						e.MouseMove.x = p.x;
						e.MouseMove.y = p.y;
						Ctx.App->OnEvent(e);
					}

					break;
				}
			}

			if (!discard) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	uint64 GetPerfCounter()
	{
		LARGE_INTEGER val;
		::QueryPerformanceCounter(&val);
		return val.QuadPart;
	}

	DllHandle LoadDllLibrary(const char* LibraryName)
	{
		return ::LoadLibraryA(LibraryName);
	}

	void FreeDllLibrary(DllHandle Module)
	{
		::FreeLibrary(Module);
	}

	FuncPointer LoadProcAddress(DllHandle Module, const char* FunctionName)
	{
		return ::GetProcAddress(Module, FunctionName);
	}
}