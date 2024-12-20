#include <fstream>

#include "gpu/util/shader_compiler.hpp"
#include "model_demo_app.hpp"
#include "model_importer.hpp"
#include "image_importer.hpp"

#include <iostream>

namespace sandbox
{
inline auto open_file(std::filesystem::path path) -> lib::string
{
	lib::string result = {};
	std::ifstream stream{ path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate };

	if (stream.good() &&
		stream.is_open())
	{
		uint32 size = static_cast<uint32>(stream.tellg());
		stream.seekg(0, std::ios::beg);

		result.reserve(size + 1);

		stream.read(result.data(), size);
		result[size] = lib::null_v<lib::string::value_type>;

		stream.close();
	}

	return result;
}

auto ModelDemoApp::start(
	core::platform::Application& application,
	core::Ref<core::platform::Window> rootWindow,
	GraphicsProcessingUnit& gpu
) -> bool
{
	m_app = &application;
	m_gpu = &gpu;
	m_rootWindowRef = rootWindow;

	// Create swapchain for root window.
	{
		make_swapchain(*m_rootWindowRef);
		if (!m_swapchain)
		{
			return false;
		}
	}

	m_rootWindowRef->push_listener(
		core::platform::WindowEvent::Type::Resize,
		[this](core::platform::WindowEvent ev) -> void
		{
			auto const dim = ev.current.value<core::platform::WindowEvent::Type::Resize>();
			m_gpu->device().wait_idle();
			m_swapchain->resize({ 
				.width = static_cast<uint32>(dim.width), 
				.height = static_cast<uint32>(dim.height) 
			});
			make_render_targets(dim.width, dim.height);

			render();
		}
	);

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

	auto make_uber_pipeline = [&](core::filewatcher::FileActionInfo const& fileAction) -> void
	{
		if (fileAction.action == core::filewatcher::FileAction::Modified)
		{
			gpu::util::ShaderCompiler shaderCompiler{};

			shaderCompiler.add_macro_definition("STORAGE_IMAGE_BINDING", gpu::STORAGE_IMAGE_BINDING);
			shaderCompiler.add_macro_definition("COMBINED_IMAGE_SAMPLER_BINDING", gpu::COMBINED_IMAGE_SAMPLER_BINDING);
			shaderCompiler.add_macro_definition("SAMPLED_IMAGE_BINDING", gpu::SAMPLED_IMAGE_BINDING);
			shaderCompiler.add_macro_definition("SAMPLER_BINDING", gpu::SAMPLER_BINDING);
			shaderCompiler.add_macro_definition("BUFFER_DEVICE_ADDRESS_BINDING", gpu::BUFFER_DEVICE_ADDRESS_BINDING);

			auto sourceCode = open_file(fileAction.path);
			if (!sourceCode.size())
			{
				return;
			}

			gpu::shader vs, ps;

			gpu::util::ShaderCompileInfo compileInfo{
				.name = "sponza vertex shader",
				.path = "data/demo/shaders/model.glsl",
				.type = gpu::ShaderType::Vertex,
				.sourceCode = std::move(sourceCode)
			};

			{
				compileInfo.add_macro_definition("VERTEX_SHADER");

				if (auto result = shaderCompiler.compile_shader(compileInfo); result.ok())
				{
					vs = gpu::Shader::from(m_gpu->device(), result.compiled_shader_info().value());
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

				if (auto result = shaderCompiler.compile_shader(compileInfo); result.ok())
				{
					ps = gpu::Shader::from(m_gpu->device(), result.compiled_shader_info().value());
				}
				else
				{
					fmt::print("{}", result.error_msg().data());
				}
			}

			if (!(vs.valid() && ps.valid()))
			{
				return;
			}

			auto compiledInfo = shaderCompiler.get_compiled_shader_info(vs->info().name.c_str());

			m_pipeline = gpu::Pipeline::from(
				m_gpu->device(),
				{
					.vertexShader = vs,
					.pixelShader = ps,
				},
				{
					.name = "sponza uber pipeline",
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
				}
			);
		}
	};

	m_pipelineShaderCodeWatchId = core::filewatcher::watch({ .path = "data/demo/shaders/model.glsl", .callback = make_uber_pipeline });

	make_uber_pipeline({ .path = "data/demo/shaders/model.glsl", .action = core::filewatcher::FileAction::Modified });

	allocate_camera_buffers();

	m_normalSampler = gpu::Sampler::from(m_gpu->device(), {
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

	make_render_targets(swapchainInfo.dimension.width, swapchainInfo.dimension.height);

	m_defaultWhiteTexture = gpu::Image::from(m_gpu->device(), {
		.name = "default white texture",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::B8G8R8A8_Srgb,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Transfer_Dst | gpu::ImageUsage::Sampled,
		.dimension = {
			.width = 1,
			.height = 1,
		},
		.mipLevel = 1
	});

	m_defaultWhiteTexture->bind({ .sampler = m_normalSampler, .index = 0u });

	gltf::Importer sponzaImporter{ "data/demo/models/sponza_updated/compressed/scene.gltf" };

	uint32 const sponzaMeshCount = sponzaImporter.num_meshes();
	uint32 imageCount = 1;

	for (uint32 i = 0; i < sponzaMeshCount; ++i)
	{
		auto result = sponzaImporter.mesh_at(i);

		if (!result.has_value())
		{
			continue;
		}

		auto mesh = result.value();

		gltf::MaterialInfo const& material = mesh.material_info();

		// Skip decals for now ...
		if (material.alphaMode != gltf::AlphaMode::Opaque)
		{
			continue;
		}

		using img_type_t = std::underlying_type_t<gltf::ImageType>;

		auto const textureInfo = material.imageInfos[static_cast<img_type_t>(gltf::ImageType::Base_Color)];

		if (textureInfo)
		{
			ImageImporter importer{ {.name = lib::format("mesh:{}, base color", i), .uri = textureInfo->uri } };

			auto&& baseColorMapInfo = importer.image_info();
			baseColorMapInfo.mipLevel = 1;

			auto& baseColorImage = m_sponzaTextures.emplace_back(gpu::Image::from(m_gpu->device(), std::move(baseColorMapInfo)), imageCount);

			m_gpu->upload_heap().upload_data_to_image({
				.image = baseColorImage.first,
				.data = importer.data(0).data(),
				.size = importer.data(0).size_bytes(),
				.mipLevel = 0
			});

			baseColorImage.first->bind({ .sampler = m_normalSampler, .index = imageCount });

			m_renderInfo.emplace_back(nullptr, imageCount);

			++imageCount;
		}
		else
		{
			m_renderInfo.emplace_back(nullptr, 0u);
		}
	}

	uint8 theColorWhite[4] = { 255, 255, 255, 255 };

	m_gpu->upload_heap().upload_data_to_image({
		.image = m_defaultWhiteTexture,
		.data = theColorWhite,
		.size = sizeof(uint8) * 4
	});

	GeometryInputLayout layout = { .inputs = GeometryInput::Position | GeometryInput::TexCoord, .interleaved = true };

	m_sponza = m_geometryCache.upload_gltf(m_gpu->upload_heap(), sponzaImporter, layout);

	Geometry const* sponzaMesh = m_geometryCache.geometry_from(m_sponza);

	for (uint32 i = 0, renderInfoIndex = 0; i < sponzaMeshCount; ++i)
	{
		auto result = sponzaImporter.mesh_at(i);

		if (!result.has_value())
		{
			continue;
		}

		// Skip decals for now ...
		if (result.value().material_info().alphaMode != gltf::AlphaMode::Opaque)
		{
			continue;
		}

		m_renderInfo[static_cast<size_t>(renderInfoIndex)].pGeometry = sponzaMesh;

		++renderInfoIndex;
		sponzaMesh = sponzaMesh->next;
	}

	m_sponzaTransform = gpu::Buffer::from(
		m_gpu->device(),
		{
			.name = "sponza transform",
			.size = sizeof(glm::mat4),
			.bufferUsage = gpu::BufferUsage::Storage | gpu::BufferUsage::Transfer_Dst,
			.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Best_Fit
		}
	);

	glm::mat4 transform = glm::scale(glm::mat4{ 1.f }, glm::vec3{ 1.f, 1.f, 1.f });
	m_gpu->upload_heap().upload_data_to_buffer({ .dst = m_sponzaTransform, .data = &transform, .size = sizeof(glm::mat4) });

	FenceInfo fenceInfo = m_gpu->upload_heap().send_to_gpu();

	{
		// Acquire resources ...
		auto cmd = m_gpu->command_queue().next_free_command_buffer();

		cmd->begin();

		for (auto&& texture : m_sponzaTextures)
		{
			cmd->pipeline_image_barrier({
				.image = *texture.first,
				.dstAccess = gpu::access::TRANSFER_WRITE,
				.oldLayout = gpu::ImageLayout::Transfer_Dst,
				.newLayout = gpu::ImageLayout::Shader_Read_Only,
				.srcQueue = gpu::DeviceQueue::Transfer,
				.dstQueue = gpu::DeviceQueue::Main
			});
		}

		cmd->pipeline_image_barrier({
			.image = *m_depthBuffer,
			.srcAccess = gpu::access::TOP_OF_PIPE_NONE,
			.oldLayout = gpu::ImageLayout::Undefined,
			.newLayout = gpu::ImageLayout::Depth_Attachment,
			.subresource = {
				.aspectFlags = gpu::ImageAspect::Depth
			}
		});

		cmd->end();

		auto ownershipTransferSubmission = m_gpu->command_queue().new_submission_group();
		ownershipTransferSubmission.wait(fenceInfo.fence, fenceInfo.value);
		ownershipTransferSubmission.submit(cmd);
	}

	m_gpu->command_queue().send_to_gpu();

	return true;
}

auto ModelDemoApp::run() -> void
{
	if (m_rootWindowRef->info().state != core::platform::WindowState::Ok)
	{
		return;
	}

	render();
}

auto ModelDemoApp::stop() -> void
{
}

auto ModelDemoApp::render() -> void
{
	auto dt = static_cast<float32>(core::platform::PerformanceCounter::delta_time());
	update_camera_state(dt);

	m_gpu->device().clear_garbage();

	auto const sponzaVb = m_geometryCache.vertex_buffer_of(m_sponza);
	auto const sponzaIb = m_geometryCache.index_buffer_of(m_sponza);

	auto cmd = m_gpu->command_queue().next_free_command_buffer();
	auto&& swapchainImage = m_swapchain->acquire_next_image();

	auto& projViewBuffer = m_cameraProjView[m_currentFrame];

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

	cmd->pipeline_image_barrier({
		.image = *swapchainImage,
		.dstAccess = gpu::access::FRAGMENT_SHADER_READ,
		.oldLayout = gpu::ImageLayout::Undefined,
		.newLayout = gpu::ImageLayout::Color_Attachment
	});

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
				.width = swapchainImage->info().dimension.width,
				.height = swapchainImage->info().dimension.height
			}
		}
	};

	auto const& swapchainInfo = swapchainImage->info();

	float32 const width = static_cast<float32>(swapchainInfo.dimension.width);
	float32 const height = static_cast<float32>(swapchainInfo.dimension.height);

	cmd->begin_rendering(renderingInfo);
	cmd->set_viewport({
		.width = width,
		.height = height,
		.minDepth = 0.f,
		.maxDepth = 1.f
	});
	cmd->set_scissor({
		.extent = {
			.width = swapchainImage->info().dimension.width,
			.height = swapchainImage->info().dimension.height
		}
	});
	cmd->bind_pipeline(*m_pipeline);

	cmd->bind_index_buffer({
		.buffer = *sponzaIb,
		.indexType = gpu::IndexType::Uint_32
	});

	for (GeometryRenderInfo const& renderInfo : m_renderInfo)
	{
		pc.baseColorMapIndex = renderInfo.baseColor;

		cmd->bind_push_constant({
			.data = &pc,
			.size = sizeof(PushConstant),
			.shaderStage = gpu::ShaderStage::All
		});

		cmd->draw_indexed({
			.indexCount = renderInfo.pGeometry->indices.count,
			.firstIndex = renderInfo.pGeometry->indices.offset,
			.vertexOffset = renderInfo.pGeometry->vertices.offset
		});

	}

	cmd->end_rendering();

	cmd->pipeline_image_barrier({
		.image = *swapchainImage,
		.srcAccess = gpu::access::FRAGMENT_SHADER_WRITE,
		.dstAccess = gpu::access::TRANSFER_READ,
		.oldLayout = gpu::ImageLayout::Color_Attachment,
		.newLayout = gpu::ImageLayout::Present_Src
	});

	cmd->end();

	auto uploadFenceInfo = m_gpu->upload_heap().send_to_gpu();

	auto submitGroup = m_gpu->command_queue().new_submission_group();

	submitGroup.submit(cmd);
	submitGroup.wait(m_swapchain->current_acquire_semaphore());
	submitGroup.signal(m_swapchain->current_present_semaphore());
	submitGroup.wait(uploadFenceInfo.fence, uploadFenceInfo.value);
	submitGroup.signal(m_swapchain->get_gpu_fence(), m_swapchain->cpu_frame_count());

	m_gpu->command_queue().send_to_gpu();

	m_gpu->device().present({ .swapchains = std::span{ &m_swapchain, 1 } });

	m_gpu->command_queue().clear();

	m_currentFrame = (m_currentFrame + 1) % m_gpu->device().config().maxFramesInFlight;
}

auto ModelDemoApp::make_swapchain(core::platform::Window& window) -> void
{
	auto const& info = window.info();
	m_swapchain = gpu::Swapchain::from(m_gpu->device(), {
		.name = "root app window",
		.surfaceInfo = {
			.name = "root app surface",
			.preferredSurfaceFormats = { gpu::Format::B8G8R8A8_Srgb },
			.instance = window.process_handle(),
			.window = info.nativeHandle
		},
		.dimension = { static_cast<uint32>(info.dim.width), static_cast<uint32>(info.dim.height) },
		.imageCount = 4,
		.imageUsage = gpu::ImageUsage::Color_Attachment | gpu::ImageUsage::Transfer_Dst,
		.presentationMode = gpu::SwapchainPresentMode::Mailbox
	}, (m_swapchain.valid()) ? m_swapchain: gpu::swapchain{});
}

auto ModelDemoApp::make_render_targets(uint32 width, uint32 height) -> void
{
	m_depthBuffer = gpu::Image::from(m_gpu->device(), {
		.name = "depth buffer",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::D32_Float,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Depth_Stencil_Attachment | gpu::ImageUsage::Sampled,
		.dimension = {
			.width = width,
			.height = height,
		},
		.clearValue = {
			.depthStencil = {
				.depth = 1.f
			}
		},
		.mipLevel = 1
	});
}

auto ModelDemoApp::allocate_camera_buffers() -> void
{
	for (size_t i = 0; i < std::size(m_cameraProjView); ++i)
	{
		m_cameraProjView[i] = gpu::Buffer::from(
			m_gpu->device(),
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

	auto const& swapchainDim = m_swapchain->info().dimension;

	float32 const frameWidth  = static_cast<float32>(swapchainDim.width);
	float32 const frameHeight = static_cast<float32>(swapchainDim.height);

	gpu::DeviceQueue srcQueue = gpu::DeviceQueue::Main;

	if (firstRun[m_currentFrame])
	{
		srcQueue = gpu::DeviceQueue::None;
		firstRun[m_currentFrame] = false;
	}

	m_cameraState.dirty = false;

	if (m_rootWindowRef->is_focused())
	{
		update_camera_on_mouse_events(dt);
		update_camera_on_keyboard_events(dt);
	}

	m_camera.frameWidth  = frameWidth;
	m_camera.frameHeight = frameHeight;

	m_camera.update_projection();
	m_camera.update_view();

	auto&& projViewBuffer = m_cameraProjView[m_currentFrame];

	updateValue.projection = m_camera.projection;
	updateValue.view = m_camera.view;

	m_gpu->upload_heap().upload_data_to_buffer({
		.dst = projViewBuffer,
		.data = &updateValue,
		.size = sizeof(CameraProjectionView),
		.srcQueue = srcQueue
	});
}

auto ModelDemoApp::update_camera_on_mouse_events(float32 dt) -> void
{
	core::Point const pos = m_app->mouse_position();
	core::Point const windowPos = m_rootWindowRef->info().pos;

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

	float32 const mouseWheelV = m_app->mouse_wheel_v();

	/**
	* Only allow zooming when the cursor is in the viewport.
	*/
	if (cursorInViewport && mouseWheelV != 0.f)
	{
		m_camera.zoom(mouseWheelV);
		m_cameraState.dirty = true;
	}

	if (m_app->mouse_held(core::IOMouseButton::Right))
	{
		m_cameraState.mode = CameraMouseMode::Pan_And_Tilt;
	}
	else if (m_app->mouse_held(core::IOMouseButton::Left))
	{
		m_cameraState.mode = CameraMouseMode::Pan_And_Dolly;
	}

	if (m_cameraState.mode == CameraMouseMode::None)
	{
		if (!m_cameraState.firstMove)
		{
			int32 x = static_cast<int32>(m_cameraState.capturedMousePos.x);
			int32 y = static_cast<int32>(m_cameraState.capturedMousePos.y);

			m_app->show_cursor();
			m_app->set_cursor_position(m_rootWindowRef, core::Point{ x, y });

			m_cameraState.capturedMousePos = glm::vec2{ 0.f };
			m_cameraState.firstMove = true;
		}
		return;
	}
	else
	{
		if (m_app->mouse_dragging(core::IOMouseButton::Left) || m_app->mouse_dragging(core::IOMouseButton::Right))
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

	m_app->show_cursor(false);
	m_app->set_cursor_position(m_rootWindowRef, core::Point{ cx, cy });

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

	if (m_app->key_pressed(core::IOKey::Q) ||
		m_app->key_held(core::IOKey::Q))
	{
		m_camera.translate(-UP, dt);
		m_cameraState.dirty = true;
	}

	if (m_app->key_pressed(core::IOKey::W) ||
		m_app->key_held(core::IOKey::W))
	{
		m_camera.translate(FORWARD, dt);
		m_cameraState.dirty = true;
	}

	if (m_app->key_pressed(core::IOKey::E) ||
		m_app->key_held(core::IOKey::E))
	{
		m_camera.translate(UP, dt);
		m_cameraState.dirty = true;
	}

	if (m_app->key_pressed(core::IOKey::A) ||
		m_app->key_held(core::IOKey::A))
	{
		m_camera.translate(-RIGHT, dt);
		m_cameraState.dirty = true;
	}

	if (m_app->key_pressed(core::IOKey::S) ||
		m_app->key_held(core::IOKey::S))
	{
		m_camera.translate(-FORWARD, dt);
		m_cameraState.dirty = true;
	}

	if (m_app->key_pressed(core::IOKey::D) ||
		m_app->key_held(core::IOKey::D))
	{
		m_camera.translate(RIGHT, dt);
		m_cameraState.dirty = true;
	}
}
}