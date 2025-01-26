#pragma once
#ifndef SANDBOX_CUBE_DEMO_APP_H
#define SANDBOX_CUBE_DEMO_APP_H

#include "core.platform/file_watcher.hpp"
#include "core.platform/application.hpp"
#include "camera.hpp"
#include "render/render.hpp"

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
		render::AsyncDevice& gpu
	) -> bool;

	auto run() -> void;
	auto stop() -> void;

private:
	static inline constexpr gpu::ImageSubresource BASE_COLOR_SUBRESOURCE = { 
		.aspectFlags = gpu::ImageAspect::Color,
		.mipLevel = 0u,
		.levelCount = 1u,
		.baseArrayLayer = 0u,
		.layerCount = 1u
	};

	static inline constexpr gpu::ImageSubresource DEPTH_SUBRESOURCE = {
		.aspectFlags = gpu::ImageAspect::Depth,
		.mipLevel = 0u,
		.levelCount = 1u,
		.baseArrayLayer = 0u,
		.layerCount = 1u
	};

	core::platform::Application* m_app = {};
	render::AsyncDevice* m_gpu = {};
	core::Ref<core::platform::Window> m_rootWindowRef = {};
	gpu::swapchain m_swapchain = {};

	struct RenderableInfo
	{
		gpu::device_address position;
		gpu::device_address normal;
		gpu::device_address uv;
		uint32 textures[3];
		uint32 hasUV;
	};

	struct MeshRenderInfo
	{
		render::Mesh* mesh;
		render::Material material;
		render::GpuPtr<RenderableInfo> info;
	};

	struct PushConstant
	{
		gpu::device_address cameraProjView;
		gpu::device_address objectTransform;
		gpu::device_address renderInfo;
	};

	struct CameraProjectionView
	{
		glm::mat4 projection;
		glm::mat4 view;
	};

	gpu::sampler m_normalSampler = {};

	gpu::image m_depthBuffer = {};
	gpu::image m_defaultWhiteTexture = {};
	gpu::image m_defaultMetallicRoughnessMap = {};
	gpu::image m_defaultNormalMap = {};

	std::array<gpu::RenderAttachment, 3> m_renderAttachments = {};
	gpu::image m_baseColorAttachment = {};
	gpu::image m_metallicRoughnessAttachment = {};
	gpu::image m_normalAttachment = {};
	
	Camera m_camera = {};
	CameraState m_cameraState = {};

	lib::array<render::Image> m_images = {};
	lib::array<render::Mesh> m_meshes = {};
	lib::array<MeshRenderInfo> m_renderInfo = {};

	render::GpuPtr<glm::mat4> m_sponzaTransform = {};
	render::GpuPtr<CameraProjectionView[2]> m_cameraProjView = {};

	gpu::pipeline m_pipeline = {};
	core::filewatcher::file_watch_id m_pipelineShaderCodeWatchId = {};

	uint32 m_currentFrame = {};

	auto render() -> void;

	auto make_swapchain(core::platform::Window& window) -> void;
	auto make_render_targets(uint32 width, uint32 height) -> void;

	auto allocate_camera_buffers() -> void;
	auto update_camera_state(float32 dt) -> void;
	auto update_camera_on_mouse_events(float32 dt) -> void;
	auto update_camera_on_keyboard_events(float32 dt) -> void;

	auto unpack_sponza() -> void;
	auto unpack_materials(render::material::util::MaterialJSON const& materialRep) -> void;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
