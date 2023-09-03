#pragma once
#ifndef SET_H
#define SET_H

#include "hash.h"

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

template <typename key, provides_memory provided_allocator = system_memory, typename hasher = std::hash<key>, std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>>
class set :
	public hash_container_base<set_traits<key>, hasher, growth_policy, provided_allocator>
{
private:

	using super				= hash_container_base<set_traits<key>, hasher, growth_policy, provided_allocator>;
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
		return super::get_impl(key) != super::invalid_bucket_v;
	}

	constexpr void clear()
	{
		super::destruct(0, this->m_capacity);
	}

	constexpr void reserve(size_t capacity)
	{
		super::reserve(capacity);
	}

	template <typename... Args>
	constexpr type& emplace(Args&&... args)
	{
		bucket_value_type bucket = super::emplace_internal(std::forward<Args>(args)...);
		return super::data()[bucket];
	}

	template <typename... Args>
	constexpr std::optional<type&> try_emplace(key_type const& key, Args&&... args)
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
	constexpr std::optional<bucket_value_type> try_insert(key_type const& key, Args&&... args)
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

	constexpr std::optional<type&> element_at_bucket(bucket_value_type bucket) const
	{
		if (super::info()[bucket].value() == nullptr)
		{
			return std::nullopt;
		}
		return std::make_optional<type&>(super::data()[bucket]);
	}
};

}

#endif // !SET_H
