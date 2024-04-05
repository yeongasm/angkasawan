#pragma once
#ifndef SANDBOX_RESOURCE_CACHE_H
#define SANDBOX_RESOURCE_CACHE_H

#include "lib/paged_array.h"
#include "lib/handle.h"
#include "rhi/device.h"

namespace sandbox
{
//using rhi::ShaderInfo;
//using rhi::BufferInfo;
//using rhi::ImageInfo;
//using rhi::SamplerInfo;
//using rhi::SwapchainInfo;
//using rhi::SemaphoreInfo;
//using rhi::FenceInfo;
//using rhi::RasterPipelineInfo;

using rhi::Device;
using rhi::Shader;	// Shaders should be a transient resource that's automatically deleted.
using rhi::Buffer;
using rhi::Image;
using rhi::Sampler;
using rhi::Swapchain;
using rhi::Semaphore;
using rhi::Fence;
using rhi::RasterPipeline;

using rhi::ResourceDeleter;

template <typename T>
using resource_handle = lib::opaque_handle<T, uint32, std::numeric_limits<uint32>::max(), class ResourceCache>;

/**
* A Resource<T> is just a compressed pair that contains a reference to the resource and it's handle.
* Resource<T> are persistent.
*/
template <typename T>
class Resource : protected lib::ref<T>
{
public:
	using resource_type = T;
	using super = lib::ref<resource_type>;
	using resource_handle = resource_handle<resource_type>;

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

	auto id() -> resource_handle { return m_hnd; }
private:
	friend class ResourceCache;
	friend class UploadHeap;

	resource_handle m_hnd;

	Resource(uint32 handleValue, T* data) :
		m_hnd{ handleValue },
		super{ data }
	{}
};

/**
* Transient<T, Deleter> is a compressed pair that contains the resource and it's deleter.
* Transient resources are move-only.
*/
template <typename T>
class Transient
{
public:
	using resource_type = T;
	using deleter_type	= ResourceDeleter<resource_type>;

	Transient() = default;
	~Transient() { deleter_type{}(m_resource); }

	Transient(Transient const&)						= delete;
	auto operator=(Transient const&) -> Transient&	= delete;

	Transient(Transient&& rhs) noexcept :
		m_resource{ std::move(rhs.m_resource) }
	{}

	auto operator=(Transient&& rhs) noexcept -> Transient& 
	{
		if (this != &rhs)
		{
			m_resource = std::move(rhs.m_resource);
			new (&rhs) Transient{};
		}
		return *this;
	}

	auto resource() const -> resource_type& { return m_resource; }
private:
	resource_type m_resource;
};

class ResourceCache
{
public:
	ResourceCache(Device& device);
	~ResourceCache() = default;

	auto device() const->rhi::Device&;

	auto create_buffer(rhi::BufferInfo&& info) -> Resource<Buffer>;
	auto get_buffer(resource_handle<Buffer> handle) -> lib::ref<rhi::Buffer>;
	auto destroy_buffer(resource_handle<Buffer> handle) -> void;

	auto create_image(rhi::ImageInfo&& info) -> Resource<rhi::Image>;
	auto get_image(resource_handle<Image> handle) -> lib::ref<rhi::Image>;
	auto destroy_image(resource_handle<Image> handle) -> void;
private:
	template <typename T> using Container = lib::paged_array<T, 64>;
	template <typename T> using resource_index = typename Container<T>::index;

	Device& m_device;
	Container<Buffer> m_buffers;
	Container<Image> m_images;
	Container<Sampler> m_samplers;
	Container<Semaphore> m_semaphores;
	Container<Fence> m_fences;
};
}

#endif // !SANDBOX_RESOURCE_CACHE_H

