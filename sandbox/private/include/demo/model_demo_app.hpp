#pragma once
#ifndef SANDBOX_CUBE_DEMO_APP_H
#define SANDBOX_CUBE_DEMO_APP_H

#include "core/file_watcher.h"

#include "sandbox_demo_application.hpp"
#include "geometry_cache.hpp"
#include "camera.hpp"

namespace sandbox
{
class ModelDemoApp final : public DemoApplication
{
public:
	ModelDemoApp(
		core::wnd::window_handle rootWindowHandle,
		gpu::util::ShaderCompiler& shaderCompiler,
		GraphicsProcessingUnit& gpu,
		gpu::swapchain rootWindowSwapchain,
		size_t const& frameIndex
	);

	virtual ~ModelDemoApp() override;

	virtual auto initialize() -> bool override;
	virtual auto run() -> void override;
	virtual auto terminate() -> void override;

private:

	struct GeometryRenderInfo
	{
		Geometry const* pGeometry;
		uint32 baseColor;
	};

	struct PushConstant
	{
		uint64 vertexBufferPtr;
		uint64 modelTransformPtr;
		uint64 cameraTransformPtr;
		uint32 baseColorMapIndex;
	};

	GeometryCache m_geometryCache;

	gpu::sampler m_normalSampler = {};
	gpu::image m_depthBuffer = {};
	gpu::image m_defaultWhiteTexture = {};
	
	Camera m_camera = {};
	CameraState m_cameraState = {};

	lib::array<std::pair<gpu::image, uint32>> m_sponzaTextures = {};
	lib::array<GeometryRenderInfo> m_renderInfo = {};

	gpu::buffer m_sponzaTransform = {};
	std::array<gpu::buffer, 2> m_cameraProjView = {};

	gpu::pipeline m_pipeline = {};
	core::filewatcher::file_watch_id m_pipelineShaderCodeWatchId = {};

	root_geometry_handle m_sponza = {};

	auto allocate_camera_buffers() -> void;
	auto update_camera_state(float32 dt) -> void;
	auto update_camera_on_mouse_events(float32 dt) -> void;
	auto update_camera_on_keyboard_events(float32 dt) -> void;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
