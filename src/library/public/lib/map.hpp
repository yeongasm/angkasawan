#pragma once
#ifndef LIB_MAP_HPP
#define LIB_MAP_HPP

#include "hash.hpp"

namespace lib
{

template<typename key, typename value>
struct map_traits
{
	using key_type			= std::remove_cv_t<key>;
	using value_type		= value;
	using type				= std::pair<const key_type, value_type>;
	using const_type		= type const;
	using reference			= type&;
	using const_reference	= type const&;
	using mutable_type		= std::pair<key_type, value_type>;
	using mut_const_ref		= mutable_type const&;

	static constexpr key_type const& extract_key(const_reference object)
	{
		return object.first;
	}

	static constexpr key_type const& extract_key(mut_const_ref object)
	{
		return object.first;
	}

	static constexpr bool compare_keys(const key_type& a, const key_type& b)
	{
		return a == b;
	}
};

// TODO:
// Figure out a way to emplace elements without specifying a key and just let the Traits::extract_key work.

template <typename key, typename value, provides_memory in_allocator = allocator<typename map_traits<key, value>::type>, typename hasher = std::hash<key>, std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>>
class map : 
	public hash_container_base<map_traits<key, value>, hasher, growth_policy, in_allocator>
{
public:
	using super				= hash_container_base<map_traits<key, value>, hasher, growth_policy, in_allocator>;
	using type				= typename super::interface_type;			// std::pair<const key_type, value_type>
	using key_type			= typename super::key_type;
	using value_type		= typename super::value_type;
	using iterator			= typename super::iterator;
	using const_iterator	= typename super::const_iterator;
	using bucket_value_type	= super::bucket_value_type;

	using super::super;
	using super::operator=;
	using super::invalid_bucket_v;

	template <typename K>
	constexpr value_type& operator[](K&& in_key) requires std::same_as<std::decay_t<K>, key_type>
	{
		bucket_value_type bucket = super::_get_impl(in_key);
		if (bucket == invalid_bucket_v)
		{
			bucket = super::_emplace_internal(std::forward<K>(in_key), value_type{});
		}
		return super::_data()[bucket].second;
	}

	constexpr bool contains(key_type const& in_key) const
	{
		return super::_get_impl(in_key) != invalid_bucket_v;
	}

	/**
	* Returns an optional with a Ref to the stored type.
	* If the key does not exist, a std::nullopt is returned instead.
	*/
	constexpr std::optional<ref<type>> at(key_type const& in_key) const
	{
		const bucket_value_type bucket = super::_get_impl(in_key);
		return (bucket != invalid_bucket_v) ? std::optional{ ref<type>{ super::_data()[bucket] } } : std::nullopt;
	}

	constexpr void clear()
	{
		super::_destruct(0, super::capacity());
		super::m_len = 0;
	}

	/**
	* Reserve rehashes the map if it is not empty.
	* We do not have a rehash() method since it would eventually do the same thing in an open addressing hash map.
	*/
	constexpr void reserve(size_t capacity)
	{
		super::_reserve(capacity);
	}

	/**
	* Emplaces a new object into the container. 
	* Elements with the same key would simply be replaced.
	*/
	template <typename... Args>
	constexpr type& emplace(Args&&... args)
	{
		const bucket_value_type bucket = super::_emplace_internal(std::forward<Args>(args)...);
		return super::_data()[bucket];
	}

	/**
	* Emplaces a new object into the container if it has yet to exist.
	* Returns std::nullopt if the key already exist.
	*/
	template <typename... Args>
	constexpr std::pair<type&, bool> try_emplace(const key_type& in_key, Args&&... args)
	{
		if (contains(in_key))
		{
			return std::pair{ at(in_key) , false};
		}
		const bucket_value_type bucket = super::_emplace_internal(std::forward<Args>(args)...);
		return std::pair{ super::_data()[bucket], true };
	}

	template <typename... Args>
	constexpr bucket_value_type insert(Args&&... args)
	{
		return super::_emplace_internal(std::forward<Args>(args)...);
	}

	template <typename... Args>
	constexpr std::optional<bucket_value_type> try_insert(const key_type& in_key, Args&&... args)
	{
		if (contains(in_key))
		{
			return std::nullopt;
		}
		const bucket_value_type bucket = super::_emplace_internal(in_key, std::forward<Args>(args)...);
		return std::make_optional<bucket_value_type>(bucket);
	}

	constexpr bool erase(key_type const& in_key)
	{
		return super::_remove_impl(in_key);
	}

	constexpr bucket_value_type bucket(key_type const& in_key) const
	{
		return super::_get_impl(in_key);
	}

	constexpr type& element_at_bucket(bucket_value_type bucket)
	{
		ASSERTION(!super::_info()[bucket].is_empty() && "Bucket value supplied is invalid!");
		return super::_data()[bucket];
	}
};

}

#endif // !LIB_MAP_HPP
