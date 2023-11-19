#include "platform_header.h"
#include "lib/array.h"
#include "event_queue.h"

namespace core
{

namespace os
{

static lib::array<std::function<void(OSEvent const&)>> gListenerCallbacks = {};

IOKey translate_key_code(size_t code)
{
	IOKey key{};
	switch (code)
	{
	case 0x41:
		key = IOKey::A;
		break;
	case 0x42:
		key = IOKey::B;
		break;
	case 0x43:
		key = IOKey::C;
		break;
	case 0x44:
		key = IOKey::D;
		break;
	case 0x45:
		key = IOKey::E;
		break;
	case 0x46:
		key = IOKey::F;
		break;
	case 0x47:
		key = IOKey::G;
		break;
	case 0x48:
		key = IOKey::H;
		break;
	case 0x49:
		key = IOKey::I;
		break;
	case 0x4A:
		key = IOKey::J;
		break;
	case 0x4B:
		key = IOKey::K;
		break;
	case 0x4C:
		key = IOKey::L;
		break;
	case 0x4D:
		key = IOKey::M;
		break;
	case 0x4E:
		key = IOKey::N;
		break;
	case 0x4F:
		key = IOKey::O;
		break;
	case 0x50:
		key = IOKey::P;
		break;
	case 0x51:
		key = IOKey::Q;
		break;
	case 0x52:
		key = IOKey::R;
		break;
	case 0x53:
		key = IOKey::S;
		break;
	case 0x54:
		key = IOKey::T;
		break;
	case 0x55:
		key = IOKey::U;
		break;
	case 0x56:
		key = IOKey::V;
		break;
	case 0x57:
		key = IOKey::W;
		break;
	case 0x58:
		key = IOKey::X;
		break;
	case 0x59:
		key = IOKey::Y;
		break;
	case 0x5A:
		key = IOKey::Z;
		break;
	case 0x30:
		key = IOKey::_0;
		break;
	case 0x31:
		key = IOKey::_1;
		break;
	case 0x32:
		key = IOKey::_2;
		break;
	case 0x33:
		key = IOKey::_3;
		break;
	case 0x34:
		key = IOKey::_4;
		break;
	case 0x35:
		key = IOKey::_5;
		break;
	case 0x36:
		key = IOKey::_6;
		break;
	case 0x37:
		key = IOKey::_7;
		break;
	case 0x38:
		key = IOKey::_8;
		break;
	case 0x39:
		key = IOKey::_9;
		break;
	case VK_F1:
		key = IOKey::F1;
		break;
	case VK_F2:
		key = IOKey::F2;
		break;
	case VK_F3:
		key = IOKey::F3;
		break;
	case VK_F4:
		key = IOKey::F4;
		break;
	case VK_F5:
		key = IOKey::F5;
		break;
	case VK_F6:
		key = IOKey::F6;
		break;
	case VK_F7:
		key = IOKey::F7;
		break;
	case VK_F8:
		key = IOKey::F8;
		break;
	case VK_F9:
		key = IOKey::F9;
		break;
	case VK_F10:
		key = IOKey::F10;
		break;
	case VK_F11:
		key = IOKey::F11;
		break;
	case VK_F12:
		key = IOKey::F12;
		break;
	case VK_CAPITAL:
		key = IOKey::Caps_Lock;
		break;
	case VK_TAB:
		key = IOKey::Tab;
		break;
	case VK_SHIFT:
		key = IOKey::Shift;
		break;
	case VK_CONTROL:
		key = IOKey::Ctrl;
		break;
	case VK_MENU:
		key = IOKey::Alt;
		break;
	case VK_RWIN:
		key = IOKey::Right_Window;
		break;
	case VK_LWIN:
		key = IOKey::Left_Window;
		break;
	case VK_BACK:
		key = IOKey::Backspace;
		break;
	case VK_RETURN:
		key = IOKey::Enter;
		break;
	case VK_OEM_5:
		key = IOKey::Pipe;
		break;
	case VK_ESCAPE:
		key = IOKey::Escape;
		break;
	case VK_OEM_3:
		key = IOKey::Tilde;
		break;
	case VK_OEM_PLUS:
		key = IOKey::Plus;
		break;
	case VK_OEM_MINUS:
		key = IOKey::Minus;
		break;
	case VK_OEM_4:
		key = IOKey::Open_Brace;
		break;
	case VK_OEM_6:
		key = IOKey::Close_Brace;
		break;
	case VK_OEM_1:
		key = IOKey::Colon;
		break;
	case VK_OEM_7:
		key = IOKey::Quote;
		break;
	case VK_OEM_COMMA:
		key = IOKey::Comma;
		break;
	case VK_OEM_PERIOD:
		key = IOKey::Period;
		break;
	case VK_OEM_2:
		key = IOKey::Slash;
		break;
	case VK_UP:
		key = IOKey::Up;
		break;
	case VK_DOWN:
		key = IOKey::Down;
		break;
	case VK_LEFT:
		key = IOKey::Left;
		break;
	case VK_RIGHT:
		key = IOKey::Right;
		break;
	case VK_SNAPSHOT:
		key = IOKey::Print_Screen;
		break;
	case VK_SCROLL:
		key = IOKey::Scroll_Lock;
		break;
	case VK_PAUSE:
		key = IOKey::Pause;
		break;
	case VK_INSERT:
		key = IOKey::Insert;
		break;
	case VK_HOME:
		key = IOKey::Home;
		break;
	case VK_PRIOR:
		key = IOKey::Page_Up;
		break;
	case VK_DELETE:
		key = IOKey::Delete;
		break;
	case VK_END:
		key = IOKey::End;
		break;
	case VK_NEXT:
		key = IOKey::Page_Down;
		break;
	case VK_SPACE:
		key = IOKey::Space;
		break;
	default:
		key = IOKey::Max;
		break;
	}
	return key;
}

OSEvent handle_mouse_input(HWND hWnd, LPARAM lParam)
{
	OSEvent ev{};
	ev.window = hWnd;

	HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>(lParam);
	UINT dataSize = {};
	GetRawInputData(hRawInput, RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));
	alignas(RAWINPUT) char dataBuf[1024];

	if (dataSize == 0 || dataSize > sizeof(dataBuf))
	{
		return {};
	}

	GetRawInputData(hRawInput, RID_INPUT, dataBuf, &dataSize, sizeof(RAWINPUTHEADER));
	const RAWINPUT* raw = (const RAWINPUT*)dataBuf;

	if (raw->header.dwType != RIM_TYPEMOUSE)
	{
		return {};
	}

	const RAWMOUSE& mouseData = raw->data.mouse;
	const USHORT flags = mouseData.usButtonFlags;
	const LONG x = mouseData.lLastX, y = mouseData.lLastY;

	const float32 wheel_delta = static_cast<float32>(static_cast<uint16>(mouseData.usButtonData));
	float32 scroll_delta = wheel_delta / WHEEL_DELTA;

	if (flags & RI_MOUSE_WHEEL)
	{
		if (scroll_delta > 1.f)
		{
			scroll_delta = -1.f;
		}
		ev.event = OSEvent::Type::Mouse_Wheel;
		ev.mouseWheelDelta.v = scroll_delta;
		return ev;
	}

	if (flags & RI_MOUSE_HWHEEL)
	{
		ev.event = OSEvent::Type::Mouse_Wheel;
		ev.mouseWheelDelta.h = scroll_delta;
		return ev;
	}

	ev.event = OSEvent::Type::Mouse_Button;

	if (flags & RI_MOUSE_LEFT_BUTTON_DOWN)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Left);
		ev.mouseButton.down = true;
	}

	if (flags & RI_MOUSE_LEFT_BUTTON_UP)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Left);
		ev.mouseButton.down = false;
	}

	if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Right);
		ev.mouseButton.down = true;
	}

	if (flags & RI_MOUSE_RIGHT_BUTTON_UP)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Right);
		ev.mouseButton.down = false;
	}

	if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Middle);
		ev.mouseButton.down = true;
	}

	if (flags & RI_MOUSE_MIDDLE_BUTTON_UP)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Middle);
		ev.mouseButton.down = false;
	}

	if (flags & RI_MOUSE_BUTTON_4_DOWN)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Extra1);
		ev.mouseButton.down = true;
	}

	if (flags & RI_MOUSE_BUTTON_4_UP)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Extra1);
		ev.mouseButton.down = false;
	}

	if (flags & RI_MOUSE_BUTTON_5_DOWN)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Extra2);
		ev.mouseButton.down = true;
	}

	if (flags & RI_MOUSE_BUTTON_5_UP)
	{
		ev.mouseButton.button = static_cast<uint32>(IOMouseButton::Extra2);
		ev.mouseButton.down = false;
	}

	if (x != 0 || y != 0)
	{
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hWnd, &p);
		ev.event = OSEvent::Type::Mouse_Move;
		ev.mouseMove.x = p.x;
		ev.mouseMove.y = p.y;
	}

	return ev;
}

LRESULT app_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	OSEvent e{};
	e.window = hwnd;
	switch (msg)
	{
	case WM_MOVE:
		e.event = OSEvent::Type::Window_Move;
		e.winMove.x = static_cast<int32>(LOWORD(lParam));
		e.winMove.y = static_cast<int32>(HIWORD(lParam));
		break;
	case WM_SIZE:
		e.event = OSEvent::Type::Window_Resize;
		e.winSize.width = static_cast<int32>(LOWORD(lParam));
		e.winSize.height = static_cast<int32>(HIWORD(lParam));
		break;
	case WM_CLOSE:
		e.event = OSEvent::Type::Window_Close;
		break;
	case WM_ACTIVATE:
		if (wParam == WA_INACTIVE) {
			while (ShowCursor(true) < 0);
			ClipCursor(nullptr);
		}
		e.event = OSEvent::Type::Focus;
		e.focus.gained = wParam != WA_INACTIVE;
		break;
	case WM_QUIT:
		e.event = OSEvent::Type::Quit;
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		e.event = OSEvent::Type::Key;
		e.key.down = true;
		e.key.code = translate_key_code(static_cast<size_t>(wParam));
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		e.event = OSEvent::Type::Key;
		e.key.down = false;
		e.key.code = translate_key_code(static_cast<size_t>(wParam));
		break;
		/*case WM_CHAR:
			e.event = OSEvent::Type::Char_Input;
			e.textInput.utf8 = 0;
			UTF32ToUTF8((uint32)msg.wParam, (int8*)&e.TextInput.Utf8);
			break;*/
	case WM_INPUT:
		e = handle_mouse_input(hwnd, lParam);
		break;
	}

	if (e.event != OSEvent::Type::None)
	{
		std::for_each(
			gListenerCallbacks.begin(),
			gListenerCallbacks.end(),
			[&e](std::function<void(OSEvent const&)> callback) -> void {
				callback(e);
			}
		);
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void EventQueue::peek_events()
{
	MSG msg{};
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void EventQueue::register_event_listener_callback(std::function<void(OSEvent const&)>&& callback)
{
	gListenerCallbacks.push_back(std::move(callback));
}

}

}