#pragma once
#ifndef LEARNVK_PLATFORM_OS_WINDOWS
#define LEARNVK_PLATFORM_OS_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define MAX_PATH_LENGTH 256

//#include "Library/Templates/Types.h"
#include "Library/Containers/String.h"

/**
* Damn you Windows for making this into a macro!!!
*/
#undef CopyFile
#undef MoveFile
#undef DeleteFile

namespace OS
{
	struct Point	 { int32 x, y; };
	struct Rectangle { int32 Left, Top, Width, Height; };

	using WindowHandle	= HWND;
	using DllHandle		= HINSTANCE;
	using FuncPointer	= FARPROC;

	struct WindowState
	{
		Rectangle	Rect;
		uint64		Style;
	};

	struct WindowCreateInformation
	{
		enum Flags : uint32
		{
			NO_DECORATION	= 1 << 0,
			NO_TASKBAR_ICON = 1 << 1
		};
		const char* Name	= nullptr;
		WindowHandle Window = nullptr;
		uint32 Flags		= 0;
		uint32 Width		= 0;
		uint32 Height		= 0;
		int32 PosX			= 0;
		int32 PosY			= 0;
		bool FullScreen		= false;
		bool HandleFileDrop = false;
	};

	enum ExecutionResult : uint32
	{
		NO_ASSOCIATION = 0xFF,
		OTHER_ERROR = 0x00,
		SUCCESS		= 0x01
	};

	enum CursorType : uint32
	{
		DEFAULT,
		SIZE_NS,
		SIZE_WE,
		SIZE_NWSE,
		LOAD,
		TEXT_INPUT,
		UNDEFINED
	};

	enum MouseButton : uint32
	{
		LEFT		= 0x00,
		RIGHT		= 0x01,
		MIDDLE		= 0x02,
		EXTENDED	= 0x03,
		MAX			= 0x10
	};

	struct Event
	{
		enum class Type : uint32
		{
			NOEVENT,
			QUIT,
			KEY,
			WINDOW_CLOSE,
			WINDOW_RESIZE,
			WINDOW_MOVE,
			MOUSE_BUTTON,
			MOUSE_MOVE,
			MOUSE_WHEEL,
			FOCUS,
			DROP_FILE,
			CHAR
		};

		WindowHandle Window;
		Type EventType;
		union {
			struct { uint32 Utf8; }					TextInput;
			struct { int32 x, y; }					MouseMove;
			struct { float32 Offset; }				MouseWheel;
			struct { bool Down; uint32 Button; }	MouseButton;
			struct { int32 x, y; }					WinMove;
			struct { int32 Width, Height; }			WinSize;
			struct { bool Down; uint32 Code; }		Key;
			struct { void* Handle; }				FileDrop;
			struct { bool Gained; }					Focus;
		};
	};

	// Should only deal with OS related functionalities...
	// There should be a global OS instance like in Lumix Engine ...
	struct Interface
	{
		virtual void OnEvent	 (const Event& e) = 0;
		virtual void OnInit		 ()			= 0;
		virtual void OnTerminate ()			= 0;
	};

	struct DllModule
	{
		using FixedString	= StaticString<MAX_PATH_LENGTH>;
		using CallbackFunc	= void(*)(void*);

		DllModule()	 = default;
		~DllModule() = default;

		DEFAULT_COPY_AND_MOVE(DllModule)

		FixedString Filename;
		FixedString TempFilename;
		DllHandle	Handle;
		FILETIME	LastModified;

		CallbackFunc OnDllLoad;
		CallbackFunc OnDllUnload;

		bool LoadDllModule();
		void UnloadDllModule();

		bool WasDllUpdated();
	};

	//struct Timer
	//{
	//};

	void			SetApplicationInstance(Interface& App);
	void			InitializeOSContext	();
	void			Sleep				(size_t Milliseconds);
	size_t			GetCurrentThreadId	();
	//ExecutionResult OpenExplorer		(const char* Path);
	void			CopyToClipboard		(const char* Text);
	void			SetCursor			(CursorType Type);
	void			ClipCursor			(int32 Screen_X, int32 Screen_Y, int32 Width, int32 Height);
	void			UnclipCursor		();
	Point			GetMouseScreenPos	();
	void			SetMousePos			(WindowHandle Wnd, int32 x, int32 y);
	void			ShowCursor			(bool Show);
	WindowHandle	CreateAppWindow		(const WindowCreateInformation& Info);
	void			DestroyAppWindow	(WindowHandle Wnd);
	Rectangle		GetWindowScreenRect	(WindowHandle Wnd);
	Rectangle		GetWindowClientRect	(WindowHandle Wnd);
	void			SetWindowClientRect	(WindowHandle Wnd, const Rectangle& Rect);
	void			SetWindowScreenRect	(WindowHandle Wnd, const Rectangle& Rect);
	void			SetWindowTitle		(WindowHandle Wnd, const char* Title);
	void			MaximizeWindow		(WindowHandle Wnd);
	WindowState		SetFullScreen		(WindowHandle Wnd);
	void			Restore				(WindowHandle Wnd, WindowState State);
	bool			IsWindowMaximized	(WindowHandle Wnd);
	WindowHandle	GetFocused			();
	bool			IsKeyDown			(uint32 Key);
	void			ProcessEvent		();
	//int32			GetDPI				();

	ENGINE_API DllHandle	LoadDllLibrary	(const char* LibraryName);
	ENGINE_API void			FreeDllLibrary	(DllHandle Module);
	ENGINE_API FuncPointer	LoadProcAddress	(DllHandle Module, const char* FunctionName);
	ENGINE_API size_t		GetCPUCount();
	ENGINE_API DllHandle	GetHandleToModule(const char* ModuleName);
	ENGINE_API uint64		GetPerfCounter		();
}

#endif // !LEARNVK_PLATFORM_OS_WINDOWS