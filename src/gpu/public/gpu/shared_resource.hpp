#pragma once
#ifndef GPU_SHARED_RESOURCE_HPP
#define GPU_SHARED_RESOURCE_HPP

#include <atomic>
#include "lib/common.hpp"
#include "meta.hpp"

namespace gpu
{
class MemoryBlock;
class Buffer;
class Image;
class Sampler;
class Semaphore;
class Fence;
class Event;
class Swapchain;
class Shader;
class Pipeline;
class CommandPool;
class Device;

using resource_id_t = uint64;

struct ref_counted_base
{
	mutable std::atomic_uint64_t refCount;

	auto reference() const -> void { refCount.fetch_add(1, std::memory_order_relaxed); }
	[[nodiscard]] auto dereference() const -> uint64 { return refCount.fetch_sub(1, std::memory_order_acq_rel) - 1; }
	[[nodiscard]] auto ref_count() const -> uint64 { return refCount.load(std::memory_order_acquire); }
};

template <typename T>
class shared;

struct shared_base
{
	template <typename T>
	requires (std::is_base_of_v<shared_base, std::decay_t<T>>)
	static auto impl_of(T&& resource) -> decltype(auto)
	{
		return resource.__self();
	}
};

template <typename T>
class shared : protected shared_base
{
protected:

	friend shared_base;
	
	ref_counted_base* m_data = nullptr;
	Device* m_device = nullptr;

	static constexpr auto _get_impl = []<typename U>(auto&& base) -> decltype(auto)
	{
		return U::of(base);
	};

	template <typename Self>
	auto __self(this Self&& self) -> decltype(auto)
	{
		return std::forward_like<Self>(_get_impl.template operator()<implementation<T>>(*self.m_data));
	}

	template <typename Self>
	auto __device(this Self&& self) -> auto&&
	{
		return std::forward_like<Self>(_get_impl.template operator()<implementation<Device>>(*self.m_device));
	}

public:

	using resource_type = T;

	shared() = default;
	~shared() { destroy(); }

	shared(ref_counted_base* data, Device* device) : 
		m_data{ data },
		m_device{ device }
	{
		m_data->reference();
	}

	shared(shared const& rhs) :
		m_data{ rhs.m_data },
		m_device{ rhs.m_device }
	{
		m_data->reference();
	}

	shared(shared&& rhs) :
		m_data{ std::exchange(rhs.m_data, {}) },
		m_device{ std::exchange(rhs.m_device, {}) }
	{}

	shared& operator=(shared const& rhs)
	{
		if (this != &rhs)
		{
			destroy();
			
			m_data = rhs.m_data;
			m_device = rhs.m_device;

			if (m_data)
			{
				m_data->reference();
			}
		}
		return *this;
	}

	shared& operator=(shared&& rhs)
	{
		if (this != &rhs)
		{
			destroy();

			m_data = std::exchange(rhs.m_data, {});
			m_device = std::exchange(rhs.m_device, {});
		}
		return *this;
	}

	auto destroy() -> void
	{
		if (m_device &&
			m_data &&
			std::cmp_equal(m_data->dereference(), 0))
		{
			T::zombify(*m_device, *m_data);
		}
		m_data = nullptr;
		m_device = nullptr;
	}

	auto device() -> Device& { return *m_device; }
	auto device() const -> Device const& { return *m_device; }
};
}

#endif // !GPU_SHARED_RESOURCE_HPP