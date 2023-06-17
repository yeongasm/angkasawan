#include "renderer.h"

EXTERN_C RENDERER_API void on_dll_load(core::Engine* pEngine, core::ModuleInfo const& info)
{
	// Get reference to module manager from engine.
	core::ModuleManager& manager = pEngine->module_manager();
	// Install the module into the system.
	auto renderer = manager.install_module<gpu::Renderer>(info.name.c_str(), info.type);
	new (renderer.get()) gpu::Renderer{ *pEngine };

	// Hook functions to engine loop stages.
	manager.register_on_initialize_fn<gpu::Renderer>(gpu::renderer_on_initialize);
	manager.register_on_update_fn<gpu::Renderer>(gpu::renderer_on_update);
	manager.register_on_terminate_fn<gpu::Renderer>(gpu::renderer_on_terminate);
}