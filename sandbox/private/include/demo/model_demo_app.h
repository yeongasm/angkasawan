#pragma once
#ifndef SANDBOX_CUBE_DEMO_APP_H
#define SANDBOX_CUBE_DEMO_APP_H

#include "sandbox_demo_application.h"

namespace sandbox
{
class CubeDemoApp final : public DemoApplication
{
public:
	CubeDemoApp(
		core::wnd::window_handle rootWindowHandle,
		rhi::util::ShaderCompiler& shaderCompiler,
		rhi::Device& device,
		rhi::Swapchain& rootWindowSwapchain,
		size_t const& frameIndex
	);

	virtual ~CubeDemoApp() override;

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
	/**
	* \brief Staging buffer will be 64 MiB with 32 MiB reserved for each frame.
	*/
	rhi::Buffer m_staging_buffer;
	rhi::BufferView m_staging_buffer_frame_0;
	rhi::BufferView m_staging_buffer_frame_1;
	/**
	* \brief One giant 256_MiB buffer
	*/
	rhi::Buffer m_buffer;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
