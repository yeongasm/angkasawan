#include "renderer.h"

DECLARE_MODULE(gpu::Renderer, RENDERER_API)

namespace gpu
{

bool Renderer::initialize(rhi::DeviceInitInfo const& info)
{
	if (!rhi::create_device(m_device, info))	{ return false; }
	if (!m_taskController.initialize())			{ return false; }
	
	return true;
}

void Renderer::terminate()
{
	destroy_device(m_device);
}

void Renderer::begin_frame()
{
	// Clear the graph queue of any that is marked for execute. This is to support dynamic toggling of executing graphs.
	/*m_graphQueue.empty();*/
}

void Renderer::update()
{
	uint32 const MAX_FRAMES_IN_FLIGHT = m_device.get_device_config().framesInFlight;
	m_frame.index = (m_frame.index + 1) % MAX_FRAMES_IN_FLIGHT;
	++m_frame.count;

	m_device.clear_resources();

	for (graph::task::TaskItem& task : m_graphs.queue)
	{
		m_taskController.process_task_item(task);
	}
	m_taskController.submit_tasks();
}

void Renderer::end_frame()
{
}

Renderer::Renderer(core::Engine& engine) :
	m_engine{ engine },
	m_device{},
	m_store{},
	m_frame{},
	m_stringPool{},
	m_graphs{ .stringPool = m_stringPool, .resourceStore = m_store },
	m_taskController{ .device = &m_device }
{}

Renderer::~Renderer()
{}

Handle<Swapchain> Renderer::create_swapchain(rhi::SwapchainInfo const& swapchainInfo, rhi::SurfaceInfo const& surfaceInfo)
{
	rhi::Swapchain swapchain{
		.surface = {
			.info = surfaceInfo
		},
		.info	= swapchainInfo,
		.state	= rhi::SwapchainState::Ok
	};

	if (!m_device.create_surface_win32(swapchain.surface) ||
		!m_device.create_swapchain(swapchain, nullptr))
	{
		return Handle<Swapchain>{ RESOURCE_INVALID_HANDLE };
	}

	auto const entry = m_store.store_swapchain(std::move(swapchain));

	return entry.hnd;
}

void Renderer::destroy_swapchain(Handle<Swapchain>& hnd)
{
	auto swapchain = m_store.get_swapchain(hnd);
	if (swapchain.valid)
	{
		m_device.destroy_swapchain(*swapchain.resource);
		m_device.destroy_surface(swapchain.resource->surface);
		
		m_store.remove_swapchain(hnd);
	}
}

Handle<Buffer> Renderer::allocate_buffer(rhi::BufferInfo const& info)
{
	rhi::Buffer buffer{
		.info	= info,
		.owner	= rhi::DeviceQueueType::None
	};

	if (!m_device.allocate_buffer(buffer))
	{
		return Handle<Buffer>{ RESOURCE_INVALID_HANDLE };
	}

	auto const entry = m_store.store_buffer(std::move(buffer));

	return entry.hnd;
}

void Renderer::release_buffer(Handle<Buffer>& hnd)
{
	auto buffer = m_store.get_buffer(hnd);
	if (buffer.valid)
	{
		m_device.release_buffer(*buffer.resource);
		m_store.remove_buffer(hnd);
	}
}

Handle<Image> Renderer::create_image(rhi::ImageInfo const& info)
{
	rhi::Image image{
		.info	= info,
		.owner	= rhi::DeviceQueueType::None
	};

	if (!m_device.create_image(image))
	{
		return Handle<Image>{ RESOURCE_INVALID_HANDLE };
	}

	auto const entry = m_store.store_image(std::move(image));

	return entry.hnd;
}

void Renderer::destroy_image(Handle<Image>& hnd)
{
	auto image = m_store.get_image(hnd);
	if (image.valid)
	{
		m_device.destroy_image(*image.resource);
		m_store.remove_image(hnd);
	}
}

Handle<Shader> Renderer::create_shader(rhi::ShaderCompileInfo const& compileInfo, std::string* error)
{
	rhi::ShaderInfo shaderInfo{};
	if (!m_device.compile_shader(compileInfo, shaderInfo, error))
	{
		return Handle<Shader>{ RESOURCE_INVALID_HANDLE };
	}

	rhi::Shader shader{ .info = std::move(shaderInfo) };

	if (!m_device.create_shader(shader))
	{
		return Handle<Shader>{ RESOURCE_INVALID_HANDLE };
	}

	auto const entry = m_store.store_shader(std::move(shader));

	return entry.hnd;
}

void Renderer::destroy_shader(Handle<Shader>& hnd)
{
	auto shader = m_store.get_shader(hnd);
	if (shader.valid)
	{
		m_device.destroy_shader(*shader.resource);
		m_store.remove_shader(hnd);
	}
}

Handle<Graph> Renderer::create_render_graph(std::string_view name)
{
	graph::Context* renderGraphContext = m_graphs.add_render_graph(name);

	if (!renderGraphContext)
	{
		return resource_invalid_handle_v<Graph>;
	}

	size_t bucket = m_graphs.ids.bucket(renderGraphContext->name);

	return Handle<Graph>{ static_cast<uint32>(bucket) };
}

std::optional<Graph> Renderer::get_render_graph(Handle<Graph> hnd)
{
	if (!hnd.is_valid())
	{
		std::nullopt;
	}

	auto map = m_graphs.ids.element_at_bucket(hnd.get());

	if (!map.has_value())
	{
		return std::nullopt;
	}

	size_t index = map.value()->second;

	return Graph{ m_graphs.contexts[index] };
}

uint32 Renderer::current_frame_index() const
{
	return m_frame.index;
}

size_t Renderer::elapsed_frame_count() const
{
	return m_frame.count;
}

// TODO:
// Make this more data driven.
bool renderer_on_initialize(void* pModule)
{
	Renderer* pRenderer = static_cast<Renderer*>(pModule);

	rhi::DeviceInitInfo init{
		.applicationName	= "Sandbox",
		.applicationVersion = { 0, 1, 0, 0 },
		.engineName			= "Angkasa",
		.engineVersion		= { 0, 1, 0, 0 },
		.config				= rhi::device_default_configuration(),
		.shadingLanguage	= rhi::ShaderLang::GLSL,
		.validation			= true,
		.data				= pRenderer,

		.callback = [](rhi::ErrorSeverity severity, literal_t message, void*) -> void {
			fmt::print("{}\n\n", message);
		}
	};

	return pRenderer->initialize(init);
}

void renderer_on_update(void* pModule)
{
	Renderer* pRenderer = static_cast<Renderer*>(pModule);

	pRenderer->begin_frame();
	pRenderer->update();
	pRenderer->end_frame();
}

void renderer_on_terminate(void* pModule)
{
	Renderer* pRenderer = static_cast<Renderer*>(pModule);
	pRenderer->terminate();
	pRenderer->~Renderer();
}

}