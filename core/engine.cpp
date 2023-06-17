#include "engine.h"
#include <chrono>
#include "fmt/chrono.h"

namespace core
{

Engine::Engine() :
	m_platform{},
	m_clock{},
	m_windowingManager{ m_platform },
	m_io{ m_platform, m_windowingManager },
	m_moduleManager{ *this },
	m_state{ EngineState::Starting },
	m_dt{ 0.f }
{}

Engine::~Engine() {};

void Engine::initialize(const EngineInitialization& init)
{
	m_state = EngineState::Running;
	m_moduleManager.initialize(init.moduleManager);

	m_platform.register_os_event_callback(
		&m_state,
		[](void* pState, OSEvent const& e) -> void {
			EngineState& state = *(static_cast<EngineState*>(pState));
			if (e.event == OSEvent::Type::Quit)
			{
				state = EngineState::Terminating;
			}
		}
	);
}

void Engine::run()
{
	while (engine_running())
	{
		on_update_begin();
		on_update();
		on_update_end();
	}
}

void Engine::terminate()
{
	m_moduleManager.shutdown();
}

bool Engine::engine_running() const
{
	return m_state == EngineState::Running;
}

PlatformOSInterface& Engine::platform_interface()
{
	return m_platform;
}

SystemClock& Engine::system_clock()
{
	return m_clock;
}

WindowingManager& Engine::windowing_manager()
{
	return m_windowingManager;
}

InputIOManager& Engine::io()
{
	return m_io;
}

ModuleManager& Engine::module_manager()
{
	return m_moduleManager;
}

EngineState Engine::set_engine_state(EngineState state)
{
	m_state = state;
	return m_state;
}

EngineState Engine::get_engine_state() const
{
	return m_state;
}

void Engine::on_update_begin()
{
	m_platform.peek_events();

	m_clock.on_tick();
	m_windowingManager.on_tick();
	m_io.on_tick(m_clock);

	m_dt = m_clock.delta_time<float32>();

	m_moduleManager.on_update_begin();
}

void Engine::on_update()
{
	m_moduleManager.on_update();
}

void Engine::on_update_end()
{
	m_moduleManager.on_update_end();
}

}