#pragma once
#ifndef LIB_HASH_H
#define LIB_HASH_H

#include "memory.h"
#include "utility.h"

namespace lib
{

template <size_t limit>
struct probe_sequence_length_limit
{
	static constexpr size_t value = limit;
};

// TODO:
// [x] Implement min, max and normal load factor.
// [x] Figure out how to pack load factor variable without making the container exceed 40 bytes.
// [x] Fix iterators now that bucket_info is condensed to only hash & psl.
// 
// Linear probed, robin-hood hashing container with backward shifting deletion.
template <typename traits, typename hasher, std::derived_from<container_growth_policy> growth_policy, provides_memory provided_allocator = default_allocator, typename probe_distance_limit = probe_sequence_length_limit<4096>>
class hash_container_base : 
	private traits,
	private hasher,
	private growth_policy,
	protected probe_distance_limit,
	protected std::conditional_t<std::is_same_v<provided_allocator, default_allocator>, system_memory_interface, allocator_memory_interface<provided_allocator>>
{
protected:

	using hash_value_type		= uint32;
	using bucket_value_type		= size_t;
	using key_type				= typename traits::key_type;
	using value_type			= typename traits::value_type;
	using type					= typename traits::type;
	using const_type			= type const;
	using pointer				= type*;
	using const_pointer			= type const*;
	using interface_type		= std::conditional_t<std::is_same_v<key_type, value_type>, const_type, type>;
	using allocator_interface	= std::conditional_t<std::is_same_v<provided_allocator, default_allocator>, system_memory_interface, allocator_memory_interface<provided_allocator>>;
	using allocator_type		= typename allocator_interface::allocator_type;

	static constexpr bucket_value_type invalid_bucket_v = std::numeric_limits<bucket_value_type>::max();

	friend class hash_container_const_iterator;

	constexpr allocator_type* get_allocator() const requires !std::same_as<allocator_type, default_allocator>
	{
		return this->get_allocator();
	}

	struct bucket_info
	{
		hash_value_type	hash;		// Hash value for keys are cached to save computation time.
		uint32			psl;		// The value of each bucket's probe sequence length has to be cached for backshift deletion to work.

		constexpr void reset() { hash = psl = 0; }

		constexpr bool is_empty() const { return hash == 0; }
	};

	struct metadata
	{
		bool			grow_on_next_insert;
		bucket_info*	p_bucket_info;
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
			// This logic is here to just skip empty buckets.
			bucket_info* end = hash->m_metadata->p_bucket_info + hash->m_capacity;

			while (info->is_empty() &&
				   info != end)
			{
				++info;
			}
		}

		hash_container_const_iterator(hash_container_const_iterator const&)				= default;
		hash_container_const_iterator(hash_container_const_iterator&&)					= default;
		hash_container_const_iterator& operator=(hash_container_const_iterator const&)	= default;
		hash_container_const_iterator& operator=(hash_container_const_iterator&&)		= default;

		constexpr bool operator== (hash_container_const_iterator const& rhs) const
		{
			return  info == rhs.info;
		}

		constexpr bool operator!= (hash_container_const_iterator const& rhs) const
		{
			return info != rhs.info;
		}

		constexpr reference	operator* () const { return hash->data()[bucket()]; }
		constexpr pointer	operator->() const { return &hash->data()[bucket()]; }

		constexpr bucket_value_type bucket() const { return static_cast<bucket_value_type>(info - hash->m_metadata->p_bucket_info) ;}

		constexpr hash_container_const_iterator& operator++()
		{
			while (true)
			{
				++info;
				// This checks if the current bucket is the final one in the hash container.
				if (info == (hash->info() + hash->m_capacity))
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

		constexpr reference	operator* () const { return this->hash->data()[this->bucket()]; }
		constexpr pointer	operator->() const { return &this->hash->data()[this->bucket()]; }
	};

	using iterator			= hash_container_iterator;
	using const_iterator	= hash_container_const_iterator;

	constexpr hash_container_base() requires std::same_as<allocator_type, default_allocator> :
		m_metadata{}, m_data{}, m_len{}, m_capacity{}, m_max_load_factor{ 0.75f }
	{}

	constexpr hash_container_base(allocator_type& allocator) requires !std::same_as<allocator_type, default_allocator> :
		m_metadata{}, m_data{}, m_len{}, m_capacity{}, m_max_load_factor{ 0.75f }, allocator_interface{ &allocator }
	{}

	constexpr ~hash_container_base() { release(); }

	constexpr hash_container_base(size_t length) requires std::same_as<allocator_type, default_allocator> :
		hash_container_base{}
	{
		reserve(length);
	}

	constexpr hash_container_base(allocator_type& allocator, size_t length) requires !std::same_as<allocator_type, default_allocator> :
		hash_container_base{ allocator }
	{
		reserve(length);
	}

	constexpr hash_container_base(hash_container_base const& rhs) requires std::same_as<allocator_type, default_allocator> :
		m_metadata{}, m_data{}, m_len{}, m_capacity{}, m_max_load_factor{ 0.75f }
	{
		*this = rhs;
	}

	constexpr hash_container_base(hash_container_base const& rhs) requires !std::same_as<allocator_type, default_allocator> :
		m_metadata{}, m_data{}, m_len{}, m_capacity{}, m_max_load_factor{ 0.75f }, allocator_interface{rhs.allocator()}
	{
		*this = rhs;
	}

	constexpr hash_container_base(hash_container_base&& rhs) :
		m_metadata{}, m_data{}, m_len{}, m_capacity{}
	{
		*this = std::move(rhs);
	}

	constexpr hash_container_base(std::initializer_list<type> const& initializer) requires std::same_as<allocator_type, default_allocator> :
		hash_container_base{}
	{
		for (type const& pair : initializer)
		{
			emplace_internal(pair);
		}
	}

	constexpr hash_container_base(allocator_type& allocator, std::initializer_list<type> const& initializer) requires !std::same_as<allocator_type, default_allocator> :
	hash_container_base{ allocator }
	{
		for (type const& pair : initializer)
		{
			emplace_internal(pair);
		}
	}

	hash_container_base& operator=(hash_container_base const& rhs)
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

			bucket_info* this_bucket_info = m_metadata->p_bucket_info;
			bucket_info* rhs_bucket_info = rhs.m_metadata->p_bucket_info;

			for (size_t i = 0; i < m_capacity; ++i)
			{
				if (rhs_bucket_info[i].is_empty())
				{
					continue;
				}
				this_bucket_info[i] = rhs_bucket_info[i];
				reinterpret_cast<typename traits::mutable_type&>(m_data[i]) = rhs.m_data[i];

				--count;
			}
		}
		return *this;
	}

	hash_container_base& operator=(hash_container_base&& rhs)
	{
		if (this != &rhs)
		{
			bool move = true;
			release();

			if constexpr (!std::is_same_v<allocator_type, default_allocator>)
			{
				if (get_allocator() != rhs.get_allocator())
				{
					reserve(rhs.size());

					m_len = rhs.m_len;
					m_max_load_factor = rhs.m_max_load_factor;

					bucket_info* this_bucket_info = m_metadata->p_bucket_info;
					bucket_info* rhs_bucket_info = rhs.m_metadata->p_bucket_info;

					for (size_t i = 0; i < rhs.m_capacity; ++i)
					{
						if (rhs.m_info[i].is_empty())
						{
							continue;
						}
						this_bucket_info[i] = std::move(rhs_bucket_info[i]);
						m_data[i] = std::move(rhs.m_data[i]);
					}

					rhs.release();
					move = false;
				}
			}

			if (move)
			{
				m_metadata	= rhs.m_metadata;
				m_data	= rhs.m_data;
				m_len	= rhs.m_len;
				m_capacity = rhs.m_capacity;
				m_max_load_factor = rhs.m_max_load_factor;
			}

			if constexpr (!std::is_same_v<allocator_type, default_allocator>)
			{
				new (&rhs) hash_container_base{ *rhs.get_allocator() };
			}
			else
			{
				new (&rhs) hash_container_base{};
			}
		}
		return *this;
	}

	constexpr void release()
	{
		destruct(0, capacity());

		if (m_capacity)
		{
			this->free_storage(m_metadata);
			this->free_storage(m_data);
		}

		m_metadata = nullptr;
		m_data = nullptr;
		m_len = m_capacity = 0;
		m_max_load_factor = .75f;
	}

	constexpr size_t	size		() const { return m_len; }
	constexpr size_t	capacity	() const { return m_capacity; }
	constexpr bool		empty		() const { return m_len == 0; }
	constexpr float32	load_factor	() const { return static_cast<float32>(m_len) / static_cast<float32>(m_capacity); }
	constexpr float32	max_load_factor() const { return m_max_load_factor; }
	constexpr void		max_load_factor(float32 factor) 
	{
		if (factor > 0.f && factor < 1.f)
		{
			m_max_load_factor = factor; 
		}
	}

	iterator		begin	()		 { return iterator{ m_metadata->p_bucket_info, this }; }
	iterator		end		()		 { return iterator{ m_metadata->p_bucket_info + m_capacity, this }; }
	const_iterator	begin	() const { return const_iterator{ m_metadata->p_bucket_info, this }; }
	const_iterator	end		() const { return const_iterator{ m_metadata->p_bucket_info + m_capacity, this }; }
	const_iterator	cbegin	() const { return const_iterator{ m_metadata->p_bucket_info, this }; }
	const_iterator	cend	() const { return const_iterator{ m_metadata->p_bucket_info + m_capacity, this }; }

protected:

	metadata* m_metadata;
	type*	m_data;
	size_t	m_len;
	size_t	m_capacity;
	float32 m_max_load_factor;

	constexpr bucket_info*	info() const	{ return m_metadata->p_bucket_info; }
	constexpr pointer		data() const	{ return m_data; }

	template <typename Arg>
	constexpr void destroy(Arg& element)
	{
		using element_type = std::decay_t<Arg>;
		if constexpr (std::is_trivially_destructible_v<element_type>)
		{
			memzero(const_cast<element_type*>(&element), sizeof(element_type));
		}
		else
		{
			element.~element_type();
		}
	}

	constexpr void destruct(size_t from, size_t to)
	{
		for (size_t i = from; i < to; ++i)
		{
			// Because not every bucket is filled, we need to check if the hash is actually set.
			if (!m_metadata->p_bucket_info[i].is_empty())
			{
				if constexpr (std::is_same_v<key_type, value_type>)
				{
					destroy(m_data[i]);
				}
				else
				{
					destroy(m_data[i].first);
					destroy(m_data[i].second);
				}
				m_data[i].~type();
			}
			m_metadata->p_bucket_info[i].reset();
		}
	}

	constexpr void try_grow_on_next_insert()
	{
		if (grow_on_next_insert())
		{
			reserve(growth_policy::new_capacity(m_capacity));
			toggle_grow_on_next_insert();
		}
	}	

	constexpr void reserve(size_t capacity)
	{
		//ASSERTION(
		//	size() < capacity && 
		//	"You are attempting to shrink the container when it has more elements than the specified capacity"
		//);

		metadata* metadataOld = m_metadata;
		type* dataOld = m_data;

		size_t metadataCapacity = sizeof(metadata) + (sizeof(bucket_info) * capacity);
		uint8* pBuffer = static_cast<uint8*>(this->allocate_storage({ .size = metadataCapacity }));

		m_metadata = reinterpret_cast<metadata*>(pBuffer);
		m_metadata->grow_on_next_insert = false;

		m_metadata->p_bucket_info = reinterpret_cast<bucket_info*>(pBuffer + sizeof(metadata));

		m_data = this->allocate_storage<type>({ .size = capacity });
		// This is not standard compliant and is by design.
		// Doing this enables us to store types that does not contain a default constructor.
		memzero(m_data, sizeof(type) * capacity);

		for (size_t i = 0; i < capacity; ++i)
		{
			new (m_metadata->p_bucket_info + i) bucket_info{};
		}

		// Capacity needs to be set before the container rehash.
		size_t capacityOld = m_capacity;
		m_capacity = capacity;

		// Rehash container when buckets are occupied.
		if (size())
		{
			// hash container's length needs to be reassigned if the container is downsized.
			if (size() > m_capacity)
			{
				m_len = m_capacity;
			}

			rehash(dataOld, metadataOld->p_bucket_info, size());

			// Destruct old elements that were not moved.
			for (size_t i = 0; i < capacityOld; ++i)
			{
				dataOld[i].~type();
			}
			this->free_storage(metadataOld);
			this->free_storage(dataOld);
		}
	}

		
	constexpr bucket_value_type get_impl(key_type const& key) const
	{
		bucket_value_type bucket = invalid_bucket_v;
		if (m_capacity != 0)
		{
			std::optional res = find_key(key);
			if (res.has_value())
			{
				bucket = res.value();
			}
		}
		return bucket;
	}

	template <typename... Args>
	constexpr bucket_value_type emplace_internal(Args&&... arguments)
	{
		// Technically, now that we've added load factor in, we will never run out of empty buckets in the container.
		if (!num_free_buckets() || 
			load_factor() > max_load_factor())
		{
			reserve(growth_policy::new_capacity(m_capacity));
		}
		else
		{
			try_grow_on_next_insert();
		}
		++m_len;

		return emplace_impl(std::forward<Args>(arguments)...);
	}

	// Implementation of remove method.
	// Backward shifting deletion.
	constexpr bool remove_impl(key_type const& key)
	{
		std::optional res = find_key(key);

		if (!res.has_value())
		{
			return false;
		}

		bucket_value_type bucket = res.value();

		destruct(bucket, bucket + 1);
		// Move the bucket to the next one so the object can be shifted down.
		bucket = next_bucket(bucket);

		bucket_info* p_info = info();

		while (p_info[bucket].psl != 0 &&
			!p_info[bucket].is_empty())
		{
			back_shift_bucket(previous_bucket(bucket), bucket);
			bucket = next_bucket(bucket);
		}
		--m_len;

		return true;
	}

private:

	// Helper function to swap contents.
	constexpr void swap_element_at(bucket_value_type bucket, type& element, bucket_info& info)
	{
		// Swap actual data.
		// We do a memcopy to avoid weird funny logic that users might implement for their move assignment operator
		type tmp{};
		memcopy(&tmp, &m_data[bucket], sizeof(type));
		new (m_data + bucket) type{ std::move(element) };
		memcopy(&element, &tmp, sizeof(type));

		bucket_info& dst = m_metadata->p_bucket_info[bucket];

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
	constexpr bucket_value_type emplace_impl(Args&&... args)
	{
		// Construct the to-be-inserted element once.
		type element{ std::forward<Args>(args)... };

		hash_value_type const hash = hashify(traits::extract_key(element));
		bucket_info* p_info = info();
		bucket_value_type bucket = bucket_for_hash(hash);

		bucket_info inserting_info{ .hash = hash };

		while (!p_info[bucket].is_empty())
		{
			if (inserting_info.psl > p_info[bucket].psl)
			{
				// Number of probes is really high, rehash container on next insert.
				if (inserting_info.psl > probe_distance_limit::value)
				{
					toggle_grow_on_next_insert();
				}
				swap_element_at(bucket, element, inserting_info);
			}
			bucket = next_bucket(bucket);
			++inserting_info.psl;
		}
		// Store the element.
		new (m_data + bucket) type{ std::move(element) };

		p_info[bucket].hash = inserting_info.hash;
		p_info[bucket].psl	= inserting_info.psl;

		return bucket;
	}

	constexpr void rehash(type* sourceData, bucket_info* sourceInfo, size_t count)
	{
		for (size_t i = 0; count != 0 && i < m_capacity; ++i)
		{
			// hash value of 0 means the bucket is empty.
			if (sourceInfo[i].is_empty()) 
			{ 
				continue; 
			}
			emplace_impl(std::move(sourceData[i]));
			--count;
		}
	}

	constexpr bucket_value_type previous_bucket(bucket_value_type current) const
	{
		bucket_value_type bucket = current - 1;
		if (bucket == -1)
		{
			bucket = m_capacity - 1;
		}
		return bucket % m_capacity;
	}


	constexpr bucket_value_type next_bucket(bucket_value_type current) const
	{
		return (current + 1) % m_capacity;
	}


	constexpr std::optional<bucket_value_type> find_key(key_type const& key) const
	{
		const hash_value_type hash = hashify(key);

		bucket_value_type bucket = bucket_for_hash(hash);
		size_t probeSequence = 0;

		bucket_info* p_info = info();

		while (probeSequence <= p_info[bucket].psl &&
			!p_info[bucket].is_empty())
		{
			if (traits::compare_keys(key, traits::extract_key(m_data[bucket])))
			{
				return std::make_optional(bucket);
			}
			bucket = next_bucket(bucket);
			++probeSequence;
		}
		return std::nullopt;
	}

	constexpr void back_shift_bucket(size_t to, size_t from)
	{
		bucket_info* p_info = info();
		p_info[to] = std::move(p_info[from]);
		memmove(&m_data[to], &m_data[from], sizeof(type));

		// Reduce the bucket's psl by 1.
		--p_info[to].psl;
		destruct(from, from + 1);
	}

	constexpr size_t num_free_buckets() const
	{
		return m_capacity - size();
	}

	constexpr bucket_value_type bucket_for_hash(hash_value_type hash) const
	{
		return hash % m_capacity;
	}

	constexpr uint32 hashify(key_type const& key) const
	{
		return static_cast<uint32>(hasher{}(key));
	}

	constexpr bool grow_on_next_insert() const
	{
		return m_metadata->grow_on_next_insert;
	}

	constexpr void toggle_grow_on_next_insert()
	{
		m_metadata->grow_on_next_insert ^= true;
	}
};

}

#endif // !LIB_HASH_H
