#pragma once
#ifndef WARP_DETAIL_HANDLE_HPP
#define WARP_DETAIL_HANDLE_HPP

#include <memory>
#include <coroutine>

namespace warp
{
namespace detail
{
template <typename Promise>
struct PromiseDeleter
{
	auto operator()(Promise* p) -> void
	{
		if (p)
		{
			std::coroutine_handle<Promise>::from_promise(*p).destroy();
		}
	}
};
template <typename Promise>
using handle = std::unique_ptr<Promise, PromiseDeleter<Promise>>;
}
}

#endif // !WARP_DETAIL_HANDLE_HPP