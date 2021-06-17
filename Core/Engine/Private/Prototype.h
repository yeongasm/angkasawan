#pragma once
#ifndef LEARNVK_ENGINE_H
#define LEARNVK_ENGINE_H

#include "Platform/EngineAPI.h"
#include "SubSystem/Time/SystemClock.h"
#include "SubSystem/Resource/ResourceManager.h"
#include "SubSystem/Thread/ThreadPool.h"
#include "SubSystem/Input/IO.h"
#include "SubSystem/Game/GameAppManager.h"
#include "System.h"


// NOTE(Ygsm):
// The engine should never be allowed to be in a paused state.
// The game pauses but not the engine.

// NOTE(Ygsm):
// This is turning out to be redundant...
enum AppState : uint32
{
	Exit	= 0x00,
	Running = 0x01
};


struct WindowInfo
{
	struct Position
	{
		int32 x;
		int32 y;
	};

	struct Extent2D
	{
		uint32 Width;
		uint32 Height;
	};

	WndHandle	Handle;
	Position	Pos;
	Extent2D	Extent;
	bool		IsFullScreened;
	bool		WindowSizeChanged;
	bool		IsMoving;
};

struct EngineBase : public ApplicationInterface
{
	String64			Name;
	WindowInfo			Window;
	ResourceManager		Manager;
	ThreadPool			JobSystem;
	//SystemManager		Systems;
	SystemClock			Clock;
	IOSystem			Io;
	AppState			State;
	GameManager			GameCoordinator;
};

#endif // !LEARNVK_ENGINE_H