#include <glaze/glaze.hpp>
#include "core.serialization/file.hpp"
#include "gpu/util/shader_compiler.hpp"
#include "model_demo_app.hpp"

template <>
struct glz::meta<render::material::ImageType>
{
	using enum render::material::ImageType;
	static constexpr auto value = glz::enumerate(Base_Color, Metallic_Roughness, Normal, Occlusion, Emissive);
};

template <>
struct glz::meta<render::material::AlphaMode>
{
	using enum render::material::AlphaMode;
	static constexpr auto value = glz::enumerate(Opaque, Mask, Blend);
};

namespace sandbox
{
auto ModelDemoApp::start(
	core::platform::Application& application,
	core::Ref<core::platform::Window> rootWindow,
	render::AsyncDevice& gpu
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

	make_render_targets(swapchainInfo.dimension.width, swapchainInfo.dimension.height);

	// TODO(afiq):
	// Check if the shader binary exist and if it doesn't compile from source.
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

			core::sbf::File const sourceCode({ .path = fileAction.path, .shareMode = core::sbf::FileShareMode::Shared_Read, .map = true });
			
			if (!sourceCode.is_open() || 
				sourceCode.data() == nullptr)
			{
				return;
			}

			gpu::shader vs, ps;

			gpu::util::ShaderCompileInfo compileInfo{
				.name = "sponza vertex shader",
				.path = "data/demo/shaders/model.glsl",
				.type = gpu::ShaderType::Vertex,
				.sourceCode = std::string_view{ static_cast<char const*>(sourceCode.data()), sourceCode.size() }
			};

			{
				compileInfo.add_macro_definition("VERTEX_SHADER");

				if (auto result = shaderCompiler.compile_shader(compileInfo); result)
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

			m_pipeline = gpu::Pipeline::from(
				m_gpu->device(),
				{
					.vertexShader = vs,
					.pixelShader = ps,
				},
				{
					.name = "sponza uber pipeline",
					.colorAttachments = {
						{ 
							.format = m_baseColorAttachment->info().format, 
							.blendInfo = { 
								.enable = true,
								.srcColorBlendFactor = gpu::BlendFactor::One,
								.dstColorBlendFactor = gpu::BlendFactor::Zero,
								.colorBlendOp = gpu::BlendOp::Add,
								.srcAlphaBlendFactor = gpu::BlendFactor::One,
								.dstAlphaBlendFactor = gpu::BlendFactor::Zero,
								.alphaBlendOp = gpu::BlendOp::Add
							} 
						},
						{
							.format = m_metallicRoughnessAttachment->info().format, 
							.blendInfo = { 
								.enable = true,
								.srcColorBlendFactor = gpu::BlendFactor::One,
								.dstColorBlendFactor = gpu::BlendFactor::Zero,
								.colorBlendOp = gpu::BlendOp::Add,
								.srcAlphaBlendFactor = gpu::BlendFactor::One,
								.dstAlphaBlendFactor = gpu::BlendFactor::Zero,
								.alphaBlendOp = gpu::BlendOp::Add
							} 				
						},
						{
							.format = m_normalAttachment->info().format, 
							.blendInfo = { 
								.enable = true,
								.srcColorBlendFactor = gpu::BlendFactor::One,
								.dstColorBlendFactor = gpu::BlendFactor::Zero,
								.colorBlendOp = gpu::BlendOp::Add,
								.srcAlphaBlendFactor = gpu::BlendFactor::One,
								.dstAlphaBlendFactor = gpu::BlendFactor::Zero,
								.alphaBlendOp = gpu::BlendOp::Add
							} 				
						}
					},
					.depthAttachmentFormat = gpu::Format::D32_Float,
					.rasterization = {
						.polygonalMode = gpu::PolygonMode::Fill,
						.cullMode = gpu::CullingMode::Back,
						.frontFace = gpu::FrontFace::Counter_Clockwise
					},
					.depthTest = {
						.depthTestCompareOp = gpu::CompareOp::Less,
						.minDepthBounds = 0.f,
						.maxDepthBounds = 1.f,
						.enableDepthBoundsTest = false,
						.enableDepthTest = true,
						.enableDepthWrite = true
					},
					.topology = gpu::TopologyType::Triangle,
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
		.unnormalizedCoordinates = false
	});

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
		.mipLevel = 1,
		.sharingMode = gpu::SharingMode::Exclusive
	});

	m_defaultMetallicRoughnessMap = gpu::Image::from(m_gpu->device(), {
		.name = "default metallic roughness map",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::R8G8_Unorm,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Transfer_Dst | gpu::ImageUsage::Sampled,
		.dimension = {
			.width = 1,
			.height = 1,
		},
		.mipLevel = 1,
		.sharingMode = gpu::SharingMode::Exclusive
	});

	m_defaultNormalMap = gpu::Image::from(m_gpu->device(), {
		.name = "default normal map",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::B8G8R8A8_Srgb,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Transfer_Dst | gpu::ImageUsage::Sampled,
		.dimension = {
			.width = 1,
			.height = 1,
		},
		.mipLevel = 1,
		.sharingMode = gpu::SharingMode::Exclusive
	});

	m_defaultWhiteTexture->bind({ .sampler = m_normalSampler, .index = 0u });
	m_defaultMetallicRoughnessMap->bind({ .sampler = m_normalSampler, .index = 1u });
	m_defaultNormalMap->bind({ .sampler = m_normalSampler, .index = 2u });

	render::material::util::MaterialJSON materialRep{};
	if (auto ec = glz::read_file_json(materialRep, "data/demo/models/sponza/sponza.material.json", std::string{}); ec)
	{
		fmt::print("Could not load material representation for sponza.sbf. {}\n", ec.custom_error_message);
	}

	unpack_sponza();
	unpack_materials(materialRep);

	uint8 theColorWhite[4] = { 255, 255, 255, 255 };
	uint8 zeroMetallicRoughness[2] = { 0, 0 };
	uint8 flatNormal[4] = { 255, 128, 128, 255 };

	m_gpu->upload_heap().upload_data_to_image({
		.image = m_defaultWhiteTexture,
		.data = theColorWhite,
		.size = sizeof(uint8) * 4
	});

	m_gpu->upload_heap().upload_data_to_image({
		.image = m_defaultMetallicRoughnessMap,
		.data = zeroMetallicRoughness,
		.size = sizeof(uint8) * 2	
	});

	m_gpu->upload_heap().upload_data_to_image({
		.image = m_defaultNormalMap,
		.data = flatNormal,
		.size = sizeof(uint8) * 4
	});

	m_sponzaTransform = render::GpuPtr<glm::mat4>::from(
		m_gpu->device(),
		{
			.name = "sponza transform",
			.bufferUsage = gpu::BufferUsage::Storage
		},
		glm::scale(glm::mat4{ 1.f }, glm::vec3{ 1.f, 1.f, 1.f })
	);

	render::FenceInfo fenceInfo = m_gpu->upload_heap().send_to_gpu();

	{
		// Acquire resources ...
		auto cmd = m_gpu->command_queue().next_free_command_buffer();

		cmd->begin();

		for (auto&& image : m_images)
		{
			cmd->pipeline_image_barrier({
				.image = *image.image,
				.dstAccess = gpu::access::TRANSFER_WRITE,
				.oldLayout = gpu::ImageLayout::Transfer_Dst,
				.newLayout = gpu::ImageLayout::Shader_Read_Only,
				.subresource = BASE_COLOR_SUBRESOURCE,
				.srcQueue = gpu::DeviceQueue::Transfer,
				.dstQueue = gpu::DeviceQueue::Main
			});
		}

		cmd->pipeline_image_barrier({
			.image = *m_depthBuffer,
			.srcAccess = gpu::access::TOP_OF_PIPE_NONE,
			.oldLayout = gpu::ImageLayout::Undefined,
			.newLayout = gpu::ImageLayout::Depth_Attachment,
			.subresource = DEPTH_SUBRESOURCE
		});

		cmd->pipeline_image_barrier({
			.image = *m_baseColorAttachment,
			.srcAccess = gpu::access::TOP_OF_PIPE_NONE,
			.oldLayout = gpu::ImageLayout::Undefined,
			.newLayout = gpu::ImageLayout::Color_Attachment,
			.subresource = BASE_COLOR_SUBRESOURCE
		});

		cmd->pipeline_image_barrier({
			.image = *m_metallicRoughnessAttachment,
			.srcAccess = gpu::access::TOP_OF_PIPE_NONE,
			.oldLayout = gpu::ImageLayout::Undefined,
			.newLayout = gpu::ImageLayout::Attachment,
			.subresource = BASE_COLOR_SUBRESOURCE
		});

		cmd->pipeline_image_barrier({
			.image = *m_normalAttachment,
			.srcAccess = gpu::access::TOP_OF_PIPE_NONE,
			.oldLayout = gpu::ImageLayout::Undefined,
			.newLayout = gpu::ImageLayout::Color_Attachment,
			.subresource = BASE_COLOR_SUBRESOURCE
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

	auto cmd = m_gpu->command_queue().next_free_command_buffer();
	auto&& swapchainImage = m_swapchain->acquire_next_image();

	cmd->reset();
	cmd->begin();

	cmd->pipeline_barrier({
		.srcAccess = gpu::access::TRANSFER_WRITE,
		.dstAccess = gpu::access::HOST_READ
	});

	cmd->pipeline_image_barrier({
		.image = *swapchainImage,
		.dstAccess = gpu::access::TRANSFER_WRITE,
		.oldLayout = gpu::ImageLayout::Undefined,
		.newLayout = gpu::ImageLayout::Transfer_Dst,
		.subresource = BASE_COLOR_SUBRESOURCE
	});

	gpu::RenderAttachment depthAttachment{
		.image = m_depthBuffer,
		.imageLayout = gpu::ImageLayout::Depth_Attachment,
		.loadOp = gpu::AttachmentLoadOp::Clear,
		.storeOp = gpu::AttachmentStoreOp::Store,
	};

	gpu::RenderingInfo renderingInfo{
		.colorAttachments = std::span{ m_renderAttachments.data(), m_renderAttachments.size() },
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

	for (MeshRenderInfo const& renderInfo : m_renderInfo)
	{
		cmd->bind_index_buffer({
			.buffer = *renderInfo.mesh->buffer,
			.offset = renderInfo.mesh->info.indices.byteOffset,
			.indexType = gpu::IndexType::Uint_32
		});

		PushConstant pc{
			.cameraProjView		= m_cameraProjView.address_at(m_currentFrame),
			.objectTransform 	= m_sponzaTransform.address(),
			.renderInfo 		= renderInfo.info.address()
		};

		cmd->bind_push_constant({
			.data = &pc,
			.size = sizeof(PushConstant),
			.shaderStage = gpu::ShaderStage::All
		});

		cmd->draw_indexed({
			.indexCount = renderInfo.mesh->info.indices.count
		});
	}

	cmd->end_rendering();

	cmd->pipeline_image_barrier({
		.image = *m_baseColorAttachment,
		.srcAccess = gpu::access::FRAGMENT_SHADER_WRITE,
		.dstAccess = gpu::access::TRANSFER_READ,
		.oldLayout = gpu::ImageLayout::Color_Attachment,
		.newLayout = gpu::ImageLayout::Transfer_Src,
		.subresource = BASE_COLOR_SUBRESOURCE
	});

	cmd->copy_image_to_image({
		.src = *m_baseColorAttachment,
		.dst = *swapchainImage,
		.srcSubresource = BASE_COLOR_SUBRESOURCE,
		.dstSubresource = BASE_COLOR_SUBRESOURCE,
		.extent = swapchainImage->info().dimension
	});

	cmd->pipeline_image_barrier({
		.image = *m_baseColorAttachment,
		.srcAccess = gpu::access::TRANSFER_READ,
		.dstAccess = gpu::access::FRAGMENT_SHADER_WRITE,
		.oldLayout = gpu::ImageLayout::Transfer_Src,
		.newLayout = gpu::ImageLayout::Color_Attachment,
		.subresource = BASE_COLOR_SUBRESOURCE
	});

	cmd->pipeline_image_barrier({
		.image = *swapchainImage,
		.srcAccess = gpu::access::TRANSFER_WRITE,
		.dstAccess = gpu::access::TRANSFER_READ,
		.oldLayout = gpu::ImageLayout::Transfer_Dst,
		.newLayout = gpu::ImageLayout::Present_Src,
		.subresource = BASE_COLOR_SUBRESOURCE
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

	m_baseColorAttachment = gpu::Image::from(m_gpu->device(), {
		.name = "albedo attachment",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::B8G8R8A8_Srgb,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Color_Attachment | gpu::ImageUsage::Sampled | gpu::ImageUsage::Transfer_Src,
		.dimension = {
			.width = width,
			.height = height,
		},
		.mipLevel = 1
	});

	m_metallicRoughnessAttachment = gpu::Image::from(m_gpu->device(), {
		.name = "metallic roughness attachment",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::R8G8_Unorm,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Color_Attachment | gpu::ImageUsage::Sampled,
		.dimension = {
			.width = width,
			.height = height,
		},
		.mipLevel = 1
	});

	m_normalAttachment = gpu::Image::from(m_gpu->device(), {
		.name = "normal attachment",
		.type = gpu::ImageType::Image_2D,
		.format = gpu::Format::B8G8R8A8_Srgb,
		.samples = gpu::SampleCount::Sample_Count_1,
		.tiling = gpu::ImageTiling::Optimal,
		.imageUsage = gpu::ImageUsage::Color_Attachment | gpu::ImageUsage::Sampled,
		.dimension = {
			.width = width,
			.height = height,
		},
		.mipLevel = 1
	});

	m_renderAttachments[0] = { 
		.image = m_baseColorAttachment,
		.imageLayout = gpu::ImageLayout::Color_Attachment,
		.loadOp = gpu::AttachmentLoadOp::Clear,
		.storeOp = gpu::AttachmentStoreOp::Store
	};

	m_renderAttachments[1] = { 
		.image = m_metallicRoughnessAttachment,
		.imageLayout = gpu::ImageLayout::Color_Attachment,
		.loadOp = gpu::AttachmentLoadOp::Clear,
		.storeOp = gpu::AttachmentStoreOp::Store
	};

	m_renderAttachments[2] = { 
		.image = m_normalAttachment,
		.imageLayout = gpu::ImageLayout::Color_Attachment,
		.loadOp = gpu::AttachmentLoadOp::Clear,
		.storeOp = gpu::AttachmentStoreOp::Store
	};	
}

auto ModelDemoApp::allocate_camera_buffers() -> void
{
	m_cameraProjView = render::GpuPtr<CameraProjectionView[2]>::from(
		m_gpu->upload_heap(),
		{
			.name 			= "camera view proj",
			.bufferUsage 	= gpu::BufferUsage::Storage
		}
	);
}

auto ModelDemoApp::update_camera_state(float32 dt) -> void
{
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

	projViewBuffer.projection = m_camera.projection;
	projViewBuffer.view = m_camera.view;

	m_cameraProjView.commit(m_currentFrame, srcQueue);
}

auto ModelDemoApp::update_camera_on_mouse_events(float32 dt) -> void
{
	core::Point const pos = m_app->mouse_position();

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
	else if (cursorInViewport && 
			m_app->mouse_held(core::IOMouseButton::Middle) &&
			m_camera.fov != 45.f)
	{
		m_camera.zoom(45.f);
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

auto ModelDemoApp::unpack_sponza() -> void
{
	core::sbf::File sponzaSbf{ "data/demo/models/sponza/sponza.sbf" };

	if (!sponzaSbf.is_open())
	{
		return;
	}

	render::MeshSbfPack meshPack{ sponzaSbf };

	m_meshes.reserve(static_cast<size_t>(meshPack.mesh_count()));

	for (size_t i = 0; auto const& [metadata, data] : meshPack)
	{
		auto& mesh = m_meshes.emplace_back();

		size_t const totalSizeBytesRequired = static_cast<size_t>(metadata.vertices.sizeBytes + metadata.indices.sizeBytes);

		// Keep track of the mesh's attributes from the pack file.
		mesh.attributes = metadata.attributes;

		// Store mesh vertices information.
		mesh.info.vertices.count 	= metadata.vertices.count;
		mesh.info.vertices.size 	= metadata.vertices.sizeBytes;

		// Store mesh indices information.
		mesh.info.indices.count = metadata.indices.count;
		mesh.info.indices.size 	= metadata.indices.sizeBytes;

		mesh.buffer = gpu::Buffer::from(
			m_gpu->device(), 
			{
				.name = lib::format("<mesh>:data/demo/models/sponza:{}", i),
				.size = totalSizeBytesRequired,
				.bufferUsage = gpu::BufferUsage::Transfer_Dst | gpu::BufferUsage::Vertex | gpu::BufferUsage::Index,
				.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Best_Fit
			}
		);

		// Vertex data will always be stored first so that's why it's 0.
		mesh.info.vertices.offset = 0u;

		uint32 const normalDataExist 	= static_cast<uint32>((mesh.attributes & render::VertexAttribute::Normal) != render::VertexAttribute::None);
		uint32 const uvDataExist 		= static_cast<uint32>((mesh.attributes & render::VertexAttribute::TexCoord) != render::VertexAttribute::None);

		// Our project requires that a mesh contain position, normal and uv data.
		// However, occassionally, some meshes will not contain some of these attributes and the sbf file does not store the offsets to each of these attributes.
		// A single flag is used to determine the type of attributes that exist for the mesh in the sbf file.
		uint32 const posCount 		= metadata.vertices.count * render::AttribInfo<render::VertexAttribute::Position>::componentCount;
		uint32 const normalCount 	= normalDataExist * metadata.vertices.count * render::AttribInfo<render::VertexAttribute::Normal>::componentCount;
		uint32 const uvCount 		= uvDataExist * metadata.vertices.count * render::AttribInfo<render::VertexAttribute::TexCoord>::componentCount;

		std::span<float32> const posRange{ &data.vertices[0], posCount };
		std::span<float32> const normalRange{ &data.vertices[normalDataExist * posCount], normalCount };
		std::span<float32> const uvRange{ &data.vertices[uvDataExist * (posCount + normalCount)], uvCount };

		// Index data will always be stored after vertex data.
		mesh.info.indices.byteOffset = (posCount + normalCount + uvCount) * sizeof(float32);

		// Upload vertex data.
		// Position.
		m_gpu->upload_heap().upload_data_to_buffer({
			.dst = mesh.buffer,
			.data = posRange.data(),
			.size = posRange.size_bytes()
		});

		mesh.position = mesh.buffer->gpu_address();

		// Normal. If exist.
		if (!normalRange.empty())
		{
			m_gpu->upload_heap().upload_data_to_buffer({
				.dst = mesh.buffer,
				.dstOffset = posCount * sizeof(float32),
				.data = normalRange.data(),
				.size = normalRange.size_bytes()
			});

			mesh.normal = mesh.buffer->gpu_address() + (posCount * sizeof(float32));
		}

		// TexCoord. If exist.
		if (!uvRange.empty())
		{
			m_gpu->upload_heap().upload_data_to_buffer({
				.dst = mesh.buffer,
				.dstOffset = (posCount + normalCount) * sizeof(float32),
				.data = uvRange.data(),
				.size = uvRange.size_bytes()
			});

			mesh.uv = mesh.buffer->gpu_address() + ((posCount + normalCount) * sizeof(float32));
		}

		m_gpu->upload_heap().upload_data_to_buffer({
			.dst = mesh.buffer,
			.dstOffset = mesh.info.indices.byteOffset,
			.data = data.indices.data(),
			.size = data.indices.size_bytes()
		});

		auto& meshRenderInfo = m_renderInfo.emplace_back(&mesh);

		meshRenderInfo.info = render::GpuPtr<RenderableInfo>::from(
			m_gpu->upload_heap(),
			{
				.name = lib::format("sponza render info:{}", i),
				.bufferUsage = gpu::BufferUsage::Transfer_Dst | gpu::BufferUsage::Storage
			},
			mesh.position,
			normalRange.empty() ? 0ull : mesh.normal,
			uvRange.empty() ? 0ull : mesh.uv
		);

		meshRenderInfo.info->textures[0] = 0;
		meshRenderInfo.info->textures[1] = 1;
		meshRenderInfo.info->textures[2] = 2;
		meshRenderInfo.info->hasUV = std::cmp_not_equal(uvCount, 0);

		++i;
	}
}

auto ModelDemoApp::unpack_materials(render::material::util::MaterialJSON const& materialRep) -> void
{
	using enum render::material::ImageType;

	if (materialRep.materials.empty())
	{
		return;
	}

	m_images.reserve(materialRep.numImages);
	m_images.emplace_back(m_defaultWhiteTexture, m_normalSampler, render::material::ImageType::Base_Color, 1u, 0u);
	m_images.emplace_back(m_defaultMetallicRoughnessMap, m_normalSampler, render::material::ImageType::Metallic_Roughness, 1u, 1u);
	m_images.emplace_back(m_defaultNormalMap, m_normalSampler, render::material::ImageType::Normal, 1u, 2u);

	std::filesystem::path input{ "data/demo/models/sponza/" };

	for (size_t i = 0; auto const& material : materialRep.materials)
	{
		auto& renderInfo = m_renderInfo[i];

		size_t const imageStoredOffset = m_images.size();

		size_t imageCount = {};

		for (auto const& image : material.images)
		{
			input /= image.uri.c_str();

			core::sbf::File const img{ input };

			if (!img.is_open())
			{
				fmt::print("Could not load image asset - {}\n", input);

				continue;
			}

			render::ImageSbfPack imageSbf{ img };

			auto& texture = m_images.emplace_back(
				gpu::Image::from(m_gpu->device(), {
					.name			= lib::format("sponza_{}", std::string_view{ image.uri.c_str(), image.uri.size() }),
					.type			= imageSbf.type(),
					.format			= imageSbf.format(),
					.imageUsage 	= gpu::ImageUsage::Transfer_Dst | gpu::ImageUsage::Sampled,
					.dimension		= imageSbf.dimension(),
					.mipLevel		= 1u,
					.sharingMode 	= gpu::SharingMode::Exclusive
				}),
				m_normalSampler, 
				image.type, 
				1u,
				static_cast<uint32>(imageStoredOffset) + static_cast<uint32>(imageCount)
			);

			texture.image->bind({ .sampler = m_normalSampler, .index = texture.binding });

			auto mipRange = imageSbf.data(0);

			m_gpu->upload_heap().upload_data_to_image({
				.image		= texture.image,
				.data		= mipRange.data(),
				.size		= mipRange.size_bytes(),
				.mipLevel	= 0
			});

			input.remove_filename();
			
			renderInfo.info->textures[std::to_underlying(image.type)] = texture.binding;

			++imageCount;
		}

		if (imageStoredOffset == m_images.size())
		{
			renderInfo.material.images = std::span{ &m_images[0], 3 };
		}
		else
		{
			renderInfo.material.images = std::span{ &m_images[imageStoredOffset], imageCount };
		}

		renderInfo.info.commit();

		++i;
	}
}
}