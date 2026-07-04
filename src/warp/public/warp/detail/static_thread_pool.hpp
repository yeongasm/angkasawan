#pragma once
#ifndef WARP_THREAD_POOL_HPP
#define WARP_THREAD_POOL_HPP

#include "common.hpp"
#include "thread.hpp"

namespace warp
{
namespace detail
{
template <thread_pool_options Options = thread_pool_options{}>
class static_thread_pool : lib::non_copyable_non_movable
{
public:
	using thread_type = thread;

	static_thread_pool() = default;

	~static_thread_pool()
	{
		request_stop();
	}

	template <typename Scheduler>
	static auto on(static_thread_pool& tp, Scheduler&& scheduler) noexcept -> void
	{
		static_assert(
			requires (thread& t, uint32 i) {
				{ scheduler.start_thread(t, i) } -> std::same_as<void>;
			},
			"Scheduler must implement auto start_thread(thread&, uint32) -> void"
		);

		for (uint32 i = 0; auto& thread : tp.m_threads)
		{
			scheduler.start_thread(thread, i);
			++i;
		};
	}

	/**
	 * @brief Get the number of worker threads in the pool.
	 */
	auto thread_count() const noexcept -> uint32 { return Options.threadCount; }

	/**
	 * @brief Request graceful shutdown of the pool.
	 *
	 * Workers will finish their current task and drain remaining tasks
	 * from all queues before exiting. This is non-blocking.
	 */
	auto request_stop() noexcept -> void
	{
		// Request stop on all worker threads
		for (auto& thread : m_threads)
		{
			thread.request_stop();
		}
	}

private:
	std::array<thread, Options.threadCount> m_threads;
};
}
} // namespace warp

/*
work_stealing_scheduler scheduler;
static_thread_pool<> tp{ scheduler }
*/

#endif // !WARP_THREAD_POOL_HPP
