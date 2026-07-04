#pragma once
#ifndef WARP_DETAIL_PROMISE_TYPE_HPP
#define WARP_DETAIL_PROMISE_TYPE_HPP

#include "awaiter.hpp"
#include "allocator_support.hpp"
#include "promise_base.hpp"
// #include "scheduler.hpp"

namespace warp
{
namespace detail
{
template <typename T, typename TaskType, typename Context>
class promise : public promise_base<T>, public allocator_support<allocator_of_t<Context>>
{
public:
	using result_type 			= T;
	using task_type 			= TaskType;
	using promise_type 			= promise<result_type, task_type, Context>;
	using coroutine_handle_type = std::coroutine_handle<promise_type>;
	using awaiter_type 			= awaiter_of_t<Context, result_type, promise_type>;
	using final_awaiter_type 	= awaiter_of_t<Context, result_type, promise_type, true>;
	using allocator_type 		= allocator_of_t<Context>;

	constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
	constexpr auto final_suspend() noexcept -> final_awaiter_type { return {}; }

	constexpr auto get_return_object() noexcept -> task_type { return task_type{ *this }; }
	// auto get_return_object_on_allocator_failure() noexcept -> task_type { return {}; }

	constexpr auto unhandled_exception() noexcept -> void
	{
		// Do nothing for now.
		// Maybe throw an assert later.
		// In the future, might want to have an exception enabled path and an exception disabled path.
		std::terminate();
	}

	constexpr auto notify_complete() noexcept -> std::coroutine_handle<>
	{
		// Same situation as above, we can either symmetrically transfer to the caller when the coroutine
		// completes or queue the caller back to the scheduler. This is not up for the promise object to
		// decide. Hence we defer the logic to the scheduler.
		if (!m_continuation.done())
		{
			return m_continuation;
		}
		return std::noop_coroutine();
	}

	constexpr auto set_continuation(std::coroutine_handle<> continuation) noexcept -> void
	{
		m_continuation = continuation;
	}

	constexpr auto continuation() noexcept -> std::coroutine_handle<>
	{
		return m_continuation;
	}

	template <typename Awaitable>
	constexpr auto await_transform(Awaitable&& awaitable) -> auto
	{
		return get_awaiter(std::forward<Awaitable>(awaitable), *this);
	}
private:
	// scheduler_type			m_scheduler;	// Value semantics, wrapper around a pointer like std::pmr::allocator
	std::coroutine_handle<> m_continuation;		// These 2 values can actually be in the awaiter. Ideally the awaiter should hold the continuation.
};
}
}

#endif // !WARP_DETAIL_PROMISE_TYPE_HPP