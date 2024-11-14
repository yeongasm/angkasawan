#include "angkasawan.h"
#include "sandbox/sandbox.hpp"

namespace core
{
	CORE_API void set_engine_instance(Engine& engine);
}

bool terminate_app_check(core::Engine const& engine)
{
	return engine.num_windows() == 0;
}

Angkasawan::Angkasawan() :
	mEngine{}
{
	core::set_engine_instance(mEngine);
}

Angkasawan::~Angkasawan() {}

bool Angkasawan::start([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	if (!mEngine.initialize(argc, argv))
	{
		return false;
	}
	if (!mEngine.register_application<sandbox::SandboxApp>())
	{
		return false;
	}
	return true;
}

void Angkasawan::run()
{
	mEngine.run();
}

void Angkasawan::stop()
{
	mEngine.terminate();
}