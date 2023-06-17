#include "resource_store.h"

namespace gpu
{

namespace resource
{

Entry<Swapchain> ResourceStore::store_swapchain(rhi::Swapchain&& swapchain)
{
	auto& data = m_swapchains.emplace(
		++m_id,
		std::move(swapchain)
	);
	return Entry<Swapchain>{ Handle<Swapchain>{ data.first }, data.second };
}

Resource<Swapchain> ResourceStore::get_swapchain(Handle<Swapchain> hnd)
{
	Swapchain* pSwapchain = nullptr;

	if (m_swapchains.contains(hnd.get()))
	{
		pSwapchain = &m_swapchains[hnd.get()];
	}

	return Resource<Swapchain>{ 
		.valid = pSwapchain != nullptr, 
		.resource = pSwapchain 
	};
}

void ResourceStore::remove_swapchain(Handle<Swapchain>& hnd)
{
	m_swapchains.erase(hnd.get());
	new (&hnd) Handle<Swapchain>{ RESOURCE_INVALID_HANDLE };
}

Entry<Shader> ResourceStore::store_shader(rhi::Shader&& shader)
{
	auto& data = m_shaders.emplace(
		++m_id,
		std::move(shader)
	);
	return Entry<Shader>{ Handle<Shader>{ data.first }, data.second };
}

Resource<Shader> ResourceStore::get_shader(Handle<Shader> hnd)
{
	Shader* pShader = nullptr;

	if (m_shaders.contains(hnd.get()))
	{
		pShader = &m_shaders[hnd.get()];
	}

	return Resource<Shader>{
		.valid = pShader != nullptr,
		.resource = pShader
	};
}

void ResourceStore::remove_shader(Handle<Shader>& hnd)
{
	m_shaders.erase(hnd.get());
	new (&hnd) Handle<Shader>{ RESOURCE_INVALID_HANDLE };
}

Entry<RasterPipeline> ResourceStore::store_raster_pipeline(rhi::RasterPipeline&& rasterPipeline)
{
	auto& data = m_rasterPipelines.emplace(
		++m_id,
		std::move(rasterPipeline)
	);
	return Entry<RasterPipeline>{ Handle<RasterPipeline>{ data.first }, data.second };
}

Resource<RasterPipeline> ResourceStore::get_raster_pipeline(Handle<RasterPipeline> hnd)
{
	RasterPipeline* pRasterPipeline = nullptr;

	if (m_rasterPipelines.contains(hnd.get()))
	{
		pRasterPipeline = &m_rasterPipelines[hnd.get()];
	}

	return Resource<RasterPipeline>{
		.valid = pRasterPipeline != nullptr,
		.resource = pRasterPipeline
	};
}

void ResourceStore::remove_raster_pipeline(Handle<RasterPipeline>& hnd)
{
	m_rasterPipelines.erase(hnd.get());
	new (&hnd) Handle<RasterPipeline>{ RESOURCE_INVALID_HANDLE };
}

Entry<Buffer> ResourceStore::store_buffer(rhi::Buffer&& buffer)
{
	auto& data = m_buffers.emplace(
		++m_id,
		std::move(buffer)
	);
	return Entry<Buffer>{ Handle<Buffer>{ data.first }, data.second };
}

Resource<Buffer> ResourceStore::get_buffer(Handle<Buffer> hnd)
{
	Buffer* pBuffer = nullptr;

	if (m_buffers.contains(hnd.get()))
	{
		pBuffer = &m_buffers[hnd.get()];
	}

	return Resource<Buffer>{
		.valid = pBuffer != nullptr,
		.resource = pBuffer
	};
}

void ResourceStore::remove_buffer(Handle<Buffer>& hnd)
{
	m_buffers.erase(hnd.get());
	new (&hnd) Handle<Buffer>{ RESOURCE_INVALID_HANDLE };
}

Entry<Image> ResourceStore::store_image(rhi::Image&& image)
{
	auto& data = m_images.emplace(
		++m_id,
		std::move(image)
	);
	return Entry<Image>{ Handle<Image>{ data.first }, data.second };
}

Resource<Image> ResourceStore::get_image(Handle<Image> hnd)
{
	Image* pImage = nullptr;

	if (m_images.contains(hnd.get()))
	{
		pImage = &m_images[hnd.get()];
	}

	return Resource<Image>{
		.valid = pImage != nullptr,
		.resource = pImage
	};
}

void ResourceStore::remove_image(Handle<Image>& hnd)
{
	m_images.erase(hnd.get());
	new (&hnd) Handle<Image>{ RESOURCE_INVALID_HANDLE };
}

}

}
