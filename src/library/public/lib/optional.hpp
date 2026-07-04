#pragma once
#ifndef LIB_OPTIONAL_HPP
#define LIB_OPTIONAL_HPP

#include <type_traits>
#include <utility>

/*
* TODO(Afiq):
* Implement optional and optional ref.
*/

namespace lib
{
struct in_place_t
{
	explicit constexpr in_place_t() = default;
};

inline constexpr in_place_t in_place{};

template <typename T>
class optional;

template <typename T>
inline constexpr bool is_optional = false;

template <typename T>
inline constexpr bool is_optional<optional<T>> = true;

template <typename T>
concept is_derived_from_optional = requires (const T& t)
{
	[]<typename U>(const optional<U>&){}(t);
};

struct nullopt_t
{
	enum class Tag { tag };

	explicit constexpr nullopt_t(Tag) noexcept {};
};

inline constexpr nullopt_t nullopt{ nullopt_t::Tag::tag };

}

#endif // !LIB_OPTIONAL_HPP