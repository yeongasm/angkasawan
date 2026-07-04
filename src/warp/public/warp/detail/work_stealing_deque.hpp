#pragma once
#ifndef WARP_WORK_STEALING_DEQUE_HPP
#define WARP_WORK_STEALING_DEQUE_HPP

#include <atomic>
#include <new>
#include "lib/common.hpp"

namespace warp
{

/**
 * @brief Status of a steal/pop operation.
 */
enum class StealStatus : uint8
{
	Success,	// Item was successfully retrieved
	Empty,		// Deque was empty, no item available
	Abort		// CAS failed due to contention, caller should retry
};

/**
 * @brief Result of a steal/pop operation containing status and the retrieved item.
 * @tparam T The task type stored in the deque.
 */
template <typename T>
struct StealResult
{
	StealStatus status;
	T value;

	auto has_value() const noexcept -> bool { return status == StealStatus::Success; }
	explicit operator bool() const noexcept { return has_value(); }
};

namespace detail
{

/**
 * @brief Circular buffer with power-of-two capacity for the Chase-Lev deque.
 *
 * Uses atomic storage for each element to support concurrent access.
 * The buffer uses a bitmask for fast index wrapping (index & mask).
 *
 * @tparam T The element type.
 */
template <typename T>
class CircularBuffer : lib::non_copyable_non_movable
{
public:
	/**
	 * @brief Construct a circular buffer with 2^log_capacity elements.
	 * @param log_capacity Log2 of the desired capacity.
	 */
	explicit CircularBuffer(size_t log_capacity) :
		m_mask{ (size_t{ 1 } << log_capacity) - 1 },
		m_buffer{ nullptr }
	{
		size_t const capacity = size_t{ 1 } << log_capacity;

		// Allocate aligned memory for atomic array
		m_buffer = static_cast<std::atomic<T>*>(
			::operator new(capacity * sizeof(std::atomic<T>), std::align_val_t{ alignof(std::atomic<T>) })
		);

		// Placement new to construct each atomic element
		for (size_t i = 0; i < capacity; ++i)
		{
			new (&m_buffer[i]) std::atomic<T>{};
		}
	}

	~CircularBuffer()
	{
		size_t const capacity = m_mask + 1;

		// Destroy each atomic element
		for (size_t i = 0; i < capacity; ++i)
		{
			m_buffer[i].~atomic();
		}

		// Free the aligned memory
		::operator delete(m_buffer, std::align_val_t{ alignof(std::atomic<T>) });
	}

	/**
	 * @brief Get the capacity of this buffer.
	 */
	auto capacity() const noexcept -> size_t { return m_mask + 1; }

	/**
	 * @brief Store a value at the given index (wraps automatically).
	 * @param index The logical index (will be masked to fit capacity).
	 * @param value The value to store.
	 */
	auto store(size_t index, T value) noexcept -> void
	{
		m_buffer[index & m_mask].store(value, std::memory_order_relaxed);
	}

	/**
	 * @brief Load a value from the given index (wraps automatically).
	 * @param index The logical index (will be masked to fit capacity).
	 * @return The value at that index.
	 */
	auto load(size_t index) const noexcept -> T
	{
		return m_buffer[index & m_mask].load(std::memory_order_relaxed);
	}

	/**
	 * @brief Create a new larger buffer and copy existing elements.
	 *
	 * Copies elements in the range [top, bottom) from this buffer to the new one.
	 * Caller is responsible for deleting the old buffer.
	 *
	 * @param bottom The current bottom index.
	 * @param top The current top index.
	 * @param new_log_capacity Log2 of the new capacity.
	 * @return Pointer to the newly allocated buffer.
	 */
	auto grow(size_t bottom, size_t top, size_t new_log_capacity) -> CircularBuffer*
	{
		auto* new_buffer = new CircularBuffer(new_log_capacity);

		// Copy existing elements to new buffer
		for (size_t i = top; i < bottom; ++i)
		{
			new_buffer->store(i, load(i));
		}

		return new_buffer;
	}

private:
	size_t m_mask;				// Bitmask for fast index wrapping (capacity - 1)
	std::atomic<T>* m_buffer;	// Array of atomic elements
};

} // namespace detail

/**
 * @brief Lock-free work-stealing deque using the Chase-Lev algorithm.
 *
 * This is an SPMC (Single-Producer Multiple-Consumer) data structure designed
 * for work-stealing schedulers:
 *
 * - The **owner thread** pushes new tasks and pops completed tasks from the **bottom**.
 *   These operations are mostly wait-free (only contention when deque has 1 item).
 *
 * - **Thief threads** steal tasks from the **top** using CAS operations.
 *   Stealing maintains FIFO order (oldest tasks stolen first).
 *
 * The deque grows dynamically when full (doubles capacity). Indices are 64-bit
 * and monotonically increasing, avoiding ABA problems.
 *
 * Memory ordering follows the Chase-Lev paper with seq_cst fences for
 * correctness at the pop/steal synchronization points.
 *
 * @tparam T The task type (should be trivially copyable or move-constructible).
 * @tparam InitialLogCapacity Log2 of initial capacity (default: 8 = 256 elements).
 */
template <typename T, size_t InitialLogCapacity = 8>
class WorkStealingDeque : lib::non_copyable_non_movable
{
public:
	WorkStealingDeque() :
		m_top{ 0 },
		m_bottom{ 0 },
		m_buffer{ new detail::CircularBuffer<T>(InitialLogCapacity) },
		m_log_capacity{ InitialLogCapacity }
	{}

	~WorkStealingDeque()
	{
		delete m_buffer.load(std::memory_order_relaxed);
	}

	/**
	 * @brief Owner only: Push a task to the bottom of the deque.
	 *
	 * If the deque is full, it will automatically grow to double capacity.
	 * This operation is wait-free (no CAS required).
	 *
	 * @param item The task to push.
	 */
	auto push(T item) noexcept -> void
	{
		size_t const b = m_bottom.load(std::memory_order_relaxed);
		size_t const t = m_top.load(std::memory_order_acquire);
		detail::CircularBuffer<T>* buffer = m_buffer.load(std::memory_order_relaxed);

		// Check if we need to grow (deque is full)
		if (b - t >= buffer->capacity() - 1)
		{
			size_t const new_log_capacity = m_log_capacity + 1;
			auto* new_buffer = buffer->grow(b, t, new_log_capacity);

			// NOTE: In a production system, the old buffer should be reclaimed
			// using hazard pointers or epoch-based reclamation to avoid use-after-free.
			// For simplicity, we delete immediately (safe if no concurrent steals).
			delete buffer;

			buffer = new_buffer;
			m_buffer.store(buffer, std::memory_order_release);
			m_log_capacity = new_log_capacity;
		}

		// Store the item at bottom index
		buffer->store(b, item);

		// Release fence ensures the item is visible before we increment bottom
		std::atomic_thread_fence(std::memory_order_release);

		// Increment bottom (makes the item available for stealing)
		m_bottom.store(b + 1, std::memory_order_relaxed);
	}

	/**
	 * @brief Owner only: Pop a task from the bottom of the deque (LIFO).
	 *
	 * Returns the most recently pushed task. When the deque has only one item,
	 * this operation may race with stealers and use CAS for synchronization.
	 *
	 * @return StealResult with Success if a task was retrieved, Empty otherwise.
	 */
	auto pop() noexcept -> StealResult<T>
	{
		// Decrement bottom speculatively
		size_t const b = m_bottom.load(std::memory_order_relaxed) - 1;
		detail::CircularBuffer<T>* buffer = m_buffer.load(std::memory_order_relaxed);
		m_bottom.store(b, std::memory_order_relaxed);

		// Full memory fence to synchronize with steal()
		// This ensures stealers see the decremented bottom before we read top
		std::atomic_thread_fence(std::memory_order_seq_cst);

		size_t t = m_top.load(std::memory_order_relaxed);

		if (t <= b)
		{
			// Deque was non-empty, retrieve the item
			T item = buffer->load(b);

			if (t == b)
			{
				// This was the last item - must race with stealers
				// Try to increment top to claim the item
				if (!m_top.compare_exchange_strong(
					t, t + 1,
					std::memory_order_seq_cst,
					std::memory_order_relaxed))
				{
					// Lost the race to a stealer - deque is now empty
					m_bottom.store(b + 1, std::memory_order_relaxed);
					return { StealStatus::Empty, {} };
				}

				// Won the race - restore bottom and return the item
				m_bottom.store(b + 1, std::memory_order_relaxed);
			}

			return { StealStatus::Success, std::move(item) };
		}
		else
		{
			// Deque was already empty, restore bottom
			m_bottom.store(b + 1, std::memory_order_relaxed);
			return { StealStatus::Empty, {} };
		}
	}

	/**
	 * @brief Thieves: Steal a task from the top of the deque (FIFO).
	 *
	 * Multiple thieves can call this concurrently. Uses CAS to atomically
	 * claim a task. On Abort status, the caller should retry.
	 *
	 * @return StealResult with Success if stolen, Empty if deque was empty,
	 *         or Abort if CAS failed (retry recommended).
	 */
	auto steal() noexcept -> StealResult<T>
	{
		// Load top first (acquire to see stores to buffer)
		size_t t = m_top.load(std::memory_order_acquire);

		// Full fence to synchronize with pop()'s decrement of bottom
		std::atomic_thread_fence(std::memory_order_seq_cst);

		// Load bottom (acquire to see owner's stores)
		size_t const b = m_bottom.load(std::memory_order_acquire);

		if (t >= b)
		{
			// Deque appears empty
			return { StealStatus::Empty, {} };
		}

		// Load the item at top (consume for dependency ordering on buffer pointer)
		detail::CircularBuffer<T>* buffer = m_buffer.load(std::memory_order_consume);
		T item = buffer->load(t);

		// Try to claim the item by incrementing top
		if (!m_top.compare_exchange_strong(
			t, t + 1,
			std::memory_order_seq_cst,
			std::memory_order_relaxed))
		{
			// Another thread stole this item or owner popped it
			// Caller should retry
			return { StealStatus::Abort, {} };
		}

		// Successfully stole the item
		return { StealStatus::Success, std::move(item) };
	}

	/**
	 * @brief Check if the deque appears empty.
	 *
	 * This is an approximate check - the result may be stale by the time
	 * it's used. Useful for heuristics, not for synchronization.
	 */
	auto empty() const noexcept -> bool
	{
		size_t const b = m_bottom.load(std::memory_order_relaxed);
		size_t const t = m_top.load(std::memory_order_relaxed);
		return b <= t;
	}

	/**
	 * @brief Get the approximate number of tasks in the deque.
	 *
	 * This is an approximate count - the result may be stale by the time
	 * it's used. Useful for heuristics and debugging.
	 */
	auto size() const noexcept -> size_t
	{
		size_t const b = m_bottom.load(std::memory_order_relaxed);
		size_t const t = m_top.load(std::memory_order_relaxed);
		return (b > t) ? (b - t) : 0;
	}

private:
	// Each atomic is on its own cache line to prevent false sharing.
	// False sharing occurs when multiple threads modify different variables
	// that happen to be on the same cache line, causing expensive cache
	// invalidation traffic between CPU cores.

	// Top index: incremented by stealers (and by owner on last-item pop)
	alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_top;

	// Bottom index: modified only by owner (push increments, pop decrements)
	alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_bottom;

	// Pointer to the circular buffer (may change on grow)
	alignas(std::hardware_destructive_interference_size) std::atomic<detail::CircularBuffer<T>*> m_buffer;

	// Current log2 capacity (for grow operations)
	size_t m_log_capacity;
};

} // namespace warp

#endif // !WARP_WORK_STEALING_DEQUE_HPP
