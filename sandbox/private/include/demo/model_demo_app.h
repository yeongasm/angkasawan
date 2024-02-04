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
		rhi::util::ShaderCompiler& shaderCompiler,
		rhi::Device& device,
		rhi::Swapchain& rootWindowSwapchain,
		size_t const& frameIndex
	);

	virtual ~ModelDemoApp() override;

	virtual auto initialize() -> bool override;
	virtual auto run() -> void override;
	virtual auto terminate() -> void override;

private:

	struct CameraProjView
	{
		glm::mat4 projection;
		glm::mat4 view;
	};

	struct PushConstant
	{
		uint32 camera_proj_view_index;
		uint32 transform_index;
		uint32 base_color_map_index;
		//uint64 camera_proj_view_address;
		//uint64 transform_address;
	};

	ResourceCache m_resource_cache;
	SubmissionQueue m_submission_queue;
	CommandQueue m_transfer_command_queue;
	CommandQueue m_main_command_queue;
	UploadHeap m_upload_heap;
	GeometryCache m_geometry_cache;

	Resource<rhi::Image> m_depth_buffer;

	root_geometry_handle m_zelda_geometry_handle;
	root_geometry_handle m_sponza_geometry_handle;
	lib::array<Resource<rhi::Image>> m_image_store;
	rhi::Sampler m_sampler;
	
	Camera m_camera;
	CameraState m_camera_state;

	Resource<rhi::Buffer> m_zelda_vertex_buffer;
	Resource<rhi::Buffer> m_zelda_index_buffer;
	Resource<rhi::Buffer> m_sponza_vertex_buffer;
	Resource<rhi::Buffer> m_sponza_index_buffer;
	std::array<Resource<rhi::Buffer>, 2> m_zelda_transforms;
	std::array<Resource<rhi::Buffer>, 2> m_sponza_transforms;
	std::array<Resource<rhi::Buffer>, 2> m_camera_proj_view;

	rhi::Shader m_vertex_shader;
	rhi::Shader m_pixel_shader;
	rhi::RasterPipeline m_pipeline;

	std::array<rhi::Fence, 2> m_fences;
	std::array<uint64, 2> m_fence_values;

	std::array<uint32, 2> m_camera_proj_view_binding_indices;
	std::array<uint32, 2> m_zelda_transform_binding_indices;
	std::array<uint32, 2> m_sponza_transform_binding_indices;

	uint32 m_bda_binding_index;

	auto allocate_camera_buffers() -> void;
	auto release_camera_buffers() -> void;
	auto update_camera_state(float32 dt) -> void;
	auto update_camera_on_mouse_events(float32 dt) -> void;
	auto update_camera_on_keyboard_events(float32 dt) -> void;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
