#pragma once
#ifndef LIB_COMMON_HPP
#define LIB_COMMON_HPP

#include <cassert>
#include <memory>
#include <new>
#include <utility>
#include <optional>
#include <algorithm>
#include <functional>
#include <compare>

#include "concepts.hpp"

using int8		= char;
using int16		= short;
using int32		= int;
using int64		= long long;

using byte		= unsigned char;

using uint8		= unsigned char;
using uint16	= unsigned short;
using uint32	= unsigned int;
using uint64	= unsigned long long;

using word		= unsigned short;
using dword		= unsigned long;

using float32	= float;
using float64	= double;

using literal_t			= const char*;
using wide_literal_t	= const wchar_t*;

#define FLOAT_EPSILON	0.0001f
#define DOUBLE_EPSILON	0.0001

#define GET_MACRO(_1, _2, NAME, ...) NAME

#if _DEBUG
#define ASSERT1(expr) assert(expr)
#define ASSERT2(expr, msg) assert(expr && msg)
#define ASSERTION(...)	GET_MACRO(__VA_ARGS__, ASSERT2, ASSERT1)(__VA_ARGS__)
#else
#define ASSERTION(...)
#endif

#if MSVC_COMPILER
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

namespace lib
{
/**
* @brief The copy constructor and copy assignment operator is implicitly deleted if there is an explicit declaration of the move constructor and move assignment operator.
*/
struct non_copyable
{
	non_copyable() = default;

	non_copyable(non_copyable&&) = default;
	auto operator= (non_copyable&&) -> non_copyable& = default;
};

using move_only = non_copyable;

/**
* @brief The move constructor and move assignment operator is implicitly not declared if the copy constructor and copy assignment operator is explicitly deleted.
* This means and move operations will attempt to invoke the copy constructor / assignment operator. But those have been declared as = delete hence making the object non-copyable & non-movable.
*/
struct non_copyable_non_movable
{
	non_copyable_non_movable() = default;

	non_copyable_non_movable(non_copyable_non_movable const&) = delete;
	auto operator= (non_copyable_non_movable const&) -> non_copyable_non_movable& = delete;
};

struct not_copy_assignable
{
	not_copy_assignable() = default;

	not_copy_assignable(not_copy_assignable const&) = default;
	not_copy_assignable(not_copy_assignable&&) = default;

	auto operator= (not_copy_assignable&&) -> not_copy_assignable& = default;

	auto operator= (not_copy_assignable const&) -> not_copy_assignable& = delete;
};
}

#endif // !LIB_COMMON_HPP
