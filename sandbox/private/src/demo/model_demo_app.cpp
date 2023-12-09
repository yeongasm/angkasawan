#include <fstream>
#include "demo/model_demo_app.h"
#include "model_importer.h"
#include "image_importer.h"

namespace sandbox
{
ModelDemoApp::ModelDemoApp(
	core::wnd::window_handle rootWindowHandle,
	rhi::util::ShaderCompiler& shaderCompiler,
	rhi::Device& device,
	rhi::Swapchain& rootWindowSwapchain,
	size_t const& frameIndex
) :
	DemoApplication{ rootWindowHandle, shaderCompiler, device, rootWindowSwapchain, frameIndex },
	m_buffer_partitions{},
	m_input_assembler{},
	m_geometry_cache{ m_input_assembler },
	m_resource_cache{ m_device },
	m_buffer_view_registry{ m_resource_cache },
	m_submission_queue{ m_device },
	m_transfer_command_queue{ m_submission_queue, rhi::DeviceQueueType::Transfer },
	m_main_command_queue{ m_submission_queue, rhi::DeviceQueueType::Main },
	m_upload_heap{ m_transfer_command_queue, m_resource_cache },
	m_zelda_geometry_handle{},
	m_zelda_transforms{},
	m_camera{},
	m_camera_state{},
	m_camera_projection_buffer{},
	m_camera_view_buffer{},
	m_vertex_shader{},
	m_pixel_shader{},
	m_pipeline{},
	m_fences{},
	m_fence_values{},
	//m_staging_buffer{},
	m_buffer{}
{}

ModelDemoApp::~ModelDemoApp() {}

auto ModelDemoApp::initialize() -> bool
{
	configure_buffer_partitions();
	m_upload_heap.initialize();

	auto&& swapchain_info = m_swapchain.info();

	float32 const swapchainWidth = static_cast<float32>(swapchain_info.dimension.width);
	float32 const swapchainHeight = static_cast<float32>(swapchain_info.dimension.height);

	m_camera.far = 1000.f;
	m_camera.near = 0.1f;
	m_camera.fov = 45.f;
	m_camera.moveSpeed = 20.f;
	m_camera.sensitivity = 5.f;
	m_camera.zoomMultiplier = 1.f;
	m_camera.frameWidth = swapchainWidth;
	m_camera.frameHeight = swapchainHeight;

	auto sourceCode = open_file("data/demo/shaders/model.glsl");

	if (!sourceCode.size())
	{
		return false;
	}

	rhi::util::ShaderCompileInfo compileInfo = {
		.name = "model_vertex_shader",
		.path = "data/demo/shaders/model.glsl",
		.type = rhi::ShaderType::Vertex,
		.sourceCode = std::move(sourceCode)
	};

	{
		compileInfo.add_macro_definition("VERTEX_SHADER");

		auto result = m_shader_compiler.compile_shader(compileInfo);

		if (result.ok())
		{
			m_vertex_shader = m_device.create_shader(result.compiled_shader_info().value());
		}
		else
		{
			fmt::print("{}", result.error_msg().data());
		}
	}

	{
		compileInfo.name = "model_pixel_shader",
		compileInfo.type = rhi::ShaderType::Pixel;
		compileInfo.clear_macro_definitions();
		compileInfo.add_macro_definition("FRAGMENT_SHADER");

		auto result = m_shader_compiler.compile_shader(compileInfo);

		if (result)
		{
			m_pixel_shader = m_device.create_shader(result.compiled_shader_info().value());
		}
		else
		{
			fmt::print("{}", result.error_msg().data());
		}
	}

	if (!m_vertex_shader.valid() ||
		!m_pixel_shader.valid())
	{
		return false;
	}

	auto compiledInfo = m_shader_compiler.get_compiled_shader_info(m_vertex_shader.info().name.c_str());

	m_pipeline = m_device.create_pipeline({
		.name = "cube",
		.colorAttachments = {
			{ .format = m_swapchain.image_format(), .blendInfo = { .enable = false } }
		},
		.vertexInputBindings = {
			{ .binding = 0, .from = 0, .to = 1, .stride = sizeof(glm::vec3) + sizeof(glm::vec2) }
		},
		.rasterization = {
			.cullMode = rhi::CullingMode::None,
			.frontFace = rhi::FrontFace::Clockwise
		},
		.pushConstantSize = sizeof(PushConstant)
	},
	{
		.vertexShader = &m_vertex_shader,
		.pixelShader = &m_pixel_shader,
		.vertexInputAttributes = compiledInfo->vertexInputAttributes
	});

	for (size_t i = 0; i < m_fences.size(); ++i)
	{
		m_fences[i] = m_device.create_fence({ .name = lib::format("cpu_timeline_{}", i) });
	}

	// Create staging buffer.
	//m_staging_buffer = m_device.allocate_buffer({
	//	.name = "staging_buffer",
	//	.size = 64_MiB,
	//	.bufferUsage = rhi::BufferUsage::Transfer_Src,
	//	.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Host_Writable,
	//});

	//m_buffer = m_device.allocate_buffer({
	//	.name = "giant_buffer",
	//	.size = 256_MiB,
	//	.bufferUsage = rhi::BufferUsage::Transfer_Dst | rhi::BufferUsage::Vertex | rhi::BufferUsage::Index | rhi::BufferUsage::Storage
	//});

	m_buffer = m_resource_cache.create_buffer({
		.name = "giant_buffer",
		.size = 256_MiB,
		.bufferUsage = rhi::BufferUsage::Transfer_Dst | rhi::BufferUsage::Vertex | rhi::BufferUsage::Index | rhi::BufferUsage::Storage
	});

	// Initialize input assembler.
	{
		InputAssemblerInitInfo info{
			.buffer = m_buffer,
			.vertexBufferInfo = m_buffer_partitions.get_partition_buffer_view_info("vertex_buffer"),
			.indexBufferInfo  = m_buffer_partitions.get_partition_buffer_view_info("index_buffer")
		};

		if (!m_input_assembler.initialize(info, m_buffer_view_registry))
		{
			return false;
		}
	}

	// Initialize camera
	initialize_camera();

	GltfImporter zeldaImporter{ "data/demo/models/zelda/compressed/zelda.gltf" };
	//GltfImporter sponzaImporter{ "data/demo/models/sponza/sponza.glb" };

	// Load materials
	//for (GltfImporter::MeshInfo& meshInfo : zeldaImporter)
	//{
	//	if (!meshInfo.material)
	//	{
	//		continue;
	//	}

	//	GltfImporter::MaterialInfo& material = *meshInfo.material;
	//	
	//	using gltf_img_type_t = std::underlying_type_t<GltfImageType>;

	//	for (gltf_img_type_t i = 0; i < static_cast<gltf_img_type_t>(GltfImageType::Max); ++i)
	//	{
	//		GltfImporter::ImageInfo const* textureMapInfo = material.imageInfos[i];
	//		if (!textureMapInfo)
	//		{
	//			continue;
	//		}
	//		ImageImporter importer{ textureMapInfo->path };
	//		auto imageResource = m_resource_cache.create_image(importer.image_info());
	//		// Upload image with staging manager.
	//		m_upload_heap.upload_data_to_image({ 
	//			.image = imageResource.id(), 
	//			.data = importer.data(0).data(), 
	//			.size = importer.data(0).size_bytes(), 
	//			.mipLevel = 0
	//		});
	//	}
	//}

	auto const initialOffsets = m_input_assembler.offsets;

	//m_zelda_geometry_handle = m_geometry_cache.store_geometries(cubeImporter);
	GeometryInputLayout layout = {
		.inputs = { GeometryInput::Position, GeometryInput::TexCoord },
		.count = 2
	};
	m_zelda_geometry_handle = m_geometry_cache.store_geometries(zeldaImporter, layout);
	//std::array<rhi::BufferWriteInfo, 2> modelTransformWrites{};

	for (size_t i = 0; i < std::size(m_zelda_transforms); ++i)
	{
		auto modelTransformUBOViewInfo = m_buffer_partitions.get_partition_buffer_view_info("model_transform", i);
		m_zelda_transforms[i] = m_buffer_view_registry.create_buffer_view(modelTransformUBOViewInfo);
		//m_zelda_transforms[i].bind();

		// Hardcode transform values for now.
		glm::mat4 transform = glm::translate(glm::mat4{ 1.f }, glm::vec3{0.f, 0.f, 10.f});
		transform = glm::scale(transform, glm::vec3{ 0.05f, 0.05f, 0.05f });

		m_upload_heap.upload_data_to_buffer({
			.buffer = m_buffer.id(),
			.offset = m_zelda_transforms[i].second->bufferOffset,
			.data = &transform,
			.size = sizeof(glm::mat4),
			.dstQueue = rhi::DeviceQueueType::Main
		});
		/*modelTransformWrites[i] = m_staging_buffer.write(&transform, sizeof(glm::mat4));*/
	}

	m_geometry_cache.stage_geometries_for_upload(m_upload_heap);
	//auto stagingUploads = m_geometry_cache.stage_geometries_for_upload(m_staging_buffer);

	auto vbh = m_buffer_view_registry.get_source_handle(m_input_assembler.vertexBuffer.first);
	auto ibh = m_buffer_view_registry.get_source_handle(m_input_assembler.indexBuffer.first);

	auto vertex_buffer = m_resource_cache.get_buffer(vbh);
	auto index_buffer  = m_resource_cache.get_buffer(ibh);

	//rhi::Fence& fence = current_fence();

	FenceInfo fenceInfo = m_upload_heap.send_to_gpu();

	{
		//auto cmd = m_transfer_command_queue.next_free_command_buffer(std::this_thread::get_id());

		//cmd->begin();

		//for (size_t i = 0; i < std::size(modelTransformWrites); ++i)
		//{
		//	rhi::BufferCopyInfo info{
		//		.srcOffset = modelTransformWrites[i].offset,
		//		.dstOffset = m_zelda_transforms[i].offset_from_buffer(),
		//		.size = modelTransformWrites[i].size
		//	};
		//	cmd->copy_buffer_to_buffer(m_staging_buffer, m_buffer, info);
		//}

		//rhi::BufferCopyInfo copyInfo{
		//	.srcOffset = stagingUploads.vertices.offset,
		//	.dstOffset = initialOffsets.vertex,
		//	.size = stagingUploads.vertices.size
		//};
		//cmd->copy_buffer_to_buffer(m_staging_buffer, m_buffer, copyInfo);

		//copyInfo.srcOffset = stagingUploads.indices.offset;
		//copyInfo.dstOffset = initialOffsets.index;
		//copyInfo.size = stagingUploads.indices.size;

		//cmd->copy_buffer_to_buffer(m_staging_buffer, m_buffer, copyInfo);

		//// Release resources ...

		//cmd->pipeline_barrier(
		//	vertex_buffer,
		//	{
		//		.srcAccess = rhi::access::TRANSFER_WRITE,
		//		.srcQueue = rhi::DeviceQueueType::Transfer,
		//		.dstQueue = rhi::DeviceQueueType::Main
		//	}
		//);

		//cmd->pipeline_barrier(
		//	index_buffer,
		//	{
		//		.srcAccess = rhi::access::TRANSFER_WRITE,
		//		.srcQueue = rhi::DeviceQueueType::Transfer,
		//		.dstQueue = rhi::DeviceQueueType::Main
		//	}
		//);

		//for (size_t i = 0; i < std::size(m_zelda_transforms); ++i)
		//{
		//	cmd->pipeline_barrier(
		//		m_zelda_transforms[i],
		//		{
		//			.srcAccess = rhi::access::TRANSFER_WRITE,
		//			.srcQueue = rhi::DeviceQueueType::Transfer,
		//			.dstQueue = rhi::DeviceQueueType::Main
		//		}
		//	);
		//}

		//cmd->end();

		//auto submissionGroup = m_submission_queue.new_submission_group(rhi::DeviceQueueType::Transfer);
		//submissionGroup.submit_command_buffer(*cmd);
		//submissionGroup.signal_fence(fence, ++m_fence_values[m_frame_index]);
	}

	{
		// Acquire resources ...
		auto cmd = m_main_command_queue.next_free_command_buffer(std::this_thread::get_id());

		cmd->begin();

		cmd->pipeline_barrier(
			*m_buffer,
			{
				.dstAccess = rhi::access::TRANSFER_WRITE,
				.srcQueue = rhi::DeviceQueueType::Transfer,
				.dstQueue = rhi::DeviceQueueType::Main
			}
		);

		//cmd->pipeline_barrier(
		//	*index_buffer,
		//	{
		//		.size = m_input_assembler.indexBuffer.second->size,
		//		.offset = m_input_assembler.indexBuffer.second->bufferOffset,
		//		.dstAccess = rhi::access::TRANSFER_WRITE,
		//		.srcQueue = rhi::DeviceQueueType::Transfer,
		//		.dstQueue = rhi::DeviceQueueType::Main
		//	}
		//);

		//for (size_t i = 0; i < std::size(m_zelda_transforms); ++i)
		//{
		//	cmd->pipeline_barrier(
		//		,
		//		{
		//			.dstAccess = rhi::access::TRANSFER_WRITE,
		//			.srcQueue = rhi::DeviceQueueType::Transfer,
		//			.dstQueue = rhi::DeviceQueueType::Main
		//		}
		//	);
		//}

		cmd->pipeline_barrier({
			.srcAccess = rhi::access::TRANSFER_WRITE,
			.dstAccess = rhi::access::HOST_READ
		});

		cmd->end();

		auto submissionGroup = m_main_command_queue.new_submission_group();
		submissionGroup.submit_command_buffer(*cmd);
		submissionGroup.wait_on_fence(*fenceInfo.fence, fenceInfo.value);
	}

	m_submission_queue.send_to_gpu_main_submissions();

	return true;
}

auto ModelDemoApp::run() -> void
{
	auto dt = core::stat::delta_time_f();

	bool val = core::io::key_held(core::IOKey::A);

	fmt::print("A key held -> {}\n", val);

	m_device.clear_destroyed_resources();
	update_camera_state(dt);

	[[maybe_unused]] Geometry const* zeldaGeometry = &m_geometry_cache.get_geometry(m_zelda_geometry_handle);

	auto cmd = m_main_command_queue.next_free_command_buffer(std::this_thread::get_id());
	auto&& swapchainImage = m_swapchain.acquire_next_image();

	auto& [_, projUBO] = m_camera_projection_buffer[m_frame_index];
	auto& [_, viewUBO] = m_camera_view_buffer[m_frame_index];
	auto& [_, modelTransformUBO] = m_zelda_transforms[m_frame_index];

	PushConstant pc{
		.projection_address = projUBO->gpuAddress,
		.view_address = viewUBO->gpuAddress,
		.transform_address = modelTransformUBO->gpuAddress,
	};

	cmd->reset();
	cmd->begin();

	if (viewUBO.owner() != rhi::DeviceQueueType::Main)
	{
		cmd->pipeline_barrier(
			viewUBO,
			{
				.dstAccess = rhi::access::TRANSFER_WRITE,
				.srcQueue = rhi::DeviceQueueType::Transfer,
				.dstQueue = rhi::DeviceQueueType::Main
			}
		);
	}

	if (projUBO.owner() != rhi::DeviceQueueType::Main)
	{
		cmd->pipeline_barrier(
			projUBO,
			{
				.dstAccess = rhi::access::TRANSFER_WRITE,
				.srcQueue = rhi::DeviceQueueType::Transfer,
				.dstQueue = rhi::DeviceQueueType::Main
			}
		);
	}

	cmd->pipeline_barrier(
		{
			.srcAccess = rhi::access::TRANSFER_WRITE,
			.dstAccess = rhi::access::HOST_READ
		}
	);

	cmd->pipeline_barrier(
		swapchainImage,
		{
			.dstAccess = rhi::access::FRAGMENT_SHADER_READ,
			.oldLayout = rhi::ImageLayout::Undefined,
			.newLayout = rhi::ImageLayout::Color_Attachment
		}
	);

	rhi::RenderAttachment swapchainImageAttachment{
		.pImage = &swapchainImage,
		.imageLayout = rhi::ImageLayout::Color_Attachment,
		.loadOp = rhi::AttachmentLoadOp::Clear,
		.storeOp = rhi::AttachmentStoreOp::Store
	};

	rhi::RenderingInfo info{
		.colorAttachments = std::span{ &swapchainImageAttachment, 1 },
		.renderArea = {
			.extent = {
				.width = swapchainImage.info().dimension.width,
				.height = swapchainImage.info().dimension.height
			}
		}
	};

	auto const& swapchain_info = swapchainImage.info();

	float32 const width  = static_cast<float32>(swapchain_info.dimension.width);
	float32 const height = static_cast<float32>(swapchain_info.dimension.height);

	cmd->begin_rendering(info);
	cmd->set_viewport({
		.width = width,
		.height = height
	});
	cmd->set_scissor({
		.extent = {
			.width = swapchainImage.info().dimension.width,
			.height = swapchainImage.info().dimension.height
		}
	});
	cmd->bind_pipeline(m_pipeline);
	cmd->bind_vertex_buffer(m_input_assembler.vertexBufferView);
	cmd->bind_index_buffer(m_input_assembler.indexBufferView);

	cmd->bind_push_constant(pc);

	while (zeldaGeometry)
	{
		cmd->draw_indexed({
			.indexCount = zeldaGeometry->indices.count,
			.firstIndex = zeldaGeometry->indices.offset,
			.vertexOffset = zeldaGeometry->vertices.offset
		});
		zeldaGeometry = zeldaGeometry->next;
	}

	cmd->end_rendering();

	cmd->pipeline_barrier(
		swapchainImage,
		{
			.srcAccess = rhi::access::FRAGMENT_SHADER_WRITE,
			.dstAccess = rhi::access::TRANSFER_READ,
			.oldLayout = rhi::ImageLayout::Color_Attachment,
			.newLayout = rhi::ImageLayout::Present_Src
		}
	);

	cmd->pipeline_barrier(
		viewUBO,
		{
			.srcAccess = rhi::access::TRANSFER_WRITE,
			.srcQueue = rhi::DeviceQueueType::Main,
			.dstQueue = rhi::DeviceQueueType::Transfer
		}
	);

	cmd->end();

	/*rhi::Fence& fence = current_fence();*/
	auto uploadFenceInfo = m_upload_heap.send_to_gpu();

	auto submitGroup = m_submission_queue.new_submission_group();
	submitGroup.submit_command_buffer(*cmd);
	submitGroup.wait_on_semaphore(m_swapchain.current_acquire_semaphore());
	submitGroup.signal_semaphore(m_swapchain.current_present_semaphore());
	submitGroup.wait_on_fence(*uploadFenceInfo.fence, uploadFenceInfo.value);
	/*submitGroup.wait_on_fence(fence, m_fence_values[m_frame_index]);*/
	submitGroup.signal_fence(m_swapchain.get_gpu_fence(), m_swapchain.cpu_frame_count());

	m_submission_queue.send_to_gpu();

	m_device.present({ .swapchains = std::span{ &m_swapchain, 1 } });

	m_submission_queue.clear();
	//m_staging_buffer.flush();
}

auto ModelDemoApp::terminate() -> void
{
	m_upload_heap.terminate();
	m_transfer_command_queue.terminate();
	m_main_command_queue.terminate();

	/*m_device.release_buffer(m_staging_buffer);*/
	m_device.release_buffer(*m_buffer);
	
	for (rhi::Fence& fence : m_fences)
	{
		m_device.destroy_fence(fence);
	}

	m_device.destroy_shader(m_vertex_shader);
	m_device.destroy_shader(m_pixel_shader);
	m_device.destroy_pipeline(m_pipeline);
}

auto ModelDemoApp::BufferPartitions::add_partition(lib::string&& name, size_t stride, size_t count) -> bool
{
	if (partitions.contains(name))
	{
		return false;
	}

	count = std::max(count, size_t{ 1 });

	partitions.emplace(std::move(name), Partition{ .offset = currentOffset, .stride = stride, .count = count });
	currentOffset += (stride * count);

	return true;
}

auto ModelDemoApp::BufferPartitions::get_partition_buffer_view_info(lib::string const& name, size_t offset) -> BufferViewInfo
{
	if (!partitions.contains(name))
	{
		ASSERTION(false && "Requested buffer view info does not exist!");
		return BufferViewInfo{};
	}
	auto&& partition = partitions[name];
	if (offset > partition.count)
	{
		offset = 0;
	}
	return BufferViewInfo{ .offset = partition.offset + (partition.stride * offset), .size = partition.stride };
}

auto ModelDemoApp::configure_buffer_partitions() -> void
{
	m_buffer_partitions.add_partition("vertex_buffer", 64_MiB);
	m_buffer_partitions.add_partition("index_buffer", 16_MiB);
	m_buffer_partitions.add_partition("camera_projection", sizeof(glm::mat4), 2);
	m_buffer_partitions.add_partition("camera_view", sizeof(glm::mat4), 2);
	m_buffer_partitions.add_partition("model_transform", sizeof(glm::mat4), 2);
}

auto ModelDemoApp::initialize_camera() -> void
{
	// Create buffer views from the big buffer.
	// 2 buffer views for projection (frame 0, frame 1).
	for (size_t i = 0; i < std::size(m_camera_projection_buffer); ++i)
	{
		auto info = m_buffer_partitions.get_partition_buffer_view_info("camera_projection", i);
		m_camera_projection_buffer[i] = m_buffer_view_registry.create_buffer_view(info);
		//m_camera_projection_buffer[i] = m_buffer.make_view(info);
		// We bind the buffer view to make them accessible within shaders.
		//m_camera_projection_buffer[i].bind();
	}

	for (size_t i = 0; i < std::size(m_camera_view_buffer); ++i)
	{
		auto info = m_buffer_partitions.get_partition_buffer_view_info("camera_view", i);
		m_camera_view_buffer[i] = m_buffer_view_registry.create_buffer_view(info);
		//m_camera_view_buffer[i].bind();
	}
}

auto ModelDemoApp::update_camera_state(float32 dt) -> void
{
	core::Engine& engine = core::engine();

	m_camera_state.dirty = false;

	if (engine.is_window_focused(m_root_app_window))
	{
		update_camera_on_mouse_events(dt);
		update_camera_on_keyboard_events(dt);
	}

	m_camera.update_projection();
	m_camera.update_view();

	auto& [_, projUBO] = m_camera_projection_buffer[m_frame_index];
	auto& [_, viewUBO] = m_camera_view_buffer[m_frame_index];

	m_upload_heap.upload_data_to_buffer({
		.buffer = m_buffer.id(),
		.offset = projUBO->bufferOffset,
		.data = &m_camera.projection,
		.size = sizeof(glm::mat4),
		.dstQueue = rhi::DeviceQueueType::Main
	});

	m_upload_heap.upload_data_to_buffer({
		.buffer = m_buffer.id(),
		.offset = viewUBO->bufferOffset,
		.data = &m_camera.view,
		.size = sizeof(glm::mat4),
		.dstQueue = rhi::DeviceQueueType::Main
	});

	//auto const projWriteInfo = m_staging_buffer.write(&m_camera.projection, sizeof(glm::mat4));
	//auto const viewWriteInfo = m_staging_buffer.write(&m_camera.view, sizeof(glm::mat4));

	//rhi::Fence& fence = current_fence();


	//auto cmd = m_transfer_command_queue.next_free_command_buffer(std::this_thread::get_id());

	//cmd->reset();
	//cmd->begin();

	// Acquire the resource.
	//if (viewUBO.owner() != rhi::DeviceQueueType::Transfer &&
	//	viewUBO.owner() != rhi::DeviceQueueType::None)
	//{
	//	cmd->pipeline_barrier(
	//		viewUBO,
	//		{
	//			.dstAccess = rhi::access::TOP_OF_PIPE_NONE,
	//			.srcQueue = viewUBO.owner(),
	//			.dstQueue = rhi::DeviceQueueType::Transfer
	//		}
	//	);

	//	cmd->pipeline_barrier(
	//		projUBO,
	//		{
	//			.dstAccess = rhi::access::TOP_OF_PIPE_NONE,
	//			.srcQueue = viewUBO.owner(),
	//			.dstQueue = rhi::DeviceQueueType::Transfer
	//		}
	//	);

	//	cmd->pipeline_barrier({
	//		.srcAccess = rhi::access::TOP_OF_PIPE_NONE,
	//		.dstAccess = rhi::access::TRANSFER_WRITE
	//	});
	//}

	{
		//rhi::BufferCopyInfo copyInfo{
		//	.srcOffset = viewWriteInfo.offset,
		//	.dstOffset = viewUBO.offset_from_buffer(),
		//	.size = viewWriteInfo.size
		//};
		//cmd->copy_buffer_to_buffer(m_staging_buffer, viewUBO.buffer(), copyInfo);
	}

	{
		//rhi::BufferCopyInfo copyInfo{
		//	.srcOffset = projWriteInfo.offset,
		//	.dstOffset = projUBO.offset_from_buffer(),
		//	.size = projWriteInfo.size
		//};
		//cmd->copy_buffer_to_buffer(m_staging_buffer, projUBO.buffer(), copyInfo);
	}

	//cmd->pipeline_barrier(
	//	viewUBO,
	//	{
	//		.srcAccess = rhi::access::TRANSFER_WRITE,
	//		.srcQueue = rhi::DeviceQueueType::Transfer,
	//		.dstQueue = rhi::DeviceQueueType::Main
	//	}
	//);

	//cmd->pipeline_barrier(
	//	projUBO,
	//	{
	//		.srcAccess = rhi::access::TRANSFER_WRITE,
	//		.srcQueue = rhi::DeviceQueueType::Transfer,
	//		.dstQueue = rhi::DeviceQueueType::Main
	//	}
	//);

	//cmd->end();

	//auto submitGroup = m_submission_queue.new_submission_group(rhi::DeviceQueueType::Transfer);
	//submitGroup.submit_command_buffer(*cmd);
	//submitGroup.wait_on_fence(fence, m_fence_values[m_frame_index]);
	//submitGroup.signal_fence(fence, ++m_fence_values[m_frame_index]);
}

auto ModelDemoApp::update_camera_on_mouse_events(float32 dt) -> void
{
	core::Engine& engine = core::engine();

	core::Point const pos = core::io::mouse_position();
	core::Point const windowPos = engine.get_window_position(m_root_app_window);

	/**
	* "dragging" acts a multiplier to the computed delta that is set to 1 when the mouse is dragging.
	* 
	* Although we've explicitly set the cursor's position to the centre of the window, the action
	* is not preseved when a new frame starts. The value of the mouse position is constantly being
	* updated in the event loop.
	* 
	* Because of that, the delta is always large even during first move (since mouse pos is only set to
	* the camera's centre on first move). Introducing the multiplier "dragging" and using it in the way
	* described above sovles the issue.
	*/
	int32 dragging = 0;

	/**
	* Not really used at the moment. Reserved just in case we support multiple viewports in the future.
	*/
	glm::vec2 originPos = {};
	glm::vec2 mousePos = { static_cast<float32>(pos.x), static_cast<float32>(pos.y) };
	glm::vec2 const cameraCentre = glm::vec2{ (originPos.x + m_camera.frameWidth) * 0.5f, (originPos.y + m_camera.frameHeight) * 0.5f };

	bool const cursorInViewport = (mousePos.x >= 0.f && mousePos.x <= m_camera.frameWidth) && (mousePos.y >= 0.f && mousePos.y <= m_camera.frameHeight);

	m_camera_state.mode = CameraMouseMode::None;

	float32 const mouseWheelV = core::io::mouse_wheel_v();

	/**
	* Only allow zooming when the cursor is in the viewport.
	*/
	if (cursorInViewport && mouseWheelV != 0.f)
	{
		m_camera.zoom(mouseWheelV);
		m_camera_state.dirty = true;
	}

	if (core::io::mouse_held(core::IOMouseButton::Right))
	{
		m_camera_state.mode = CameraMouseMode::Pan_And_Tilt;
	}
	else if (core::io::mouse_held(core::IOMouseButton::Left))
	{
		m_camera_state.mode = CameraMouseMode::Pan_And_Dolly;
	}

	if (m_camera_state.mode == CameraMouseMode::None)
	{
		if (!m_camera_state.firstMove)
		{
			int32 x = static_cast<int32>(m_camera_state.capturedMousePos.x);
			int32 y = static_cast<int32>(m_camera_state.capturedMousePos.y);

			engine.show_cursor();
			engine.set_cursor_position(m_root_app_window, core::Point{ x, y });

			m_camera_state.capturedMousePos = glm::vec2{ 0.f };
			m_camera_state.firstMove = true;
		}
		return;
	}
	else
	{
		if (core::io::mouse_dragging(core::IOMouseButton::Left) || core::io::mouse_dragging(core::IOMouseButton::Right))
		{
			dragging = 1;
		}
	}

	if (m_camera_state.firstMove)
	{
		m_camera_state.capturedMousePos = mousePos;
		m_camera_state.firstMove = false;
		mousePos = cameraCentre;
	}

	int32 const cx = static_cast<int32>(cameraCentre.x);
	int32 const cy = static_cast<int32>(cameraCentre.y);

	engine.show_cursor(false);
	engine.set_cursor_position(m_root_app_window, core::Point{ cx, cy });

	glm::vec2 delta = mousePos - cameraCentre;
	glm::vec2 const magnitude = glm::abs(delta);

	if (magnitude.x < Camera::DELTA_EPSILON.min.x)
	{
		delta.x = 0.f;
	}

	if (magnitude.y < Camera::DELTA_EPSILON.min.y)
	{
		delta.y = 0.f;
	}

	delta *= static_cast<float32>(dragging);

	switch (m_camera_state.mode)
	{
	case CameraMouseMode::Pan_And_Tilt:
		m_camera.rotate(glm::vec3{ delta.x, -delta.y, 0.f, }, dt);
		m_camera_state.dirty = true;
		break;
	case CameraMouseMode::Pan_And_Dolly:
		// Mouse moves downwards.
		if (delta.y >  Camera::DELTA_EPSILON.min.y) { m_camera.translate(-m_camera.get_forward_vector(), dt); }
		// Mouse moves upwards.
		if (delta.y < -Camera::DELTA_EPSILON.min.y) { m_camera.translate( m_camera.get_forward_vector(), dt); }
		m_camera.rotate(glm::vec3{ delta.x, 0.f, 0.f }, dt);
		m_camera_state.dirty = true;
		break;
	default:
		break;
	}
}

auto ModelDemoApp::update_camera_on_keyboard_events(float32 dt) -> void
{
	glm::vec3 const UP		= m_camera.get_up_vector();
	glm::vec3 const RIGHT	= m_camera.get_right_vector();
	glm::vec3 const FORWARD = m_camera.get_forward_vector();

	if (core::io::key_held(core::IOKey::Q))
	{
		m_camera.translate(-UP, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_held(core::IOKey::W))
	{
		m_camera.translate(FORWARD, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_held(core::IOKey::E))
	{
		m_camera.translate(UP, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_held(core::IOKey::A))
	{
		m_camera.translate(-RIGHT, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_held(core::IOKey::S))
	{
		m_camera.translate(-FORWARD, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_held(core::IOKey::D))
	{
		m_camera.translate(RIGHT, dt);
		m_camera_state.dirty = true;
	}
}

auto ModelDemoApp::current_fence() -> rhi::Fence&
{
	return m_fences[m_frame_index];
}

}