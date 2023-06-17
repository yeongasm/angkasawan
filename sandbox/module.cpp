#include "engine.h"
#include "sandbox.h"

EXTERN_C SANDBOX_API void on_dll_load(core::Engine* pEngine, core::ModuleInfo const& info)
{
	// Get reference to module manager from engine.
	core::ModuleManager& manager = pEngine->module_manager();

	// Install the module into the system.
	auto application = manager.install_module<sandbox::Application>(info.name.c_str(), info.type);
	
	auto renderer = manager.get_module<gpu::Renderer>();

	if (renderer.is_null())
	{
		return;
	}

	new (application.get()) sandbox::Application{ *pEngine, *renderer.get() };

	// Hook functions to engine loop stages.
	manager.register_on_initialize_fn<sandbox::Application>(sandbox::sandbox_on_initialize);
	manager.register_on_update_fn<sandbox::Application>(sandbox::sandbox_on_update);
	manager.register_on_terminate_fn<sandbox::Application>(sandbox::sandbox_on_terminate);
}