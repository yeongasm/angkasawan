#include <fstream>
#include <utility>
#include "sandbox.h"
#include "triangle_demo_app.h"

namespace sandbox
{

SandboxApp::SandboxApp() :
	m_root_app_window{},
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
	m_root_app_window = engine.create_window({
		.title = L"Sandbox",
		.position = { 0, 0 },
		.dimension = { 800, 600 }
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
	},
		L"on_main_window_close"
	);
	m_instance = rhi::create_instance();
	if (!m_instance)
	{
		return false;
	}
	m_device = &rhi::create_device(
		m_instance,
		{
			.name = "Main GPU Device",
			.appName = "AngkasawanRenderer",
			.appVersion = { 0, 1, 0, 0 },
			.engineName = "AngkasawanRenderingEngine",
			.engineVersion = { 0, 1, 0, 0 },
			.preferredDevice = rhi::DeviceType::Discrete_Gpu,
			.config = { .framesInFlight = 2, .swapchainImageCount = 2 },
			.shadingLanguage = rhi::ShaderLang::GLSL,
			.validation = true,
			.callback = [](
				[[maybe_unused]] rhi::ErrorSeverity severity,
				[[maybe_unused]] literal_t message
			) -> void
			{
				fmt::print("{}\n\n", message);
			}
		}
	);
	auto dim = engine.get_window_dimension(m_root_app_window);
	m_swapchain = m_device->create_swapchain({
		.name = "main_window",
		.surfaceInfo = {
			.name = "main_window",
			.preferredSurfaceFormats = { rhi::ImageFormat::B8G8R8A8_Srgb },
			.instance = engine.get_application_handle(),
			.window = engine.get_window_native_handle(m_root_app_window)
		},
		.dimension = { static_cast<uint32>(dim.width), static_cast<uint32>(dim.height) },
		.imageCount = 4,
		.imageUsage = rhi::ImageUsage::Color_Attachment | rhi::ImageUsage::Transfer_Dst,
		.presentationMode = rhi::SwapchainPresentMode::Mailbox
	});

	m_demo_applications.push_back(std::make_unique<TriangleDemoApp>(m_root_app_window, *m_device, m_swapchain, m_frame_index));

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

	m_frame_index = (m_frame_index + 1) % static_cast<size_t>(config.framesInFlight);
}

void SandboxApp::terminate()
{
	for (auto&& app : m_demo_applications)
	{
		app->terminate();
	}
	m_device->destroy_swapchain(m_swapchain, true);
	rhi::destroy_device(*m_device);
	rhi::destroy_instance();
	core::Engine& engine = core::engine();
	engine.destroy_window(m_root_app_window);
}

}