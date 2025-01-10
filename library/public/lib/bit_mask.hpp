#pragma once
#ifndef LIB_BIT_MASK_HPP
#define LIB_BIT_MASK_HPP

#include <utility>
#include "concepts.hpp"

template <lib::bit_mask T>
constexpr auto operator| (T a, T b) -> T
{
	if constexpr (std::is_enum_v<T>)
	{
		return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
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
		return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
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
		return static_cast<T>(std::to_underlying(a) ^ std::to_underlying(b));
	}
	else
	{
		return a ^ b;
	}
}

template <lib::bit_mask T>
constexpr auto operator|= (T& a, T b) -> T&
{
	if constexpr (std::is_enum_v<T>)
	{
		return a = static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
	}
	else
	{
		return a |= b;
	}
}

template <lib::bit_mask T>
constexpr auto operator&= (T& a, T b) -> T&
{
	if constexpr (std::is_enum_v<T>)
	{
		return a = static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
	}
	else
	{
		return a &= b;
	}
}

template <lib::bit_mask T>
constexpr auto operator^= (T& a, T b) -> T&
{
	if constexpr (std::is_enum_v<T>)
	{
		return a = static_cast<T>(std::to_underlying(a) ^ std::to_underlying(b));
	}
	else
	{
		return a ^= b;
	}
}

#endif // !LIB_BIT_MASK_HPP
