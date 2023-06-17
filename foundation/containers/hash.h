#pragma once
#ifndef FOUNDATION_HASH_H
#define FOUNDATION_HASH_H

#include "memory/memory.h"
#include "utility.h"

FTLBEGIN

template <size_t Limit>
struct PslLimit
{
	static constexpr size_t value = Limit;
};

// TODO:
// [ ] Implement min, max and normal load factor.
// 
// Linear probed, robin-hood hashing container with backward shifting deletion.
template <typename Traits, typename Hasher, std::derived_from<ContainerGrowthPolicy> GrowthPolicy, ProvidesMemory AllocatorType = SystemMemory, typename ProbeDistanceLimit = PslLimit<4096>>
class HashImpl : 
	private Hasher,
	private Traits,
	private GrowthPolicy,
	protected ProbeDistanceLimit,
	protected std::conditional_t<std::is_same_v<AllocatorType, SystemMemory>, SystemMemoryInterface<typename Traits::Type>, AllocatorMemoryInterface<typename Traits::Type, AllocatorType>>
{
protected:
	using hash_t	= uint32;
	using bucket_t	= size_t;

	using KeyType		= typename Traits::KeyType;
	using ValueType		= typename Traits::ValueType;
	using Type			= typename Traits::Type;
	using InterfaceType = std::conditional_t<std::is_same_v<KeyType, ValueType>, const Type, Type>;

	using Allocator = std::conditional_t<std::is_same_v<AllocatorType, SystemMemory>, SystemMemoryInterface<Type>, AllocatorMemoryInterface<Type, AllocatorType>>;

	static constexpr size_t		max_probe_distance	() { return ProbeDistanceLimit::value; }
	static constexpr bucket_t	invalid_bucket		() { return static_cast<bucket_t>(-1); }

	constexpr void check_allocator_referenced() const
	{
		if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
		{
			ASSERTION(Allocator::allocator_referenced() && "No allocator given to array!");
		}
	}

	constexpr AllocatorType* allocator() const requires !std::same_as<AllocatorType, SystemMemory>
	{
		return Allocator::get_allocator();
	}

	class BucketInfo
	{
	public:
		BucketInfo() : 
			m_data{}, m_psl{}, m_hash{}, m_bucket{}, m_lastBucket{}
		{}

		BucketInfo(uint32 hash, uint32 psl) : 
			m_data{}, m_hash{ hash }, m_psl{ psl }, m_bucket{}, m_lastBucket{}
		{}

		~BucketInfo() 
		{
			m_data = nullptr;
			m_psl = m_hash = 0;
			m_bucket = 0;
			m_lastBucket = false;
		}

		BucketInfo(const BucketInfo& rhs) { *this = rhs; }
		BucketInfo(BucketInfo&& rhs) noexcept { *this = std::move(rhs); }

		BucketInfo& operator=(const BucketInfo& rhs)
		{
			m_hash	= rhs.m_hash;
			m_psl	= rhs.m_psl;
			m_data	= rhs.m_data;
			m_bucket = rhs.m_bucket;
			m_lastBucket = rhs.m_lastBucket;
			return *this;
		}

		BucketInfo& operator=(BucketInfo&& rhs) noexcept
		{
			m_hash	= rhs.m_hash;
			m_psl	= rhs.m_psl;
			m_data	= rhs.m_data;
			m_bucket = rhs.m_bucket;
			m_lastBucket = rhs.m_lastBucket;
			new (&rhs) BucketInfo{};
			return *this;
		}

		constexpr void increment_probe_sequence() { ++m_psl; }
		constexpr void decrement_probe_sequence() { --m_psl; }

		constexpr uint32 hash() const { return m_hash; }
		constexpr uint32 probe_sequence_length() const { return m_psl; }

		constexpr bool is_empty() const { return m_hash == 0; }

		constexpr Type* value() const { return m_data; }

		constexpr void update_bucket_info(hash_t hash, uint32 psl, Type* data, bucket_t bucket)
		{
			m_data = data;
			m_hash = hash;
			m_psl = psl;
			m_bucket = bucket;
		}

		constexpr void set_psl(uint32 psl) { m_psl = psl; }

		//constexpr void	set_value(Type& object) { m_data = &object; }

		constexpr bool is_last_bucket() const	{ return m_lastBucket; }
		constexpr void set_as_last_bucket()		{ m_lastBucket = true; }

	private:
		Type*		m_data;
		hash_t		m_hash;
		bucket_t	m_bucket;
		uint32		m_psl;
		bool		m_lastBucket;
	};

	template <bool Const>
	class IteratorImpl
	{
	public:
		using Type = std::conditional_t<Const, const std::decay_t<Type>, std::decay_t<Type>>;
		using Pointer = Type*;
		using Reference = Type&;

		constexpr IteratorImpl() = default;
		constexpr ~IteratorImpl() = default;

		constexpr IteratorImpl(BucketInfo* data) :
			m_bucket{ data }, m_visited{}
		{
			while (m_bucket && m_bucket->is_empty() && !m_bucket->is_last_bucket())
			{
				++m_bucket;
			}
		}

		IteratorImpl(const IteratorImpl&) = default;
		IteratorImpl(IteratorImpl&&) = default;
		IteratorImpl& operator=(const IteratorImpl&) = default;
		IteratorImpl& operator=(IteratorImpl&&) = default;

		constexpr bool operator== (const IteratorImpl& rhs) const
		{
			return  m_bucket == rhs.m_bucket;
		}

		constexpr bool operator!= (const IteratorImpl& rhs) const
		{
			if (m_bucket && m_bucket == rhs.m_bucket && !m_visited)
			{
				return !m_bucket->is_empty();
			}
			return m_bucket != rhs.m_bucket;
		}

		constexpr Reference operator* () const { return *(m_bucket->value()); }
		constexpr Pointer	operator->() const { return m_bucket->value(); }

		constexpr IteratorImpl& operator++()
		{
			if (m_bucket)
			{
				if (m_bucket->is_last_bucket() &&
					!m_visited)
				{
					m_visited = true;
					return *this;
				}
				while (true)
				{
					++m_bucket;
					if (!m_bucket->is_empty() ||
						m_bucket->is_last_bucket())
					{
						break;
					}
				}
			}
			return *this;
		}
	private:
		BucketInfo* m_bucket;
		bool m_visited;
	};

public:
	using Iterator = IteratorImpl<false>;
	using ConstIterator = IteratorImpl<true>;

	constexpr HashImpl() requires std::same_as<AllocatorType, SystemMemory> :
		m_info{}, m_data{}, m_len{}, m_capacity{}, m_grow_on_next_insert{}
	{}

	constexpr HashImpl(AllocatorType& allocator) requires !std::same_as<AllocatorType, SystemMemory> :
		m_info{}, m_data{}, m_len{}, m_capacity{}, m_grow_on_next_insert{}, Allocator{ &allocator }
	{}

	constexpr ~HashImpl() { release(); }

	constexpr HashImpl(size_t length) requires std::same_as<AllocatorType, SystemMemory> :
		HashImpl{}
	{
		reserve(length);
	}

	constexpr HashImpl(size_t length, AllocatorType& allocator) requires !std::same_as<AllocatorType, SystemMemory> :
		HashImpl{ allocator }
	{
		reserve(length);
	}

	constexpr HashImpl(const HashImpl& rhs) requires std::same_as<AllocatorType, SystemMemory> :
		m_info{}, m_data{}, m_len{}, m_capacity{}, m_grow_on_next_insert{}
	{ 
		*this = rhs; 
	}

	constexpr HashImpl(const HashImpl& rhs) requires !std::same_as<AllocatorType, SystemMemory> :
		m_info{}, m_data{}, m_len{}, m_capacity{}, m_grow_on_next_insert{}, Allocator{ rhs.allocator() }
	{
		*this = rhs;
	}

	constexpr HashImpl(HashImpl&& rhs) :
		m_info{}, m_data{}, m_len{}, m_capacity{}, m_grow_on_next_insert{}
	{ 
		*this = std::move(rhs); 
	}

	HashImpl& operator=(const HashImpl& rhs)
	{
		if (this != &rhs)
		{
			destruct(0, m_capacity);

			if (m_capacity < rhs.m_capacity)
			{
				reserve(rhs.m_capacity);
			}

			m_len = rhs.m_len;

			size_t count = m_len;

			for (size_t i = 0; i < m_capacity; ++i)
			{
				if (rhs.m_info[i].is_empty())
				{
					continue;
				}

				m_info[i] = rhs.m_info[i];
				reinterpret_cast<Traits::MutableType&>(m_data[i]) = rhs.m_data[i];

				--count;
			}
		}
		return *this;
	}

	HashImpl& operator=(HashImpl&& rhs)
	{
		if (this != &rhs)
		{
			bool move = true;
			release();

			if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
			{
				if (allocator() != rhs.allocator())
				{
					reserve(rhs.size());

					m_len = rhs.m_len;
					m_grow_on_next_insert = rhs.m_grow_on_next_insert;

					for (size_t i = 0; i < rhs.m_capacity; ++i)
					{
						if (rhs.m_info[i].is_empty())
						{
							continue;
						}
						m_info[i] = std::move(rhs.m_info[i]);
						m_data[i] = std::move(rhs.m_data[i]);
					}
						
					rhs.release();
					move = false;
				}
			}

			if (move)
			{
				m_info = rhs.m_info;
				m_data = rhs.m_data;
				m_len = rhs.m_len;
				m_capacity = rhs.m_capacity;
				m_grow_on_next_insert = rhs.m_grow_on_next_insert;
			}

			if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
			{
				new (&rhs) HashImpl{ *rhs.allocator() };
			}
			else
			{
				new (&rhs) HashImpl{};
			}
		}
		return *this;
	}

	constexpr void release()
	{
		check_allocator_referenced();
		destruct(0, m_len);

		if (m_capacity)
		{
			Allocator::template free_storage_for<BucketInfo>(m_info);
			Allocator::free_storage(m_data);
		}

		m_info = nullptr;
		m_data = nullptr;
		m_len = m_capacity = 0;
		m_grow_on_next_insert = false;

		if constexpr (!std::is_same_v<AllocatorType, SystemMemory>)
		{
			Allocator::release_allocator();
		}
	}

	constexpr size_t	length	() const { return m_len; }
	constexpr size_t	size	() const { return m_capacity; }
	constexpr bool		empty	() const { return m_len == 0; }


	Iterator		begin	()		 { return Iterator{ m_info }; }
	Iterator		end		()		 { return Iterator{ m_info != nullptr ? m_info + (m_capacity - 1) : m_info }; }
	ConstIterator	begin	() const { return ConstIterator{ m_info }; }
	ConstIterator	end		() const { return ConstIterator{ m_info != nullptr ? m_info + (m_capacity - 1) : m_info }; }
	ConstIterator	cbegin	() const { return ConstIterator{ m_info }; }
	ConstIterator	cend	() const { return ConstIterator{ m_info != nullptr ? m_info + (m_capacity - 1) : m_info }; }

protected:

	BucketInfo* m_info;
	Type*	m_data;	// NOTE: For a map, we currently store them as a pair. Ideally, keys and values should be in their own array.
	size_t	m_len;
	size_t	m_capacity;
	bool	m_grow_on_next_insert;

	constexpr BucketInfo*		info()			{ return m_info; }
	constexpr const BucketInfo* info() const	{ return m_info; }

	constexpr Type*			data()			{ return m_data; }
	constexpr const Type*	data() const	{ return m_data; }

	constexpr void destruct(size_t from, size_t to, bool reconstruct = false)
	{
		for (size_t i = from; i < to; ++i)
		{
			m_data[i].~Type();
			m_info[i].~BucketInfo();

			if (reconstruct)
			{
				new (m_data + i) Type{};
				new (m_info + i) BucketInfo{};
			}
		}
	}

	constexpr void try_grow_next_insert()
	{
		if (m_grow_on_next_insert)
		{
			reserve(GrowthPolicy::new_capacity(m_capacity));
			m_grow_on_next_insert = false;
		}
	}	

	constexpr void reserve(size_t capacity)
	{
		ASSERTION(
			m_len < capacity && 
			"You are attempting to shrink the container when it has more elements than the specified capacity"
		);

		check_allocator_referenced();

		BucketInfo* infoOld = m_info;
		Type* dataOld = m_data;

		m_info = Allocator::template allocate_storage_for<BucketInfo>(capacity);
		m_data = Allocator::allocate_storage(capacity);

		for (size_t i = 0; i < capacity; ++i)
		{
			new (m_info + i) BucketInfo{};
			new (m_data + i) Type{};
		}

		m_info[capacity - 1].set_as_last_bucket();

		// Capacity needs to be set before the container rehash.
		size_t capacityOld = m_capacity;
		m_capacity = capacity;

		// Rehash container when buckets are occupied.
		if (m_len)
		{
			rehash(dataOld, infoOld, m_len);

			if (m_len > m_capacity)
			{
				m_len = m_capacity;
			}

			// Destruct old elements that were not moved.
			for (size_t i = 0; i < capacityOld; ++i)
			{
				dataOld[i].~Type();
			}

			Allocator::template free_storage_for<BucketInfo>(infoOld);
			Allocator::free_storage(dataOld);
		}
	}

		
	constexpr bucket_t get_impl(const KeyType& key) const
	{
		bucket_t bucket = invalid_bucket();
		if (m_capacity != 0)
		{
			find_key(key, &bucket);
		}
		return bucket;
	}

	template <typename... Args>
	constexpr bucket_t emplace_internal(Args&&... arguments)
	{
		if (!num_free_buckets())
		{
			reserve(GrowthPolicy::new_capacity(m_capacity));
		}
		else
		{
			try_grow_next_insert();
		}

		++m_len;

		return emplace_impl(std::forward<Args>(arguments)...);
	}

	// Implementation of remove method.
	// Backward shifting deletion.
	constexpr bool remove_impl(const KeyType& key)
	{
		bucket_t bucket = 0;

		if (!find_key(key, &bucket))
		{
			return false;
		}

		destruct(bucket, bucket + 1);
		// Move the bucket to the next one so the object can be shifted down.
		bucket = next_bucket(bucket);

		while (m_info[bucket].probe_sequence_length() != 0 &&
			!m_info[bucket].is_empty())
		{
			back_shift_bucket(previous_bucket(bucket), bucket);
			bucket = next_bucket(bucket);
		}

		--m_len;

		return true;
	}

private:

	template <typename StoredType = Type>
	constexpr void store_element_at(bucket_t bucket, StoredType&& object, const BucketInfo& info)
	{
		new (m_data + bucket) StoredType{ std::forward<StoredType>(object) };
		m_info[bucket].update_bucket_info(info.hash(), info.probe_sequence_length(), &m_data[bucket], bucket);
	}

	// Implementation of emplace method.
	// For a map, arguments NEEDS to be a std::pair or Pair struct with a key() method specified.
	template <typename... Args>
	constexpr bucket_t emplace_impl(Args&&... arguments)
	{
		// Construct the to-be-inserted element once.
		Type element{ std::forward<Args>(arguments)... };

		const hash_t hash = hashify(extract_key(element));
		bucket_t bucket = bucket_for_hash(hash);

		BucketInfo info{ hash, 0 };

		while (!m_info[bucket].is_empty())
		{
			if (info.probe_sequence_length() > m_info[bucket].probe_sequence_length())
			{
				// Number of probes is really high, rehash container on next insert.
				if (info.probe_sequence_length() > max_probe_distance())
				{
					m_grow_on_next_insert = true;
				}
				swap_element_at(bucket, element, info);
			}

			bucket = next_bucket(bucket);
			info.increment_probe_sequence();
		}

		store_element_at(bucket, std::move(element), info);

		return bucket;
	}

	constexpr void rehash(Type* sourceData, BucketInfo* sourceInfo, size_t count)
	{
		for (size_t i = 0, emplaced = 0; count != 0 && emplaced < m_capacity; ++i)
		{
			// hash value of 0 means the bucket is empty.
			if (sourceInfo[i].is_empty()) { continue; }

			emplace_impl(std::move(sourceData[i]));

			++emplaced;
			--count;
		}
	}

	constexpr bucket_t previous_bucket(bucket_t current) const
	{
		bucket_t bucket = current - 1;
		if (bucket == -1)
		{
			bucket = m_capacity - 1;
		}
		return bucket % m_capacity;
	}


	constexpr bucket_t next_bucket(bucket_t current) const
	{
		return (current + 1) % m_capacity;
	}


	constexpr bool find_key(const KeyType& key, bucket_t* outBucket) const
	{
		bool found = false;
		const hash_t hash = hashify(key);

		bucket_t bucket = bucket_for_hash(hash);
		size_t probeSequence = 0;

		while (probeSequence <= m_info[bucket].probe_sequence_length() && 
			!m_info[bucket].is_empty())
		{
			if (compare_key(key, extract_key(m_data[bucket])))
			{
				if (outBucket)
				{
					*outBucket = bucket;
				}
				found = true;
				break;
			}
			bucket = next_bucket(bucket);
			++probeSequence;
		}
		return found;
	}


	// Helper function to swap contents.
	constexpr void swap_element_at(bucket_t bucket, Type& element, BucketInfo& info)
	{
		// NOTE:
		// Swap actual data.
		// We do a memcopy to avoid weird funny logic that users might implement for their move assignment operator
		Type temporary{};
		ftl::memcopy(&temporary, &m_data[bucket], sizeof(Type));
		new (m_data + bucket) Type{ std::move(element) };
		ftl::memcopy(&element, &temporary, sizeof(Type));

		uint32 psl = m_info[bucket].probe_sequence_length();
		m_info[bucket].update_bucket_info(info.hash(), info.probe_sequence_length(), &m_data[bucket], bucket);
		info.set_psl(psl);
	}

	constexpr void back_shift_bucket(size_t to, size_t from)
	{
		m_info[to] = std::move(m_info[from]);
		ftl::memmove(&m_data[to], &m_data[from], sizeof(Type));

		// Reduce the bucket's psl by 1.
		m_info[to].decrement_probe_sequence();
		destruct(from, from + 1);
	}

	constexpr size_t num_free_buckets() const
	{
		return m_capacity - m_len;
	}

	constexpr bucket_t bucket_for_hash(hash_t hash) const
	{
		return hash % m_capacity;
	}

	constexpr uint32 hashify(const KeyType& key) const
	{
		return static_cast<uint32>(Hasher{}(key));
	}

	constexpr bool compare_key(const KeyType& a, const KeyType& b) const
	{
		return Traits::compare_keys(a, b);
	}

	constexpr decltype(auto) extract_key(const Type& element) const
	{
		return Traits::extract_key(element);
	}
};

FTLEND

#endif // !FOUNDATION_HASH_H
