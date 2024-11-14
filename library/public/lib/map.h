#pragma once
#ifndef LIB_MAP_H
#define LIB_MAP_H

#include "hash.h"

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

template <typename key, typename value, provides_memory provided_allocator = default_allocator, typename hasher = std::hash<key>, std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>>
class map : 
	public hash_container_base<map_traits<key, value>, hasher, growth_policy, provided_allocator>
{
public:
	using super				= hash_container_base<map_traits<key, value>, hasher, growth_policy, provided_allocator>;
	using type				= typename super::interface_type;			// std::pair<const key_type, value_type>
	using key_type			= typename super::key_type;
	using value_type		= typename super::value_type;
	using iterator			= typename super::iterator;
	using const_iterator	= typename super::const_iterator;
	using bucket_value_type	= super::bucket_value_type;

	using super::hash_container_base;
	using super::operator=;
	using super::invalid_bucket_v;

	template <typename K>
	constexpr value_type& operator[](K&& key) requires std::same_as<std::decay_t<K>, key_type>
	{
		bucket_value_type bucket = super::_get_impl(key);
		if (bucket == super::invalid_bucket_v)
		{
			bucket = super::_emplace_internal(std::forward<K>(key), value_type{});
		}
		return super::_data()[bucket].second;
	}

	constexpr bool contains(key_type const& key) const
	{
		return super::_get_impl(key) != super::invalid_bucket_v;
	}

	/**
	* Returns an optional with a Ref to the stored type.
	* If the key does not exist, a std::nullopt is returned instead.
	*/
	constexpr ref<type> at(key_type const& key) const
	{
		ref<type> result{};
		const bucket_value_type bucket = super::_get_impl(key);
		//ASSERTION(bucket != super::invalid_bucket_v && "Key supplied is invalid!");
		if (bucket != super::invalid_bucket_v)
		{
			new (&result) ref<type>{ super::_data()[bucket] };
		}
		return result;
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
	constexpr std::pair<type&, bool> try_emplace(const key_type& key, Args&&... args)
	{
		if (contains(key))
		{
			return std::pair{ at(key) , false};
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
	constexpr std::optional<bucket_value_type> try_insert(const key_type& key, Args&&... args)
	{
		if (contains(key))
		{
			return std::nullopt;
		}
		const bucket_value_type bucket = super::_emplace_internal(key, std::forward<Args>(args)...);
		return std::make_optional<bucket_value_type>(bucket);
	}

	constexpr bool erase(key_type const& key)
	{
		return super::_remove_impl(key);
	}

	constexpr bucket_value_type bucket(key_type const& key) const
	{
		return super::_get_impl(key);
	}

	constexpr type& element_at_bucket(bucket_value_type bucket)
	{
		ASSERTION(!super::_info()[bucket].is_empty() && "Bucket value supplied is invalid!");
		return super::_data()[bucket];
	}
};

}

#endif // !LIB_MAP_H
