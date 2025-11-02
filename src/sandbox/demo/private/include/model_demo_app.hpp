#pragma once
#include "gpu/shared_resource.hpp"
#ifndef SANDBOX_CUBE_DEMO_APP_H
#define SANDBOX_CUBE_DEMO_APP_H

#include "core.platform/file_watcher.hpp"
#include "core.platform/application.hpp"
#include "camera.hpp"
#include "gpu/shader_compiler.hpp"
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
	gpu::Swapchain m_swapchain = {};

	struct RenderableInfo
	{
		gpu::device_address position;
		gpu::device_address normal;
		gpu::device_address uv;
		uint64 textures[3];
		uint64 sampler;
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

	struct ComputePushConstant
	{
		gpu::resource_id_t imageId;
		glm::vec2 dimension;
	};

	struct CameraProjectionView
	{
		glm::mat4 projection;
		glm::mat4 view;
	};

	gpu::Sampler m_normalSampler = {};

	gpu::Image m_depthBuffer = {};
	gpu::Image m_defaultWhiteTexture = {};
	gpu::Image m_defaultMetallicRoughnessMap = {};
	gpu::Image m_defaultNormalMap = {};

	std::array<gpu::RenderAttachment, 3> m_renderAttachments = {};
	gpu::Image m_renderablePosAttachment = {};
	gpu::Image m_baseColorAttachment = {};
	gpu::Image m_metallicRoughnessAttachment = {};
	gpu::Image m_normalAttachment = {};

	Camera m_camera = {};
	CameraState m_cameraState = {};

	lib::array<render::Image> m_images = {};
	lib::array<render::Mesh> m_meshes = {};
	lib::array<MeshRenderInfo> m_renderInfo = {};

	render::GpuPtr<glm::mat4> m_sponzaTransform = {};
	render::GpuPtr<CameraProjectionView[2]> m_cameraProjView = {};

	gpu::Pipeline m_pipeline = {};

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

	auto setup_shader_compiler_and_pipelines() -> void;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
