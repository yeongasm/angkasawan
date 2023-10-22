#pragma once
#ifndef LIB_BIT_MASK_H
#define LIB_BIT_MASK_H

#include "concepts.h"

template <lib::bit_mask T>
constexpr auto operator| (T a, T b) -> T
{
	if constexpr (std::is_enum_v<T>)
	{
		using underlying_type = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<underlying_type>(a) | static_cast<underlying_type>(b));
	}
	else
	{
		return a | b;
	}
}

template <lib::bit_mask T>
constexpr auto operator& (T a, T b) -> T
{
	if constexpr (std::is_enum_v<T>)
	{
		using underlying_type = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<underlying_type>(a) & static_cast<underlying_type>(b));
	}
	else
	{
		return a & b;
	}
}

template <lib::bit_mask T>
constexpr auto operator^ (T a, T b) -> T
{
	if constexpr (std::is_enum_v<T>)
	{
		using underlying_type = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<underlying_type>(a) ^ static_cast<underlying_type>(b));
	}
	else
	{
		return a ^ b;
	}
}

#endif // !LIB_BIT_MASK_H
