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
	uint32 index = std::numeric_limits<uint32>::max();
	rhi::Buffer* out = nullptr;

	auto&& [location, buf] = m_buffers.emplace(m_device.allocate_buffer(std::move(info)));

	if (buf.valid())
	{
		index = location.id;
		out = &buf;
	}
	else
	{
		m_buffers.erase(location);
	}

	return Resource<rhi::Buffer>{ index, out };
}

auto ResourceCache::get_buffer(buffer_handle handle) -> lib::ref<rhi::Buffer>
{
	return lib::ref<rhi::Buffer>{ m_buffers.at(static_cast<uint32>(handle.get())) };
}

auto ResourceCache::destroy_buffer(buffer_handle handle) -> void
{
	uint32 const location = handle.get();

	rhi::Buffer* buffer = m_buffers.at(location);
	if (buffer &&
		buffer->valid())
	{
		m_device.release_buffer(*buffer);
		m_buffers.erase(location);
	}
}

auto ResourceCache::create_image(rhi::ImageInfo&& info) -> Resource<rhi::Image>
{
	uint32 index = std::numeric_limits<uint32>::max();
	rhi::Image* out = nullptr;

	auto&& [location, img] = m_images.emplace(m_device.create_image(std::move(info)));

	if (img.valid())
	{
		index = location.id;
		out = &img;
	}
	else
	{
		m_images.erase(location);
	}

	return Resource<rhi::Image>{ index, out };
}

auto ResourceCache::get_image(image_handle handle) -> lib::ref<rhi::Image>
{
	return lib::ref<rhi::Image>{ m_images.at(static_cast<uint32>(handle.get())) };
}

auto ResourceCache::destroy_image(image_handle handle) -> void
{
	uint32 const location = handle.get();

	rhi::Image* img = m_images.at(location);
	if (img && 
		img->valid())
	{
		m_device.destroy_image(*img);
		m_images.erase(location);
	}
}
}