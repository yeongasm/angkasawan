#pragma once
#ifndef WARP_DETAIL_AWAITER_HPP
#define WARP_DETAIL_AWAITER_HPP

#include <coroutine>
#include "awaiter_traits.hpp"
#include "result_container.hpp"

namespace warp
{
namespace detail
{
/*
* The compiler roughly transform the code to something like this when it encounters an awaitable.
* -----------------------------------------------------------------------------------------------
* {
*   auto&& value = <expr>;
*   auto&& awaitable = get_awaitable(promise, static_cast<decltype(value)>(value));
*   auto&& awaiter = get_awaiter(static_cast<decltype(awaitable)>(awaitable));
*   if (!awaiter.await_ready())
*   {
*     using handle_t = std::experimental::coroutine_handle<P>;
*
*     using await_suspend_result_t =
*       decltype(awaiter.await_suspend(handle_t::from_promise(promise)));
*
*     <suspend-coroutine>
*
*     if constexpr (std::is_void_v<await_suspend_result_t>)
*     {
*       awaiter.await_suspend(handle_t::from_promise(promise));
*       <return-to-caller-or-resumer>
*     }
*     else
*     {
*       static_assert(
*          std::is_same_v<await_suspend_result_t, bool>,
*          "await_suspend() must return 'void' or 'bool'.");
*
*       if (awaiter.await_suspend(handle_t::from_promise(promise)))
*       {
*         <return-to-caller-or-resumer>
*       }
*     }
*
*     <resume-point>
*   }
*
*   return awaiter.await_resume();
* }
* -----------------------------------------------------------------------------------------------
*/

/*
* The awaiter returned by final_suspend() when a coroutine
* reaches the end before it destroys it's coroutine frame.
*/
struct final_awaiter
{
		/*
		* Referring to the compiler transform above, returning false
		* would allow our program to reach the call to await_suspend()
		* allowing us to use symmetric transfer to continue children coroutines.
		*
		* Remember, await_ready() here means has the coroutine that returns this awaiter
		* and being awaited by the caller finish executing.
		*/
		constexpr auto await_ready() const noexcept -> bool { return false; }

		/*
		* Use symmetric transfer to resume the caller's execution.
		*/
		template <typename Promise>
		constexpr auto await_suspend(std::coroutine_handle<Promise> coroutine) const noexcept -> std::coroutine_handle<>
		{
			// The coroutine promise should contain a method called on_complete() or something like that to handle continuation.
			return coroutine.promise().continuation();
		}

		/*
		* This will never be called for a final_awaiter because await_suspend() would've
		* resumed our caller.
		*/
		constexpr auto await_resume() noexcept -> void {};
};

template <typename ResultType, typename Promise>
struct awaiter : result_container<ResultType>
{
	using promise_type = Promise;

	awaiter(promise_type& promise) :
		coroutine{ promise }
	{}

	constexpr auto await_ready() const noexcept -> bool
	{
		return false;
	}

	template <typename CallerPromiseType>
	constexpr auto await_suspend(std::coroutine_handle<CallerPromiseType> caller) noexcept -> auto
	{
		/*
		* Without thinking too much about how to store the scheduler in the awaiter,
		* I think the best thing to do is to store the scheduler in the coroutine's
		* promise object and do the entire scheduling logic in the coroutine's start
		* function.
		*/
		coroutine.set_continuation(caller);
		coroutine.set_result(*this);

		// /*
		// * If the caller is derived from `promise_result_container`, we make the `result_container<T>*`
		// * it owns to point to this awaiter (which is derived from `result_container<T>`). This way,
		// * caller coroutines, don't need to own a separate instance of `result_container<T>`.
		// */
		// if constexpr (requires { caller.promise().set_result(*this); })
		// {
		// 	caller.promise().set_result(*this);
		// }

		return std::coroutine_handle<promise_type>::from_promise(coroutine);
	}

	/*
	* This is called when the coroutine has completed execution and needs to return the result
	* to the continuing coroutine.
	*/
	constexpr auto await_resume() noexcept -> auto
	{
		return this->result();
	}
protected:
	promise_type& coroutine;
};

template <typename T>
concept has_awaiter_type = requires { typename T::awaiter_type; };

template <typename T>
concept has_final_awaiter_type = requires { typename T::final_awaiter_type; };

template <typename, typename ReturnType, typename PromiseType, bool FinalAwaiter = false>
struct awaiter_of
{
	using type = std::conditional_t<FinalAwaiter, final_awaiter, awaiter<ReturnType, PromiseType>>;
};

template <typename Context, typename ReturnType, typename PromiseType, bool FinalAwaiter>
requires ((!FinalAwaiter && has_awaiter_type<Context>) || ( FinalAwaiter && has_final_awaiter_type<Context>))
struct awaiter_of<Context, ReturnType, PromiseType, FinalAwaiter>
{
	using type = std::conditional_t<FinalAwaiter, typename Context::final_awaiter_type, typename Context::awaiter_type>;
	static_assert(
		is_awaiter<type>,
		"awaiter needs to be a coroutine compliant awaiter. Refer to https://en.cppreference.com/w/cpp/language/coroutines.html."
	);
};

template <typename Context, typename ReturnType, typename PromiseType, bool FinalAwaiter = false>
using awaiter_of_t = awaiter_of<Context, ReturnType, PromiseType, FinalAwaiter>::type;
}
}

#endif // !WARP_DETAIL_AWAITER_HPP