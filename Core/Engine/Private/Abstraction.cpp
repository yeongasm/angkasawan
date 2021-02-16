#include "Abstraction.h"

//EngineImpl::EngineImpl()  {}
//EngineImpl::~EngineImpl() {}

//void EngineImpl::UpdateIOStates()
//{
//	// For now this is fine because we only support a single window.
//	// Don't do anything if window is not focused ...
//	//if (Window.Handle != OS::GetFocused()) return;
//	if (!IsWindowFocused()) return;
//
//	for (int32 i = 0; i < Io_MouseButton_Max; i++)
//	{
//		if (!Io.MouseButtons[i])
//		{
//			Io.MouseClickPos[i] = vec2(0.f);
//			Io.MouseState[i] = Io_State_Released;
//			continue;
//		}
//
//		if (Io.MouseState[i] == Io_State_Pressed)
//		{	
//			if (Clock.FElapsedTime() - Io.MouseClickTime[i] >= Io.MinDurationForHold)
//			{
//				Io.MouseState[i] = Io_State_Held;
//			}
//			if (Io.MouseClickTime[i] - Io.MousePrevClickTime[i] < Io.MouseDoubleClickTime)
//			{
//				vec2 clickPosDelta = Io.MousePos - Io.MouseClickPos[i];
//				float32 lengthSquared = clickPosDelta.x * clickPosDelta.x + clickPosDelta.y * clickPosDelta.y;
//				if (lengthSquared < Io.MouseDoubleClickMaxDistance * Io.MouseDoubleClickMaxDistance)
//				{
//					Io.MouseState[i] = Io_State_Repeat;
//				}
//			}
//		}
//
//		if (Io.MouseState[i] == Io_State_Held)
//		{
//			Io.MouseHoldDuration[i] = Clock.FElapsedTime() - Io.MouseClickTime[i];
//		}
//
//		if (Io.MouseState[i] == Io_State_Released)
//		{
//			Io.MouseClickPos[i] = Io.MousePos;
//			Io.MouseState[i] = Io_State_Pressed;
//			Io.MousePrevClickTime[i] = Io.MouseClickTime[i];
//			Io.MouseClickTime[i] = Clock.FElapsedTime();
//		}
//	}
//
//	for (int32 i = 0; i < Io_Key_Max; i++)
//	{
//		if (!Io.Keys[i])
//		{
//			Io.KeyState[i] = Io_State_Released;
//			continue;
//		}
//
//		if (Io.KeyState[i] == Io_State_Pressed)
//		{
//			if (Clock.ElapsedTime() - Io.KeyPressTime[i] >= Io.MinDurationForHold)
//			{
//				Io.KeyState[i] = Io_State_Held;
//			}
//			if (Io.KeyPressTime[i] - Io.KeyPrevPressTime[i] < Io.KeyDoubleTapTime)
//			{
//				Io.KeyState[i] = Io_State_Repeat;
//			}
//		}
//
//		if (Io.KeyState[i] == Io_State_Held)
//		{
//			Io.KeyHoldDuration[i] = Clock.FElapsedTime() - Io.KeyPressTime[i];
//		}
//
//		if (Io.KeyState[i] == Io_State_Released)
//		{
//			Io.KeyState[i] = Io_State_Pressed;
//			Io.KeyPrevPressTime[i] = Io.KeyPressTime[i];
//			Io.KeyPressTime[i] = Clock.FElapsedTime();
//		}
//	}
//}

void EngineImpl::ReserveMemoryForSystems()
{
	SystemAllocator.Initialize(KILOBYTES(512));
	EngineSystems.Reserve(16);
	GameSystems.Reserve(16);
}

void EngineImpl::UpdateMouseStates()
{
	if (!IsWindowFocused()) return;

	for (int32 i = 0; i < Io_MouseButton_Max; i++)
	{
		if (!Io.MouseButtons[i])
		{
			Io.MouseClickPos[i] = vec2(0.f);
			Io.MouseState[i] = Io_State_Released;
			continue;
		}

		if (Io.MouseState[i] == Io_State_Pressed)
		{
			if (Clock.FElapsedTime() - Io.MouseClickTime[i] >= Io.MinDurationForHold)
			{
				Io.MouseState[i] = Io_State_Held;
			}
			if (Io.MouseClickTime[i] - Io.MousePrevClickTime[i] < Io.MouseDoubleClickTime)
			{
				vec2 clickPosDelta = Io.MousePos - Io.MouseClickPos[i];
				float32 lengthSquared = clickPosDelta.x * clickPosDelta.x + clickPosDelta.y * clickPosDelta.y;
				if (lengthSquared < Io.MouseDoubleClickMaxDistance * Io.MouseDoubleClickMaxDistance)
				{
					Io.MouseState[i] = Io_State_Repeat;
				}
			}
		}

		if (Io.MouseState[i] == Io_State_Held)
		{
			Io.MouseHoldDuration[i] = Clock.FElapsedTime() - Io.MouseClickTime[i];
		}

		if (Io.MouseState[i] == Io_State_Released)
		{
			Io.MouseClickPos[i] = Io.MousePos;
			Io.MouseState[i] = Io_State_Pressed;
			Io.MousePrevClickTime[i] = Io.MouseClickTime[i];
			Io.MouseClickTime[i] = Clock.FElapsedTime();
		}
	}
}

void EngineImpl::UpdateKeyStates()
{
	if (!IsWindowFocused()) return;

	for (int32 i = 0; i < Io_Key_Max; i++)
	{
		if (!Io.Keys[i])
		{
			Io.KeyState[i] = Io_State_Released;
			continue;
		}

		if (Io.KeyState[i] == Io_State_Pressed)
		{
			if (Clock.ElapsedTime() - Io.KeyPressTime[i] >= Io.MinDurationForHold)
			{
				Io.KeyState[i] = Io_State_Held;
			}
			if (Io.KeyPressTime[i] - Io.KeyPrevPressTime[i] < Io.KeyDoubleTapTime)
			{
				Io.KeyState[i] = Io_State_Repeat;
			}
		}

		if (Io.KeyState[i] == Io_State_Held)
		{
			Io.KeyHoldDuration[i] = Clock.FElapsedTime() - Io.KeyPressTime[i];
		}

		if (Io.KeyState[i] == Io_State_Released)
		{
			Io.KeyState[i] = Io_State_Pressed;
			Io.KeyPrevPressTime[i] = Io.KeyPressTime[i];
			Io.KeyPressTime[i] = Clock.FElapsedTime();
		}
	}
}

void EngineImpl::OnEvent(const OS::Event& e)
{
	switch (e.EventType)
	{
		case OS::Event::Type::MOUSE_MOVE:
			Io.MousePos.x = static_cast<float32>(e.MouseMove.x);
			Io.MousePos.y = static_cast<float32>(e.MouseMove.y);
			break;
		case OS::Event::Type::FOCUS:
			break;
		case OS::Event::Type::MOUSE_BUTTON:
			Io.MouseButtons[e.MouseButton.Button] = e.MouseButton.Down;
			break;
		case OS::Event::Type::MOUSE_WHEEL:
			Io.MouseWheel = e.MouseWheel.Offset;
			break;
		case OS::Event::Type::WINDOW_CLOSE:
			State = AppState::Exit;
			break;
		case OS::Event::Type::WINDOW_MOVE:
			Window.PosX = e.WinMove.x;
			Window.PosY = e.WinMove.y;
			//Window.IsFullScreened = OS::IsWindowMaximized(Window.Handle);
			break;
		case OS::Event::Type::WINDOW_RESIZE:
			Window.Width = e.WinSize.Width;
			Window.Height = e.WinSize.Height;
			Window.WindowSizeChanged = true;
			//Window.IsFullScreened = OS::IsWindowMaximized(Window.Handle);
			break;
		case OS::Event::Type::KEY:
			Io.Keys[e.Key.Code] = e.Key.Down;
			break;
		case OS::Event::Type::QUIT:
			State = AppState::Exit;
			break;
		default:
			return;
	}
}

void EngineImpl::OnInit()
{
	OS::InitializeOSContext();
	OS::SetApplicationInstance(*dynamic_cast<OS::Interface*>(this));
	ReserveMemoryForSystems();

	OS::WindowCreateInformation wndInfo;
	wndInfo.Name = this->Name.C_Str();
	wndInfo.FullScreen = Window.IsFullScreened;
	wndInfo.HandleFileDrop = false;
	wndInfo.PosX = Window.PosX;
	wndInfo.PosY = Window.PosY;
	wndInfo.Width = Window.Width;
	wndInfo.Height = Window.Height;
	Window.Handle = OS::CreateAppWindow(wndInfo);

	if (Window.IsFullScreened)
	{
		OS::MaximizeWindow(Window.Handle);
	}
}

void EngineImpl::OnTerminate()
{
	OS::DestroyAppWindow(Window.Handle);
}

void EngineImpl::BeginFrame()
{
	// NOTE(Ygsm):
	// We do this because the app can be closed in the middle of the game loop;
	//if (State == AppState::Exit) { return; }

	// NOTE(Ygsm):
	// Clock needs to be reworked.
	// Should be divided into system clock and application clock as suggested in Game Engine Architecture.
	// Delta time should be the average of 5 frames.
	Clock.Tick();
	OS::ProcessEvent();
	UpdateMouseStates();
	UpdateKeyStates();
}

void EngineImpl::EndFrame()
{
	Window.WindowSizeChanged = false;
}

void EngineImpl::InitializeEngine(const EngineCreationInfo& Info)
{
	EngineImpl::InitializeGlobalContext(this);
	Clock.StartSystemClock();

	Name	= Info.AppName;
	Window.PosX = Info.StartPosX;
	Window.PosY = Info.StartPosY;
	Window.Width = Info.Width;
	Window.Height = Info.Height;
	Window.IsFullScreened = Info.FullScreen;
	State = AppState::Running;

	OnInit();

	if (Info.Multithreaded)
	{
		// Do not include the main thread.
		JobSystem.Initialize(OS::GetCPUCount() - 1);
	}
}

void EngineImpl::RegisterGame(const EngineCreationInfo& Info)
{
#if HOTRELOAD_ENABLED
	bool hotReloadable = true;
#else
	bool hotReloadable = false;
#endif
	GameCoordinator.RegisterGameDll(Info.GameModule, false, hotReloadable);
}

void EngineImpl::RunGameAndUpdateSystems()
{
	const float32 deltaTime = Clock.FTimestep();

	GameCoordinator.RunGame(deltaTime);

	for (SystemInterface* gameSystem : GameSystems)		{ gameSystem->OnUpdate(); }
	for (SystemInterface* engineSystem : EngineSystems) { engineSystem->OnUpdate(); }
}

void EngineImpl::Run()
{
	while (State != AppState::Exit)
	{
		BeginFrame();
		RunGameAndUpdateSystems();
		EndFrame();
	}
}

void EngineImpl::TerminateEngine()
{
	GameCoordinator.TerminateGame();
	OnTerminate();
	JobSystem.Shutdown();
	FreeAllSystems();
	Manager.FreeAllCaches();
}

IOSystem& EngineImpl::GetIO()
{
	return Io;
}

//void EngineImpl::KickJob(JobFunction Callback, Job::Args Args, Job* Parent)
//{
//	if (!JobSystem.Workers.Size())
//	{
//		return;
//	}
//	Job& newJob = JobSystem.CreateNewJob();
//	newJob.Callback = Callback;
//	newJob.Argument = Args;
//	newJob.Parent = Parent;
//	JobSystem.PushJob(newJob);
//}

ResourceCache& EngineImpl::CreateNewResourceCache(ResourceType Type)
{
	Manager.AddCache(Type);
	return *Manager.FetchCacheForType(Type);
}

ResourceCache* EngineImpl::FetchResourceCacheForType(ResourceType Type)
{
	if (Manager.CacheTypeExist(Type))
	{
		return Manager.FetchCacheForType(Type);
	}
	return nullptr;
}

bool EngineImpl::DeleteResourceCacheForType(ResourceType Type)
{
	if (!Manager.CacheTypeExist(Type))
	{
		return false;
	}
	Manager.RemoveCache(Type);
	return true;
}

//bool EngineImpl::IsKeyPressed(IOKeys Key)
//{
//	return Io.IsKeyPressed(Key);
//}
//
//bool EngineImpl::IsKeyDoubleTapped(IOKeys Key)
//{
//	return Io.IsKeyDoubleTapped(Key);
//}
//
//bool EngineImpl::IsKeyHeld(IOKeys Key)
//{
//	return Io.IsKeyHeld(Key);
//}
//
//bool EngineImpl::IsKeyReleased(IOKeys Key)
//{
//	return Io.IsKeyReleased(Key);
//}
//
//bool EngineImpl::IsMouseClicked(IOMouseButton Button)
//{
//	return Io.IsMouseClicked(Button);
//}
//
//bool EngineImpl::IsMouseDoubleClicked(IOMouseButton Button)
//{
//	return Io.IsMouseDoubleClicked(Button);
//}
//
//bool EngineImpl::IsMouseHeld(IOMouseButton Button)
//{
//	return Io.IsMouseHeld(Button);
//}
//
//bool EngineImpl::IsMouseReleased(IOMouseButton Button)
//{
//	return Io.IsMouseReleased(Button);
//}
//
//bool EngineImpl::IsMouseDragging(IOMouseButton Button)
//{
//	return Io.IsMouseDragging(Button);
//}
//
//bool EngineImpl::MouseDragDelta(IOMouseButton Button, vec2& Buf)
//{
//	return Io.MouseDragDelta(Button, Buf);
//}
//
//bool EngineImpl::IsCtrlPressed()
//{
//	return Io.Ctrl();
//}
//
//bool EngineImpl::IsAltPressed()
//{
//	return Io.Alt();
//}
//
//bool EngineImpl::IsShiftPressed()
//{
//	return Io.Shift();
//}

EngineImpl::CWndInfoRef EngineImpl::GetWindowInformation() const
{
	return Window;
}

bool EngineImpl::IsWindowFocused() const
{
	return Window.Handle == OS::GetFocused();
}

bool EngineImpl::HasWindowSizeChanged() const
{
	return Window.WindowSizeChanged;
}

void EngineImpl::ShowCursor(bool Show)
{
	OS::ShowCursor(Show);
}

void EngineImpl::SetMousePosition(float32 x, float32 y)
{
	OS::SetMousePos(Window.Handle, static_cast<int32>(x), static_cast<int32>(y));
}

void* EngineImpl::AllocateAndRegisterSystem(size_t Size, SystemType Type, Handle<ISystem>* Hnd)
{
	using SystemContainer = Array<SystemInterface*>*;
	SystemInterface* system = reinterpret_cast<SystemInterface*>(SystemAllocator.Malloc(Size));
	SystemContainer container = (Type == System_Engine_Type) ? &EngineSystems : &GameSystems;
	*Hnd = container->Push(system);
	return system;
}

void EngineImpl::FreeAllSystems()
{
	for (SystemInterface* engineSystem : EngineSystems) 
	{ 
		engineSystem->OnTerminate(); 
	}

	EngineSystems.Release();
	GameSystems.Release();
	SystemAllocator.Terminate();
}

SystemInterface* EngineImpl::GetRegisteredSystem(SystemType Type, Handle<ISystem> Hnd)
{
	using SystemContainer = Array<SystemInterface*>*;
	SystemContainer container = (Type == System_Engine_Type) ? &EngineSystems : &GameSystems;
	return (*container)[Hnd];
}