#pragma once
#ifndef WARP_DETAIL_ALLOCATOR_SUPPORT_HPP
#define WARP_DETAIL_ALLOCATOR_SUPPORT_HPP


#include <concepts>
#include "lib/memory.hpp"

namespace warp
{
namespace detail
{
template <typename>
struct allocator_of
{
	using type = lib::allocator<std::byte>;
};

template <typename Context>
requires requires { typename Context::allocator_type; }
struct allocator_of<Context>
{
	using type = typename Context::allocator_type;
	static_assert(
		requires (type t, size_t s, std::byte* ptr)
		{
			{ t.allocate(s) } -> std::same_as<std::byte*>;
			t.deallocate(ptr, s);
		},
		"allocator_type needs to be an allocator of std::byte."
	);
};

template <typename Context>
using allocator_of_t = allocator_of<Context>::type;

// template <typename AllocatorType, typename First, typename... Rest>
// static auto find_allocator(First&& first, Rest&&... rest) -> AllocatorType
// {
// 	if constexpr (std::constructible_from<AllocatorType, First>)
// 	{
// 		return AllocatorType{ std::forward<First>(first) };
// 	}
// 	else
// 	{
// 		find_allocator<AllocatorType>(std::forward<Rest>(rest)...);
// 	}
// }

// template <typename AllocatorType, typename... Args>
// static auto find_allocator() -> AllocatorType
// {
// 	static_assert(
// 		sizeof...(Args) == 0,
// 		"No argument in the parameter pack can construct an allocator"
// 	);
// 	return AllocatorType{};
// }

/*
* TODO(afiq):
* I would rather not propogate the allocator down to every child coroutine as a function argument.
* Look into https://wg21.link/p4003 -- Frame Allocator section.
*
* Look at Dietmar's defer_frame implementation.
*/
template <typename Allocator>
struct allocator_support
{
	using allocator_type 	= Allocator;
	using allocator_traits 	= std::allocator_traits<allocator_type>;

	static auto aligned_size(size_t size) -> size_t
	{
		return (size + alignof(allocator_type) - 1u) & ~(alignof(allocator_type) - 1);
	}

	static auto get_allocator(void* ptr, size_t size) -> allocator_type*
	{
		auto data = static_cast<std::byte*>(ptr) + aligned_size(size);
		return std::launder(reinterpret_cast<allocator_type*>(data));
	}

	template <typename... Args>
	static void* operator new(size_t size, [[maybe_unused]] Args&&... args)
	{
		// if constexpr (std::same_as<allocator_type, std::allocator<std::byte>>)
		// {
			allocator_type alloc{};
			return allocator_traits::allocate(alloc, size);
		// }
		// else
		// {
		// 	allocator_type allocator{ find_allocator<allocator_type>(std::forward<Args>(args)...) };

		// 	void* ptr = allocator_traits::allocate(allocator, aligned_size(size) + sizeof(allocator_type));

		// 	std::construct_at(get_allocator(ptr, size), allocator_type{ allocator });

		// 	return ptr;
		// }
	}

	template <typename... Args>
	static void operator delete(void* ptr, size_t size, Args&&...)
	{
		allocator_support::operator delete(ptr, size);
	}

	static void operator delete(void* ptr, size_t size)
	{
		// if constexpr (std::same_as<allocator_type, std::allocator<std::byte>>)
		// {
			allocator_type allocator{};
			allocator_traits::deallocate(allocator, static_cast<std::byte*>(ptr), size);
		// }
		// else
		// {
		// 	allocator_type* aptr = get_allocator(ptr, size);
		// 	// Make a copy!
		// 	allocator_type allocator{ *aptr };

		// 	std::destroy_at(aptr);

		// 	allocator_traits::deallocate(allocator, static_cast<std::byte*>(ptr), aligned_size(size) + sizeof(allocator_type));
		// }
	}
};
}
}

#endif // !WARP_DETAIL_ALLOCATOR_SUPPORT_HPP