#pragma once
#ifndef RENDERER_PRIVATE_RESOURCE_CACHE_H
#define RENDERER_PRIVATE_RESOURCE_CACHE_H

#include "containers/map.h"
#include "types/handle.h"
#include "rhi.h"

namespace gpu
{

template <typename T>
using Handle = ftl::Handle<T, uint32>;
inline static constexpr uint32 const RESOURCE_INVALID_HANDLE = std::numeric_limits<uint32>::max();
template <typename T>
inline static constexpr Handle<T> resource_invalid_handle_v{ RESOURCE_INVALID_HANDLE };


using Swapchain			= rhi::Swapchain;
using RasterPipeline	= rhi::RasterPipeline;
using Buffer			= rhi::Buffer;
using Image				= rhi::Image;
using Shader			= rhi::Shader;

namespace resource
{

template <typename T>
struct Entry
{
	Handle<T>	hnd;
	ftl::Ref<T>	resource;

	bool valid() const 
	{ 
		return hnd != resource_invalid_handle_v<T>; 
	}
};

template <typename T>
struct Resource
{
	bool const	valid;
	ftl::Ref<T>	resource;
};

class ResourceStore
{
private:
	template <typename T>
	using ResourceMap = ftl::Map<uint32, T>;

	ResourceMap<Swapchain>		m_swapchains;
	ResourceMap<RasterPipeline>	m_rasterPipelines;
	ResourceMap<Buffer>			m_buffers;
	ResourceMap<Image>			m_images;
	ResourceMap<Shader>			m_shaders;
	uint32						m_id;
public:
	ResourceStore()		= default;
	~ResourceStore()	= default;

	Entry<Swapchain>			store_swapchain			(rhi::Swapchain&& swapchain);
	Resource<Swapchain>			get_swapchain			(Handle<Swapchain> hnd);
	void						remove_swapchain		(Handle<Swapchain>& hnd);
	Entry<Shader>				store_shader			(rhi::Shader&& shader);
	Resource<Shader>			get_shader				(Handle<Shader> hnd);
	void						remove_shader			(Handle<Shader>& hnd);
	Entry<RasterPipeline>		store_raster_pipeline	(rhi::RasterPipeline&& rasterPipeline);
	Resource<RasterPipeline>	get_raster_pipeline		(Handle<RasterPipeline> hnd);
	void						remove_raster_pipeline	(Handle<RasterPipeline>& hnd);
	Entry<Buffer>				store_buffer			(rhi::Buffer&& buffer);
	Resource<Buffer>			get_buffer				(Handle<Buffer> hnd);
	void						remove_buffer			(Handle<Buffer>& hnd);
	Entry<Image>				store_image				(rhi::Image&& image);
	Resource<Image>				get_image				(Handle<Image> hnd);
	void						remove_image			(Handle<Image>& hnd);
};

}

}

#endif // !RENDERER_PRIVATE_RESOURCE_CACHE_H
