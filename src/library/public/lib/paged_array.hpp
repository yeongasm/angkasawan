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

		ASSERTION(page < m_pages.size() && "Page in index exceeded page count of the container.");
		ASSERTION(offset < m_pages[page]->buffer.size() && "Offset in index exceeded buffer capacity of the page.");

		[[maybe_unused]] uint32 const ver = m_pages[page]->version[offset].load(std::memory_order_acquire);

		ASSERTION(idx.version == ver && "Version do not match! Data retrieved is faulty.");

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

		ASSERTION(page < m_pages.size() && "Page in index exceeded page count of the container.");
		if (page < m_pages.size())
		{
			auto& buffer = m_pages[page]->buffer;

			ASSERTION(offset < buffer.size() && "Offset in index exceeded buffer capacity of the page.");

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
		ASSERTION(pidx < m_pages.size() && "Page index exceeded number of pages available in the pool");
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

#if 0
template <
	typename allocator
>
struct paged_array_config
{
	type_wrapper<allocator> allocator_type;
	uint16 page_size;
};

template <typename T, paged_array_config config = { .allocator_type = type<allocator<T>>, .page_size = 4_KiB / sizeof(std::decay_t<T>) }>
class paged_array
{
public:
	using value_type = T;
	using allocator_type = typename decltype(config.allocator_type)::type;
	using size_type = size_t;
	using difference_type = size_t;
	using reference = value_type&;
	using const_reference = value_type const&;
	using pointer = value_type*;
	using const_pointer = value_type const*;
private:
	struct empty_node
	{
		empty_node* next;
	}

	using empty_node_pointer_type = empty_node*;

	union stored_type
	{
		value_type 	object;
		empty_node 	emptynode;

		stored_type() :
			emptynode{}
		{}
	};

	struct page
	{
		stored_type* 			data;
		std::atomic_uint32_t* 	skipfield;
		std::atomic_uint32_t* 	version;
		empty_node_pointer_type freelist;
		page* 					previous;
		page* 					next;
		size_type 				size;
	};

	using page_pointer_type = page*;

	allocator_type m_allocator;
	page_pointer_type m_head, m_current;
	size_t m_size;	// size here refers to the number of pages that have been allocated.

	auto allocate_new_page() -> page_pointer_type
	{
		using page_allocator = std::allocator_traits<allocator_type>::template rebind_alloc<page>;
		page_allocator allocator{ m_allocator.resource() };

		return std::allocator_traits<page_allocator>::allocate(allocator, sizeof(page));
	}

	auto allocate_and_construct_new_page() -> page_pointer_type
	{
		using stored_type_allocator = std::allocator_traits<allocator_type>::template rebind_alloc<stored_type>;
		using atomic_uint32_allocator = std::allocator_traits<allocator_type>::template rebind_alloc<std::atomic_uint32_t>;

		auto pg = allocate_new_page();

		ASSERTION(pg && "Could not allocate new page");
		
		if (pg)
		{
			std::construct_at(pg);

			{
				stored_type_allocator allocator{ m_allocator.resource() };
				pg->data = std::allocator_traits<stored_type_allocator>::allocate(allocator, config.page_size);
			}

			{
				atomic_uint32_allocator allocator{ m_allocator.resource() };
				pg->skipfield = std::allocator_traits<atomic_uint32_allocator>::allocate(allocator, config.page_size);
				pg->version = std::allocator_traits<atomic_uint32_allocator>::allocate(allocator, config.page_size)
			}

			empty_node dummyFreelistHead;
			empty_node_pointer_type freelistTail = &dummyFreelistHead;

			for (size_t i = 0; i < config.page_size; ++i)
			{
				std::construct_at(&pg->data[i]);
				std::construct_at(&pg->skipfield[i]);
				std::construct_at(&pg->version[i]);

				freelistTail->next = &pg->data[i].emptynode;
				freelistTail = freelistTail->next;
			}

			pg->freelist = dummyFreelistHead->next;
			pg->size = 0;
		}

		return pg;
	}
};

#endif

}

#endif // !LIB_PAGED_ARRAY_HPP
