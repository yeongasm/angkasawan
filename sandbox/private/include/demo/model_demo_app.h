#pragma once
#ifndef SANDBOX_CUBE_DEMO_APP_H
#define SANDBOX_CUBE_DEMO_APP_H

#include "sandbox_demo_application.h"
#include "geometry_cache.h"
#include "command_queue.h"
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
	struct BufferPartitions
	{
		struct Partition
		{
			size_t offset;
			size_t stride;
			size_t count;
		};
		lib::map<lib::string, Partition> partitions;
		size_t currentOffset;

		auto add_partition(lib::string&& name, size_t stride, size_t count = 1) -> bool;
		auto get_partition_buffer_view_info(lib::string const& name, size_t offset = 0) -> rhi::BufferViewInfo;
	};

	struct PushConstant
	{
		uint64 projection_address;
		uint64 view_address;
		uint64 transform_address;
	};

	BufferPartitions m_buffer_partitions;

	InputAssembler m_input_assembler;
	GeometryCache m_geometry_cache;
	SubmissionQueue m_submission_queue;
	CommandQueue m_transfer_command_queue;
	CommandQueue m_main_command_queue;

	geometry_handle m_zelda_geometry_handle;
	std::array<rhi::BufferView, 2> m_zelda_transforms;

	Camera m_camera;
	CameraState m_camera_state;
	std::array<rhi::BufferView, 2> m_camera_projection_buffer;
	std::array<rhi::BufferView, 2> m_camera_view_buffer;

	rhi::Shader m_vertex_shader;
	rhi::Shader m_pixel_shader;
	rhi::RasterPipeline m_pipeline;

	std::array<rhi::Fence, 2> m_fences;
	std::array<uint64, 2> m_fence_values;

	/**
	* \brief Staging buffer will be 64 MiB with 32 MiB reserved for each frame.
	*/
	rhi::Buffer m_staging_buffer;
	/**
	* \brief One giant 256_MiB buffer
	*/
	rhi::Buffer m_buffer;

	auto configure_buffer_partitions() -> void;
	auto initialize_camera() -> void;
	auto update_camera_state(float32 dt) -> void;
	auto update_camera_on_mouse_events(float32 dt) -> void;
	auto update_camera_on_keyboard_events(float32 dt) -> void;
	auto current_fence() -> rhi::Fence&;
};
}

#endif // !SANDBOX_CUBE_DEMO_APP_H
