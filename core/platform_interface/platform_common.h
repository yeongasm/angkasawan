#pragma once
#ifndef CORE_WINDOWING_WINDOWING_COMMON_H
#define CORE_WINDOWING_WINDOWING_COMMON_H

#include "core_minimal.h"
#include "os_shims.h"

COREBEGIN

enum IOMouseButton
{
	Io_Mouse_Button_Left = 0,
	Io_Mouse_Button_Right,
	Io_Mouse_Button_Middle,
	Io_Mouse_Button_Extra1,
	Io_Mouse_Button_Extra2,
	Io_Mouse_Button_Extra3,
	Io_Mouse_Button_Extra4,
	Io_Mouse_Button_Extra5,
	Io_Mouse_Button_Max
};

enum IOState
{
	Io_State_Released = 0,
	Io_State_Pressed,
	Io_State_Held,
	Io_State_Repeat,
	Io_State_Max
};

// TODO:
// Add support for numpad.
enum IOKey
{
	Io_Key_A = 0,
	Io_Key_B,
	Io_Key_C,
	Io_Key_D,
	Io_Key_E,
	Io_Key_F,
	Io_Key_G,
	Io_Key_H,
	Io_Key_I,
	Io_Key_J,
	Io_Key_K,
	Io_Key_L,
	Io_Key_M,
	Io_Key_N,
	Io_Key_O,
	Io_Key_P,
	Io_Key_Q,
	Io_Key_R,
	Io_Key_S,
	Io_Key_T,
	Io_Key_U,
	Io_Key_V,
	Io_Key_W,
	Io_Key_X,
	Io_Key_Y,
	Io_Key_Z,
	Io_Key_0,
	Io_Key_1,
	Io_Key_2,
	Io_Key_3,
	Io_Key_4,
	Io_Key_5,
	Io_Key_6,
	Io_Key_7,
	Io_Key_8,
	Io_Key_9,
	Io_Key_F1,
	Io_Key_F2,
	Io_Key_F3,
	Io_Key_F4,
	Io_Key_F5,
	Io_Key_F6,
	Io_Key_F7,
	Io_Key_F8,
	Io_Key_F9,
	Io_Key_F10,
	Io_Key_F11,
	Io_Key_F12,
	Io_Key_Caps_Lock,
	Io_Key_Tab,
	Io_Key_Shift,
	Io_Key_Ctrl,
	Io_Key_Alt,
	Io_Key_Right_Window,
	Io_Key_Left_Window,
	Io_Key_Backspace,
	Io_Key_Enter,
	Io_Key_Pipe,
	Io_Key_Escape,
	Io_Key_Tilde,
	Io_Key_Plus,
	Io_Key_Minus,
	Io_Key_Open_Brace,
	Io_Key_Close_Brace,
	Io_Key_Colon,
	Io_Key_Quote,
	Io_Key_Comma,
	Io_Key_Period,
	Io_Key_Slash,
	Io_Key_Up,
	Io_Key_Down,
	Io_Key_Left,
	Io_Key_Right,
	Io_Key_Print_Screen,
	Io_Key_Scroll_Lock,
	Io_Key_Pause,
	Io_Key_Insert,
	Io_Key_Home,
	Io_Key_Page_Up,
	Io_Key_Delete,
	Io_Key_End,
	Io_Key_Page_Down,
	Io_Key_Space,
	Io_Key_Max
};

enum IOCursorType
{
	Io_Cursor_Type_Default = 0,
	Io_Cursor_Type_Size_NS,
	Io_Cursor_Type_Size_WE,
	Io_Cursor_Type_Size_NWSE,
	Io_Cursor_Type_Load,
	Io_Cursor_Type_Text_Input,
	Io_Cursor_Type_Max
};

struct Point
{
	int32 x, y;
};

struct Rect
{
	int32 left;
	int32 top;
	int32 width;
	int32 height;
};

struct Dimension
{
	int32 width, height;
};

enum WindowType
{
	Window_Type_None,
	Window_Type_Main,
	Window_Type_Child
};

using WindowID = size_t;

struct OSEvent
{
	enum class Type
	{
		None	= 0,
		Quit,
		Key,
		Window_Close,
		Window_Resize,
		Window_Move,
		Mouse_Button,
		Mouse_Move,
		Mouse_Wheel,
		Focus,
		Drop_File,
		Char_Input,
		Max
	};

	os::WndHandle windowHandle;
	Type event;
	union 
	{
		struct { uint32 utf8; }					textInput;
		struct { int32 x, y; }					mouseMove;
		struct { float32 offset; }				mouseWheel;
		struct { bool down; uint32 button; }	mouseButton;
		struct { int32 x, y; }					winMove;
		struct { int32 width, height; }			winSize;
		struct { bool down; IOKey code; }		key;
		struct { void* handle; }				fileDrop;
		struct { bool gained; }					focus;
	};
};

struct PlatformWindowCreateInfo
{
	wide_literal_t title;

	struct
	{
		uint32 x, y;
	} position;

	struct
	{
		uint32 width, height;
	} dimension;

	bool isChildWindow;
	bool allowFileDrop;
	bool catchInput;
};

COREEND

#endif // !CORE_WINDOWING_WINDOWING_COMMON_H
