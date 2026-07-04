#pragma once
#ifndef WARP_COMMON_HPP
#define WARP_COMMON_HPP

#include <concepts>
#include <string_view>
#include "lib/common.hpp"

namespace warp
{
template <typename T>
concept is_thread_pool = requires(T t)
{
	typename T::thread_type;

	{ t.thread_count() } -> std::convertible_to<uint32>;
	{ t.request_stop() } -> std::same_as<void>;
};

/*
* A wrapper around std::jthread, mainly to set thread affinity and name.
*/
struct thread_info
{
	std::string_view name;
	uint32 cpuCore;
};

/**
 * @brief Configuration options for the StaticThreadPool.
 */
struct thread_pool_options
{
	// Number of worker threads in the pool
	uint32 threadCount;
	// If true, pin each worker thread to a specific CPU core
	bool affinitive;
};

inline static constexpr size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;
}

#endif // !WARP_COMMON_HPP