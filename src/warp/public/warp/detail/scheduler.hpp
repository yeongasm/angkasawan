#pragma once
#ifndef WARP_DETAIL_SCHEDULER_HPP
#define WARP_DETAIL_SCHEDULER_HPP

#include <concepts>
#include <coroutine>

namespace warp
{
namespace detail
{
struct scheduler
{
	// 1. We want to be able to schedule a "task" / "generator", basically an awaitable
};

template <typename>
struct scheduler_of
{
	using type = scheduler;
};

template <typename Environment>
requires requires { typename Environment::scheduler_type; }
struct scheduler_of<Environment>
{
	using type = Environment::scheduler_type;
	static_assert(
		requires(type t)
		{
			// TBD
			// Need to figure out more constraints.
			{ t.schedule(/*Need to pass in params that are unclear to me right now*/) } -> std::convertible_to<std::coroutine_handle<>>;
		},
		"scheduler_type needs to respect scheduler interface"
	);
};

template <typename Environment>
using scheduler_of_t = scheduler_of<Environment>::type;
}
}
/*
 * auto task = some_func(); // initial_suspend returns suspend_always hence the coroutine is suspended.
 * warp::schedule(task, exec);
 *
 * inside of warp::schedule() -> task<result_type>
 * either
 * exec.schedule(task);	// This is the easiest way. All we have to do is take task's coroutine handle which we can.
 * 						// This method however doesn't give us an awaitable that we can await on via warp::sync_wait, warp::when_all and etc
 * or
 * co_return co_await task	//
 */

#endif // !WARP_DETAIL_SCHEDULER_HPP