#pragma once
#ifndef LEARNVK_CORE_ENGINE_APPLICATION_H
#define LEARNVK_CORE_ENGINE_APPLICATION_H

#include "Platform/EngineAPI.h"
#include "Platform/Minimal.h"

struct GameAppBase
{
	GameAppBase() {};
	virtual ~GameAppBase() {};
	virtual void Initialize()			= 0;
	virtual void Run(float32 Timestep)	= 0;
	virtual void Terminate()			= 0;
};

#endif // !LEARNVK_CORE_ENGINE_APPLICATION_H