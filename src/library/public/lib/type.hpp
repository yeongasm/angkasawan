#pragma once
#ifndef LIB_TYPE_WRAPPER_HPP
#define LIB_TYPE_WRAPPER_HPP

#include "common.hpp"

namespace lib
{
template <bool val>
struct boolean_type {};

/**
* Wraps a type.
*/
template <typename T>
struct type_wrapper
{
	using type = T;
};

using void_type = type_wrapper<void>;

template <typename T>
using type = type_wrapper<T>;

template <typename T>
inline constexpr type<T> type_v{};

template <typename... T>
struct type_tuple;

/**
* An empty type_tuple.
*/
template <>
struct type_tuple<>
{
	using type			= type_tuple;
	using this_type		= type;
	using next_tuple	= this_type;
};

/**
* A type_tuple.
*/
template <typename T, typename... Others>
struct type_tuple<T, Others...> : type_tuple<Others...>
{
	using type			= T;
	using this_type		= type_tuple;
	using next_tuple	= type_tuple<Others...>;
};

template <typename T>
concept is_type_tuple = std::same_as<T, type_tuple<>> || std::derived_from<T, type_tuple<>>;

template <typename T, is_type_tuple tuple>
constexpr bool exist_in_type_tuple()
{
	using type = T;
	if (std::is_same_v<tuple, type_tuple<>>)
	{
		return false;
	}
	else if (std::is_same_v<type, typename tuple::type>)
	{
		return true;
	}
	return exist_in_type_tuple<type, typename tuple::next_tuple>();
}

}

#endif // !LIB_TYPE_WRAPPER_HPP
