#include <fstream>
#include "triangle_demo_app.h"

namespace sandbox
{

TriangleDemoApp::TriangleDemoApp(
	core::wnd::window_handle rootWindowHandle,
	rhi::Device& device,
	rhi::Swapchain& rootWindowSwapchain,
	size_t const& frameIndex
) :
	DemoApplication{ rootWindowHandle, device, rootWindowSwapchain, frameIndex },
	m_pipeline{},
	m_cmd_pool{},
	m_cmd_buffer_0{},
	m_cmd_buffer_1{}
{}

TriangleDemoApp::~TriangleDemoApp() {}

auto TriangleDemoApp::initialize() -> bool
{
	std::ifstream shader{ "data/test_shaders/triangle.glsl", std::ios_base::in | std::ios_base::binary | std::ios_base::ate };
	if (!shader.good())
	{
		return false;
	}
	rhi::Shader vertexShader;
	rhi::Shader pixelShader;

	rhi::ShaderCompileInfo compileInfo{
		.type = rhi::ShaderType::Vertex,
		.name = "triangle",
		.entryPoint = "main"
	};
	size_t size = shader.tellg();
	shader.seekg(0, std::ios_base::beg);
	compileInfo.sourceCode.reserve(static_cast<uint32>(size) + 1u);
	shader.read(compileInfo.sourceCode.data(), size);
	compileInfo.sourceCode[static_cast<uint32>(size)] = '\0';
	shader.close();

	{
		compileInfo.add_preprocessor("VERTEX_SHADER");
		auto [result, vertexShaderInfo] = rhi::compile_shader(compileInfo);
		if (result)
		{
			vertexShader = m_device.create_shader(std::move(vertexShaderInfo));
		}
	}

	{
		compileInfo.type = rhi::ShaderType::Pixel;
		compileInfo.name = "triangle";
		compileInfo.clear_preprocessors();
		compileInfo.add_preprocessor("FRAGMENT_SHADER");

		auto [result, pixelShaderInfo] = rhi::compile_shader(compileInfo);
		if (result)
		{
			pixelShader = m_device.create_shader(std::move(pixelShaderInfo));
		}
	}

	m_pipeline = m_device.create_pipeline({
		.name = "triangle",
		.vertexShader = &vertexShader,
		.pixelShader = &pixelShader,
		.colorAttachments = {
			{.format = m_swapchain.image_format(), .blendInfo = {.enable = false } }
		},
		.rasterization = {
			.cullMode = rhi::CullingMode::Back,
			.frontFace = rhi::FrontFace::Clockwise
		}
	});

	if (m_pipeline.valid())
	{
		m_cmd_pool = m_device.create_command_pool({ .name = "main_thread", .queue = rhi::DeviceQueueType::Graphics });
		if (m_cmd_pool.valid())
		{
			m_cmd_buffer_0 = &m_cmd_pool.allocate_command_buffer({ .name = "main_thread_frame_0" });
			m_cmd_buffer_1 = &m_cmd_pool.allocate_command_buffer({ .name = "main_thread_frame_1" });
		}
		m_device.destroy_shader(vertexShader);
		m_device.destroy_shader(pixelShader);
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
	m_cmd_pool.free_command_buffer(*m_cmd_buffer_0);
	m_cmd_pool.free_command_buffer(*m_cmd_buffer_1);
	m_device.destroy_pipeline(m_pipeline);
	m_device.destroy_command_pool(m_cmd_pool);
}

}