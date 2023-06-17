#pragma once
#ifndef FOUNDATION_MAP_H
#define FOUNDATION_MAP_H

#include "hash.h"

FTLBEGIN

template<typename Key, typename Value>
struct MapTraits
{
	using KeyType	= std::remove_cv_t<Key>;
	using ValueType = Value;
	using Type = std::pair<const KeyType, ValueType>;
	using MutableType = std::pair<KeyType, ValueType>;

	static constexpr const KeyType& extract_key(const Type& object)
	{
		return object.first;
	}

	static constexpr bool compare_keys(const KeyType& a, const KeyType& b)
	{
		return a == b;
	}
};

// TODO:
// Figure out a way to emplace elements without specifying a key and just let the Traits::extract_key work.

template <typename Key, typename Value, ProvidesMemory AllocatorType = SystemMemory, typename Hasher = std::hash<Key>, std::derived_from<ContainerGrowthPolicy> GrowthPolicy = ShiftGrowthPolicy<4>>
class Map : 
	public HashImpl<MapTraits<Key, Value>, Hasher, GrowthPolicy, AllocatorType>
{
private:

	using Super			= HashImpl<MapTraits<Key, Value>, Hasher, GrowthPolicy, AllocatorType>;
	using Type			= typename Super::InterfaceType;			// std::pair<const KeyType, ValueType>
	using KeyType		= typename Super::KeyType;
	using ValueType		= typename Super::ValueType;
	using Iterator		= typename Super::Iterator;
	using ConstIterator = typename Super::ConstIterator;
	using bucket_t		= Super::bucket_t;

public:

	using Super::HashImpl;
	using Super::operator=;
	using Super::invalid_bucket;

	template <typename K = KeyType>
	constexpr ValueType& operator[](K&& key)
	{
		bucket_t bucket = Super::get_impl(key);
		if (bucket == invalid_bucket())
		{
			bucket = Super::emplace_internal(std::forward<K>(key), ValueType{});
		}
		return Super::data()[bucket].second;
	}

	constexpr bool contains(const KeyType& key) const
	{
		return Super::get_impl(key) != invalid_bucket();
	}

	/**
	* Returns an optional with a Ref to the stored type.
	* If the key does not exist, a std::nullopt is returned instead.
	*/
	constexpr std::optional<Ref<Type>> at(const KeyType& key)
	{
		const bucket_t bucket = Super::get_impl(key);
		if (bucket == invalid_bucket())
		{
			return std::nullopt;
		}
		return std::make_optional<Ref<Type>>(&Super::data()[bucket]);
	}

	constexpr void clear()
	{
		Super::destruct(0, Super::size(), true);
		Super::m_len = 0;
		Super::m_info[Super::m_capacity - 1].set_as_last_bucket();
	}

	/**
	* Reserve rehashes the map if it is not empty.
	* We do not have a rehash() method since it would eventually do the same thing in an open addressing hash map.
	*/
	constexpr void reserve(size_t capacity)
	{
		Super::reserve(capacity);
	}

	/**
	* Emplaces a new object into the container. 
	* Elements with the same key would simply be replaced.
	*/
	template <typename... Args>
	constexpr Type& emplace(Args&&... args)
	{
		const bucket_t bucket = Super::emplace_internal(std::forward<Args>(args)...);
		return Super::data()[bucket];
	}

	/**
	* Emplaces a new object into the container if it has yet to exist.
	* Returns std::nullopt if the key already exist.
	*/
	template <typename... Args>
	constexpr std::optional<Ref<Type>> try_emplace(const KeyType& key, Args&&... args)
	{
		if (contains(key))
		{
			return std::nullopt;
		}
		const bucket_t bucket = Super::emplace_internal(std::forward<Args>(args)...);
		return std::make_optional<Ref<Type>>(&Super::data()[bucket]);
	}

	template <typename... Args>
	constexpr bucket_t insert(Args&&... args)
	{
		return Super::emplace_internal(std::forward<Args>(args)...);
	}

	template <typename... Args>
	constexpr std::optional<bucket_t> try_insert(const KeyType& key, Args&&... args)
	{
		if (contains(key))
		{
			return std::nullopt;
		}
		const bucket_t bucket = Super::emplace_internal(std::forward<Args>(args)...);
		return std::make_optional<bucket_t>(bucket);
	}

	constexpr bool erase(const KeyType& key)
	{
		return Super::remove_impl(key);
	}

	constexpr bucket_t bucket(const KeyType& key) const
	{
		return Super::get_impl(key);
	}

	constexpr std::optional<Ref<Type>> element_at_bucket(bucket_t bucket)
	{
		if (Super::info()[bucket].value() == nullptr)
		{
			return std::nullopt;
		}
		return std::make_optional<Ref<Type>>(&Super::data()[bucket]);
	}
};

FTLEND

#endif // !FOUNDATION_MAP_H
