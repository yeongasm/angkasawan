#pragma once
#ifndef WARP_THREAD_HPP
#define WARP_THREAD_HPP

#include <thread>
#include "common.hpp"

namespace warp
{
namespace detail
{
class thread : lib::non_copyable
{
public:
	~thread() = default;

	thread(thread&&) = default;
	auto operator=(thread&&) -> thread& = default;

	/*
	* More often than not, we don't really want to be executing the thread when we're constructing the thread.
	* This simply constructs the thread object.
	*/
	template <typename Fn, typename... Args>
	static auto from(Fn&& fn, Args&&... args) -> thread requires (std::invocable<Fn, Args...>)
	{
		return thread{ std::jthread{ std::forward<Fn>(fn), std::forward<Args>(args)... } };
	}

	template <typename Fn, typename... Args>
	static auto from(thread_info const& info, Fn&& fn, Args&&... args) -> thread requires (std::invocable<Fn, Args...>)
	{
		std::jthread thrd{ std::forward<Fn>(fn), std::forward<Args>(args)... };
		_apply_thread_info(thrd, info);
		return thread{ std::move(thrd) };
	}

	/*
	* Different from std::thread::hardware_concurrency which returns an approximation of logical processors on your CPU, this function returns the physical core count on your CPU.
	*/
	static auto hardware_concurrency() -> uint32;
	/*
	* Returns the logical core count of your CPU.
	*/
	static auto logical_concurrency() -> uint32;

	auto native_handle() -> std::jthread::native_handle_type
	{
		return m_thread.native_handle();
	}

	auto get_id() const -> std::jthread::id
	{
		return m_thread.get_id();
	}

	auto get_stop_source() -> std::stop_source 		{ return m_thread.get_stop_source(); }
	auto get_stop_token() const -> std::stop_token 	{ return m_thread.get_stop_token(); }
	auto request_stop() -> bool 					{ return m_thread.request_stop(); }

private:
	std::jthread m_thread;

	thread() = default;
	thread(std::jthread&& thread);

	static auto _apply_thread_info(std::jthread& thread, thread_info const& info) -> void;
};
}
}

#endif // !WARP_THREAD_HPP