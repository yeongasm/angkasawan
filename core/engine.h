#pragma once
#ifndef CORE_ENGINE_ENGINE_H
#define CORE_ENGINE_ENGINE_H

#include "time/system_clock.h"
#include "input/io.h"
#include "module/module_manager.h"
#include "shared/engine_common.h"
#include "windowing/windowing.h"
#include "utility/bootstrapper.h"

namespace core
{

class ENGINE_API Engine
{
public:
	Engine();
	~Engine();

	void initialize	(const EngineInitialization& init);
	void run		();
	void terminate	();

	bool engine_running() const;

	PlatformOSInterface&	platform_interface	();
	SystemClock&			system_clock		();
	WindowingManager&		windowing_manager	();
	InputIOManager&			io					();
	ModuleManager&			module_manager		();
	EngineState				set_engine_state	(EngineState state);
	EngineState				get_engine_state	() const;

private:
	PlatformOSInterface m_platform;
	SystemClock			m_clock;
	WindowingManager	m_windowingManager;
	InputIOManager		m_io;
	ModuleManager		m_moduleManager;
	EngineState			m_state;
	float32				m_dt;

	void on_update_begin		();
	void on_update				();
	void on_update_end			();
};

}

#endif // !CORE_ENGINE_ENGINE_H
