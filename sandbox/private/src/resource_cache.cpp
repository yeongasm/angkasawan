#include "resource_cache.h"

namespace sandbox
{
ResourceCache::ResourceCache(rhi::Device& device) :
	m_device{ device },
	m_images{}
{}

auto ResourceCache::device() const -> rhi::Device&
{
	return m_device;
}

auto ResourceCache::create_buffer(rhi::BufferInfo&& info) -> Resource<rhi::Buffer>
{
	uint32 idx = std::numeric_limits<uint32>::max();
	rhi::Buffer* out = nullptr;

	auto&& [location, buf] = m_buffers.emplace(m_device.allocate_buffer(std::move(info)));

	if (buf.valid())
	{
		idx = location.to_uint32();
		out = &buf;
	}
	else
	{
		m_buffers.erase(location);
	}

	return Resource<rhi::Buffer>{ idx, out };
}

auto ResourceCache::get_buffer(resource_handle<Buffer> handle) -> lib::ref<rhi::Buffer>
{
	using index = typename decltype(m_buffers)::index;
	
	return lib::ref<rhi::Buffer>{ m_buffers.at(index::from(handle.access(*this))) };
}

auto ResourceCache::destroy_buffer(resource_handle<Buffer> handle) -> void
{
	using index = typename decltype(m_buffers)::index;

	uint32 const location = handle.access(*this);

	rhi::Buffer* buffer = m_buffers.at(index::from(location));
	if (buffer &&
		buffer->valid())
	{
		m_device.release_buffer(*buffer);
		m_buffers.erase(index::from(location));
	}
}

auto ResourceCache::create_image(rhi::ImageInfo&& info) -> Resource<rhi::Image>
{
	uint32 idx = std::numeric_limits<uint32>::max();
	rhi::Image* out = nullptr;

	auto&& [location, img] = m_images.emplace(m_device.create_image(std::move(info)));

	if (img.valid())
	{
		idx = location.to_uint32();
		out = &img;
	}
	else
	{
		m_images.erase(location);
	}

	return Resource<rhi::Image>{ idx, out };
}

auto ResourceCache::get_image(resource_handle<Image> handle) -> lib::ref<rhi::Image>
{
	using index = typename decltype(m_images)::index;

	return lib::ref<rhi::Image>{ m_images.at(index::from(handle.access(*this))) };
}

auto ResourceCache::destroy_image(resource_handle<Image> handle) -> void
{
	using index = typename decltype(m_images)::index;

	uint32 const location = handle.access(*this);

	rhi::Image* img = m_images.at(index::from(location));
	if (img && 
		img->valid())
	{
		m_device.destroy_image(*img);
		m_images.erase(index::from(location));
	}
}
}