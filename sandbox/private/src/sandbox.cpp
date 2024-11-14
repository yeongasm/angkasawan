#include <fstream>
#include <utility>
#include "sandbox.hpp"
#include "demo/model_demo_app.hpp"

#include "core/file_watcher.h"

namespace sandbox
{

SandboxApp::SandboxApp() :
	m_root_app_window{},
	m_shader_compiler{},
	m_demo_applications{},
	m_showcased_demo{},
	m_gpu{},
	m_swapchain{},
	m_frame_index{}
{}

SandboxApp::~SandboxApp() {}

bool SandboxApp::initialize()
{
	core::Engine& engine = core::engine();

	// TODO(afiq):
	// This is ugly, change it.
	core::io::update_configuration(core::IOConfiguration::Key_Double_Tap_Time, 0.25f);
	core::io::update_configuration(core::IOConfiguration::Key_Min_Duration_For_Hold, 0.5f);
	core::io::update_configuration(core::IOConfiguration::Mouse_Double_Click_Distance, 5.f);
	core::io::update_configuration(core::IOConfiguration::Mouse_Double_Click_Time, 1.f);
	core::io::update_configuration(core::IOConfiguration::Mouse_Min_Duration_For_Hold, 0.05f);

	// initialize file watcher.
	core::filewatcher::initialize_file_watcher();

	m_root_app_window = engine.create_window({
		.title = L"Sandbox",
		.position = { 0, 0 },
		.dimension = { 1920, 1080 },
		.config = {
			.catchInput = true,
			.borderless = true
		}
	});

	if (!m_root_app_window.valid())
	{
		return false;
	}

	engine.register_window_listener(
		m_root_app_window,
		core::wnd::WindowEvent::Close,
		[]() -> void {
		core::Engine& engine = core::engine();
		engine.set_state(core::EngineState::Terminating);
	}, L"on_main_window_close");

#if DEBUG
	constexpr bool validation = true;
#else
	constexpr bool validation = false;
#endif

	if (auto&& result = GraphicsProcessingUnit::from({
		.name = "Main GPU Device",
		.appName = "AngkasawanRenderer",
		.appVersion = { 0, 1, 0, 0 },
		.engineName = "AngkasawanRenderingEngine",
		.engineVersion = { 0, 1, 0, 0 },
		.preferredDevice = gpu::DeviceType::Discrete_Gpu,
		.config = {
			.maxFramesInFlight = 2,
			.swapchainImageCount = 2
		},
		.shadingLanguage = gpu::ShaderLang::GLSL,
		.validation = validation,
		.callback = [](
			[[maybe_unused]] gpu::ErrorSeverity severity,
			[[maybe_unused]] literal_t message
		) -> void
		{
			fmt::print("{}\n\n", message);
		}
	}); result.has_value())
	{
		m_gpu = std::move(*result);
	}

	auto dim = engine.get_window_dimension(m_root_app_window);

	m_swapchain = gpu::Swapchain::from(m_gpu->device(), {
		.name = "main_window",
		.surfaceInfo = {
			.name = "main_window",
			.preferredSurfaceFormats = { gpu::Format::B8G8R8A8_Srgb },
			.instance = engine.get_application_handle(),
			.window = engine.get_window_native_handle(m_root_app_window)
		},
		.dimension = { static_cast<uint32>(dim.width), static_cast<uint32>(dim.height) },
		.imageCount = 4,
		.imageUsage = gpu::ImageUsage::Color_Attachment | gpu::ImageUsage::Transfer_Dst,
		.presentationMode = gpu::SwapchainPresentMode::Mailbox
	});

	m_shader_compiler.add_macro_definition("STORAGE_IMAGE_BINDING", gpu::STORAGE_IMAGE_BINDING);
	m_shader_compiler.add_macro_definition("COMBINED_IMAGE_SAMPLER_BINDING", gpu::COMBINED_IMAGE_SAMPLER_BINDING);
	m_shader_compiler.add_macro_definition("SAMPLED_IMAGE_BINDING", gpu::SAMPLED_IMAGE_BINDING);
	m_shader_compiler.add_macro_definition("SAMPLER_BINDING", gpu::SAMPLER_BINDING);
	m_shader_compiler.add_macro_definition("BUFFER_DEVICE_ADDRESS_BINDING", gpu::BUFFER_DEVICE_ADDRESS_BINDING);

	m_demo_applications.push_back(std::make_unique<ModelDemoApp>(m_root_app_window, m_shader_compiler, *m_gpu, m_swapchain, m_frame_index));

	for (auto&& demo : m_demo_applications)
	{
		if (!demo->initialize())
		{
			return false;
		}
	}

	m_showcased_demo = m_demo_applications.front().get();

	return true;
}

void SandboxApp::run()
{
	gpu::DeviceConfig const& config = m_gpu->device().config();

	if (!m_showcased_demo.is_null())
	{
		m_showcased_demo->run();
	}

	m_frame_index = (m_frame_index + 1) % static_cast<size_t>(config.maxFramesInFlight);
}

void SandboxApp::terminate()
{
	for (auto&& app : m_demo_applications)
	{
		app->terminate();
		auto& deleter = app.get_deleter();
		deleter(app.release());
	}
	
	core::Engine& engine = core::engine();
	engine.destroy_window(m_root_app_window);

	core::filewatcher::terminate_file_watcher();
}

}