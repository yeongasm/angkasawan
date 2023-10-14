#include "engine.h"

namespace core
{

namespace io
{
extern IOContext gIOCtx;
}

namespace stat
{

extern void update_stat_timings();

}

static Engine* gpEngineInst = nullptr;

Engine::Engine() :
	mEventQueue{},
	mWindowContext{},
	mState{},
	mApp{ nullptr }
{}

Engine::~Engine() {};

bool Engine::initialize([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	mState = EngineState::Starting;
	io::IOContext& io = io::gIOCtx;
	mEventQueue.register_event_listener_callback(
		[this, &io](OSEvent const& ev) -> void
		{
			if (ev.event == OSEvent::Type::Quit)
			{
				mState = EngineState::Terminating;
			}

			io.listen_to_event(ev);
			mWindowContext.listen_to_event(ev);

			auto [whnd, pWindow] = mWindowContext.window_from_native_handle(ev.window);
			io.enableStateUpdate = (pWindow && pWindow->focused && !is_window_minimized(whnd));
		}
	);
	return true;
}

void Engine::run()
{
	mState = EngineState::Running;

	// Main engine loop.
	while (running())
	{
		// Update the clock timings such as frame time and delta time.
		stat::update_stat_timings();
		// Look into the OS's message loop and trigger the callback for all registered listeners.
		mEventQueue.peek_events();
		// Update I/O state of the application.
		io::gIOCtx.update_state();
		// Update state of windows that are created by the application. Destroy if need be.
		mWindowContext.on_tick();
		//check_if_engine_should_terminate();
		if (mApp) [[likely]]
		{
			mApp->run();
		}
	}
}

void Engine::terminate()
{
	if (mApp) [[likely]]
	{
		mApp->terminate();
	}
	mWindowContext.destroy_windows();
}

bool Engine::running() const
{
	return mState == EngineState::Running;
}

EngineState Engine::set_state(EngineState state)
{
	mState = state;
	return mState;
}

EngineState Engine::get_state() const
{
	return mState;
}

CORE_API void set_engine_instance(Engine& engine)
{
	if (!gpEngineInst)
	{
		gpEngineInst = &engine;
	}
}

#pragma warning(push)
#pragma warning(disable: 6011)
Engine& engine()
{
	ASSERTION(gpEngineInst != nullptr && "Fatal Error. Engine instance is not set!");
	return *gpEngineInst;
}
#pragma warning(pop)

}