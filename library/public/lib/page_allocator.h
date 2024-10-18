#ifndef FOUNDATION_PAGE_ALLOCATOR_H
#define FOUNDATION_PAGE_ALLOCATOR_H

#include "memory/memory.h"

FTLBEGIN

template <size_t PAGE_SIZE>
class PageAllocator : protected AllocatorBase
{
private:

	// There are 2 parts to this allocator.
	// 1. The memory page.
	// 2. The free list.

	struct Allocation
	{
		size_t		size;	// The allocation's size.
		Allocation*	pNext;	// Next free allocation.
	};

	//void link_allocation(Allocation* previousAllocation, Allocation* allocation);
	//void break_allocation(Allocation* previousAllocation, Allocation* allocation);

	struct MemoryPage
	{
		MemoryPage* pPrevious;
		MemoryPage* pNext;
	};

	struct PageHeader
	{
		PageHeader*	pPrevious;		// Pointer to the previous page.
		PageHeader*	pNext;			// Pointer to the next page.
		Allocation*	pAllocation;	// Free-list set up.
		//size_t		remainingSpace;	// Remaining space in the page.
	};

	uint8* m_page;

	// --- methods ---

	PageHeader* get_page_header(void* pointer)
	{
		uint8* address = static_cast<uint8*>(pointer);
		return reinterpret_cast<PageHeader*>(address - sizeof(PageHeader));
	}

	bool is_pointer_in_page(void* pointer)
	{
		uint8* address = static_cast<uint8*>(pointer);
		return (address >= m_page) && (address < m_page + PAGE_SIZE - sizeof(Allocation));
	}

	void allocate_new_memory_page(std::source_location location)
	{
		// Allocate the page.
		void* page = ftl::allocate_memory(sizeof(PageHeader) + PAGE_SIZE, __STDCPP_DEFAULT_NEW_ALIGNMENT__, location);

		if (!m_page)
		{
			// If this is the first time the allocator is allocating a page,
			// simply set up the page's header information.
			new (page) PageHeader{ nullptr, nullptr, nullptr, PAGE_SIZE };
			// Assign the page pointer to point to the newly allocated page.
			m_page = static_cast<uint8*>(page) + sizeof(PageHeader);
		}
		else
		{
			// If there was an existing page.
			void* previousPageRootAddress = m_page - sizeof(Allocation);

			PageHeader* previousHeader = get_page_header(previousPageRootAddress);
			PageHeader* newHeader = static_cast<PageHeader*>(page);

			// Placement new to initalize header information.
			new (newHeader) PageHeader{ previousHeader, nullptr, nullptr, PAGE_SIZE };
			// Set previous header to point to new header's address.
			previousHeader->pNext = newHeader;
			// Root address of the page pointer is updated to point to the newly allocated page.
			m_page = static_cast<uint8*>(page) + sizeof(PageHeader);

		}
		// Set up free list.
		PageHeader* header = get_page_header(m_page);

		header->remainingSpace -= sizeof(Allocation);

		new (m_page) Allocation{ header->remainingSpace, nullptr };
		header->pAllocation = reinterpret_cast<Allocation*>(m_page);

		m_page += sizeof(Allocation);
	}

public:

	PageAllocator() :
		m_page{}
	{}

	~PageAllocator()
	{

	}

	void* allocate_memory(size_t size, size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__, std::source_location location = std::source_location::current())
	{
		// First step is to get the page's free-list pointer.
		if (!m_page)
		{
			// If this is the first time something is being allocated into the allocator, we request for a new memory page.
			allocate_new_memory_page(location);
		}
		else if ()
		{
			// Now we check if 
		}
		
		return nullptr;
	}

};

FTLEND

#endif // !FOUNDATION_PAGE_ALLOCATOR_H