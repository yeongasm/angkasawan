#pragma once
#ifndef SANDBOX_CUBE_DEMO_APP_H
#define SANDBOX_CUBE_DEMO_APP_H

#include "sandbox_demo_application.h"
#include "geometry_cache.h"
#include "camera.h"

namespace sandbox
{
class ModelDemoApp final : public DemoApplication
{
public:
	ModelDemoApp(
		core::wnd::window_handle rootWindowHandle,
		gpu::util::ShaderCompiler& shaderCompiler,
		gpu::Device& device,
		gpu::swapchain rootWindowSwapchain,
		size_t const& frameIndex
	);

	virtual ~ModelDemoApp() override;

	virtual auto initialize() -> bool override;
	virtual auto run() -> void override;
	virtual auto terminate() -> void override;

private:

	struct PushConstant
	{
		uint64 vertexBufferPtr;
		uint64 modelTransformPtr;
		uint64 cameraTransformPtr;
		uint32 baseColorMapIndex;
	};

	CommandQueue m_commandQueue;
	UploadHeap m_uploadHeap;
	GeometryCache m_geometryCache;

	gpu::sampler m_normalSampler = {};
	gpu::image m_depthBuffer = {};
	
	Camera m_camera = {};
	CameraState m_cameraState = {};

	lib::array<std::pair<gpu::image, uint32>> m_sponzaTextures = {};

	gpu::buffer m_sponzaTransform = {};
	std::array<gpu::buffer, 2> m_cameraProjView = {};

	gpu::pipeline m_pipeline = {};

	root_geometry_handle m_sponza = {};

	auto allocate_camera_buffers() -> void;
	auto update_camera_state(float32 dt) -> void;
	auto update_camera_on_mouse_events(float32 dt) -> void;
	auto update_camera_on_keyboard_events(float32 dt) -> void;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
