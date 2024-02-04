#pragma once
#ifndef SANDBOX_RESOURCE_CACHE_H
#define SANDBOX_RESOURCE_CACHE_H

#include "lib/paged_array.h"
#include "lib/handle.h"
#include "rhi/device.h"

namespace sandbox
{
/**
* A Resource<T> is just a compressed pair that contains a reference to the resource and it's handle.
*/
template <typename T>
class Resource : protected lib::ref<T>
{
public:
	using super = lib::ref<T>;
	using resource_type = std::decay_t<T>;
	using pointer = resource_type*;
	using const_pointer = resource_type const*;
	using resource_handle = lib::handle<T, uint32, std::numeric_limits<uint32>::max()>;

	Resource() = default;
	~Resource() = default;

	Resource(Resource const& rhs) :
		m_hnd{ rhs.m_hnd },
		super{ rhs }
	{}

	auto operator=(Resource const& rhs) -> Resource&
	{
		if (this != &rhs)
		{
			m_hnd = rhs.m_hnd;
			super::operator=(rhs);
		}
		return *this;
	}

	Resource(Resource&& rhs) noexcept :
		m_hnd{ std::move(rhs.m_hnd) },
		super{ std::move(rhs) }
	{}

	auto operator=(Resource&& rhs) noexcept -> Resource&
	{
		if (this != &rhs)
		{
			m_hnd = std::move(rhs.m_hnd);
			super::operator=(std::move(rhs));
			new (&rhs) Resource{};
		}
		return *this;
	}

	using super::operator->;
	using super::operator*;
	using super::is_null;

	auto id() -> resource_handle { return m_hnd; }
private:
	friend class ResourceCache;
	friend class UploadHeap;
	friend class BufferViewRegistry;

	resource_handle m_hnd;

	Resource(uint32 handleValue, T* data) :
		m_hnd{ handleValue },
		super{ data }
	{}
};

using buffer_handle = Resource<rhi::Buffer>::resource_handle;
using image_handle	= Resource<rhi::Image>::resource_handle;

class ResourceCache
{
public:
	ResourceCache(rhi::Device& device);
	~ResourceCache() = default;

	auto device() const -> rhi::Device&;

	auto create_buffer(rhi::BufferInfo&& info) -> Resource<rhi::Buffer>;
	auto get_buffer(buffer_handle handle) -> lib::ref<rhi::Buffer>;
	auto destroy_buffer(buffer_handle handle) -> void;

	auto create_image(rhi::ImageInfo&& info) -> Resource<rhi::Image>;
	auto get_image(image_handle handle) -> lib::ref<rhi::Image>;
	auto destroy_image(image_handle handle) -> void;
private:
	rhi::Device& m_device;
	lib::paged_array<rhi::Buffer, 64>	m_buffers;
	lib::paged_array<rhi::Image, 64>	m_images;
};
}

#endif // !SANDBOX_RESOURCE_CACHE_H

