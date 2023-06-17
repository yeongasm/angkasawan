#include "application_window.h"

namespace sandbox
{

ApplicationWindow::ApplicationWindow(gpu::Renderer& renderer, core::WindowingManager& windowingManager) :
	m_renderer{ &renderer },
	m_windowManager{ &windowingManager },
	m_window{},
	m_swapchain{}
{}

bool ApplicationWindow::init(core::NativeWindowCreateInfo const& info)
{
	auto res0 = m_windowManager->create_window(info, true);
	if (res0.status != core::ErrorCode::Ok)
	{
		return false;
	}

	auto res1 = m_windowManager->get_window(res0.payload);
	if (res1.status != core::ErrorCode::Ok)
	{
		return false;
	}

	m_window = res1.payload;

	rhi::SurfaceInfo surfaceInfo{
		.instance	= m_window->get_application_handle(),
		.window		= m_window->get_raw_handle()
	};

	core::Dimension const windowDim = m_window->get_dimension();

	rhi::SwapchainInfo swapchainInfo{
		.dimension			= { 
			.width = static_cast<uint32>(windowDim.width), 
			.height = static_cast<uint32>(windowDim.height) 
		},
		.imageCount			= 2,
		.imageUsage			= rhi::ImageUsage::Color_Attachment | rhi::ImageUsage::Transfer_Dst,
		.presentationMode	= rhi::SwapchainPresentMode::Mailbox
	};

	m_swapchain = m_renderer->create_swapchain(swapchainInfo, surfaceInfo);

	if (m_swapchain == gpu::resource_invalid_handle_v<gpu::Swapchain>)
	{
		m_window->destroy();
		m_window = nullptr;

		return false;
	}

	return true;
}

void ApplicationWindow::terminate()
{
	if (m_swapchain != gpu::resource_invalid_handle_v<gpu::Swapchain>)
	{
		m_renderer->destroy_swapchain(m_swapchain);
	}
}

bool ApplicationWindow::on(core::NativeWindowEvent ev, std::function<void()>&& callback)
{
	return m_window->on(ev, std::move(callback));
}

rhi::Viewport ApplicationWindow::get_viewport() const
{
	core::Point const pos = m_window->get_position();
	core::Dimension const dim = m_window->get_dimension();

	return { 
		.x			= static_cast<float32>(pos.x), 
		.y			= static_cast<float32>(pos.y), 
		.width		= static_cast<float32>(dim.width), 
		.height		= static_cast<float32>(dim.height), 
		.minDepth	= 0.f, 
		.maxDepth	= 1.f 
	};
}

rhi::Extent2D ApplicationWindow::get_extent() const
{
	core::Dimension const dim = m_window->get_dimension();

	return {
		.width	= static_cast<uint32>(dim.width),
		.height = static_cast<uint32>(dim.height)
	};
}

gpu::Handle<gpu::Swapchain> ApplicationWindow::get_swapchain_handle() const
{
	return m_swapchain;
}

}