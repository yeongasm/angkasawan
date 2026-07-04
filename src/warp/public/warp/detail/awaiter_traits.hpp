#pragma once
#ifndef WARP_DETAIL_AWAITER_TRAITS_HPP
#define WARP_DETAIL_AWAITER_TRAITS_HPP

#include <concepts>
#include <coroutine>
#include <utility>

namespace warp
{
namespace detail
{
struct ___promise {};

template <typename T>
concept await_suspend_compatible = std::same_as<T, void> || std::same_as<T, bool> || std::convertible_to<T, std::coroutine_handle<>>;

template <typename T>
concept is_awaiter = requires (T t)
{
	{ t.await_ready() } -> std::convertible_to<bool>;
	{ t.await_suspend(std::coroutine_handle<>{}) } -> await_suspend_compatible;
	{ t.await_resume() };
};

template <typename T>
concept member_co_awaitable = requires (T t)
{
	{ t.operator co_await() } -> is_awaiter;
};

template <typename T>
concept global_co_awaitable = requires (T&& t)
{
	{ operator co_await(std::forward<T>(t)) } -> is_awaiter;
};

template <typename T>
concept has_as_awaiter = requires (T t, ___promise p)
{
	{ t.as_awaiter(p) } -> is_awaiter;
};

template <typename T>
concept awaitable = is_awaiter<T> || member_co_awaitable<T> || global_co_awaitable<T> || has_as_awaiter<T>;

template <awaitable Awaitable, typename Promise>
constexpr auto get_awaiter(Awaitable&& a, [[maybe_unused]] Promise&& p) -> auto
{
	if constexpr (member_co_awaitable<Awaitable>)
	{
		return std::forward<Awaitable>(a).operator co_await();
	}
	else if constexpr (global_co_awaitable<Awaitable>)
	{
		return operator co_await(std::forward<Awaitable>(a));
	}
	else if constexpr (has_as_awaiter<Awaitable>)
	{
		return std::forward<Awaitable>(a).as_awaiter(p);
	}
	else
	{
		return std::forward<Awaitable>(a);
	}
};

template <awaitable T>
struct awaitable_traits
{
	using awaiter_type = decltype(get_awaiter(std::declval<T>(), std::declval<___promise&>()));
	using awaiter_result_type = decltype(get_awaiter(std::declval<T>(), std::declval<___promise&>()).await_resume());
};

template <awaitable T>
using awaiter_t = typename awaitable_traits<T>::awaiter_type;

template <awaitable T>
using awaiter_result_t = typename awaitable_traits<T>::awaiter_result_type;
}
}

#endif // !WARP_DETAIL_AWAITER_TRAITS_HPP