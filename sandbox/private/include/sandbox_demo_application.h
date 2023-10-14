#pragma once
#ifndef SANDBOX_DEMO_APPLICATION_H
#define SANDBOX_DEMO_APPLICATION_H

#include "core/engine.h"
#include "rhi/rhi.h"

namespace sandbox
{
class DemoApplication : public core::Application
{
public:
	DemoApplication() = default;

	DemoApplication(
		core::wnd::window_handle rootWindowHandle, 
		rhi::Device& device, 
		rhi::Swapchain& rootWindowSwapchain,
		size_t const& frameIndex 
	) :
		m_root_app_window{ rootWindowHandle },
		m_device{ device },
		m_swapchain{ rootWindowSwapchain },
		m_frame_index{ frameIndex }
	{};

	virtual ~DemoApplication() override = default;

	virtual bool initialize() = 0;
	virtual void run() = 0;
	virtual void terminate() = 0;
protected:
	core::wnd::window_handle m_root_app_window;
	rhi::Device& m_device;
	rhi::Swapchain& m_swapchain;
	size_t const& m_frame_index;
};
}

#endif // !SANDBOX_DEMO_APPLICATION_H
