#include <fstream>
#include <utility>
#include "sandbox.h"
#include "demo/triangle_demo_app.h"
#include "demo/model_demo_app.h"

namespace sandbox
{

SandboxApp::SandboxApp() :
	m_root_app_window{},
	m_shader_compiler{},
	m_demo_applications{},
	m_showcased_demo{},
	m_instance{},
	m_device{},
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

	m_instance = rhi::create_instance();

	m_device = &m_instance->create_device({
		.name = "Main GPU Device",
		.appName = "AngkasawanRenderer",
		.appVersion = { 0, 1, 0, 0 },
		.engineName = "AngkasawanRenderingEngine",
		.engineVersion = { 0, 1, 0, 0 },
		.preferredDevice = rhi::DeviceType::Discrete_Gpu,
		.config = { 
			.maxFramesInFlight = 2, 
			.swapchainImageCount = 2 
		},
		.shadingLanguage = rhi::ShaderLang::GLSL,
		.validation = true,
		.callback = [](
			[[maybe_unused]] rhi::ErrorSeverity severity,
			[[maybe_unused]] literal_t message
		) -> void
		{
			fmt::print("{}\n\n", message);
		}
	});

	auto dim = engine.get_window_dimension(m_root_app_window);
	m_swapchain = m_device->create_swapchain({
		.name = "main_window",
		.surfaceInfo = {
			.name = "main_window",
			.preferredSurfaceFormats = { rhi::Format::B8G8R8A8_Srgb },
			.instance = engine.get_application_handle(),
			.window = engine.get_window_native_handle(m_root_app_window)
		},
		.dimension = { static_cast<uint32>(dim.width), static_cast<uint32>(dim.height) },
		.imageCount = 4,
		.imageUsage = rhi::ImageUsage::Color_Attachment | rhi::ImageUsage::Transfer_Dst,
		.presentationMode = rhi::SwapchainPresentMode::Mailbox
	});

	m_shader_compiler.add_macro_definition("STORAGE_IMAGE_BINDING", rhi::STORAGE_IMAGE_BINDING);
	m_shader_compiler.add_macro_definition("COMBINED_IMAGE_SAMPLER_BINDING", rhi::COMBINED_IMAGE_SAMPLER_BINDING);
	m_shader_compiler.add_macro_definition("SAMPLED_IMAGE_BINDING", rhi::SAMPLED_IMAGE_BINDING);
	m_shader_compiler.add_macro_definition("SAMPLER_BINDING", rhi::SAMPLER_BINDING);
	m_shader_compiler.add_macro_definition("BUFFER_DEVICE_ADDRESS_BINDING", rhi::BUFFER_DEVICE_ADDRESS_BINDING);

	//m_demo_applications.push_back(std::make_unique<TriangleDemoApp>(m_root_app_window, m_shader_compiler, *m_device, m_swapchain, m_frame_index));
	m_demo_applications.push_back(std::make_unique<ModelDemoApp>(m_root_app_window, m_shader_compiler, *m_device, m_swapchain, m_frame_index));

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
	rhi::DeviceConfig const& config = m_device->device_config();

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
	}
	m_device->destroy_swapchain(m_swapchain, true);
	m_instance->destroy_device(*m_device);
	rhi::destroy_instance();
	core::Engine& engine = core::engine();
	engine.destroy_window(m_root_app_window);
}

}