#pragma once
#ifndef LEARNVK_SUBSYSTEM_INPUT_IO
#define LEARNVK_SUBSYSTEM_INPUT_IO

#include "Platform/Minimal.h"
#include "Platform/EngineAPI.h"
#include "Library/Math/Vec2.h"

enum IOMouseButton : uint32
{
	Io_MouseButton_Left		= 0x00,
	Io_MouseButton_Right	= 0x01,
	Io_MouseButton_Middle	= 0x02,
	Io_MouseButton_Extra1	= 0x03,
	Io_MouseButton_Extra2	= 0x04,
	Io_MouseButton_Max
};

enum IOState : uint32
{
	Io_State_Released	= 0x00,
	Io_State_Pressed	= 0x01,
	Io_State_Held		= 0x02,
	Io_State_Repeat		= 0x03
};

/**
* TODO(Ygsm):
* Add Numpad section
*/
enum IOKeys : uint32
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
	Io_Key_CapsLck,
	Io_Key_Tab,
	Io_Key_Shft,
	Io_Key_Ctrl,
	Io_Key_Alt,
	Io_Key_RgtWnd,
	Io_Key_LftWnd,
	Io_Key_Backspace,
	Io_Key_Enter,
	Io_Key_Pipe,
	Io_Key_Escape,
	Io_Key_Tilde,
	Io_Key_Plus,
	Io_Key_Minus,
	Io_Key_OpenBrace,
	Io_Key_CloseBrace,
	Io_Key_Colon,
	Io_Key_Quote,
	Io_Key_Comma,
	Io_Key_Period,
	Io_Key_Slash,
	Io_Key_Up,
	Io_Key_Down,
	Io_Key_Left,
	Io_Key_Right,
	Io_Key_PrintScreen,
	Io_Key_ScrollLock,
	Io_Key_Pause,
	Io_Key_Insert,
	Io_Key_Home,
	Io_Key_PgUp,
	Io_Key_Delete,
	Io_Key_End,
	Io_Key_PgDown,
	Io_Key_Space,
	Io_Key_Max
};

/**
* IO will be a mega struct.
*/
struct ENGINE_API IOSystem
{
	IOState MouseState[Io_MouseButton_Max];
	float32 MouseClickTime[Io_MouseButton_Max];
	float32 MousePrevClickTime[Io_MouseButton_Max];
	float32	MouseHoldDuration[Io_MouseButton_Max];

	IOState KeyState[Io_Key_Max];
	float32 KeyPressTime[Io_Key_Max];
	float32 KeyPrevPressTime[Io_Key_Max];
	float32	KeyHoldDuration[Io_Key_Max];

	// Configurations ...
	float32 KeyDoubleTapTime;
	float32 MouseDoubleClickTime	= 500.0f;
	float32 MouseDoubleClickMaxDistance = 5.0f;
	float32 MinDurationForHold = 1000.f;
	float32 MouseWheel;
	float32 MouseWheelH;
	vec2	MousePos;
	vec2	MouseClickPos[Io_MouseButton_Max];
	bool	MouseButtons[Io_MouseButton_Max];
	bool	Keys[Io_Key_Max];

	bool IsKeyPressed(IOKeys Key);
	bool IsKeyDoubleTapped(IOKeys Key);
	bool IsKeyHeld(IOKeys Key);
	bool IsKeyReleased(IOKeys Key);
	bool IsMouseClicked(IOMouseButton Button);
	bool IsMouseDoubleClicked(IOMouseButton Button);
	bool IsMouseHeld(IOMouseButton Button);
	bool IsMouseReleased(IOMouseButton Button);
	bool IsMouseDragging(IOMouseButton Button);
	bool MouseDragDelta(IOMouseButton Button, vec2& Buf);
	bool Ctrl();
	bool Alt();
	bool Shift();
};

#endif // !LEARNVK_SUBSYSTEM_INPUT_IO