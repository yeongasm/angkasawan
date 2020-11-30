#include "IO.h"

//IOSystem::IOSystem() :
//	MouseState{},
//	KeyState{},
//	MouseHoldDuration{},
//	KeyHoldDuration{},
//	KeyPressTime{},
//	MouseClickTime{},
//	KeyDoubleTapTime(0.f),
//	MouseDoubleClickTime(0.f),
//	MouseDoubleClickMaxDistance(0.f),
//	MinDurationForHold(0.f),
//	MouseWheel(0.f),
//	MouseWheelH(0.f),
//	MousePos(0.f),
//	MouseClickPos(0.f),
//	MouseButtons{},
//	Keys{}
//{}
//
//IOSystem::~IOSystem() {}

bool IOSystem::IsKeyPressed(IOKeys Key)
{
	return KeyState[Key] == Io_State_Pressed;
}

bool IOSystem::IsKeyDoubleTapped(IOKeys Key)
{
	return KeyState[Key] == Io_State_Repeat;
}

bool IOSystem::IsKeyHeld(IOKeys Key)
{
	return KeyState[Key] == Io_State_Held;
}

bool IOSystem::IsKeyReleased(IOKeys Key)
{
	return KeyState[Key] == Io_State_Released;
}

bool IOSystem::IsMouseClicked(IOMouseButton Button)
{
	return MouseState[Button] == Io_State_Pressed;
}

bool IOSystem::IsMouseDoubleClicked(IOMouseButton Button)
{
	return MouseState[Button] == Io_State_Repeat;
}

bool IOSystem::IsMouseHeld(IOMouseButton Button)
{
	return MouseState[Button] == Io_State_Held;
}

bool IOSystem::IsMouseReleased(IOMouseButton Button)
{
	return MouseState[Button] == Io_State_Released;
}

bool IOSystem::MouseDragDelta(IOMouseButton Button, vec2& Buf)
{
	if (!IsMouseHeld(Button)) return false;
	Buf = MousePos - MouseClickPos[Button];
	return true;
}

bool IOSystem::IsMouseDragging(IOMouseButton Button)
{
	vec2 dragDelta = {};
	if (!MouseDragDelta(Button, dragDelta))
	{
		return false;
	}

	return CompareFloat(dragDelta.x, 0.0f, FLOAT_EPSILON) && CompareFloat(dragDelta.y, 0.0f, FLOAT_EPSILON);
}

bool IOSystem::Ctrl()
{
	return IsKeyHeld(Io_Key_Ctrl);
}

bool IOSystem::Alt()
{
	return IsKeyHeld(Io_Key_Alt);
}

bool IOSystem::Shift()
{
	return IsKeyHeld(Io_Key_Shft);
}