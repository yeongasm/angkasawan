#pragma once
#ifndef LIB_PAGED_ARRAY_H
#define LIB_PAGED_ARRAY_H

#include <span>
#include <array>
#include "array.h"

namespace lib
{

template <typename T, uint16 PAGE_SIZE, provides_memory allocator = default_allocator, std::derived_from<container_growth_policy> growth_policy = shift_growth_policy<4>>
class paged_array
{
public:
	using element_type = T;

	struct page_buffer
	{
		std::array<element_type, PAGE_SIZE> buffer;
		size_t tail; // This marks the current end index of the buffer above.
	};

	using element_page = unique_ptr<page_buffer>;

	struct index
	{
		uint16 page;
		uint16 offset;

		explicit operator uint32() const
		{
			return std::bit_cast<uint32>(*this);
		}

		auto to_uint32() const -> uint32
		{
			return static_cast<uint32>(*this);
		}

		static auto from(uint32 value) -> index
		{
			return std::bit_cast<index>(value);
		}

		//auto flatten() const -> size_t
		//{
		//	size_t const stride = static_cast<size_t>(page) * static_cast<size_t>(PAGE_SIZE);
		//	size_t const offset = static_cast<size_t>(offset);
		//	return stride + offset;
		//}
	};

	paged_array() = default;

	~paged_array() { release(); }

	std::span<element_type> get_page(uint16 p)
	{
		constexpr size_t pg = static_cast<size_t>(p);
		ASSERTION(pg < m_pages.size() && "Page in index exceeded page count of the container.");
		if (pg >= m_pages.size())
		{
			return std::span{};
		}
		ref page = m_pages[pg];
		return std::span{ page->buffer.data(), page->buffer.size() };
	}

	constexpr element_type& operator[](index idx) const
	{
		size_t const page = static_cast<size_t>(idx.page);
		size_t const offset = static_cast<size_t>(idx.offset);

		ASSERTION(page < m_pages.size() && "Page in index exceeded page count of the container.");
		ASSERTION(offset < m_pages[page]->buffer.size() && "Offset in index exceeded buffer capacity of the page.");

		return m_pages[page]->buffer[offset];
	}

	constexpr element_type* at(index idx) const
	{
		size_t const page = static_cast<size_t>(idx.page);
		size_t const offset = static_cast<size_t>(idx.offset);

		if (page < m_pages.size() &&
			offset < m_pages[page]->buffer.size())
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
				buffer[offset].~element_type();
				//memzero(&buffer[offset], sizeof(element_type));
				m_indices.push_back(idx);
			}
		}
	}

	size_t page_count() const
	{
		return m_current;
	}

	index index_of(element_type const* ele) const
	{
		size_t const num_pages = m_current + 1;
		for (size_t i = 0; i < num_pages; ++i)
		{
			page_buffer const& currentPage = *m_pages[i].get();
			if (ele >= currentPage.buffer.data() && 
				ele < currentPage.buffer.data() + currentPage.buffer.size())
			{
				size_t offset = ele - currentPage.buffer.data();
				return index{ .page = static_cast<uint16>(i), .offset = static_cast<uint16>(offset) };
			}
		}
		return index::from(std::numeric_limits<uint32>::max());
	}

	index index_of(element_type const& ele) const
	{
		return index_of(&ele);
	}

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
	array<element_page, allocator, growth_policy> m_pages;
	array<index, allocator, growth_policy> m_indices;
	size_t m_current = 0;

	std::pair<index, element_type&> get_slot()
	{
		if (m_current == m_pages.capacity() ||
			m_pages[m_current]->tail == m_pages[m_current]->buffer.size())
		{
			auto it = m_pages.emplace(m_pages.end(), make_unique<page_buffer>());
			m_current = std::distance(m_pages.begin(), it);
		}

		element_type* element = nullptr;
		index idx = {};

		if (m_indices.size())
		{
			idx = m_indices.back();
			m_indices.pop();
			element = &m_pages[(size_t)idx.page]->buffer[(size_t)idx.offset];
		}
		else
		{
			ref pg = m_pages[m_current];

			idx.page = static_cast<uint16>(m_current);
			idx.offset = static_cast<uint16>(pg->tail);

			element = &pg->buffer[pg->tail++];
		}
		return std::pair<index, element_type&>{ idx, *element };
	}
};

}

#endif // !LIB_PAGED_ARRAY_H
