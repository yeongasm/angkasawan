#include "engine.h"
#include "renderer/renderer.h"
#include "sandbox_api.h"

namespace sandbox
{

class ApplicationWindow
{
public:

	ApplicationWindow(gpu::Renderer& renderer, core::WindowingManager& windowingManager);

	bool						init				(core::NativeWindowCreateInfo const& info);
	void						terminate			();
	bool						on					(core::NativeWindowEvent ev, std::function<void()>&& callback);
	rhi::Viewport				get_viewport		() const;
	rhi::Extent2D				get_extent			() const;
	rhi::Extent3D				get_extent_3d		() const;
	gpu::Handle<gpu::Swapchain>	get_swapchain_handle() const;

private:
	ftl::Ref<gpu::Renderer>		m_renderer;
	core::WindowingManager*		m_windowManager;
	core::NativeWindow*			m_window;
	gpu::Handle<gpu::Swapchain> m_swapchain;
};

}