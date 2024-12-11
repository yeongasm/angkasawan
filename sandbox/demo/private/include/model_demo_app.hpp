#pragma once
#ifndef SANDBOX_CUBE_DEMO_APP_H
#define SANDBOX_CUBE_DEMO_APP_H

#include "core.platform/file_watcher.hpp"
#include "core.platform/application.hpp"
#include "graphics_processing_unit.hpp"
#include "geometry_cache.hpp"
#include "camera.hpp"

namespace sandbox
{
class ModelDemoApp final
{
public:
	ModelDemoApp() = default;
	~ModelDemoApp() = default;

	auto start(
		core::platform::Application& application,
		core::Ref<core::platform::Window> rootWindow,
		GraphicsProcessingUnit& gpu
	) -> bool;

	auto run() -> void;
	auto stop() -> void;

private:
	core::platform::Application* m_app = {};
	GraphicsProcessingUnit* m_gpu = {};
	core::Ref<core::platform::Window> m_rootWindowRef = {};
	gpu::swapchain m_swapchain = {};

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

	GeometryCache m_geometryCache = {};

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
	uint32 m_currentFrame = {};

	auto render() -> void;

	auto make_swapchain(core::platform::Window& window) -> void;
	auto make_render_targets(uint32 width, uint32 height) -> void;

	auto allocate_camera_buffers() -> void;
	auto update_camera_state(float32 dt) -> void;
	auto update_camera_on_mouse_events(float32 dt) -> void;
	auto update_camera_on_keyboard_events(float32 dt) -> void;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
