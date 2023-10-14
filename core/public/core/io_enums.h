#pragma once
#ifndef CORE_IO_ENUMS_H
#define CORE_IO_ENUMS_H

namespace core
{

enum class IOMouseButton
{
	Left = 0,
	Right,
	Middle,
	Extra1,
	Extra2,
	Extra3,
	Extra4,
	Extra5,
	Max
};

enum class IOState
{
	Released = 0,
	Pressed,
	Held,
	Repeat,
	Max
};

// TODO(afiq):
// Add support for numpad.
enum class IOKey
{
	A = 0,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	_0,
	_1,
	_2,
	_3,
	_4,
	_5,
	_6,
	_7,
	_8,
	_9,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	Caps_Lock,
	Tab,
	Shift,
	Ctrl,
	Alt,
	Right_Window,
	Left_Window,
	Backspace,
	Enter,
	Pipe,
	Escape,
	Tilde,
	Plus,
	Minus,
	Open_Brace,
	Close_Brace,
	Colon,
	Quote,
	Comma,
	Period,
	Slash,
	Up,
	Down,
	Left,
	Right,
	Print_Screen,
	Scroll_Lock,
	Pause,
	Insert,
	Home,
	Page_Up,
	Delete,
	End,
	Page_Down,
	Space,
	Max
};

enum class IOCursorType
{
	Default = 0,
	Size_NS,
	Size_WE,
	Size_NWSE,
	Load,
	Text_Input,
	Max
};

enum class IOConfiguration
{
	Key_Double_Tap_Time,
	Key_Min_Duration_For_Hold,
	Mouse_Double_Click_Time,
	Mouse_Double_Click_Distance,
	Mouse_Min_Duration_For_Hold,
	Max
};

}

#endif // !CORE_IO_ENUMS_H
