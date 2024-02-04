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
	m_resource_cache{ m_device },
	m_submission_queue{ m_device },
	m_transfer_command_queue{ m_submission_queue, rhi::DeviceQueueType::Transfer },
	m_main_command_queue{ m_submission_queue, rhi::DeviceQueueType::Main },
	m_upload_heap{ m_transfer_command_queue, m_resource_cache },
	m_geometry_cache{ m_upload_heap },
	m_depth_buffer{},
	m_zelda_geometry_handle{},
	m_sponza_geometry_handle{},
	m_camera{},
	m_camera_state{},
	m_zelda_vertex_buffer{},
	m_zelda_index_buffer{},
	m_sponza_vertex_buffer{},
	m_sponza_index_buffer{},
	m_zelda_transforms{},
	m_sponza_transforms{},
	m_camera_proj_view{},
	m_vertex_shader{},
	m_pixel_shader{},
	m_pipeline{},
	m_fences{},
	m_fence_values{},
	m_camera_proj_view_binding_indices{},
	m_zelda_transform_binding_indices{},
	m_sponza_transform_binding_indices{},
	m_bda_binding_index{}
{}

ModelDemoApp::~ModelDemoApp() {}

auto ModelDemoApp::initialize() -> bool
{
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
		.name = "model_render",
		.colorAttachments = {
			{.format = m_swapchain.image_format(), .blendInfo = {.enable = true } }
		},
		.depthAttachmentFormat = rhi::Format::D32_Float,
		.vertexInputBindings = {
			{.binding = 0, .from = 0, .to = 1, .stride = sizeof(glm::vec3) + sizeof(glm::vec2) }
		},
		.rasterization = {
			.cullMode = rhi::CullingMode::Back,
			.frontFace = rhi::FrontFace::Counter_Clockwise
		},
		.depthTest = {
			.depthTestCompareOp = rhi::CompareOp::Less,
			.enableDepthTest = true,
			.enableDepthWrite = true
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

	allocate_camera_buffers();

	m_sampler = m_device.create_sampler({
		.name = "normal_sampler",
		.minFilter = rhi::TexelFilter::Linear,
		.magFilter = rhi::TexelFilter::Linear,
		.mipmapMode = rhi::MipmapMode::Linear,
		.addressModeU = rhi::SamplerAddress::Repeat,
		.addressModeV = rhi::SamplerAddress::Repeat,
		.addressModeW = rhi::SamplerAddress::Repeat,
		.mipLodBias = 0.f,
		.maxAnisotropy = 0.f,
		.compareOp = rhi::CompareOp::Never,
		.minLod = 0.f,
		.maxLod = 0.f,
		.borderColor = rhi::BorderColor::Float_Transparent_Black,
	});

	m_device.update_sampler_descriptor({ .sampler = m_sampler, .index = 0 });

	m_depth_buffer = m_resource_cache.create_image({
		.name = "depth_buffer",
		.type = rhi::ImageType::Image_2D,
		.format = rhi::Format::D32_Float,
		.samples = rhi::SampleCount::Sample_Count_1,
		.tiling = rhi::ImageTiling::Optimal,
		.imageUsage = rhi::ImageUsage::Depth_Stencil_Attachment | rhi::ImageUsage::Sampled,
		.dimension = {
			.width = m_swapchain.info().dimension.width,
			.height = m_swapchain.info().dimension.height,
		},
		.clearValue = {
			.depthStencil = {
				.depth = 1.f
			}
		},
		.mipLevel = 1
	});

	//gltf::Importer zeldaImporter{ "data/demo/models/zelda/compressed/zelda.gltf" };
	gltf::Importer sponzaImporter{ "data/demo/models/sponza/compressed/sponza.gltf" };

	// TODO(afiq):
	// !!! Figure out how to map textures.
	// Load materials
	uint32 textureIndex = 0;

	//uint32 const zeldaMeshCount = zeldaImporter.num_meshes();

	//for (uint32 i = 0; i < zeldaMeshCount; ++i)
	//{
	//	auto result = zeldaImporter.mesh_at(i);

	//	if (!result.has_value())
	//	{
	//		continue;
	//	}

	//	auto mesh = result.value();
	//	gltf::MaterialInfo const& material = mesh.material_info();

	//	using img_type_t = std::underlying_type_t<gltf::ImageType>;

	//	auto const textureInfo = material.imageInfos[static_cast<img_type_t>(gltf::ImageType::Base_Color)];

	//	if (textureInfo)
	//	{
	//		ImageImporter importer{ { .name = lib::format("{}_base_color", material.name), .uri = textureInfo->uri } };

	//		auto&& baseColorMapInfo = importer.image_info();
	//		baseColorMapInfo.mipLevel = 1;

	//		m_image_store.emplace_back(m_resource_cache.create_image(std::move(baseColorMapInfo)));

	//		Resource<rhi::Image>& imageResource = m_image_store.back();

	//		m_upload_heap.upload_data_to_image({
	//			.image = *imageResource,
	//			.data = importer.data(0).data(),
	//			.size = importer.data(0).size_bytes(),
	//			.mipLevel = 0
	//		});

	//		m_device.update_image_descriptor({
	//			.image = *imageResource,
	//			.pSampler = nullptr,
	//			.index = textureIndex
	//		});
	//	}

	//	++textureIndex;
	//}

	uint32 const sponzaMeshCount = sponzaImporter.num_meshes();

	for (uint32 i = 0; i < sponzaMeshCount; ++i)
	{
		auto result = sponzaImporter.mesh_at(i);

		if (!result.has_value())
		{
			continue;
		}

		auto mesh = result.value();
		gltf::MaterialInfo const& material = mesh.material_info();

		using img_type_t = std::underlying_type_t<gltf::ImageType>;

		auto const textureInfo = material.imageInfos[static_cast<img_type_t>(gltf::ImageType::Base_Color)];

		if (textureInfo)
		{
			ImageImporter importer{ { .name = lib::format("{}_base_color", i), .uri = textureInfo->uri } };

			auto&& baseColorMapInfo = importer.image_info();
			baseColorMapInfo.mipLevel = 1;

			m_image_store.emplace_back(m_resource_cache.create_image(std::move(baseColorMapInfo)));

			Resource<rhi::Image>& imageResource = m_image_store.back();

			m_upload_heap.upload_data_to_image({
				.image = *imageResource,
				.data = importer.data(0).data(),
				.size = importer.data(0).size_bytes(),
				.mipLevel = 0
			});

			m_device.update_image_descriptor({
				.image = *imageResource,
				.pSampler = nullptr,
				.index = textureIndex
			});
		}

		++textureIndex;
	}

	GeometryInputLayout layout = { .inputs = GeometryInput::Position | GeometryInput::TexCoord, .interleaved = true };

	//m_zelda_geometry_handle = m_geometry_cache.store_geometries(zeldaImporter, layout);
	m_sponza_geometry_handle = m_geometry_cache.store_geometries(sponzaImporter, layout);

	//RootGeometryInfo const zeldaGeometryInfo = m_geometry_cache.geometry_info(m_zelda_geometry_handle);
	RootGeometryInfo const sponzaGeometryInfo = m_geometry_cache.geometry_info(m_sponza_geometry_handle);

	//m_zelda_vertex_buffer = m_resource_cache.create_buffer({
	//	.name = "zelda_vertex_buffer",
	//	.size = zeldaGeometryInfo.verticesSizeBytes,
	//	.bufferUsage = rhi::BufferUsage::Transfer_Dst | rhi::BufferUsage::Vertex,
	//	.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Best_Fit
	//});

	//m_zelda_index_buffer = m_resource_cache.create_buffer({
	//	.name = "zelda_index_buffer",
	//	.size = zeldaGeometryInfo.indicesSizeBytes,
	//	.bufferUsage = rhi::BufferUsage::Transfer_Dst | rhi::BufferUsage::Index,
	//	.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Best_Fit
	//});

	m_sponza_vertex_buffer = m_resource_cache.create_buffer({
		.name = "sponza_vertex_buffer",
		.size = sponzaGeometryInfo.verticesSizeBytes,
		.bufferUsage = rhi::BufferUsage::Transfer_Dst | rhi::BufferUsage::Vertex,
		.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Best_Fit
	});

	m_sponza_index_buffer = m_resource_cache.create_buffer({
		.name = "sponza_index_buffer",
		.size = sponzaGeometryInfo.indicesSizeBytes,
		.bufferUsage = rhi::BufferUsage::Transfer_Dst | rhi::BufferUsage::Index,
		.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Best_Fit
	});

	//for (size_t i = 0; i < std::size(m_zelda_transforms); ++i)
	//{
	//	uint32 bdaBindingIndex = m_bda_binding_index++;

	//	m_zelda_transforms[i] = m_resource_cache.create_buffer({
	//		.name = lib::format("zelda_transform_{}", i),
	//		.size = sizeof(glm::mat4),
	//		.bufferUsage = rhi::BufferUsage::Storage | rhi::BufferUsage::Transfer_Dst,
	//		.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Best_Fit
	//	});

	//	m_device.update_buffer_descriptor({
	//		.buffer = *m_zelda_transforms[i],
	//		.offset = 0,
	//		.size = m_zelda_transforms[i]->size(),
	//		.index = bdaBindingIndex
	//	});

	//	m_zelda_transform_binding_indices[i] = bdaBindingIndex;

	//	// Hardcode transform values for now.
	//	glm::mat4 transform = glm::rotate(glm::scale(glm::mat4{ 1.f }, glm::vec3{ 0.005f, 0.005f, 0.005f }), glm::radians(-90.f), glm::vec3{ 0.f, 1.f, 0.f });

	//	m_upload_heap.upload_data_to_buffer({
	//		.buffer = *m_zelda_transforms[i],
	//		.data = &transform,
	//		.size = sizeof(glm::mat4)
	//	});
	//}

	for (size_t i = 0; i < std::size(m_sponza_transforms); ++i)
	{
		uint32 bdaBindingIndex = m_bda_binding_index++;

		m_sponza_transforms[i] = m_resource_cache.create_buffer({
			.name = lib::format("sponza_transform_{}", i),
			.size = sizeof(glm::mat4),
			.bufferUsage = rhi::BufferUsage::Storage | rhi::BufferUsage::Transfer_Dst,
			.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Best_Fit
		});

		m_device.update_buffer_descriptor({
			.buffer = *m_sponza_transforms[i],
			.offset = 0,
			.size = m_sponza_transforms[i]->size(),
			.index = bdaBindingIndex
		});

		m_sponza_transform_binding_indices[i] = bdaBindingIndex;

		// Hardcode transform values for now.
		glm::mat4 transform = glm::scale(glm::mat4{ 1.f }, glm::vec3{1.f, 1.f, 1.f});

		m_upload_heap.upload_data_to_buffer({
			.buffer = *m_sponza_transforms[i],
			.data = &transform,
			.size = sizeof(glm::mat4)
		});
	}

	//m_geometry_cache.stage_geometries_for_upload({
	//	.geometry = m_zelda_geometry_handle,
	//	.vb = *m_zelda_vertex_buffer,
	//	.ib = *m_zelda_index_buffer
	//});

	m_geometry_cache.stage_geometries_for_upload({
		.geometry = m_sponza_geometry_handle,
		.vb = *m_sponza_vertex_buffer,
		.ib = *m_sponza_index_buffer
	});

	FenceInfo fenceInfo = m_upload_heap.send_to_gpu();

	{
		//Geometry const* zeldaGeometry = &m_geometry_cache.get_geometry(m_zelda_geometry_handle);

		// Acquire resources ...
		auto cmd = m_main_command_queue.next_free_command_buffer(std::this_thread::get_id());

		cmd->begin();

		//while (zeldaGeometry)
		//{
		//	cmd->pipeline_barrier(
		//		*m_zelda_vertex_buffer,
		//		{
		//			.size = zeldaGeometry->vertices.size,
		//			.offset = zeldaGeometry->vertices.offset * zeldaGeometryInfo.stride,
		//			.dstAccess = rhi::access::TRANSFER_WRITE,
		//			.srcQueue = rhi::DeviceQueueType::Transfer,
		//			.dstQueue = rhi::DeviceQueueType::Main
		//		}
		//	);

		//	cmd->pipeline_barrier(
		//		*m_zelda_index_buffer,
		//		{
		//			.size = zeldaGeometry->indices.size,
		//			.offset = zeldaGeometry->indices.offset * sizeof(uint32),
		//			.dstAccess = rhi::access::TRANSFER_WRITE,
		//			.srcQueue = rhi::DeviceQueueType::Transfer,
		//			.dstQueue = rhi::DeviceQueueType::Main
		//		}
		//	);

		//	zeldaGeometry = zeldaGeometry->next;
		//}

		Geometry const* sponzaGeometry = &m_geometry_cache.get_geometry(m_sponza_geometry_handle);

		while (sponzaGeometry)
		{
			cmd->pipeline_barrier(
				*m_sponza_vertex_buffer,
				{
					.size = sponzaGeometry->vertices.size,
					.offset = sponzaGeometry->vertices.offset * sponzaGeometryInfo.stride,
					.dstAccess = rhi::access::TRANSFER_WRITE,
					.srcQueue = rhi::DeviceQueueType::Transfer,
					.dstQueue = rhi::DeviceQueueType::Main
				}
			);

			cmd->pipeline_barrier(
				*m_sponza_index_buffer,
				{
					.size = sponzaGeometry->indices.size,
					.offset = sponzaGeometry->indices.offset * sizeof(uint32),
					.dstAccess = rhi::access::TRANSFER_WRITE,
					.srcQueue = rhi::DeviceQueueType::Transfer,
					.dstQueue = rhi::DeviceQueueType::Main
				}
			);

			sponzaGeometry = sponzaGeometry->next;
		}

		//for (size_t i = 0; i < std::size(m_zelda_transforms); ++i)
		//{
		//	cmd->pipeline_barrier(
		//		*m_zelda_transforms[i],
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

		for (auto&& imageResource : m_image_store)
		{
			cmd->pipeline_barrier(
				*imageResource,
				{

					.dstAccess = rhi::access::TRANSFER_WRITE,
					.oldLayout = rhi::ImageLayout::Transfer_Dst,
					.newLayout = rhi::ImageLayout::Shader_Read_Only,
					.srcQueue = rhi::DeviceQueueType::Transfer,
					.dstQueue = rhi::DeviceQueueType::Main
				}
			);
		}

		cmd->pipeline_barrier(
			*m_depth_buffer,
			{
				.srcAccess = rhi::access::TOP_OF_PIPE_NONE,
				.oldLayout = rhi::ImageLayout::Undefined,
				.newLayout = rhi::ImageLayout::Depth_Attachment,
				.subresource = {
					.aspectFlags = rhi::ImageAspect::Depth
				}
			}
		);

		cmd->end();

		auto submissionGroup = m_main_command_queue.new_submission_group();
		submissionGroup.wait_on_fence(fenceInfo.fence, fenceInfo.value);
		submissionGroup.submit_command_buffer(*cmd);
	}

	m_submission_queue.send_to_gpu_main_submissions();

	return true;
}

auto ModelDemoApp::run() -> void
{
	auto dt = core::stat::delta_time_f();

	m_device.clear_destroyed_resources();
	update_camera_state(dt);

	//Geometry const* zeldaGeometry = &m_geometry_cache.get_geometry(m_zelda_geometry_handle);
	Geometry const* sponzaGeometry = &m_geometry_cache.get_geometry(m_sponza_geometry_handle);

	auto cmd = m_main_command_queue.next_free_command_buffer(std::this_thread::get_id());
	auto&& swapchainImage = m_swapchain.acquire_next_image();

	auto& projViewBuffer = m_camera_proj_view[m_frame_index];

	PushConstant pc{
		.camera_proj_view_index = m_camera_proj_view_binding_indices[m_frame_index],
		//.transform_index = m_zelda_transform_binding_indices[m_frame_index],
	};

	cmd->reset();
	cmd->begin();

	cmd->pipeline_barrier(
		*projViewBuffer,
		{
			.dstAccess = rhi::access::TRANSFER_WRITE,
			.srcQueue = rhi::DeviceQueueType::Transfer,
			.dstQueue = rhi::DeviceQueueType::Main
		}
	);

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

	rhi::RenderAttachment depthAttachment{
		.pImage = &(*m_depth_buffer),
		.imageLayout = rhi::ImageLayout::Depth_Attachment,
		.loadOp = rhi::AttachmentLoadOp::Clear,
		.storeOp = rhi::AttachmentStoreOp::Store,
	};

	rhi::RenderingInfo renderingInfo{
		.colorAttachments = std::span{ &swapchainImageAttachment, 1 },
		.depthAttachment = &depthAttachment,
		.renderArea = {
			.extent = {
				.width = swapchainImage.info().dimension.width,
				.height = swapchainImage.info().dimension.height
			}
		}
	};

	auto const& swapchain_info = swapchainImage.info();

	float32 const width = static_cast<float32>(swapchain_info.dimension.width);
	float32 const height = static_cast<float32>(swapchain_info.dimension.height);

	cmd->begin_rendering(renderingInfo);
	cmd->set_viewport({
		.width = width,
		.height = height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	});
	cmd->set_scissor({
		.extent = {
			.width = swapchainImage.info().dimension.width,
			.height = swapchainImage.info().dimension.height
		}
	});
	cmd->bind_pipeline(m_pipeline);
	//cmd->bind_vertex_buffer(
	//	*m_zelda_vertex_buffer,
	//	{
	//		.firstBinding = 0u
	//	}
	//);
	//cmd->bind_index_buffer(
	//	*m_zelda_index_buffer,
	//	{
	//		.indexType = rhi::IndexType::Uint_32
	//	}
	//);

	//while (zeldaGeometry)
	//{
	//	cmd->bind_push_constant({
	//		.data = &pc,
	//		.size = sizeof(PushConstant),
	//		.shaderStage = rhi::ShaderStage::All
	//	});

	//	cmd->draw_indexed({
	//		.indexCount = zeldaGeometry->indices.count,
	//		.firstIndex = zeldaGeometry->indices.offset,
	//		.vertexOffset = zeldaGeometry->vertices.offset
	//	});
	//	zeldaGeometry = zeldaGeometry->next;

	//	++pc.base_color_map_index;
	//}

	cmd->bind_vertex_buffer(
		*m_sponza_vertex_buffer,
		{
			.firstBinding = 0u
		}
	);
	cmd->bind_index_buffer(
		*m_sponza_index_buffer,
		{
			.indexType = rhi::IndexType::Uint_32
		}
	);

	pc.transform_index = m_sponza_transform_binding_indices[m_frame_index];

	while (sponzaGeometry)
	{
		cmd->bind_push_constant({
			.data = &pc,
			.size = sizeof(PushConstant),
			.shaderStage = rhi::ShaderStage::All
		});

		cmd->draw_indexed({
			.indexCount = sponzaGeometry->indices.count,
			.firstIndex = sponzaGeometry->indices.offset,
			.vertexOffset = sponzaGeometry->vertices.offset
		});
		sponzaGeometry = sponzaGeometry->next;

		++pc.base_color_map_index;
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
		*projViewBuffer,
		{
			.srcAccess = rhi::access::TRANSFER_WRITE,
			.srcQueue = rhi::DeviceQueueType::Main,
			.dstQueue = rhi::DeviceQueueType::Transfer
		}
	);

	cmd->end();

	auto uploadFenceInfo = m_upload_heap.send_to_gpu();

	auto submitGroup = m_submission_queue.new_submission_group();
	submitGroup.submit_command_buffer(*cmd);
	submitGroup.wait_on_semaphore(m_swapchain.current_acquire_semaphore());
	submitGroup.signal_semaphore(m_swapchain.current_present_semaphore());
	submitGroup.wait_on_fence(uploadFenceInfo.fence, uploadFenceInfo.value);
	submitGroup.signal_fence(m_swapchain.get_gpu_fence(), m_swapchain.cpu_frame_count());

	m_submission_queue.send_to_gpu();

	m_device.present({ .swapchains = std::span{ &m_swapchain, 1 } });

	m_submission_queue.clear();
}

auto ModelDemoApp::terminate() -> void
{
	release_camera_buffers();

	for (size_t i = 0; i < std::size(m_zelda_transforms); ++i)
	{
		m_resource_cache.destroy_buffer(m_zelda_transforms[i].id());
	}

	for (size_t i = 0; i < std::size(m_sponza_transforms); ++i)
	{
		m_resource_cache.destroy_buffer(m_sponza_transforms[i].id());
	}

	for (auto&& imageResource : m_image_store)
	{
		m_resource_cache.destroy_image(imageResource.id());
	}

	m_resource_cache.destroy_buffer(m_zelda_vertex_buffer.id());
	m_resource_cache.destroy_buffer(m_zelda_index_buffer.id());
	m_resource_cache.destroy_buffer(m_sponza_vertex_buffer.id());
	m_resource_cache.destroy_buffer(m_sponza_index_buffer.id());
	m_resource_cache.destroy_image(m_depth_buffer.id());
	m_device.destroy_sampler(m_sampler);

	m_upload_heap.terminate();
	m_transfer_command_queue.terminate();
	m_main_command_queue.terminate();

	for (rhi::Fence& fence : m_fences)
	{
		m_device.destroy_fence(fence);
	}

	m_device.destroy_shader(m_vertex_shader);
	m_device.destroy_shader(m_pixel_shader);
	m_device.destroy_pipeline(m_pipeline);
}

auto ModelDemoApp::allocate_camera_buffers() -> void
{
	for (size_t i = 0; i < std::size(m_camera_proj_view); ++i)
	{
		uint32 bdaBindingIndex = m_bda_binding_index++;

		m_camera_proj_view[i] = m_resource_cache.create_buffer({
			.name = lib::format("camera_view_proj_{}", i),
			.size = sizeof(glm::mat4) + sizeof(glm::mat4),
			.bufferUsage = rhi::BufferUsage::Transfer_Dst | rhi::BufferUsage::Storage,
			.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Best_Fit
			});

		m_device.update_buffer_descriptor({
			.buffer = *m_camera_proj_view[i],
			.offset = 0,
			.size = m_camera_proj_view[i]->size(),
			.index = bdaBindingIndex
			});

		m_camera_proj_view_binding_indices[i] = bdaBindingIndex;
	}
}

auto ModelDemoApp::release_camera_buffers() -> void
{
	for (size_t i = 0; i < std::size(m_camera_proj_view); ++i)
	{
		m_resource_cache.destroy_buffer(m_camera_proj_view[i].id());
	}
}

auto ModelDemoApp::update_camera_state(float32 dt) -> void
{
	static bool firstRun[2] = { true, true };

	rhi::DeviceQueueType srcQueue = rhi::DeviceQueueType::Main;

	if (firstRun[m_frame_index])
	{
		srcQueue = rhi::DeviceQueueType::None;
		firstRun[m_frame_index] = false;
	}

	core::Engine& engine = core::engine();

	m_camera_state.dirty = false;

	if (engine.is_window_focused(m_root_app_window))
	{
		update_camera_on_mouse_events(dt);
		update_camera_on_keyboard_events(dt);
	}

	m_camera.update_projection();
	m_camera.update_view();

	auto&& projViewBuffer = m_camera_proj_view[m_frame_index];

	CameraProjView update{
		.projection = m_camera.projection,
		.view = m_camera.view
	};

	m_upload_heap.upload_data_to_buffer({
		.buffer = *projViewBuffer,
		.data = &update,
		.size = sizeof(CameraProjView),
		.srcQueue = srcQueue
		});
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
		if (delta.y > Camera::DELTA_EPSILON.min.y) { m_camera.translate(-m_camera.get_forward_vector(), dt); }
		// Mouse moves upwards.
		if (delta.y < -Camera::DELTA_EPSILON.min.y) { m_camera.translate(m_camera.get_forward_vector(), dt); }
		m_camera.rotate(glm::vec3{ delta.x, 0.f, 0.f }, dt);
		m_camera_state.dirty = true;
		break;
	default:
		break;
	}
}

auto ModelDemoApp::update_camera_on_keyboard_events(float32 dt) -> void
{
	glm::vec3 const UP = m_camera.get_up_vector();
	glm::vec3 const RIGHT = m_camera.get_right_vector();
	glm::vec3 const FORWARD = m_camera.get_forward_vector();

	if (core::io::key_pressed(core::IOKey::Q) || 
		core::io::key_held(core::IOKey::Q))
	{
		m_camera.translate(-UP, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::W) || 
		core::io::key_held(core::IOKey::W))
	{
		m_camera.translate(FORWARD, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::E) || 
		core::io::key_held(core::IOKey::E))
	{
		m_camera.translate(UP, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::A) || 
		core::io::key_held(core::IOKey::A))
	{
		m_camera.translate(-RIGHT, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::S) || 
		core::io::key_held(core::IOKey::S))
	{
		m_camera.translate(-FORWARD, dt);
		m_camera_state.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::D) || 
		core::io::key_held(core::IOKey::D))
	{
		m_camera.translate(RIGHT, dt);
		m_camera_state.dirty = true;
	}
}
}