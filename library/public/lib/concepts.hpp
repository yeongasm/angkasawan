#pragma once
#ifndef LIB_CONCEPTS_HPP
#define LIB_CONCEPTS_HPP

#include <concepts>
#include <type_traits>

namespace lib
{
template <typename T>
concept is_handle_value_type = std::integral<T> || std::is_pointer_v<T>;

template <typename flag_type>
concept is_enum = std::is_enum_v<flag_type>;

template <typename T>
concept is_pointer = std::is_pointer_v<T>;

template <typename T>
concept is_char_type = std::same_as<T, char> || std::same_as<T, wchar_t>;

template <typename T>
concept is_bit_compatible = std::is_enum_v<T> || std::is_integral_v<T>;

template <typename T>
concept bit_mask = is_enum<T> || std::signed_integral<T> || std::unsigned_integral<T>;

template <typename T>
concept is_trivial = std::is_trivial_v<T>;

template <typename From, typename To>
concept is_assignable_to = std::is_assignable_v<To, From> || std::is_convertible_v<From, To>;
}

#endif // !LIB_CONCEPTS_HPP
