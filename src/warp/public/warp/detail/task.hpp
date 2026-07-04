#pragma once
#ifndef WARP_TASK_HPP
#define WARP_TASK_HPP

#include "handle.hpp"
#include "promise_type.hpp"

/*
* TODO(afiq):
* Implement in the following order (Estimation 2 weeks of continuous dev. Excluding interview prep):
* [~] result type object -- holds a variant of monostate, the return value (should it exist) and optionally error types.
* 		-- the basics is in. Might want to extend it to support more use cases as we progress with the library.
* [~] State object -- Refer to beman::task, I think there's something very interesting there. Particularly how to control coroutines at certain control points. For ours, lets not call it state base but rather awaiter_base.
* [~] Awaiter -- Implement an awaiter interface. Refer to beman::task. I like how with beman::task, you can override default behaviors on how coroutines behave. Customizable and should be supplied into our task Options.
* [~] Promise base
* [~] Promise types -- again, refer beman::task as well as libcoro. Marry the best of both worlds.
* [~] Task object -- Join them all together into the final task object.
* 	[ ] - Add support for error types into result_container.
* 	[ ] - Implement cancellation. Should any task signal a cancellation, we need to ensure that child tasks are not queued for execution.
* 	[ ] - Affine tasks onto parent scheduler.
* [ ] Scheduler -- Still needs to be done.
*
* Overall goal:
* Working work graph. Change of plans, don't make them retrofit into senders / receivers like beman::task
* instead use them to complement senders / receivers. There is this paper, https://wg21.link/p2583 which I agree a lot with.
*
*/
namespace warp
{
namespace detail
{
// struct promise_base
// {
// 	friend struct final_awaiter;

// 	/*
// 	* DEFINITIONS:
// 	*
// 	* 1) Awaitable -- a type that supports the co_await operator.
// 	* 2) Awaiter -- a type that implements the await_ready(), await_suspend() and await_resume() methods.
// 	*
// 	* A type can be both an Awaitable type and an Awaiter type.
// 	*
// 	* NOTE: Awaiters are stored on the caller's coroutine frame and lives across suspension points.
// 	*/

// 	promise_base() noexcept 	= default;
// 	~promise_base() noexcept 	= default;

// 	auto initial_suspend() noexcept -> std::suspend_always { return {}; };
// 	auto final_suspend() noexcept -> final_awaiter { return {}; };

// 	auto called_by(std::coroutine_handle<> caller) noexcept -> void { m_caller = caller; }
// protected:
// 	std::coroutine_handle<> m_caller{ nullptr };
// };

/*
* Compiler roughly transforms a coroutine body to look like this.
* ---------------------------------------------------------------
* {
*   co_await promise.initial_suspend();
*   try
*   {
*     <body-statements>
*   }
*   catch (...)
*   {
*     promise.unhandled_exception();
*   }
* FinalSuspend:
*   co_await promise.final_suspend();
* }
* ---------------------------------------------------------------
*/

struct default_context {};

/*
* task needs to be movable since people might want to store them into containers.
*/
template <typename T, typename Context = default_context>
class task : public lib::non_copyable
{
public:
	using result_type 		= T;
	using task_type 		= task;
	using promise_type 		= promise<result_type, task_type, Context>;
	using awaiter_type 		= awaiter_of_t<Context, result_type, promise_type>;

	template <typename CallerPromise>
	auto as_awaiter([[maybe_unused]] CallerPromise&& caller) -> awaiter_type
	{
		return awaiter_type{ *m_handle.get() };
	}
private:
	friend promise_type;

	task(promise_type& promise) :
		m_handle{ &promise }
	{}
	// Probable wanna do something smarter like beman::task where
	// the promise object is wrapped inside of a std::unique_ptr like
	// handle. That way we don't need to define the move operations.
	handle<promise_type> m_handle;
};
}
}

#endif // !WARP_TASK_HPP