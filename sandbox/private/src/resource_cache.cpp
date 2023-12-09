#include "resource_cache.h"

namespace sandbox
{
resource_index::resource_index(uint32 parent, uint32 id) :
	_metadata{ parent, id }
{}

resource_index::resource_index(uint64 alias) :
	_alias{ alias }
{}

ResourceCache::ResourceCache(rhi::Device& device) :
	m_device{ device },
	m_images{}
{}

auto ResourceCache::create_image(rhi::ImageInfo&& info) -> Resource<rhi::Image>
{
	resource_index idx = std::numeric_limits<uint64>::max();
	rhi::Image* out = nullptr;

	auto&& [index, img] = m_images.emplace(m_device.create_image(std::move(info)));

	if (img.valid())
	{
		idx._metadata.id = index.id;
		out = &img;
	}
	else
	{
		m_images.erase(index);
	}

	return Resource<rhi::Image>{ idx._alias, out };
}

auto ResourceCache::get_image(image_handle handle) -> lib::ref<rhi::Image>
{
	return lib::ref<rhi::Image>{ m_images.at(handle.get()) };
}

auto ResourceCache::destroy_image(image_handle handle) -> void
{
	rhi::Image* img = m_images.at(handle.get());
	if (img && 
		img->valid())
	{
		img->unbind();
		m_device.destroy_image(*img);
		m_images.erase(handle.get());
	}
}

auto ResourceCache::create_buffer(rhi::BufferInfo&& info) -> Resource<rhi::Buffer>
{
	resource_index idx = std::numeric_limits<uint64>::max();
	rhi::Buffer* out = nullptr;

	auto&& [index, buf] = m_buffers.emplace(m_device.allocate_buffer(std::move(info)));

	if (buf.valid())
	{
		idx._metadata.id = index.id;
		out = &buf;
	}
	else
	{
		m_buffers.erase(index);
	}

	return Resource<rhi::Buffer>{ idx._alias, out };
}

auto ResourceCache::get_buffer(buffer_handle handle) -> lib::ref<rhi::Buffer>
{
	return lib::ref<rhi::Buffer>{ m_buffers.at(handle.get()) };
}

auto ResourceCache::destroy_buffer(buffer_handle handle) -> void
{
	rhi::Buffer* buffer = m_buffers.at(handle.get());
	if (buffer &&
		buffer->valid())
	{
		buffer->unbind();
		m_device.release_buffer(*buffer);
		m_buffers.erase(handle.get());
	}
}

auto ResourceCache::create_fence(rhi::FenceInfo&& info) -> Resource<rhi::Fence>
{
	resource_index idx = std::numeric_limits<uint64>::max();
	rhi::Fence* out = nullptr;

	auto&& [index, buf] = m_fences.emplace(m_device.create_fence(std::move(info)));

	if (buf.valid())
	{
		idx._metadata.id = index.id;
		out = &buf;
	}
	else
	{
		m_fences.erase(index);
	}

	return Resource<rhi::Fence>{ idx._alias, out };
}

auto ResourceCache::get_fence(fence_handle handle) -> lib::ref<rhi::Fence>
{
	return lib::ref<rhi::Fence>{ m_fences.at(handle.get()) };
}

auto ResourceCache::destroy_fence(fence_handle handle) -> void
{
	rhi::Fence* fence = m_fences.at(handle.get());
	if (fence &&
		fence->valid())
	{
		m_device.destroy_fence(*fence);
		m_fences.erase(handle.get());
	}
}
}