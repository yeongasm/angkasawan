#include <fstream>

#include "demo/model_demo_app.h"
#include "model_importer.h"
#include "image_importer.h"

namespace sandbox
{
ModelDemoApp::ModelDemoApp(
	core::wnd::window_handle rootWindowHandle,
	gpu::util::ShaderCompiler& shaderCompiler,
	gpu::Device& device,
	gpu::swapchain rootWindowSwapchain,
	size_t const& frameIndex
) :
	DemoApplication{ rootWindowHandle, shaderCompiler, device, rootWindowSwapchain, frameIndex },
	m_commandQueue{ m_device },
	m_uploadHeap{ m_device, m_commandQueue },
	m_geometryCache{ m_uploadHeap }
{}

ModelDemoApp::~ModelDemoApp() {}

auto ModelDemoApp::initialize() -> bool
{
	m_uploadHeap.initialize();

	auto&& swapchainInfo = m_swapchain->info();

	float32 const swapchainWidth = static_cast<float32>(swapchainInfo.dimension.width);
	float32 const swapchainHeight = static_cast<float32>(swapchainInfo.dimension.height);

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

	gpu::Resource<gpu::Shader> vertexShader;
	gpu::Resource<gpu::Shader> pixelShader;

	gpu::util::ShaderCompileInfo compileInfo{
		.name = "sponza vertex shader",
		.path = "data/demo/shaders/model.glsl",
		.type = gpu::ShaderType::Vertex,
		.sourceCode = std::move(sourceCode)
	};

	{
		compileInfo.add_macro_definition("VERTEX_SHADER");

		if (auto result = m_shader_compiler.compile_shader(compileInfo); result.ok())
		{
			vertexShader = gpu::Shader::from(m_device, result.compiled_shader_info().value());
		}
		else
		{
			fmt::print("{}", result.error_msg().data());
		}
	}

	{
		compileInfo.name = "sponza pixel shader",
		compileInfo.type = gpu::ShaderType::Pixel;
		compileInfo.clear_macro_definitions();
		compileInfo.add_macro_definition("FRAGMENT_SHADER");

		if (auto result = m_shader_compiler.compile_shader(compileInfo); result.ok())
		{
			pixelShader = gpu::Shader::from(m_device, result.compiled_shader_info().value());
		}
		else
		{
			fmt::print("{}", result.error_msg().data());
		}
	}

	if (!vertexShader.valid() || !pixelShader.valid())
	{
		return false;
	}

	auto compiledInfo = m_shader_compiler.get_compiled_shader_info(vertexShader->info().name.c_str());

	m_pipeline = gpu::Pipeline::from(
		m_device,
		{
			.vertexShader = std::move(vertexShader),
			.pixelShader = std::move(pixelShader),
		},
		{
		.name = "sponza render",
		.colorAttachments = {
			{ .format = m_swapchain->image_format(), .blendInfo = { .enable = true } }
		},
		.depthAttachmentFormat = gpu::Format::D32_Float,
		.rasterization = {
			.cullMode = gpu::CullingMode::Back,
			.frontFace = gpu::FrontFace::Counter_Clockwise
		},
		.depthTest = {
			.depthTestCompareOp = gpu::CompareOp::Less,
			.enableDepthTest = true,
			.enableDepthWrite = true
		},
		.pushConstantSize = sizeof(PushConstant)
	});

	allocate_camera_buffers();

	m_normalSampler = gpu::Sampler::from(m_device, {
		.name = "normal sampler",
		.minFilter = gpu::TexelFilter::Linear,
		.magFilter = gpu::TexelFilter::Linear,
		.mipmapMode = gpu::MipmapMode::Linear,
		.addressModeU = gpu::SamplerAddress::Repeat,
		.addressModeV = gpu::SamplerAddress::Repeat,
		.addressModeW = gpu::SamplerAddress::Repeat,
		.mipLodBias = 0.f,
		.maxAnisotropy = 0.f,
		.compareOp = gpu::CompareOp::Never,
		.minLod = 0.f,
		.maxLod = 0.f,
		.borderColor = gpu::BorderColor::Float_Transparent_Black,
	});

	m_depthBuffer = gpu::Image::from(m_device, {
		.name = "depth buffer",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::D32_Float,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Depth_Stencil_Attachment | gpu::ImageUsage::Sampled,
		.dimension = {
			.width = m_swapchain->info().dimension.width,
			.height = m_swapchain->info().dimension.height,
		},
		.clearValue = {
			.depthStencil = {
				.depth = 1.f
			}
		},
		.mipLevel = 1
	});

	gltf::Importer sponzaImporter{ "data/demo/models/sponza/compressed/sponza.gltf" };

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
			ImageImporter importer{ { .name = lib::format("mesh:{}, base color", i), .uri = textureInfo->uri } };

			auto&& baseColorMapInfo = importer.image_info();
			baseColorMapInfo.mipLevel = 1;

			auto& baseColorImage = m_sponzaTextures.emplace_back(gpu::Image::from(m_device, std::move(baseColorMapInfo)), i);

			m_uploadHeap.upload_data_to_image({
				.image = baseColorImage.first,
				.data = importer.data(0).data(),
				.size = importer.data(0).size_bytes(),
				.mipLevel = 0
			});

			baseColorImage.first->bind({ .sampler = m_normalSampler, .index = i });
		}
	}

	GeometryInputLayout layout = { .inputs = GeometryInput::Position | GeometryInput::TexCoord, .interleaved = true };

	m_sponza = m_geometryCache.upload_gltf(sponzaImporter, layout);

	m_sponzaTransform = gpu::Buffer::from(
		m_device,
		{
			.name = "sponza transform",
			.size = sizeof(glm::mat4),
			.bufferUsage = gpu::BufferUsage::Storage | gpu::BufferUsage::Transfer_Dst,
			.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Best_Fit
		}
	);

	glm::mat4 transform = glm::scale(glm::mat4{ 1.f }, glm::vec3{ 1.f, 1.f, 1.f });
	m_uploadHeap.upload_data_to_buffer({ .dst = m_sponzaTransform, .data = &transform, .size = sizeof(glm::mat4) });

	FenceInfo fenceInfo = m_uploadHeap.send_to_gpu();

	{
		// Acquire resources ...
		auto cmd = m_commandQueue.next_free_command_buffer();

		cmd->begin();

		for (auto&& texture : m_sponzaTextures)
		{
			cmd->pipeline_barrier(
				*texture.first,
				{
					.dstAccess = gpu::access::TRANSFER_WRITE,
					.oldLayout = gpu::ImageLayout::Transfer_Dst,
					.newLayout = gpu::ImageLayout::Shader_Read_Only,
					.srcQueue = gpu::DeviceQueue::Transfer,
					.dstQueue = gpu::DeviceQueue::Main
				}
			);
		}

		cmd->pipeline_barrier(
			*m_depthBuffer,
			{
				.srcAccess = gpu::access::TOP_OF_PIPE_NONE,
				.oldLayout = gpu::ImageLayout::Undefined,
				.newLayout = gpu::ImageLayout::Depth_Attachment,
				.subresource = {
					.aspectFlags = gpu::ImageAspect::Depth
				}
			}
		);

		cmd->end();

		auto ownershipTransferSubmission = m_commandQueue.new_submission_group();
		ownershipTransferSubmission.wait(fenceInfo.fence, fenceInfo.value);
		ownershipTransferSubmission.submit(cmd);
	}

	m_commandQueue.send_to_gpu();

	return true;
}

auto ModelDemoApp::run() -> void
{
	auto dt = core::stat::delta_time_f();

	m_device.clear_garbage();
	update_camera_state(dt);

	Geometry const* sponzaMesh = m_geometryCache.geometry_from(m_sponza);
	auto const sponzaVb = m_geometryCache.vertex_buffer_of(m_sponza);
	auto const sponzaIb = m_geometryCache.index_buffer_of(m_sponza);

	auto cmd = m_commandQueue.next_free_command_buffer();
	auto&& swapchainImage = m_swapchain->acquire_next_image();

	auto& projViewBuffer = m_cameraProjView[m_frame_index];

	PushConstant pc{
		.vertexBufferPtr = sponzaVb->gpu_address(),
		.modelTransformPtr = m_sponzaTransform->gpu_address(),
		.cameraTransformPtr = projViewBuffer->gpu_address()
	};

	cmd->reset();
	cmd->begin();

	cmd->pipeline_barrier({
		.srcAccess = gpu::access::TRANSFER_WRITE,
		.dstAccess = gpu::access::HOST_READ
	});

	cmd->pipeline_barrier(
		*swapchainImage,
		{
			.dstAccess = gpu::access::FRAGMENT_SHADER_READ,
			.oldLayout = gpu::ImageLayout::Undefined,
			.newLayout = gpu::ImageLayout::Color_Attachment
		}
	);

	gpu::RenderAttachment swapchainImageAttachment{
		.image = swapchainImage,
		.imageLayout = gpu::ImageLayout::Color_Attachment,
		.loadOp = gpu::AttachmentLoadOp::Clear,
		.storeOp = gpu::AttachmentStoreOp::Store
	};

	gpu::RenderAttachment depthAttachment{
		.image = m_depthBuffer,
		.imageLayout = gpu::ImageLayout::Depth_Attachment,
		.loadOp = gpu::AttachmentLoadOp::Clear,
		.storeOp = gpu::AttachmentStoreOp::Store,
	};

	gpu::RenderingInfo renderingInfo{
		.colorAttachments = std::span{ &swapchainImageAttachment, 1 },
		.depthAttachment = &depthAttachment,
		.renderArea = {
			.extent = {
				.width	= swapchainImage->info().dimension.width,
				.height = swapchainImage->info().dimension.height
			}
		}
	};

	auto const& swapchainInfo = swapchainImage->info();

	float32 const width		= static_cast<float32>(swapchainInfo.dimension.width);
	float32 const height	= static_cast<float32>(swapchainInfo.dimension.height);

	cmd->begin_rendering(renderingInfo);
	cmd->set_viewport({
		.width = width,
		.height = height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	});
	cmd->set_scissor({
		.extent = {
			.width	= swapchainImage->info().dimension.width,
			.height = swapchainImage->info().dimension.height
		}
	});
	cmd->bind_pipeline(*m_pipeline);

	cmd->bind_index_buffer(
		*sponzaIb,
		{
			.indexType = gpu::IndexType::Uint_32
		}
	);

	while (sponzaMesh)
	{
		cmd->bind_push_constant({
			.data = &pc,
			.size = sizeof(PushConstant),
			.shaderStage = gpu::ShaderStage::All
		});

		cmd->draw_indexed({
			.indexCount = sponzaMesh->indices.count,
			.firstIndex = sponzaMesh->indices.offset,
			.vertexOffset = sponzaMesh->vertices.offset
		});

		sponzaMesh = sponzaMesh->next;

		++pc.baseColorMapIndex;
	}

	cmd->end_rendering();

	cmd->pipeline_barrier(
		*swapchainImage,
		{
			.srcAccess = gpu::access::FRAGMENT_SHADER_WRITE,
			.dstAccess = gpu::access::TRANSFER_READ,
			.oldLayout = gpu::ImageLayout::Color_Attachment,
			.newLayout = gpu::ImageLayout::Present_Src
		}
	);

	cmd->end();

	auto uploadFenceInfo = m_uploadHeap.send_to_gpu();

	auto submitGroup = m_commandQueue.new_submission_group();
	submitGroup.submit(cmd);
	submitGroup.wait(m_swapchain->current_acquire_semaphore());
	submitGroup.signal(m_swapchain->current_present_semaphore());
	submitGroup.wait(uploadFenceInfo.fence, uploadFenceInfo.value);
	submitGroup.signal(m_swapchain->get_gpu_fence(), m_swapchain->cpu_frame_count());

	m_commandQueue.send_to_gpu();

	m_device.present({ .swapchains = std::span{ &m_swapchain, 1 } });

	m_commandQueue.clear();
}

auto ModelDemoApp::terminate() -> void
{
	m_uploadHeap.terminate();
}

auto ModelDemoApp::allocate_camera_buffers() -> void
{
	for (size_t i = 0; i < std::size(m_cameraProjView); ++i)
	{
		m_cameraProjView[i] = gpu::Buffer::from(
			m_device, 
			{
				.name = lib::format("camera view proj {}", i),
				.size = sizeof(glm::mat4) + sizeof(glm::mat4),
				.bufferUsage = gpu::BufferUsage::Transfer_Dst | gpu::BufferUsage::Storage,
				.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Best_Fit
			}
		);
	}
}

auto ModelDemoApp::update_camera_state(float32 dt) -> void
{
	struct CameraProjectionView
	{
		glm::mat4 projection;
		glm::mat4 view;
	} updateValue;

	static bool firstRun[2] = { true, true };

	gpu::DeviceQueue srcQueue = gpu::DeviceQueue::Main;

	if (firstRun[m_frame_index])
	{
		srcQueue = gpu::DeviceQueue::None;
		firstRun[m_frame_index] = false;
	}

	core::Engine& engine = core::engine();

	m_cameraState.dirty = false;

	if (engine.is_window_focused(m_root_app_window))
	{
		update_camera_on_mouse_events(dt);
		update_camera_on_keyboard_events(dt);
	}

	m_camera.update_projection();
	m_camera.update_view();

	auto&& projViewBuffer = m_cameraProjView[m_frame_index];

	updateValue.projection = m_camera.projection;
	updateValue.view = m_camera.view;

	m_uploadHeap.upload_data_to_buffer({
		.dst = projViewBuffer,
		.data = &updateValue,
		.size = sizeof(CameraProjectionView),
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

	m_cameraState.mode = CameraMouseMode::None;

	float32 const mouseWheelV = core::io::mouse_wheel_v();

	/**
	* Only allow zooming when the cursor is in the viewport.
	*/
	if (cursorInViewport && mouseWheelV != 0.f)
	{
		m_camera.zoom(mouseWheelV);
		m_cameraState.dirty = true;
	}

	if (core::io::mouse_held(core::IOMouseButton::Right))
	{
		m_cameraState.mode = CameraMouseMode::Pan_And_Tilt;
	}
	else if (core::io::mouse_held(core::IOMouseButton::Left))
	{
		m_cameraState.mode = CameraMouseMode::Pan_And_Dolly;
	}

	if (m_cameraState.mode == CameraMouseMode::None)
	{
		if (!m_cameraState.firstMove)
		{
			int32 x = static_cast<int32>(m_cameraState.capturedMousePos.x);
			int32 y = static_cast<int32>(m_cameraState.capturedMousePos.y);

			engine.show_cursor();
			engine.set_cursor_position(m_root_app_window, core::Point{ x, y });

			m_cameraState.capturedMousePos = glm::vec2{ 0.f };
			m_cameraState.firstMove = true;
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

	if (m_cameraState.firstMove)
	{
		m_cameraState.capturedMousePos = mousePos;
		m_cameraState.firstMove = false;
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

	switch (m_cameraState.mode)
	{
	case CameraMouseMode::Pan_And_Tilt:
		m_camera.rotate(glm::vec3{ -delta.x, -delta.y, 0.f, }, dt);
		m_cameraState.dirty = true;
		break;
	case CameraMouseMode::Pan_And_Dolly:
		// Mouse moves downwards.
		if (delta.y > Camera::DELTA_EPSILON.min.y) { m_camera.translate(-m_camera.get_forward_vector(), dt); }
		// Mouse moves upwards.
		if (delta.y < -Camera::DELTA_EPSILON.min.y) { m_camera.translate(m_camera.get_forward_vector(), dt); }
		m_camera.rotate(glm::vec3{ -delta.x, 0.f, 0.f }, dt);
		m_cameraState.dirty = true;
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
		m_cameraState.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::W) ||
		core::io::key_held(core::IOKey::W))
	{
		m_camera.translate(FORWARD, dt);
		m_cameraState.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::E) ||
		core::io::key_held(core::IOKey::E))
	{
		m_camera.translate(UP, dt);
		m_cameraState.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::A) ||
		core::io::key_held(core::IOKey::A))
	{
		m_camera.translate(-RIGHT, dt);
		m_cameraState.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::S) ||
		core::io::key_held(core::IOKey::S))
	{
		m_camera.translate(-FORWARD, dt);
		m_cameraState.dirty = true;
	}

	if (core::io::key_pressed(core::IOKey::D) ||
		core::io::key_held(core::IOKey::D))
	{
		m_camera.translate(RIGHT, dt);
		m_cameraState.dirty = true;
	}
}
}