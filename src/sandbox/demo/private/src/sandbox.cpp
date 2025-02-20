#include <utility>
#include "sandbox_settings.hpp"
#include "sandbox.hpp"
#include "gpu/constants.hpp"
#include "model_demo_app.hpp"

#include "core.platform/file_watcher.hpp"
#include "core.cmdline/cmdline.hpp"

namespace sandbox
{
SandboxApp::SandboxApp([[maybe_unused]] int32 argc, [[maybe_unused]] char** argv) :
	m_rootWindow{},
	m_gpu{},
	m_applet{},
	m_isRunning{true}
{
	core::platform::IOContext::update_configuration({
		.keyDoubleTapTime			= 0.25f,
		.keyMinDurationForHold		= 0.5f,
		.mouseDoubleClickTime		= 1.f,
		.mouseDoubleClickDistance	= 5.f,
		.mouseMinDurationForHold	= 0.05f
	});

	core::filewatcher::initialize_file_watcher();

	// Create root window.
	{
		auto&& result = core::platform::Window::from(
			*this, 
			{
				.title = L"Sandbox",
				.position = { 0, 0 },
				.dimension = { 1024, 768 },
				.config = core::platform::WindowConfig::Borderless | core::platform::WindowConfig::Catch_Input
			}
		);
		if (m_isRunning = result.has_value(); !result)
		{
			return;
		}
		m_rootWindow = std::move(*result);
	}

	m_rootWindow->push_listener(
		core::platform::WindowEvent::Type::Focus, 
		[this]([[maybe_unused]] core::platform::WindowEvent info) -> void 
		{
			enable_io_state_update(m_rootWindow->is_focused() && !m_rootWindow->is_minimized());
		}
	);
	m_rootWindow->push_listener(
		core::platform::WindowEvent::Type::Close,
		[this]([[maybe_unused]] core::platform::WindowEvent info) -> void
		{
			// NOTE(afiq):
			// Possibly close all child windows in the future.
			terminate();
		}
	);

	// Create GPU interface.
	{
		auto&& result = render::AsyncDevice::from({
			.name = "Main GPU Device",
			.appName = "AngkasawanRenderer",
			.appVersion = { 0, 1, 0, 0 },
			.engineName = "AngkasawanRenderingEngine",
			.engineVersion = { 0, 1, 0, 0 },
			.preferredDevice = gpu::DeviceType::Discrete_Gpu,
			.config = {
				.maxFramesInFlight = 2,
				.swapchainImageCount = 3,
				.maxBuffers = gpu::MAX_BUFFERS,
				.maxImages = gpu::MAX_IMAGES,
				.maxSamplers = gpu::MAX_SAMPLERS,
				.pushConstantMaxSize = std::numeric_limits<uint32>::max()
			},
			.callback = [](
				[[maybe_unused]] gpu::ErrorSeverity severity,
				[[maybe_unused]] literal_t message
			) -> void
			{
				fmt::print("{}\n\n", message);
			}
		});
		if (m_isRunning = result.has_value(); !result)
		{
			return;
		}
		m_gpu = std::move(*result);
	}

	m_isRunning = m_applet.start(*this, m_rootWindow, *m_gpu);;
}

auto SandboxApp::run() -> void
{
	while (m_isRunning)
	{
		core::platform::Application::update();
		m_applet.run();

		reset_mouse_wheel_state();
		destroy_windows();
	}
}

auto SandboxApp::stop() -> void
{
	m_applet.stop();

	m_rootWindow.destroy();
	core::filewatcher::terminate_file_watcher();

	core::platform::WindowContext::destroy_windows();
}

auto SandboxApp::terminate() -> void
{
	m_isRunning = false;
}
}