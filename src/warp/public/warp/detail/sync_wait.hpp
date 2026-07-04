#pragma once
#ifndef WARP_DETAIL_SYNC_WAIT_HPP
#define WARP_DETAIL_SYNC_WAIT_HPP

#include <coroutine>
#include <semaphore>

#include "lib/common.hpp"

#include "awaiter_traits.hpp"
#include "promise_base.hpp"
#include "allocator_support.hpp"
#include "handle.hpp"

namespace warp
{
namespace detail
{
struct sync_wait_final_suspend
{
	constexpr auto await_ready() const noexcept -> bool { return false; }

	template <typename Promise>
	constexpr auto await_suspend(std::coroutine_handle<Promise> coroutine) const noexcept -> void
	{
		if (coroutine.promise().m_semaphore)
		{
			coroutine.promise().m_semaphore->release();
		}
	}
	constexpr auto await_resume() const noexcept -> void {};
};

template <typename T>
class sync_wait_task;

struct default_sync_task_context {};

/*
* For sync wait's promise, since we are never going to co_await on this promise, the only way to signal
* the binary_semaphore is through final_suspend().
*/
template <typename T, typename Context = default_sync_task_context>
class sync_wait_promise : public promise_base<T>, public allocator_support<allocator_of_t<Context>>
{
private:
	friend sync_wait_final_suspend;

	using coroutine_type 	= std::coroutine_handle<sync_wait_promise<T, Context>>;
	using task_type 		= sync_wait_task<T>;

	std::binary_semaphore* m_semaphore = {};
public:
	constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }
	constexpr auto final_suspend() const noexcept -> sync_wait_final_suspend { return {}; }
	constexpr auto get_return_object() noexcept -> task_type { return task_type{ *this }; }
	// auto get_return_object_on_allocator_failure() noexcept -> task_type { return {}; }

	constexpr auto unhandled_exception() noexcept -> void
	{
		// Do nothing for now.
		// Maybe throw an assert later.
 		// In the future, might want to have an exception enabled path and an exception disabled path.
		std::terminate();
	}

	constexpr auto start(std::binary_semaphore& semaphore) noexcept -> void
	{
		m_semaphore = &semaphore;
		coroutine_type::from_promise(*this).resume();
	}
};

// NOTE(afiq):
// sync_wait_task needs to accept a context object as well. Work for the future I definitely.
template <typename T>
class sync_wait_task : lib::non_copyable
{
public:
	using result_type 		= T;
	using promise_type 		= sync_wait_promise<result_type>;

	constexpr auto sync_wait(result_container<result_type>& container, std::binary_semaphore& semaphore) -> void
	{
		m_handle->set_result(container);
		m_handle->start(semaphore);
	}
private:
	friend promise_type;

	sync_wait_task(promise_type& promise) :
		m_handle{ &promise }
	{}

	handle<promise_type> m_handle;
};

template <typename Awaitable>
auto make_sync_wait_task(Awaitable&& a) -> sync_wait_task<awaiter_result_t<Awaitable>>
{
	using type = awaiter_result_t<Awaitable>;

	if constexpr (std::is_void_v<type>)
	{
		co_await std::forward<Awaitable>(a);
	}
	else
	{
		co_return co_await std::forward<Awaitable>(a);
	}
};

template <typename Awaitable>
auto sync_wait(Awaitable&& a) -> awaiter_result_t<Awaitable>
{
	using result_type = awaiter_result_t<Awaitable>;

	std::binary_semaphore sem{ 0 };
	result_container<result_type> container;

	auto wrapper = make_sync_wait_task(std::forward<Awaitable>(a));
	wrapper.sync_wait(container, sem);
	sem.acquire();

	return container.result();
};
}
}

#endif // !WARP_DETAIL_SYNC_WAIT_HPP