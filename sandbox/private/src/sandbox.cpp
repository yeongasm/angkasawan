#include "sandbox.h"
#include "stream/fstream.h"

DECLARE_MODULE(sandbox::Application, SANDBOX_API)

namespace sandbox
{

Application::Application(core::Engine& engine, gpu::Renderer& renderer) :
	m_engine{ &engine },
	m_renderer{ &renderer },
	m_appWindow{ renderer, engine.windowing_manager() }
{}

bool Application::init(core::NativeWindowCreateInfo const& windowInfo)
{
	// Initialize the root window of the application.
	if (!m_appWindow.init(windowInfo))
	{
		return false;
	}
	// Signal the application to terminate when the window is closed.
	m_appWindow.on(core::NativeWindowEvent::Close, [this]() -> void {
		m_engine->set_engine_state(core::EngineState::Terminating);
	});

	if (!setup_render_pipeline())
	{
		return false;
	}

	return true;
}

void Application::terminate()
{
	m_appWindow.terminate();
}

bool Application::setup_render_pipeline()
{
	std::string errorStringBuffer;
	
	// Load default shaders.
	ftl::Ifstream shader{ "data/test_shaders/triangle.glsl" };
	if (!shader.good())
	{
		return false;
	}
	std::string shaderCode;
	shaderCode.resize(shader.size() + 1);
	shader.read(shaderCode.data(), shader.size());

	// Compile vertex shader.
	rhi::ShaderCompileInfo triangleShader{
		.type		= rhi::ShaderType::Vertex,
		.filename	= "data/test_shaders/triangle.glsl",
		.entryPoint = "main",
		.sourceCode = std::move(shaderCode)
	};
	triangleShader.add_preprocessor("VERTEX_SHADER");

	auto vertexShaderHnd = m_renderer->create_shader(triangleShader, &errorStringBuffer);

	if (vertexShaderHnd == gpu::resource_invalid_handle_v<gpu::Shader>)
	{
		return false;
	}

	// Compile fragment shader.
	triangleShader.type = rhi::ShaderType::Fragment;
	triangleShader.clear_preprocessors();
	triangleShader.add_preprocessor("FRAGMENT_SHADER");

	auto fragmentShaderHnd = m_renderer->create_shader(triangleShader, &errorStringBuffer);

	if (fragmentShaderHnd == gpu::resource_invalid_handle_v<gpu::Shader>)
	{
		return false;
	}

	// CTAD rocks!
	std::array triangleRenderShaders{ vertexShaderHnd, fragmentShaderHnd };

	// Create frame graph
	auto graphHandle = m_renderer->create_render_graph("application_frame_graph");
	auto graph = m_renderer->get_render_graph(graphHandle);

	bool result = graph->add_pass({
		.name			= "triangle_render_pass",
		.viewport		= m_appWindow.get_viewport(),
		.pipelineInfo	= {
			.shaders = triangleRenderShaders
		},
		.colorAttachments = {
			gpu::graph::RenderPassAttachmentInfo{
				.name = "triangle_output_attachment",
				.info = gpu::graph::AttachmentInfo{
					.imageInfo = {
						.type		= rhi::ImageType::Image_2D,
						.format		= rhi::ImageFormat::R8G8B8A8_Srgb,
						.samples	= rhi::SampleCount::Sample_Count_1,
						.tiling		= rhi::ImageTiling::Optimal,
						.imageUsage = rhi::ImageUsage::Color_Attachment | rhi::ImageUsage::Transfer_Src,
						.locality	= rhi::MemoryLocality::Gpu,
						.dimension	= m_appWindow.get_extent_3d()
					}
				}
			}
		}/*,
		.execute = [](gpu::command::CommandItem& commandItem) {
			commandItem.bind_pipeline();
			commandItem.set_viewport();
			commandItem.set_scissor();
			commandItem.draw();
		}*/
	},
	&errorStringBuffer);

	if (!result)
	{
		return false;
	}
	
	if (graph.value().compile())
	{
		return false;
	}

	return true;
}

bool sandbox_on_initialize(void* pModule)
{
	Application* app = static_cast<Application*>(pModule);

	core::NativeWindowCreateInfo windowInfo{
		.title = L"Sandbox",
		.position = core::Point{ 0, 0 },
		.dimension = core::Dimension{ 800, 600 },
		.flags = core::Native_Window_Flags_Root_Window | core::Native_Window_Flags_Catch_Input
	};

	if (!app->init(windowInfo))
	{
		return false;
	}

	return true;
}

void sandbox_on_update(void* pModule)
{
	[[maybe_unused]] Application* app = static_cast<Application*>(pModule);
}

void sandbox_on_terminate(void* pModule)
{
	Application* app = static_cast<Application*>(pModule);
	app->~Application();
}

}