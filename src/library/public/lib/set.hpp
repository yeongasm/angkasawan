#pragma once
#ifndef LIB_SET_HPP
#define LIB_SET_HPP

#include "hash.hpp"

namespace lib
{

template<typename key>
struct set_traits
{
	using key_type		= std::remove_cv_t<key>;
	using value_type	= key_type;
	using type			= key_type;
	using mutable_type	= key_type;

	static constexpr key_type const& extract_key(type const& element)
	{
		return element;
	}

	static constexpr bool compare_keys(key_type const& a, key_type const& b)
	{
		return a == b;
	}
};

template <typename key, provides_memory in_allocator = allocator<typename set_traits<key>::type>, typename hasher = std::hash<key>, std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>>
class set :
	public hash_container_base<set_traits<key>, hasher, growth_policy, in_allocator>
{
private:

	using super				= hash_container_base<set_traits<key>, hasher, growth_policy, in_allocator>;
	using type				= typename super::interface_type;
	using key_type			= typename super::key_type;
	using value_type		= typename super::value_type;
	using iterator			= typename super::iterator;
	using const_iterator	= typename super::const_iterator;
	using bucket_value_type	= super::bucket_value_type;

public:

	using super::hash_container_base;
	using super::operator=;
	using super::invalid_bucket_v;

	constexpr bool contains(key_type const& key) const
	{
		return super::_get_impl(key) != super::invalid_bucket_v;
	}

	constexpr void clear()
	{
		super::_destruct(0, super::capacity());
	}

	constexpr void reserve(size_t capacity)
	{
		super::_reserve(capacity);
	}

	template <typename... Args>
	constexpr type& emplace(Args&&... args)
	{
		bucket_value_type bucket = super::_emplace_internal(std::forward<Args>(args)...);
		return super::_data()[bucket];
	}

	template <typename... Args>
	constexpr bucket_value_type insert(Args&&... args)
	{
		return super::_emplace_internal(std::forward<Args>(args)...);
	}

	constexpr bool erase(key_type const& key)
	{
		return super::_remove_impl(key);
	}

	constexpr bucket_value_type bucket(key_type const& key) const
	{
		return super::_get_impl(key);
	}

	constexpr type& element_at_bucket(bucket_value_type bucket) const
	{
		ASSERTION(super::_info()[bucket].value() != nullptr && "Bucket value supplied is invalid!");
		return super::_data()[bucket];
	}
};

}

#endif // !LIB_SET_HPP
