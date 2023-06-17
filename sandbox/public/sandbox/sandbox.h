#include "sandbox_api.h"
#include "engine.h"
#include "renderer/renderer.h"
#include "application_window.h"

namespace sandbox
{

class Application final
{
private:

	ftl::Ref<core::Engine>	m_engine;
	ftl::Ref<gpu::Renderer>	m_renderer;
	ApplicationWindow		m_appWindow;

public:
	Application(core::Engine& engine, gpu::Renderer& renderer);
	~Application() = default;

	bool init(core::NativeWindowCreateInfo const& windowInfo);
	void terminate();

private:

	bool setup_render_pipeline();

	MAKE_SINGLETON(Application)
};

bool sandbox_on_initialize(void*);
void sandbox_on_update(void*);
void sandbox_on_terminate(void*);

}