#pragma once
#ifndef CORE_MINIMAL_H
#define CORE_MINIMAL_H

#include <string_view>
#include "lib/common.h"
#include "engine_api.h"
#include "io_enums.h"

namespace core
{

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

struct OSEvent
{
	enum class Type
	{
		None = 0,
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
	void*	window;
	Type	event;
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

//struct Version
//{
//	uint32 major;
//	uint32 minor;
//	uint32 patch;
//};
//
//struct EngineInfo
//{
//	std::wstring_view applicationName;
//	Version version;
//};

enum class EngineState
{
	None = 0,
	Starting,
	Running,
	Terminating
};

}

#endif // !CORE_MINIMAL_H
