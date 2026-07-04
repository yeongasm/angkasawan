#pragma once
#ifndef WARP_DETAIL_WHEN_ALL_HPP
#define WARP_DETAIL_WHEN_ALL_HPP

#include <atomic>
#include <coroutine>

#include "lib/utility.hpp"

#include "awaiter_traits.hpp"
#include "promise_base.hpp"
#include "allocator_support.hpp"
#include "handle.hpp"

/*
* NOTE(afiq):
* There might be some value in doing what libfork does which is to explicitly bind the result
* object to the coroutine. Feels less natural but has the highest optimization possibility.
*/

namespace warp
{
namespace detail
{
struct when_all_final_suspend
{
	auto await_ready() const noexcept -> bool { return false; }

	template <typename Promise>
	auto await_suspend(std::coroutine_handle<Promise> coroutine) const noexcept -> void
	{
		// When a when_all_task completes, we decrement the task count. If all the tasks are completed, we continue when the awaiter.
		coroutine.promise().notify_complete();
	}

	auto await_result() const noexcept -> void {}
};

// This is the primitive that tracks how many tasks are remaining and also somehow resumes the caller.
class when_all_latch : lib::non_copyable
{
public:
	when_all_latch(size_t numAwaitables) :
		m_count{},
		m_awaitingCoroutine{}
	{
		m_count.store(numAwaitables + 1, std::memory_order_relaxed);
	}

	when_all_latch(when_all_latch&& other) :
		m_count{ other.m_count.load(std::memory_order_acquire) },
		m_awaitingCoroutine{ std::exchange(other.m_awaitingCoroutine, nullptr) }
	{}

	auto operator=(when_all_latch&& other) -> when_all_latch&
	{
		if (this != std::addressof(other))
		{
			m_count.store(other.m_count.load(std::memory_order_acquire), std::memory_order_relaxed);
			m_awaitingCoroutine = std::exchange(other.m_awaitingCoroutine, nullptr);
		}
		return *this;
	}

	auto is_ready() const noexcept -> bool
	{
		return m_awaitingCoroutine != nullptr && m_awaitingCoroutine.done();
	}

	auto try_await(std::coroutine_handle<> caller) noexcept -> bool
	{
		m_awaitingCoroutine = caller;
		return m_count.fetch_sub(1, std::memory_order_acq_rel) > 1;
	}
	// Maybe there's a way to resume the caller without keeping the awaiting coroutine in this primitve.
	auto notify_complete() noexcept -> void
	{
		if (m_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			m_awaitingCoroutine.resume();
		}
	}
private:
	// The number of tasks that are currently being awaited.
	std::atomic_size_t m_count;
	// This is the coroutine that will continue when co_awaiting on the when_all_task_container
	std::coroutine_handle<> m_awaitingCoroutine;
};

template <typename T, typename Context>
class when_all_task;

template <typename T, typename Context>
class when_all_promise : public promise_base<T>, public allocator_support<allocator_of_t<Context>>
{
private:
	/*
	* NOTE:
	* We don't have an `await_transform` method here because we will never be co_await-ing
	* on the `when_all_task`. `when_all_task` is a wrapper of a single task from the list
	* of tasks supplied to the `when_all` algorithm. These `when_all_task`'s are placed inside
	* of a `when_all_ready_awaiter` which will then be `co_await`-ed.
	*/

	friend when_all_final_suspend;

	using coroutine_type 	= std::coroutine_handle<when_all_promise<T, Context>>;
	using task_type 		= when_all_task<T, Context>;

	/*
	* Until I find a more elegant solution, this stays here. We can't place this in an awaiter
	* because `when_all_task` will never be `co_await`-ed as was mentioned above.
	*/
	when_all_latch* m_latch = {};
public:

	constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

	constexpr auto final_suspend() const noexcept -> when_all_final_suspend
	{
		return {};
	}

	constexpr auto get_return_object() const noexcept -> task_type
	{
		return task_type{ *this };
	}

	constexpr auto unhandled_exception() noexcept -> void
	{
		// Do nothing for now.
		// Maybe throw an assert later.
 		// In the future, might want to have an exception enabled path and an exception disabled path.
		std::terminate();
	}

	constexpr auto start(when_all_latch& latch) noexcept -> void
	{
		m_latch = &latch;
		coroutine_type::from_promise(*this).resume();
	}

	constexpr auto notify_complete() const noexcept -> void
	{
		m_latch->notify_complete();
	}
};

struct default_when_all_context {};

// This task will never be co_await-ed, we're just going to let it run until we
template <typename T, typename Context = default_when_all_context>
class when_all_task : public lib::non_copyable
{
public:
	using result_type 		= T;
	using task_type 		= when_all_task;
	using promise_type 		= when_all_promise<T, Context>;
	using coroutine_type 	= std::coroutine_handle<promise_type>;

	constexpr auto wait_for_completion(result_container<result_type>& container, when_all_latch& latch) noexcept -> void
	{
		m_handle->set_result(container);
		m_handle->start(latch);
	}
private:
	friend when_all_promise<T, Context>;

	when_all_task(promise_type& promise) :
		m_handle{ &promise }
	{}

	handle<promise_type> m_handle;
};

template <typename TaskContainerType>
class when_all_ready_awaiter;

template <>
class when_all_ready_awaiter<std::tuple<>>
{
public:
	constexpr auto await_ready() const noexcept -> bool { return false; }
	constexpr auto await_suspend(std::coroutine_handle<>) const noexcept -> void {};
	constexpr auto await_resume() const noexcept -> void {};
};

template <typename... AwaitableTypes>
class when_all_ready_awaiter<std::tuple<AwaitableTypes...>>
{
public:
	template <typename... Awaitables>
	requires (std::same_as<AwaitableTypes, Awaitables> && ...)
	constexpr when_all_ready_awaiter(Awaitables&&... awaitables) noexcept :
		m_latch{ sizeof...(AwaitableTypes) },
		m_awaitables{ std::forward<Awaitables>(awaitables)... },
		m_results{}
	{}

	constexpr when_all_ready_awaiter(std::tuple<AwaitableTypes...>&& awaitables) noexcept :
		m_latch{ sizeof...(AwaitableTypes) },
		m_awaitables{ std::move(awaitables) },
		m_results{}
	{}

	constexpr when_all_ready_awaiter(when_all_ready_awaiter&& other) :
		m_latch{ std::move(other.m_latch) },
		m_awaitables{ std::move(other.m_awaitables) },
		m_results{ std::move(other.m_results) }
	{}

	constexpr when_all_ready_awaiter(when_all_ready_awaiter const&) 			= delete;
	constexpr when_all_ready_awaiter& operator=(when_all_ready_awaiter const&) 	= delete;
	constexpr when_all_ready_awaiter& operator=(when_all_ready_awaiter&&) 		= delete;

	constexpr auto await_ready() noexcept -> bool
	{
		return m_latch.is_ready();
	}

	constexpr auto await_suspend(std::coroutine_handle<> caller) noexcept -> bool
	{
		[this]<size_t... I>(std::index_sequence<I...>)
		{
			((std::get<I>(m_awaitables).wait_for_completion(std::get<I>(m_results), m_latch)), ...);
		}(std::index_sequence_for<AwaitableTypes...>{});
		return m_latch.try_await(caller);
	}

	constexpr auto await_resume() noexcept -> auto
	{
		return [this]<size_t... I>(std::index_sequence<I...>)
		{
			constexpr auto result_of = []<size_t Idx>(result_tuple& tup) -> auto
			{
				using tuple_element_result_type = decltype(std::get<Idx>(tup).result());
				if constexpr (std::is_void_v<tuple_element_result_type>)
				{
					return void_result{};
				}
				else
				{
					return std::move(std::get<Idx>(tup).result());
				}
			};
			return std::make_tuple(result_of.template operator()<I>(m_results)...);
		}(std::index_sequence_for<AwaitableTypes...>{});
	}

private:
	using result_tuple 	= std::tuple<result_container<awaiter_result_t<AwaitableTypes>>...>;
	using awaitable_tuple = std::tuple<AwaitableTypes...>;

	when_all_latch 	m_latch;
	awaitable_tuple m_awaitables;
	result_tuple 	m_results;
};

template <awaitable Awaitable>
auto make_when_all_task(Awaitable&& awaitable) noexcept -> when_all_task<awaiter_result_t<Awaitable>>
{
	using result_type = awaiter_result_t<Awaitable>;
	if constexpr (std::is_void_v<result_type>)
	{
		co_await std::forward<Awaitable>(awaitable);
		co_return;
	}
	else
	{
		co_return co_await std::forward<Awaitable>(awaitable);
	}
}

template <awaitable... Awaitables>
[[nodiscard]] auto when_all(Awaitables&&... awaitables) -> when_all_ready_awaiter<Awaitables...>
{
	return when_all_ready_awaiter{ std::forward<Awaitables>(awaitables)... };
}
}
}

#endif // WARP_DETAIL_WHEN_ALL_HPP