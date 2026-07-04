#pragma once
#ifndef WARP_HPP
#define WARP_HPP

#include "detail/task.hpp"
#include "detail/static_thread_pool.hpp"
#include "detail/sync_wait.hpp"
#include "detail/when_all.hpp"

namespace warp
{
using detail::task;
using detail::thread;
using detail::static_thread_pool;
using detail::awaitable;
using detail::awaitable_traits;
using detail::awaiter_t;
using detail::awaiter_result_t;
using detail::sync_wait;
using detail::when_all;
}

#endif // !WARP_HPP