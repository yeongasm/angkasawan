#pragma once
#ifndef FOUNDATION_SET_H
#define FOUNDATION_SET_H

#include "hash.h"

FTLBEGIN

template<typename Key>
struct SetTraits
{
	using KeyType = std::remove_cv_t<Key>;
	using ValueType = KeyType;
	using Type = KeyType;
	using MutableType = KeyType;

	static constexpr const KeyType& extract_key(const Type& element)
	{
		return element;
	}

	static constexpr bool compare_keys(const KeyType& a, const KeyType& b)
	{
		return a == b;
	}
};

template <typename Key, ProvidesMemory AllocatorType = SystemMemory, typename Hasher = std::hash<Key>, std::derived_from<ContainerGrowthPolicy> GrowthPolicy = ShiftGrowthPolicy<4>>
class Set :
	public HashImpl<SetTraits<Key>, Hasher, GrowthPolicy, AllocatorType>
{
private:

	using Super			= HashImpl<SetTraits<Key>, Hasher, GrowthPolicy, AllocatorType>;
	using Type			= typename Super::InterfaceType;
	using KeyType		= typename Super::KeyType;
	using ValueType		= typename Super::ValueType;
	using Iterator		= typename Super::Iterator;
	using ConstIterator = typename Super::ConstIterator;
	using bucket_t		= Super::bucket_t;

public:

	using Super::HashImpl;
	using Super::operator=;
	using Super::invalid_bucket;

	constexpr bool contains(const KeyType& key) const
	{
		return Super::get_impl(key) != invalid_bucket();
	}

	constexpr void clear()
	{
		Super::destruct(0, this->m_capacity);
	}

	constexpr void reserve(size_t capacity)
	{
		Super::reserve(capacity);
	}

	template <typename... Args>
	constexpr Type& emplace(Args&&... args)
	{
		bucket_t bucket = Super::emplace_internal(std::forward<Args>(args)...);
		return Super::data()[bucket];
	}

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

	constexpr std::optional<Ref<Type>> element_at_bucket(bucket_t bucket) const
	{
		if (Super::info()[bucket].value() == nullptr)
		{
			return std::nullopt;
		}
		return std::make_optional<Ref<Type>>(&Super::data()[bucket]);
	}
};

FTLEND

#endif // !FOUNDATION_SET_H
