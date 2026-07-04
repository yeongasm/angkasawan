/*
* TODO(Afiq):
* Implement variant as an exercise.
*/
#pragma once
#include <type_traits>
#ifndef LIB_VARIANT_HPP
#define LIB_VARIANT_HPP

#include <concepts>
#include <utility>

#include "common.hpp"

namespace lib
{
namespace detail
{
template <typename T, typename... Args>
constexpr auto find_index_of_impl() -> size_t
{
	size_t index = {};
	for (bool same : { std::is_same_v<T, Args>... })
	{
		if (same)
		{
			break;
		}
		++index;
	}
	return index;
};

template <typename T, typename... Args>
constexpr size_t find_index_of_v = find_index_of_impl<T, Args...>();

template <typename T, typename... Args>
concept is_type_within_bounds = (find_index_of_impl<T, Args...>() < sizeof...(Args));

template <size_t Num>
constexpr auto unsigned_atleast() -> auto
{
	if constexpr (Num < std::numeric_limits<uint8>::max())
	{
		return uint8{};
	}
	else if constexpr (Num < std::numeric_limits<uint16>::max())
	{
		return uint16{};
	}
	else if constexpr (Num < std::numeric_limits<uint32>::max())
	{
		return uint32{};
	}
	else
	{
		return uint64{};
	}
};

template <template <typename...> typename TemplateType, typename What>
struct instance_of_impl : std::false_type {};

template <template <typename...> typename What, typename... Ts>
struct instance_of_impl<What, What<Ts...>> : std::true_type {};

template <typename T, template <typename...> typename TemplateType>
concept instance_of = instance_of_impl<TemplateType, T>::value;

// template <size_t I, typename T>
// struct type_leaf {};

// template <typename IndexSequence, typename... Ts>
// struct type_map;

// template <size_t... Is, typename... Ts>
// struct type_map<std::index_sequence<Is...>, Ts...> : type_leaf<Is, Ts>... {};

// template <size_t I, typename T>
// constexpr auto extract_type(type_leaf<I, T>) -> std::type_identity<T>;

// template <size_t N, typename... Types>
// requires (N < sizeof...(Types))
// using nth_type_t = typename decltype(extract_type<N>(type_map<std::index_sequence_for<Types...>, Types...>{}))::type;

template <size_t Num>
using unsigned_atleast_t = decltype(unsigned_atleast<Num>());

template <size_t N, typename... Types>
constexpr auto nth_type() -> auto
{
	return []<size_t... Is>(std::index_sequence<Is...>) -> auto
	{
		return [](decltype((void*)Is)..., auto nth, auto...)
		{
			return *nth;
		}((&std::type_identity<Types>{})...);
	}(std::make_index_sequence<N>{});
};

template <size_t N, typename... Types>
requires (N < sizeof...(Types))
using nth_type_t = typename decltype(nth_type<N, Types...>())::type;

template <typename This, typename... Rest>
struct union_storage
{
	using this_type = This;

	union
	{
		This head;
		union_storage<Rest...> tail;
	};

	constexpr union_storage() = default;
	constexpr union_storage(std::in_place_index_t<0>, auto&&... args) requires std::constructible_from<This, decltype(args)...> :
		head{ std::forward<decltype(args)>(args)... }
	{}

	template <size_t Idx>
	constexpr union_storage(std::in_place_index_t<Idx>, auto&&... args) :
		tail{ std::in_place_index_t<Idx - 1>{}, std::forward<decltype(args)>(args)... }
	{}

	template <typename Self>
	constexpr auto data(this Self&& self) -> decltype(auto)
	{
		return std::forward_like<Self>(self.head);
	}
};

template <typename This>
struct union_storage<This>
{
	using this_type = This;

	This head;

	constexpr union_storage() = default;
	constexpr union_storage(std::in_place_index_t<0>, auto&&... args) requires std::constructible_from<This, decltype(args)...> :
		head{ std::forward<decltype(args)>(args)... }
	{}
};

template <typename>
struct union_size;

template <typename... Ts>
struct union_size<union_storage<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template <typename Union>
constexpr size_t union_size_v = union_size<Union>::value;

template <size_t Idx, typename Self>
constexpr auto get_impl(Self&& self) -> decltype(auto)
requires instance_of<std::remove_cvref_t<Self>, union_storage> && (Idx < union_size_v<Self>)
{
	if constexpr (Idx == 0)
	{
		return self.head;
	}
	return get<Idx - 1>(std::forward_like<Self>(self.tail));
};
}

template <typename... Types>
class variant
{
public:
	using type = variant<Types...>;

	constexpr variant() noexcept = default;
	constexpr ~variant() noexcept { clear(); }

	template <size_t Idx, typename... Args>
	constexpr variant(std::in_place_index_t<Idx>, Args&&... args) noexcept requires std::constructible_from<detail::nth_type_t<Idx, Types...>, Args...> :
		m_storage{},
		m_idx{ Idx }
	{
		construct_at<Idx>(m_storage, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	constexpr variant(std::in_place_type_t<T>, Args&&... args) noexcept requires (detail::is_type_within_bounds<T, Types...> && std::constructible_from<T, Args...>) :
		m_storage{},
		m_idx{ detail::find_index_of_v<T, Types...> }
	{
		construct_at<detail::find_index_of_v<T, Types...>>(m_storage, std::forward<Args>(args)...);
	}

	// Implement copy-constructor, copy-assignment operator, move-constructor and move-assignment operator.

	constexpr auto has_value() const noexcept -> bool
	{
		return m_idx != std::numeric_limits<size_t>::max();
	}

	constexpr operator bool() const noexcept
	{
		return has_value();
	}

	template <typename T>
	constexpr auto operator=(T&& src) noexcept -> type& requires detail::is_type_within_bounds<T, Types...>
	{
		if (has_value())
		{
			clear();
		}
		constexpr size_t idx = detail::find_index_of_v<T, Types...>;
		construct_at<idx>(m_storage, std::forward<T>(src));
		m_idx = idx;
		return *this;
	}

	template <typename T, typename... Args>
	constexpr auto emplace(Args&&... args) noexcept -> T& requires (detail::is_type_within_bounds<T, Types...> && std::constructible_from<T, Args...>)
	{
		m_idx = detail::find_index_of_v<T, Types...>;
		return construct_at<detail::find_index_of_v<T, Types...>>(m_storage, std::forward<Args>(args)...);
	}

	constexpr auto clear() noexcept -> void
	{
		[&]<size_t... Idx>(std::index_sequence<Idx...>)
		{
			(((m_idx == Idx) ? (destroy_at<Idx>(m_storage), 0) : 0), ...);
		}(std::index_sequence_for<Types...>{});
	}

	constexpr auto index() const noexcept -> size_t { return m_idx; }

	// implement swap

private:
	detail::union_storage<Types...> m_storage;
	size_t m_idx = std::numeric_limits<size_t>::max();

	using union_storage_type = detail::union_storage<Types...>;

	template <size_t Idx, typename... Args>
	constexpr auto construct_at(union_storage_type& storage, Args&&... args) noexcept -> decltype(auto)
	{
		return *std::construct_at(&detail::get_impl<Idx>(storage), std::in_place_index<Idx>, std::forward<Args>(args)...);
	}

	template <size_t Idx>
	constexpr auto destroy_at(union_storage_type& storage) noexcept -> void
	{
		std::destroy_at(&detail::get_impl<Idx>(storage));

	};
};

using detail::find_index_of_v;
using detail::unsigned_atleast_t;
using detail::instance_of;
using detail::nth_type_t;
}

#endif // !LIB_VARIANT_HPP