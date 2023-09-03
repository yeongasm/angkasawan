#pragma once
#ifndef MAP_H
#define MAP_H

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

	static constexpr key_type const& extract_key(const_reference object)
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

template <typename key, typename value, provides_memory provided_allocator = system_memory, typename hasher = std::hash<key>, std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>>
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

	template <typename K = key_type>
	constexpr value_type& operator[](K&& key)
	{
		bucket_value_type bucket = super::get_impl(key);
		if (bucket == super::invalid_bucket_v)
		{
			bucket = super::emplace_internal(std::forward<K>(key), value_type{});
		}
		return super::data()[bucket].second;
	}

	constexpr bool contains(key_type const& key) const
	{
		return super::get_impl(key) != super::invalid_bucket_v;
	}

	/**
	* Returns an optional with a Ref to the stored type.
	* If the key does not exist, a std::nullopt is returned instead.
	*/
	constexpr std::optional<type&> at(key_type const& key)
	{
		const bucket_value_type bucket = super::get_impl(key);
		if (bucket == super::invalid_bucket_v)
		{
			return std::nullopt;
		}
		return std::make_optional<type&>(super::data()[bucket]);
	}

	constexpr void clear()
	{
		super::destruct(0, super::size());
		super::m_len = 0;
	}

	/**
	* Reserve rehashes the map if it is not empty.
	* We do not have a rehash() method since it would eventually do the same thing in an open addressing hash map.
	*/
	constexpr void reserve(size_t capacity)
	{
		super::reserve(capacity);
	}

	/**
	* Emplaces a new object into the container. 
	* Elements with the same key would simply be replaced.
	*/
	template <typename... Args>
	constexpr type& emplace(Args&&... args)
	{
		const bucket_value_type bucket = super::emplace_internal(std::forward<Args>(args)...);
		return super::data()[bucket];
	}

	/**
	* Emplaces a new object into the container if it has yet to exist.
	* Returns std::nullopt if the key already exist.
	*/
	template <typename... Args>
	constexpr std::optional<type&> try_emplace(const key_type& key, Args&&... args)
	{
		if (contains(key))
		{
			return std::nullopt;
		}
		const bucket_value_type bucket = super::emplace_internal(std::forward<Args>(args)...);
		return std::make_optional<type&>(super::data()[bucket]);
	}

	template <typename... Args>
	constexpr bucket_value_type insert(Args&&... args)
	{
		return super::emplace_internal(std::forward<Args>(args)...);
	}

	template <typename... Args>
	constexpr std::optional<bucket_value_type> try_insert(const key_type& key, Args&&... args)
	{
		if (contains(key))
		{
			return std::nullopt;
		}
		const bucket_value_type bucket = super::emplace_internal(std::forward<Args>(args)...);
		return std::make_optional<bucket_value_type>(bucket);
	}

	constexpr bool erase(key_type const& key)
	{
		return super::remove_impl(key);
	}

	constexpr bucket_value_type bucket(key_type const& key) const
	{
		return super::get_impl(key);
	}

	constexpr std::optional<type&> element_at_bucket(bucket_value_type bucket)
	{
		if (super::info()[bucket].value() == nullptr)
		{
			return std::nullopt;
		}
		return std::make_optional<type&>(super::data()[bucket]);
	}
};

}

#endif // !MAP_H
