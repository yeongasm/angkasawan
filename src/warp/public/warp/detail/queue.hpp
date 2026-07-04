#pragma once
#include "lib/common.hpp"
#include <concepts>
#include <type_traits>
#ifndef WARP_QUEUE_HPP
#define WARP_QUEUE_HPP

#include <new>
#include <atomic>
#include "lib/memory.hpp"

#include "common.hpp"

namespace warp
{
/*
* Note(afiq):
* Reference implementation	- https://github.com/CharlesFrasch/cppcon2023/blob/main/Fifo4a.hpp
*							- https://github.com/cameron314/readerwriterqueue/blob/master/readerwritercircularbuffer.h
*
* - This should be a container adaptor that treats the underlying container like a SPSC queue.
*	- We can have 2 containers for this adaptor:
*		1. circular buffer
*		2. linked-list of fixed sized queues.
* - For now, let this be a circullar buffer.
* - We probably want different ways to retrieve the current index dependending if the capacity is a power of 2 or not.
*	- moodycamel's readerwritercircularbuffer has a neat trick that's worth checking out https://github.com/cameron314/readerwriterqueue/blob/master/readerwritercircularbuffer.h.
* - Perhaps have a similar interface as std::queue and since std::queue does not support iterators, neither should we.
*
*/
// template <
// 	typename T,
// 	size_t Capacity
// >
// class queue final : lib::non_copyable_non_movable
// {
// public:
// 	using value_type				= T;
// 	// using allocator_type			= allocator;
// 	using size_type                 = size_t;
// 	using difference_type           = std::ptrdiff_t;
// 	using reference                 = value_type&;
// 	using const_reference           = value_type const&;
// 	using pointer                   = value_type*;
// 	using const_pointer             = value_type const*;

// 	static_assert(std::is_reference_v<T> == false, "queue does not support storing reference types.");
// 	static_assert(std::is_nothrow_move_assignable_v<value_type>, "element stored in the queue has to be move assignable.");

// 	queue() = default;
// 	~queue()
// 	{
// 		while (!empty())
// 		{
// 			auto popCursor = m_popCursor.load(std::memory_order_acquire);
// 			std::destroy_at(m_data.data(), popCursor);
// 			m_popCursor.store(popCursor + 1, std::memory_order_release);
// 		}
// 	}

// 	template <typename... Args>
// 	auto emplace(Args&&... args) noexcept -> bool
// 	{
// 		auto pushCursor = m_pushCursor.load(std::memory_order_relaxed);

// 		if (_is_full(pushCursor, m_cachedPopCursor))
// 		{
// 			m_cachedPopCursor = m_popCursor.load(std::memory_order_acquire);
// 			if (_is_full(pushCursor, m_cachedPopCursor))
// 			{
// 				return false;
// 			}
// 		}

// 		_emplace_at(_location(pushCursor), std::forward<Args>(args)...);
// 		m_pushCursor.store(pushCursor + 1, std::memory_order_release);

// 		return true;
// 	}

// 	auto pop(value_type& element) noexcept -> bool
// 	{
// 		auto popCursor = m_popCursor.load(std::memory_order_relaxed);
// 		if (_is_empty(popCursor, m_cachedPushCursor))
// 		{
// 			m_cachedPushCursor = m_pushCursor.load(std::memory_order_relaxed);
// 			if (_is_empty(popCursor, m_cachedPushCursor))
// 			{
// 				return false;
// 			}
// 		}

// 		element = std::move(m_data[popCursor]);
// 		std::destroy_at(m_data.data() + popCursor);

// 		m_popCursor.store(popCursor + 1, std::memory_order_release);

// 		return true;
// 	}

// 	auto size() const noexcept -> size_type
// 	{
// 		auto pushCursor = m_pushCursor.load(std::memory_order_relaxed);
// 		auto popCursor = m_popCursor.load(std::memory_order_relaxed);

// 		return pushCursor - popCursor;
// 	}

// 	auto full() const noexcept -> bool
// 	{
// 		return size() == Capacity;
// 	}

// 	auto empty() const noexcept -> bool
// 	{
// 		return size() == 0;
// 	}

// 	auto capacity() const noexcept -> size_type
// 	{
// 		return Capacity;
// 	}

// 	auto clear() noexcept -> void
// 	{
// 		if constexpr (!std::is_standard_layout_v<value_type> && !std::is_trivially_destructible_v<value_type>)
// 		{
// 			while (!empty())
// 			{
// 				auto popCursor = m_popCursor.load(std::memory_order_acquire);
// 				std::destroy_at(m_data.data(), popCursor);
// 				m_popCursor.store(popCursor + 1, std::memory_order_release);
// 			}
// 			// Has to be release because we can't afford to let the compiler reorder the code above after this write.
// 			m_popCursor.store(0, std::memory_order_release);
// 		}
// 		else
// 		{
// 			m_popCursor.store(0, std::memory_order_relaxed);
// 		}

// 		m_cachedPushCursor = 0;
// 		m_cachedPopCursor = 0;
// 		m_pushCursor.store(0, std::memory_order_relaxed);
// 	}

// private:

// 	auto _is_full(size_type pushAt, size_type popAt) const noexcept -> bool
// 	{
// 		return (pushAt - popAt) == Capacity;
// 	}

// 	auto _is_empty(size_type popAt, size_type pushAt) const noexcept -> bool
// 	{
// 		return popAt == pushAt;
// 	}

// 	template <typename... Args>
// 	auto _emplace_internal(size_t pos, Args&&... args) noexcept -> value_type&
// 	{
// 		new (m_data.data() + pos) value_type{ std::forward<Args>(args)... };
// 		return m_data[pos];
// 	}

// 	auto _location(size_type pos) const noexcept -> size_type { return pos & _mask(); }
// 	auto _mask() const noexcept -> size_type { return Capacity - 1; }

// 	alignas(CACHE_LINE_SIZE) std::array<value_type, Capacity> m_data = {};
// 	alignas(CACHE_LINE_SIZE) std::atomic<size_type> m_pushCursor = {};
// 	alignas(CACHE_LINE_SIZE) size_type m_cachedPushCursor = {};
// 	alignas(CACHE_LINE_SIZE) std::atomic<size_type> m_popCursor = {};
// 	alignas(CACHE_LINE_SIZE) size_type m_cachedPopCursor = {};

// };

template <typename T>
class weak_atomic : lib::non_copyable_non_movable
{
public:
	using value_type = T;

	weak_atomic() = default;
	~weak_atomic() = default;

	template <typename U>
	weak_atomic(U&& value) :
		m_data{}
	{
		store(std::forward<U>(value));
	}

	template <typename U>
	auto operator=(U&& value) -> weak_atomic& requires std::convertible_to<std::decay_t<U>, value_type>
	{
		store(std::forward<U>(value));
		return *this;
	}

	auto load() const -> value_type
	{
		return m_data.load(std::memory_order_relaxed);
	}

	template <typename U>
	auto store(U&& value) -> weak_atomic& requires std::convertible_to<std::decay_t<U>, value_type>
	{
		m_data.store(std::forward<U>(value), std::memory_order_relaxed);
		return *this;
	}

	template <typename... Args>
	auto store(Args&&... args) -> weak_atomic& requires std::constructible_from<value_type, Args...>
	{
		m_data.store(value_type{ std::forward<Args>(args)... }, std::memory_order_relaxed);
		return *this;
	}

private:
	std::atomic<value_type> m_data;
};

/*
* Fixed size atomic ringbuffer.
*/
template <typename T, size_t Capacity>
class ringbuffer
{
public:
	using value_type 		= T;
	using reference 		= value_type&;
	using pointer 			= value_type*;
	using const_reference 	= value_type const&;
	using const_pointer 	= value_type const*;

	ringbuffer() = default;
	~ringbuffer() noexcept
	{
		clear();
	}

	auto peek() noexcept -> pointer
	{
		size_t const popCursor = m_popCursor.load();

		if (is_empty(m_cachedPushCursor, popCursor))
		{
			m_cachedPushCursor = m_pushCursor.load();

			std::atomic_thread_fence(std::memory_order_acquire);

			if (is_empty(m_cachedPushCursor, popCursor))
			{
				return nullptr;
			}
		}

		return m_data.data() + location_of(popCursor);
	}

	template <typename... Args>
	auto push(Args&&... args) noexcept -> pointer requires std::constructible_from<value_type, Args...>
	{
		size_t const pushCursor = m_pushCursor.load();

		if (is_full(pushCursor, m_cachedPopCursor))
		{
			m_cachedPopCursor = m_popCursor.load();

			std::atomic_thread_fence(std::memory_order_acquire);

			if (is_full(pushCursor, m_cachedPopCursor))
			{
				return nullptr;
			}
		}

		auto&& element = emplace_internal(location_of(pushCursor), std::forward<Args>(args)...);

		std::atomic_thread_fence(std::memory_order_release);

		m_pushCursor.store(pushCursor + 1ull);

		return &element;
	}

	auto pop() noexcept -> void
	{
		size_t const popCursor = m_popCursor.load();

		if (is_empty(m_cachedPushCursor, popCursor))
		{
			m_cachedPushCursor = m_pushCursor.load();

			std::atomic_thread_fence(std::memory_order_acquire);

			if (is_empty(m_cachedPushCursor, popCursor))
			{
				return;
			}
		}

		if constexpr (!std::is_standard_layout_v<value_type> || !std::is_trivially_destructible_v<value_type>)
		{
			std::destroy_at(m_data.data() + location_of(popCursor));
		}

		// No release fence is needed here before storing to m_popCursor.
		//
		// A release fence would only be necessary if there were non-atomic data
		// written before it that another thread needs to observe — as is the
		// case in push(), where the constructed object in m_data must happen-before
		// the *consumer* sees the updated m_pushCursor, allowing it to safely
		// read the constructed object (fence-fence synchronization).
		//
		// In pop(), the only non-atomic write before this store is std::destroy_at,
		// whose result the producer never reads — it unconditionally overwrites the
		// freed slot via placement new in emplace_internal(). There is no non-atomic
		// load on the producer side that needs to happen-after the destructor, so
		// fence-fence and atomic-fence synchronization are not a concern here.
		//
		// m_popCursor is a single atomic variable. Even with memory_order_relaxed,
		// modification order consistency guarantees that all threads observe writes
		// to it in the same total order — the producer will always eventually see
		// the updated value without any additional fencing.

		// std::atomic_thread_fence(std::memory_order_release);

		m_popCursor.store(popCursor + 1ull);
	}

	/*
	 * Clears the contents of the ringbuffer.
	 * This function is not thread-safe.
	 */
	auto clear() noexcept -> void
	{
		if constexpr (!std::is_trivially_destructible_v<value_type>)
		{
			while (!empty())
			{
				size_t const popAt = m_popCursor.load();
				std::destroy_at(m_data.data() + location_of(popAt));
				m_popCursor.store(popAt + 1ull);
			}
		}
		m_pushCursor = 0ull;
		m_popCursor = 0ull;
		m_cachedPushCursor = 0ull;
		m_cachedPopCursor = 0ull;
	}

	auto empty() const noexcept -> bool
	{
		return size() == 0;
	}

	auto full() const noexcept -> bool
	{
		return size() == capacity();
	}

	auto capacity() const noexcept -> size_t
	{
		return Capacity;
	}

	auto size() const noexcept -> size_t
	{
		size_t const pushAt = m_pushCursor.load();
		size_t const popAt 	= m_popCursor.load();

		return pushAt - popAt;
	}

private:

	constexpr auto location_of(size_t value) const noexcept -> size_t
	{
		if constexpr (lib::is_power_of_two(Capacity))
		{
			return value & (Capacity - 1ull);
		}
		else
		{
			return value % Capacity;
		}
	}

	template <typename... Args>
	auto emplace_internal(size_t location, Args&&... args) noexcept -> reference requires std::constructible_from<value_type, Args...>
	{
		auto element = new (m_data.data() + location) value_type{ std::forward<Args>(args)... };
		return *element;
	}

	auto is_full(size_t push, size_t pop) const noexcept -> bool
	{
		return std::cmp_equal(push - pop, Capacity);
	}

	auto is_empty(size_t push, size_t pop) const noexcept -> bool
	{
		return push == pop;
	}

	alignas(CACHE_LINE_SIZE) std::array<T, Capacity> m_data;
	alignas(CACHE_LINE_SIZE) weak_atomic<size_t> m_pushCursor;
	alignas(CACHE_LINE_SIZE) size_t m_cachedPushCursor;
	alignas(CACHE_LINE_SIZE) weak_atomic<size_t> m_popCursor;
	alignas(CACHE_LINE_SIZE) size_t m_cachedPopCursor;
};
}

#endif // ! WARP_QUEUE_HPP