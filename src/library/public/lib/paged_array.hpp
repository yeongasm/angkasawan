#pragma once
#ifndef LIB_PAGED_ARRAY_HPP
#define LIB_PAGED_ARRAY_HPP

#include <span>
#include <array>
#include "type.hpp"
#include "array.hpp"

namespace lib
{
template <typename T, uint16 PAGE_SIZE>
class paged_array
{
public:
	using element_type = T;

	struct page_buffer
	{
		using data_buffer	 = std::array<element_type, PAGE_SIZE>;
		using version_buffer = std::array<std::atomic_uint32_t, PAGE_SIZE>;

		data_buffer		buffer;
		version_buffer	version;
		size_t			tail; // This marks the current end index of the buffer above.
	};

	using element_page = std::unique_ptr<page_buffer>;

	/**
	* index can no longer be 4 bytes. The slot version count needs to be included with the index.
	*/
	struct index
	{
		uint16 page;
		uint16 offset;
		uint32 version;

		explicit operator uint64() const
		{
			return std::bit_cast<uint64>(*this);
		}

		[[nodiscard]] auto to_uint64() const -> uint64
		{
			return std::bit_cast<uint64>(*this);
		}

		[[nodiscard]] static auto from(uint64 value) -> index
		{
			return std::bit_cast<index>(value);
		}
	};

	paged_array() = default;

	~paged_array() { release(); }

	constexpr auto operator[](index idx) const -> element_type&
	{
		size_t const page = static_cast<size_t>(idx.page);
		size_t const offset = static_cast<size_t>(idx.offset);

		ASSERTION(page < m_pages.size(), "Page in index exceeded page count of the container.");
		ASSERTION(offset < m_pages[page]->buffer.size(), "Offset in index exceeded buffer capacity of the page.");

		[[maybe_unused]] uint32 const ver = m_pages[page]->version[offset].load(std::memory_order_acquire);

		ASSERTION(idx.version == ver, "Version do not match! Data retrieved is faulty.");

		return m_pages[page]->buffer[offset];
	}

	constexpr auto at(index idx) const -> element_type*
	{
		size_t const page = static_cast<size_t>(idx.page);
		size_t const offset = static_cast<size_t>(idx.offset);

		if (page < m_pages.size() &&
			offset < m_pages[page]->buffer.size() &&
			idx.version == m_pages[page]->version[offset].load(std::memory_order_acquire))
		{
			return &m_pages[page]->buffer[offset];
		}
		return nullptr;
	}

	template <typename... Args>
	std::pair<index, element_type&> emplace(Args&&... args)
	{
		auto result = get_slot();
		new (&result.second) element_type{ std::forward<Args>(args)... };
		return result;
	}

	std::pair<index, element_type&> insert(element_type const& ele)
	{
		return emplace(ele);
	}

	std::pair<index, element_type&> insert(element_type&& ele)
	{
		return emplace(std::move(ele));
	}

	//std::pair<index, element_type&> request()
	//{
	//	return get_slot();
	//}

	void erase(index idx)
	{
		size_t const page = static_cast<size_t>(idx.page);
		size_t const offset = static_cast<size_t>(idx.offset);

		ASSERTION(page < m_pages.size(), "Page in index exceeded page count of the container.");
		if (page < m_pages.size())
		{
			auto& buffer = m_pages[page]->buffer;

			ASSERTION(offset < buffer.size(), "Offset in index exceeded buffer capacity of the page.");

			if (offset < buffer.size())
			{
				std::atomic_uint32_t& versionCounter = m_pages[page]->version[offset];

				if (idx.version == versionCounter.load(std::memory_order_acquire))
				{
					buffer[offset].~element_type();
					m_indices.push_back(idx);

					versionCounter.fetch_add(1, std::memory_order_relaxed);
				}
			}
		}
	}

	//size_t page_count() const
	//{
	//	return m_current;
	//}

	//index index_of(element_type const* ele) const
	//{
	//	size_t const num_pages = m_current + 1;
	//	for (size_t i = 0; i < num_pages; ++i)
	//	{
	//		page_buffer const& currentPage = *m_pages[i].get();
	//		if (ele >= currentPage.buffer.data() && 
	//			ele < currentPage.buffer.data() + currentPage.buffer.size())
	//		{
	//			size_t offset = ele - currentPage.buffer.data();
	//			uint32 const ver = currentPage.version[offset].load(std::memory_order_relaxed);

	//			return index{ .page = static_cast<uint16>(i), .offset = static_cast<uint16>(offset), .version = ver };
	//		}
	//	}
	//	return index::from(std::numeric_limits<uint32>::max());
	//}

	//index index_of(element_type const& ele) const
	//{
	//	return index_of(&ele);
	//}

	/**
	* Clears the content of a page that conincides with the supplied index.
	*/
	auto clear(size_t pidx) -> void
	{
		ASSERTION(pidx < m_pages.size(), "Page index exceeded number of pages available in the pool");
		if (pidx < m_pages.size())
		{
			element_page& page = m_pages[pidx];

			for (size_t i = 0; element_type& ele : page->buffer)
			{
				if (i >= page->tail)
				{
					break;
				}
				ele.~element_type();

				uint32 const val = page->version[i].load(std::memory_order_acquire);
				page->version[i].compare_exchange_strong(val, 0, std::memory_order_relaxed);

				++i;
			}

			page->tail = 0;

			auto it = std::remove_if(
				m_indices.begin(),
				m_indices.end(),
				[pidx](index const& idx) -> bool
				{
					return pidx == static_cast<size_t>(idx.page);
				}
			);

			m_indices.erase(it, m_indices.end());
		}
	}

	/**
	* Clears the contents of the container.
	*/
	auto clear() -> void
	{
		for (element_page& page : m_pages)
		{
			for (size_t i = 0; element_type& ele : page->buffer)
			{
				if (i >= page->tail)
				{
					break;
				}

				ele.~element_type();
			}

			page->tail = 0;
		}

		m_indices.clear();
	}

	auto release() -> void
	{
		clear();
		
		m_pages.clear();
	}

private:
	array<element_page> m_pages		= {};
	array<index>		m_indices	= {};
	size_t				m_current	= {};

	std::pair<index, element_type&> get_slot()
	{
		if (m_current == m_pages.capacity() ||
			m_pages[m_current]->tail == m_pages[m_current]->buffer.size())
		{
			auto it		= m_pages.emplace(m_pages.end(), std::make_unique<page_buffer>());
			m_current	= std::distance(m_pages.begin(), it);
		}

		element_type* element = nullptr;
		index idx = {};

		if (m_indices.size())
		{
			idx = m_indices.back();

			size_t const page	= static_cast<size_t>(idx.page);
			size_t const offset = static_cast<size_t>(idx.offset);

			m_indices.pop();

			element		= &m_pages[page]->buffer[offset];
			idx.version = m_pages[page]->version[offset].load(std::memory_order_acquire);
		}
		else
		{
			ref pg = m_pages[m_current];

			idx.page	= static_cast<uint16>(m_current);
			idx.offset	= static_cast<uint16>(pg->tail);
			idx.version = pg->version[pg->tail].load(std::memory_order_acquire);

			element = &pg->buffer[pg->tail++];
		}

		return std::pair<index, element_type&>{ idx, *element };
	}
};

template <size_t N>
struct pool_page_size
{
	static constexpr size_t value = N;
};

template <
	typename AllocatorType, 
	typename PoolPageSize,
	typename SizeType = type<size_t>
>
struct pool_config
{
	type<AllocatorType> allocator;
	type<PoolPageSize> pool_size;
	type<SizeType> size_type = {};
};

template <typename T, pool_config config = { .allocator = type_v<allocator<T>>, .pool_size = type_v<pool_page_size<4_KiB / sizeof(T)>> }>
class pool
{
public:
	struct pool_const_iterator
	{
		using value_type 		= T;
		using difference_type 	= ptrdiff_t;
		using pointer 			= value_type const*;
		using reference 		= value_type const&;

		pointer m_data;
	};

	struct pool_iterator : public pool_const_iterator
	{
		using super = pool_const_iterator;

		using typename super::value_type;
		using typename super::difference_type;
		using pointer = value_type*;
		using reference = value_type&;
	};

	using allocator_type 	= typename decltype(config.allocator)::type;
	using size_type			= typename decltype(config.size_type)::type;
	using difference_type 	= ptrdiff_t;
	using value_type 		= T;
	using pointer 			= value_type*;
	using const_pointer 	= value_type const*;
	using reference 		= value_type&;
	using const_reference 	= value_type const&;
	using iterator 			= pool_iterator;
	using const_iterator 	= pool_const_iterator;

	constexpr pool() = default;

	constexpr ~pool()
	{
		_release();
	}

	// TODO(afiq):
	// Figure out copy construction & copy assignment.
	// Copy should be similar to an unordered map, the "structure" need not be the same.

	pool(pool&& rhs) :
		m_allocator{ std::exchange(rhs.m_allocator, {}) },
		m_begin{ std::exchange(rhs.m_begin, {}) },
		m_end{ std::exchange(rhs.m_end, {}) },
		m_pFreelist( std::exchange(rhs.m_pFreelist, {}) ),
		m_freelistTail( std::exchange(rhs.m_freelistTail, {}) )
	{}

	auto operator=(pool&& rhs) -> pool&
	{
		if (this != &rhs)
		{
			_release();

			m_allocator = std::exchange(rhs.m_allocator, {});
			m_begin = std::exchange(rhs.m_begin, {});
			m_end = std::exchange(rhs.m_end, {});
			m_pFreelist = std::exchange(rhs.m_pFreelist, {});
			m_freelistTail = std::exchange(rhs.m_freelistTail, {});

			// No placement new is needed to be done on rhs as it is already in an valid but unspecified state.
		}
		return *this;
	}

	template <typename... Args>
	constexpr auto emplace(Args&&... args) -> iterator
	{
		if (m_pFreelist == nullptr)
		{
			_request_page();
		}

		return _try_emplace_at(m_pFreelist, std::forward<Args>(args)...);
	};

	// A problem will arise when we implement shrink to fit.
	// How do we know that a page has nothing stored and can be removed?
	// Perhaps we can have an element count variable in the page itself that tells us how many objects are being stored in that page.
	// But that would mean we need to store the page pointer inside of the iterator itself.
	// Seems necessary because we don't have a way to tell us which page is the data pointer in the iterator from (if we're strictly avoiding pointer math).

private:

	using pool_page_size_type = typename decltype(config.page_size)::type;

	/*
	* Points to the next available index.
	*/
	struct free_node
	{
		free_node* next;
	};

	struct alignas(alignof(value_type)) storage
	{
		using stored_type = std::conditional_t<sizeof(free_node) < sizeof(value_type), value_type, free_node>;

		std::byte data[sizeof(stored_type)];
	};

	struct page
	{
		storage* buffer;
		free_node* nextFreeNode;
		page* previous;
		page* next;
		size_t size;
	};

	using storage_allocator_type 	= typename std::allocator_traits<allocator_type>::template rebind_alloc<storage>;
	using page_allocator_type 		= typename std::allocator_traits<allocator_type>::template rebind_alloc<page>;

	using page_pointer 			= page*;
	using free_node_pointer 	= free_node*;

	allocator_type m_allocator;
	page_pointer m_pageHead;	// This is the beginning of the page linked-list
	page_pointer m_pageTail;	// This is the end of the page linked-list
	free_node_pointer m_freelistHead;
	free_node_pointer m_freelistTail;

	template <typename... Args>
	constexpr auto _try_emplace_at(free_node_pointer& ptr, Args&&... args) -> iterator
	{
		pointer data = nullptr;

		if (ptr)
		{
			auto location = ptr;
			// Move the freelist head onto the next node.
			ptr = ptr->next;
			// To avoid UB, we need to end the lifetime of the freelist_node.
			location->~free_node();

			data = new (reinterpret_cast<pointer>(location)) value_type{ std::forward<Args>(args)... };
		}

		return iterator{ data };
	}

	constexpr auto _request_page() -> void
	{				
		auto np = _allocate_page();
		
		if (!np)
		{
			return;
		}

		np->buffer = _allocate_and_construct_storage_buffer_in_page(*np);

		if (!np->buffer)
		{
			return;
		}

		_setup_begin_and_end_page(*np);

		_setup_page_freelist(*np);

		_setup_free_list(*np);
	}

	constexpr auto _allocate_page() -> page*
	{
		page_allocator_type allocator{ m_allocator.resource() };

		// Allocate a new page buffer. np - new page.
		auto np = std::allocator_traits<page_allocator_type>::allocate(allocator, sizeof(page));

		if (np)
		{
			// Start the lifetime of the buffer.
			std::construct_at(np);
		}

		return np;
	}

	constexpr auto _allocate_and_construct_storage_buffer_in_page(page& pg) -> storage*
	{
		storage_allocator_type allocator{ m_allocator.resource() };

		auto ptr = std::allocator_traits<storage_allocator_type>::allocate(allocator, pool_page_size_type::value * sizeof(storage));

		if (ptr)
		{
			std::memset(ptr, 0, pool_page_size_type::value * sizeof(storage));
		}

		return ptr;
	}

	constexpr auto _setup_begin_and_end_page(page& pg) -> void
	{
		// Assign the next pointer in m_end to the newly allocated page.
		if (m_end != nullptr)
		{
			m_end->next = &pg;
		}

		pg.previous = m_end;
		m_end = &pg;

		// If this is the first time the page is allocated, we need to set the m_begin page_pointer.
		if (m_begin == nullptr)
		{
			m_begin = m_end;
		}
	}

	constexpr auto _setup_page_freelist(page& pg) -> void
	{
		free_node dummyhead = {};
		free_node_pointer tail = &dummyhead;

		// Start the lifetime of each element storage in the buffer as a freelist head.
		for (auto&& storage : std::span{ pg.buffer, pool_page_size_type::value })
		{
			free_node_pointer node = reinterpret_cast<free_node_pointer>(storage.data);
			// start lifetime.
			std::construct_at(node);

			tail->next = node;
			tail = node;
		}
		// Set the freelist head in the page to the first free byte buffer in the storage.
		pg.nextFreeNode = dummyhead.next;
	}

	constexpr auto _setup_free_list(page& pg) -> void
	{
		if (m_pFreelist == nullptr)
		{
			m_pFreelist = pg.nextFreeNode;
		}

		std::span buffer{ pg.buffer, pool_page_size_type::value };

		if (m_freelistTail == nullptr)
		{
			m_freelistTail = reinterpret_cast<free_node_pointer>(&buffer.back().data);
		}
		else
		{
			m_freelistTail->next = pg.nextFreeNode;
			m_freelistTail = reinterpret_cast<free_node_pointer>(&buffer.back().data);
		}
	}

	constexpr auto _release() -> void
	{
		// TODO(afiq):
		// Implement release.
	}
};

}

#endif // !LIB_PAGED_ARRAY_HPP
