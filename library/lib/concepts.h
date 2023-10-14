#pragma once
#ifndef LIB_CONCEPTS_H
#define LIB_CONCEPTS_H

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

}

#endif // !LIB_CONCEPTS_H
