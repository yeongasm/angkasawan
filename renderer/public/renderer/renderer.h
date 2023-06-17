#pragma once
#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include "types/handle.h"
#include "engine.h"
#include "task_controller.h"

namespace gpu
{

// GPU wrapper. Named Renderer for legacy reasons.
class Renderer final
{
private:
	ftl::Ref<core::Engine>		m_engine;
	rhi::RenderDevice			m_device;
	resource::ResourceStore		m_store;
	rhi::FrameInfo				m_frame;
	ftl::StringPool				m_stringPool;
	graph::RenderGraphStore		m_graphs;
	task_controller::TaskController m_taskController;
	/*graph::GraphContextQueue	m_graphQueue;*/

	bool	initialize				(rhi::DeviceInitInfo const& info);
	void	terminate				();
	void	begin_frame				();
	void	update					();
	void	end_frame				();

	friend bool renderer_on_initialize	(void*);
	friend void renderer_on_update		(void*);
	friend void renderer_on_terminate	(void*);
public:
	Renderer(core::Engine& engine);
	~Renderer();
	
	RENDERER_API Handle<Swapchain>		create_swapchain		(rhi::SwapchainInfo const& swapchainInfo, rhi::SurfaceInfo const& surfaceInfo);
	RENDERER_API void					destroy_swapchain		(Handle<Swapchain>& hnd);
	RENDERER_API Handle<Buffer>			allocate_buffer			(rhi::BufferInfo const& info);
	RENDERER_API void					release_buffer			(Handle<Buffer>& hnd);
	RENDERER_API Handle<Image>			create_image			(rhi::ImageInfo const& info);
	RENDERER_API void					destroy_image			(Handle<Image>& hnd);
	RENDERER_API Handle<Shader>			create_shader			(rhi::ShaderCompileInfo const& compileInfo, std::string* error = nullptr);
	RENDERER_API void					destroy_shader			(Handle<Shader>& hnd);
	RENDERER_API Handle<Graph>			create_render_graph		(std::string_view name);
	RENDERER_API std::optional<Graph>	get_render_graph		(Handle<Graph> hnd);
	/*RENDERER_API bool					run_render_graph		(Handle<Graph> hnd);
	RENDERER_API bool					stop_render_graph		(Handle<Graph> hnd);*/
	RENDERER_API uint32					current_frame_index		() const;
	RENDERER_API size_t					elapsed_frame_count		() const;

private:
	MAKE_SINGLETON(Renderer)
};

bool renderer_on_initialize(void*);
void renderer_on_update(void*);
void renderer_on_terminate(void*);

}

#endif // !RENDERER_RENDERER_H
