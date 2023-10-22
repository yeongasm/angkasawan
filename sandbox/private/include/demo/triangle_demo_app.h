#pragma once
#ifndef SANDBOX_TRIANGLE_DEMO_APP_H
#define SANDBOX_TRIANGLE_DEMO_APP_H

#include "sandbox_demo_application.h"

namespace sandbox
{
class TriangleDemoApp final : public DemoApplication
{
public:
	TriangleDemoApp(
		core::wnd::window_handle rootWindowHandle,
		rhi::util::ShaderCompiler& shaderCompiler,
		rhi::Device& device,
		rhi::Swapchain& rootWindowSwapchain,
		size_t const& frameIndex
	);

	virtual ~TriangleDemoApp() override;

	virtual auto initialize() -> bool override;
	virtual auto run() -> void override;
	virtual auto terminate() -> void override;
private:
	rhi::Shader m_vertex_shader;
	rhi::Shader m_pixel_shader;
	rhi::RasterPipeline m_pipeline;
	rhi::CommandPool m_cmd_pool;
	rhi::CommandBuffer* m_cmd_buffer_0;
	rhi::CommandBuffer* m_cmd_buffer_1;
};
}

#endif // !SANDBOX_TRIANGLE_DEMO_APP_H
