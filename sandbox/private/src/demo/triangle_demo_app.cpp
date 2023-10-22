#include <fstream>
#include "demo/triangle_demo_app.h"

namespace sandbox
{

TriangleDemoApp::TriangleDemoApp(
	core::wnd::window_handle rootWindowHandle,
	rhi::util::ShaderCompiler& shaderCompiler,
	rhi::Device& device,
	rhi::Swapchain& rootWindowSwapchain,
	size_t const& frameIndex
) :
	DemoApplication{ rootWindowHandle, shaderCompiler, device, rootWindowSwapchain, frameIndex },
	m_vertex_shader{},
	m_pixel_shader{},
	m_pipeline{},
	m_cmd_pool{},
	m_cmd_buffer_0{},
	m_cmd_buffer_1{}
{}

TriangleDemoApp::~TriangleDemoApp() {}

auto TriangleDemoApp::initialize() -> bool
{
	auto sourceCode = open_file("data/demo/shaders/triangle.glsl");

	if (!sourceCode.size())
	{
		return false;
	}

	rhi::util::ShaderCompileInfo compileInfo = {
		.name = "triangle_vertex_shader",
		.path = "data/demo/shaders/triangle.glsl",
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
	}

	{
		compileInfo.name = "triangle_pixel_shader";
		compileInfo.type = rhi::ShaderType::Pixel;
		compileInfo.clear_macro_definitions();
		compileInfo.add_macro_definition("FRAGMENT_SHADER");

		auto result = m_shader_compiler.compile_shader(compileInfo);

		if (result)
		{
			m_pixel_shader = m_device.create_shader(result.compiled_shader_info().value());
		}
	}

	if (!m_pixel_shader.valid() ||
		!m_vertex_shader.valid())
	{
		return false;
	}

	auto compiledInfo = m_shader_compiler.get_compiled_shader_info(m_vertex_shader.info().name.c_str());

	m_pipeline = m_device.create_pipeline({
		.name = "triangle",
		.colorAttachments = {
			{ .format = m_swapchain.image_format(), .blendInfo = {.enable = false } }
		},
		.rasterization = {
			.cullMode = rhi::CullingMode::Back,
			.frontFace = rhi::FrontFace::Clockwise
		}
	},
	{
		.vertexShader = &m_vertex_shader,
		.pixelShader = &m_pixel_shader
	});

	if (m_pipeline.valid())
	{
		m_cmd_pool = m_device.create_command_pool({ .name = "main_thread", .queue = rhi::DeviceQueueType::Graphics });
		if (m_cmd_pool.valid())
		{
			m_cmd_buffer_0 = &m_cmd_pool.allocate_command_buffer({ .name = "main_thread_frame_0" });
			m_cmd_buffer_1 = &m_cmd_pool.allocate_command_buffer({ .name = "main_thread_frame_1" });
		}
	}

	return true;
}

auto TriangleDemoApp::run() -> void
{
	rhi::CommandBuffer* commandBuffer[] = { m_cmd_buffer_0, m_cmd_buffer_1 };
	rhi::CommandBuffer& cmdBuffer = *commandBuffer[m_frame_index];
	auto&& swapchainImage = m_swapchain.acquire_next_image();

	cmdBuffer.reset();
	cmdBuffer.begin();

	cmdBuffer.pipeline_barrier(
		swapchainImage,
		{
			.dstAccess = rhi::access::FRAGMENT_SHADER_READ,
			.oldLayout = rhi::ImageLayout::Undefined,
			.newLayout = rhi::ImageLayout::Color_Attachment
		}
	);

	rhi::RenderAttachment swapchainImageAttachment = {
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

	cmdBuffer.begin_rendering(info);
	cmdBuffer.set_viewport({
		.width = static_cast<float32>(swapchainImage.info().dimension.width),
		.height = static_cast<float32>(swapchainImage.info().dimension.height)
	});
	cmdBuffer.set_scissor({
		.extent = {
			.width = swapchainImage.info().dimension.width,
			.height = swapchainImage.info().dimension.height
		}
	});
	cmdBuffer.bind_pipeline(m_pipeline);
	cmdBuffer.draw({ .vertexCount = 3 });
	cmdBuffer.end_rendering();

	cmdBuffer.pipeline_barrier(
		swapchainImage,
		{
			.srcAccess = rhi::access::FRAGMENT_SHADER_WRITE,
			.dstAccess = rhi::access::TRANSFER_READ,
			.oldLayout = rhi::ImageLayout::Color_Attachment,
			.newLayout = rhi::ImageLayout::Present_Src
		}
	);
	cmdBuffer.end();

	std::pair signalGpuTimeline{ &m_swapchain.get_gpu_fence(), m_swapchain.cpu_frame_count() };

	m_device.submit({
		.queue = rhi::DeviceQueueType::Main,
		.commandBuffers = std::span{ &cmdBuffer, 1 },
		.waitSemaphores = std::span{ &m_swapchain.current_acquire_semaphore(), 1 },
		.signalSemaphores = std::span{ &m_swapchain.current_present_semaphore(), 1 },
		.signalFences = std::span{ &signalGpuTimeline, 1 }
	});

	m_device.present({ .swapchains = std::span{ &m_swapchain, 1 } });
}

auto TriangleDemoApp::terminate() -> void
{
	m_device.destroy_shader(m_vertex_shader);
	m_device.destroy_shader(m_pixel_shader);
	m_cmd_pool.free_command_buffer(*m_cmd_buffer_0);
	m_cmd_pool.free_command_buffer(*m_cmd_buffer_1);
	m_device.destroy_pipeline(m_pipeline);
	m_device.destroy_command_pool(m_cmd_pool);
}

}