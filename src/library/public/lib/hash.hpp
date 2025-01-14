#pragma once
#ifndef LIB_HPPASH_HPP
#define LIB_HPPASH_HPP

#include "memory.hpp"
#include "utility.hpp"

namespace lib
{

template <size_t limit>
struct probe_sequence_length_limit
{
	static constexpr size_t value = limit;
};

template <float32 factor>
struct load_factor_limit
{
	static constexpr float32 value = factor;
};

// TODO:
// [x] Implement min, max and normal load factor.
// [x] Figure out how to pack load factor variable without making the container exceed 40 bytes.
// [x] Fix iterators now that bucket_info is condensed to only hash & psl.
// 
// Linear probed, robin-hood hashing container with backward shifting deletion.
template <
	typename traits, 
	typename hasher, 
	std::derived_from<container_growth_policy> growth_policy, 
	provides_memory allocator, 
	typename probe_distance_limit = probe_sequence_length_limit<4096>,
	typename default_load_factor = load_factor_limit<0.75f>
>
class hash_container_base
{
protected:

	using hash_value_type		= uint32;
	using bucket_value_type		= size_t;
	using key_type				= typename traits::key_type;
	using value_type			= typename traits::value_type;
	using type					= typename traits::type;
	using mutable_type			= typename traits::mutable_type;
	using const_type			= type const;
	using pointer				= type*;
	using const_pointer			= type const*;
	using interface_type		= std::conditional_t<std::is_same_v<key_type, value_type>, const_type, type>;
	using allocator_type		= allocator;

	static constexpr bucket_value_type invalid_bucket_v = std::numeric_limits<bucket_value_type>::max();

	friend class hash_container_const_iterator;

	struct bucket_info
	{
		hash_value_type	hash;		// Hash value for keys are cached to save computation time.
		uint32			psl;		// The value of each bucket's probe sequence length has to be cached for backshift deletion to work.

		constexpr void reset() { hash = psl = 0; }

		constexpr bool is_empty() const { return hash == 0; }
	};

	struct metadata
	{
		bool			growOnNextInsert;
		bucket_info*	pBucketInfo;
	};

public:
	
	struct hash_container_const_iterator
	{
		using pointer	= type const*;
		using reference	= type const&;

		constexpr hash_container_const_iterator() = default;
		constexpr ~hash_container_const_iterator() = default;

		constexpr hash_container_const_iterator(bucket_info* data, hash_container_base const* hash_map) :
			info{ data }, hash{ hash_map }
		{
			if (data && hash->m_box->metadata)
			{
				// This logic is here to just skip empty buckets.
				bucket_info* end = hash->m_box->metadata->pBucketInfo + hash->m_capacity;

				while (info->is_empty() &&
					   info != end)
				{
					++info;
				}
			}
		}

		hash_container_const_iterator(hash_container_const_iterator const&)				= default;
		hash_container_const_iterator(hash_container_const_iterator&&)					= default;
		hash_container_const_iterator& operator=(hash_container_const_iterator const&)	= default;
		hash_container_const_iterator& operator=(hash_container_const_iterator&&)		= default;

		friend constexpr bool operator== (hash_container_const_iterator const& lhs, hash_container_const_iterator const& rhs)
		{
			return  lhs.info == rhs.info;
		}

		constexpr reference	operator* () const { return hash->_data()[bucket()]; }
		constexpr pointer	operator->() const { return &hash->_data()[bucket()]; }

		constexpr bucket_value_type bucket() const { return static_cast<bucket_value_type>(info - hash->m_box->metadata->pBucketInfo) ;}

		constexpr hash_container_const_iterator& operator++()
		{
			while (true)
			{
				++info;
				// This checks if the current bucket is the final one in the hash container.
				if (info == (hash->_info() + hash->m_capacity))
				{
					break;
				}
				// These conditions cannot be in a single line as the are both valid checks.
				if (!info->is_empty())
				{
					break;
				}
			}
			return *this;
		}

		bucket_info* info;
		hash_container_base const* hash;
	};

	struct hash_container_iterator : hash_container_const_iterator
	{
		using super		= hash_container_const_iterator;
		using pointer	= type*;
		using reference = type&;

		using super::hash_container_const_iterator;

		constexpr reference	operator* () const { return this->hash->_data()[this->bucket()]; }
		constexpr pointer	operator->() const { return &this->hash->_data()[this->bucket()]; }
	};

	using iterator			= hash_container_iterator;
	using const_iterator	= hash_container_const_iterator;

	constexpr hash_container_base() :
		m_box{}, 
		m_len{}, 
		m_capacity{}, 
		m_maxLoadFactor{ default_load_factor::value }
	{}

	constexpr hash_container_base(allocator_type const& allocator) :
		m_box{ stored_type{}, allocator }, 
		m_len{}, 
		m_capacity{},
		m_maxLoadFactor{ default_load_factor::value }
	{}

	constexpr ~hash_container_base() { release(); }

	constexpr hash_container_base(size_t length, allocator_type const& allocator = allocator_type{}) :
		hash_container_base{ allocator }
	{
		_reserve(length);
	}

	constexpr hash_container_base(hash_container_base const& other) :
		hash_container_base{ std::allocator_traits<allocator_type>::select_on_containere_copy_construction(other.m_box) }
	{
		_deep_copy(other);
	}

	constexpr hash_container_base(hash_container_base&& other) :
		m_box{ std::exchange(other.m_box, {}) },
		m_len{ std::exchange(other.m_len, {}) },
		m_capacity{ std::exchange(other.m_capacity, {}) },
		m_maxLoadFactor{ std::exchange(other.m_maxLoadFactor, default_load_factor::value) }
	{}

	constexpr hash_container_base(std::initializer_list<type> const& initializer, allocator_type const& allocator = allocator_type{}) :
		hash_container_base{ allocator }
	{
		for (type const& pair : initializer)
		{
			_emplace_internal(pair);
		}
	}

	hash_container_base& operator=(hash_container_base const& other)
	{
		if (this != &other)
		{
			_destruct(0, m_capacity);
			_deep_copy(other);
		}
		return *this;
	}

	hash_container_base& operator=(hash_container_base&& other)
	{
		if (this != &other)
		{
			release();

			m_box = std::exchange(other.m_box, {});
			m_len = std::exchange(other.m_len, {});
			m_capacity = std::exchange(other.m_capacity, {});
			m_maxLoadFactor = std::exchange(other.m_maxLoadFactor, default_load_factor::value);
		}
		return *this;
	}

	constexpr void release()
	{
		_destruct(0, capacity());

		if (m_capacity)
		{
			if (m_box->metadata != nullptr)
			{
				using byte_allocator = typename std::allocator_traits<allocator_type>::template rebind_alloc<std::byte>;
				size_t const metadataCapacity = sizeof(metadata) + (sizeof(bucket_info) * m_capacity);

				byte_allocator byteAllocator{ m_box.resource() };

				std::allocator_traits<byte_allocator>::deallocate(byteAllocator, reinterpret_cast<std::byte*>(m_box->metadata), metadataCapacity);
			}

			if (m_box->data != nullptr)
			{
				std::allocator_traits<allocator_type>::deallocate(m_box, m_box->data, m_capacity);
			}
		}

		m_len = m_capacity = 0;
		m_maxLoadFactor = default_load_factor::value;

		m_box.~box_type();
	}

	constexpr size_t	size		() const { return m_len; }
	constexpr size_t	capacity	() const { return m_capacity; }
	constexpr bool		empty		() const { return m_len == 0; }
	constexpr float32	load_factor	() const { return static_cast<float32>(m_len) / static_cast<float32>(m_capacity); }
	constexpr float32	max_load_factor() const { return m_maxLoadFactor; }
	constexpr void		max_load_factor(float32 factor) 
	{
		if (factor > 0.f && factor < 1.f)
		{
			m_maxLoadFactor = factor; 
		}
	}

	iterator		begin	()		 { return iterator{ m_box->metadata ? m_box->metadata->pBucketInfo : nullptr, this }; }
	iterator		end		()		 { return iterator{ m_box->metadata ? m_box->metadata->pBucketInfo + m_capacity : nullptr, this }; }
	const_iterator	begin	() const { return const_iterator{ m_box->metadata ? m_box->metadata->pBucketInfo : nullptr, this }; }
	const_iterator	end		() const { return const_iterator{ m_box->metadata ? m_box->metadata->pBucketInfo + m_capacity : nullptr, this }; }
	const_iterator	cbegin	() const { return const_iterator{ m_box->metadata ? m_box->metadata->pBucketInfo : nullptr, this }; }
	const_iterator	cend	() const { return const_iterator{ m_box->metadata ? m_box->metadata->pBucketInfo + m_capacity : nullptr, this }; }

protected:

	struct stored_type
	{
		metadata* metadata;
		pointer data;
	};

	using box_type = box<stored_type, allocator_type>;

	box_type m_box;
	size_t	m_len;
	size_t	m_capacity;
	float32 m_maxLoadFactor;

	constexpr bucket_info*	_info() const	{ return m_box->metadata->pBucketInfo; }
	constexpr pointer		_data() const	{ return m_box->data; }

	template <typename Arg>
	constexpr void _destroy(Arg& element)
	{
		using element_type = std::decay_t<Arg>;
		element.~element_type();
	}

	constexpr void _destruct(size_t from, size_t to)
	{
		for (size_t i = from; i < to; ++i)
		{
			// Because not every bucket is filled, we need to check if the hash is actually set.
			if (!m_box->metadata->pBucketInfo[i].is_empty())
			{
				if constexpr (std::is_same_v<key_type, value_type>)
				{
					_destroy(m_box->data[i]);
				}
				else
				{
					_destroy(m_box->data[i].first);
					_destroy(m_box->data[i].second);
				}
				m_box->data[i].~type();
			}
			m_box->metadata->pBucketInfo[i].reset();
		}
	}

	constexpr void _try_grow_on_next_insert()
	{
		if (_grow_on_next_insert())
		{
			_reserve(growth_policy::new_capacity(m_capacity));
			_toggle_grow_on_next_insert();
		}
	}	

	constexpr void _reserve(size_t capacity)
	{
		using byte_allocator = typename std::allocator_traits<allocator_type>::template rebind_alloc<std::byte>;

		byte_allocator byteAllocator{ m_box.resource() };

		size_t const metadataCapacity = sizeof(metadata) + (sizeof(bucket_info) * capacity);
		std::byte* pBuffer = std::allocator_traits<byte_allocator>::allocate(byteAllocator, metadataCapacity);

		metadata* metadataOld = m_box->metadata;
		type* dataOld = m_box->data;

		m_box->metadata = reinterpret_cast<metadata*>(pBuffer);
		m_box->metadata->growOnNextInsert = false;

		m_box->metadata->pBucketInfo = reinterpret_cast<bucket_info*>(pBuffer + sizeof(metadata));

		m_box->data = std::allocator_traits<allocator_type>::allocate(m_box, capacity);

		for (size_t i = 0; i < capacity; ++i)
		{
			new (m_box->metadata->pBucketInfo + i) bucket_info{};
		}

		// Capacity needs to be set before the container rehash.
		size_t const capacityOld = m_capacity;
		m_capacity = capacity;

		// Rehash container when buckets are occupied.
		if (size())
		{
			// hash container's length needs to be reassigned if the container is downsized.
			if (size() > m_capacity)
			{
				m_len = m_capacity;
			}

			_rehash(dataOld, metadataOld->pBucketInfo, size());

			// Destruct old elements that were not moved.
			for (size_t i = 0; i < capacityOld; ++i)
			{
				dataOld[i].~type();
			}

			if (metadataOld != nullptr)
			{
				std::allocator_traits<byte_allocator>::deallocate(byteAllocator, reinterpret_cast<std::byte*>(metadataOld), sizeof(metadata) * (sizeof(bucket_info) * capacityOld));
			}

			if (dataOld != nullptr)
			{
				std::allocator_traits<allocator_type>::deallocate(m_box, dataOld, capacityOld);
			}
		}
	}

		
	constexpr bucket_value_type _get_impl(key_type const& key) const
	{
		bucket_value_type bucket = invalid_bucket_v;
		if (m_capacity != 0)
		{
			std::optional res = _find_key(key);
			if (res.has_value())
			{
				bucket = res.value();
			}
		}
		return bucket;
	}

	template <typename... Args>
	constexpr bucket_value_type _emplace_internal(Args&&... arguments)
	{
		// Technically, now that we've added load factor in, we will never run out of empty buckets in the container.
		if (!_num_free_buckets() || 
			load_factor() > max_load_factor())
		{
			_reserve(growth_policy::new_capacity(m_capacity));
		}
		else
		{
			_try_grow_on_next_insert();
		}
		++m_len;

		return _emplace_impl(std::forward<Args>(arguments)...);
	}

	// Implementation of remove method.
	// Backward shifting deletion.
	constexpr bool _remove_impl(key_type const& key)
	{
		std::optional res = _find_key(key);

		if (!res.has_value())
		{
			return false;
		}

		bucket_value_type bucket = res.value();

		_destruct(bucket, bucket + 1);
		// Move the bucket to the next one so the object can be shifted down.
		bucket = _next_bucket(bucket);

		bucket_info* p_info = _info();

		while (p_info[bucket].psl != 0 &&
			!p_info[bucket].is_empty())
		{
			_back_shift_bucket(_previous_bucket(bucket), bucket);
			bucket = _next_bucket(bucket);
		}
		--m_len;

		return true;
	}

private:

	constexpr auto _deep_copy(hash_container_base const& other) -> void
	{
		if (m_capacity < other.m_capacity)
		{
			_reserve(other.m_capacity);
		}

		m_len = other.m_len;

		size_t count = m_len;

		bucket_info* bucketInfo		= m_box->metadata->pBucketInfo;
		bucket_info* rhsBucketInfo	= other.m_box->metadata->pBucketInfo;

		for (size_t i = 0; i < m_capacity; ++i)
		{
			if (rhsBucketInfo[i].is_empty())
			{
				continue;
			}
			bucketInfo[i] = rhsBucketInfo[i];
			reinterpret_cast<mutable_type&>(m_box->data[i]) = other.m_box->data[i];

			--count;
		}
	}

	// Helper function to swap contents.
	constexpr void _swap_element_at(bucket_value_type bucket, mutable_type& element, bucket_info& info)
	{
		// Swap actual data.
		// We do a memcopy to avoid weird funny logic that users might implement for their move assignment operator
		if constexpr (std::is_standard_layout_v<type> && std::is_trivially_copyable_v<type>)
		{
			mutable_type tmp{};
			mutable_type& data = *reinterpret_cast<mutable_type*>(&m_box->data[bucket]);

			std::memcpy(&tmp, &data, sizeof(mutable_type));
			new (&data) mutable_type{ std::move(element.first), std::move(element.second) };
			std::memcpy(&element, &tmp, sizeof(mutable_type));
		}
		else
		{
			if constexpr (std::is_same_v<key_type, value_type>)
			{
				type& data = *const_cast<type*>(&m_box->data[bucket]);
				mutable_type tmp{ std::move(data) };

				new (&data) type{ std::move(element) };
				new (&element) type{ std::move(tmp) };
			}
			else
			{
				key_type& key = *const_cast<key_type*>(&m_box->data[bucket].first);
				value_type& value = m_box->data[bucket].second;

				mutable_type tmp{ std::move(key), std::move(value) };

				key = std::move(element.first);
				value = std::move(element.second);

				new (&element) type{ std::move(tmp.first), std::move(tmp.second) };
			}
		}

		bucket_info& dst = m_box->metadata->pBucketInfo[bucket];

		uint32 psl = dst.psl;
		// Swap the contents of the bucket info.
		dst.hash = info.hash;
		dst.psl = info.psl;
		// Update the current bucket's psl with the one that's being swapped out.
		info.psl = psl;
	}

	// Implementation of emplace method.
	// For a map, arguments NEEDS to be a std::pair or Pair struct with a key() method specified.
	template <typename... Args>
	constexpr bucket_value_type _emplace_impl(Args&&... args)
	{
		// Construct the to-be-inserted element once.
		mutable_type element{ std::forward<Args>(args)... };

		hash_value_type const hash = _hashify(traits::extract_key(element));
		bucket_info* p_info = _info();
		bucket_value_type bucket = _bucket_for_hash(hash);

		bucket_info inserting_info{ .hash = hash };
		bucket_value_type first_swapped_bucket = std::numeric_limits<bucket_value_type>::max();

		while (!p_info[bucket].is_empty())
		{
			if (inserting_info.psl > p_info[bucket].psl)
			{
				// Number of probes is really high, rehash container on next insert.
				if (inserting_info.psl > probe_distance_limit::value)
				{
					_toggle_grow_on_next_insert();
				}
				if (first_swapped_bucket == std::numeric_limits<bucket_value_type>::max())
				{
					first_swapped_bucket = bucket;
				}
				_swap_element_at(bucket, element, inserting_info);
			}
			bucket = _next_bucket(bucket);
			++inserting_info.psl;
		}

		// Store the element.
		if constexpr (std::is_same_v<key_type, value_type>)
		{
			new (m_box->data + bucket) type{ std::move(element) };
		}
		else
		{
			new (m_box->data + bucket) type{ std::move(element.first), std::move(element.second) };
		}

		p_info[bucket].hash = inserting_info.hash;
		p_info[bucket].psl	= inserting_info.psl;

		return (first_swapped_bucket != std::numeric_limits<bucket_value_type>::max()) ? first_swapped_bucket : bucket;
	}

	constexpr void _rehash(type* sourceData, bucket_info* sourceInfo, size_t count)
	{
		for (size_t i = 0; count != 0 && i < m_capacity; ++i)
		{
			// hash value of 0 means the bucket is empty.
			if (sourceInfo[i].is_empty()) 
			{ 
				continue; 
			}
			_emplace_impl(std::move(sourceData[i]));
			--count;
		}
	}

	constexpr bucket_value_type _previous_bucket(bucket_value_type current) const
	{
		bucket_value_type bucket = current - 1;
		if (bucket == -1)
		{
			bucket = m_capacity - 1;
		}
		return bucket % m_capacity;
	}


	constexpr bucket_value_type _next_bucket(bucket_value_type current) const
	{
		return (current + 1) % m_capacity;
	}


	constexpr std::optional<bucket_value_type> _find_key(key_type const& key) const
	{
		const hash_value_type hash = _hashify(key);

		bucket_value_type bucket = _bucket_for_hash(hash);
		size_t probeSequence = 0;

		bucket_info* p_info = _info();

		while (probeSequence <= p_info[bucket].psl &&
			!p_info[bucket].is_empty())
		{
			if (traits::compare_keys(key, traits::extract_key(m_box->data[bucket])))
			{
				return std::make_optional(bucket);
			}
			bucket = _next_bucket(bucket);
			++probeSequence;
		}
		return std::nullopt;
	}

	constexpr void _back_shift_bucket(size_t to, size_t from)
	{
		bucket_info* p_info = _info();
		p_info[to] = std::move(p_info[from]);
		std::memcpy(&m_box->data[to], &m_box->data[from], sizeof(type));

		// Reduce the bucket's psl by 1.
		--p_info[to].psl;
		_destruct(from, from + 1);
	}

	constexpr size_t _num_free_buckets() const
	{
		return m_capacity - size();
	}

	constexpr bucket_value_type _bucket_for_hash(hash_value_type hash) const
	{
		return hash % m_capacity;
	}

	constexpr uint32 _hashify(key_type const& key) const
	{
		return static_cast<uint32>(hasher{}(key));
	}

	constexpr bool _grow_on_next_insert() const
	{
		return m_box->metadata->growOnNextInsert;
	}

	constexpr void _toggle_grow_on_next_insert()
	{
		m_box->metadata->growOnNextInsert ^= true;
	}
};

}

#endif // !LIB_HPPASH_HPP
