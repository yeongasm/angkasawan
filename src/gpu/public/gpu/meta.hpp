#pragma once
#ifndef GPU_RESOURCE_TYPES_HPP
#define GPU_RESOURCE_TYPES_HPP

#include <source_location>
#include "lib/common.hpp"

namespace gpu
{
namespace reflect
{
namespace detail
{
consteval auto fnv1a_32(const char* str) -> uint32
{
	constexpr uint32 FNV_OFFSET_BASIS = 0x811C9DC5u; // 2166136261
	constexpr uint32 FNV_PRIME = 0x01000193u;        // 16777619

	uint32 hash = FNV_OFFSET_BASIS;
	while (*str) 
	{
		hash ^= static_cast<uint8_t>(*str++);
		hash *= FNV_PRIME;
	}
	return hash;
}
}

template <typename T>
struct type_id
{
	using value_type = uint32;
	static constexpr value_type value = detail::fnv1a_32(std::source_location::current().function_name());
};

template <typename T>
static constexpr uint32 type_id_v = type_id<T>::value;

struct _ResourceMeta
{
	uint32 id;
	uint32 type;
};
}

template <typename T>
struct implementation;
}

#endif // !GPU_RESOURCE_TYPES_HPP