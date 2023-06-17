#pragma once
#ifndef FOUNDATION_ALLOCATOR_LINEAR_ALLOCATOR_H
#define FOUNDATION_ALLOCATOR_LINEAR_ALLOCATOR_H

#include "memory/memory.h"

FTLBEGIN

class LinearAllocator : protected AllocatorBase
{
public:

	using Deleter	= DeleterInterface<LinearAllocator>;
	using Super		= AllocatorBase;
	using MemoryHeader = typename Super::MemoryHeader<LinearAllocator>;

	LinearAllocator() :
		m_block{}, m_offset{}, m_size{}
	{}

	LinearAllocator(size_t capacity, std::source_location location = std::source_location::current()) : 
		m_block{}, m_offset{}, m_size{ capacity }
	{
		request_memory_internal(location);
	}

	~LinearAllocator() { release_memory_internal(); }

	/**
	* Indicates if the allocator is good for use.
	*/
	bool is_good() const { return (m_block != nullptr) && (m_offset < m_size); }

	void* allocate_memory(size_t size, size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__)
	{
		ASSERTION(is_power_of_two(alignment) && "alignment needs to be a power of two!");
		ASSERTION(((m_offset + sizeof(MemoryHeader) + size + alignment) < m_size) && 
					"Allocation will exceed the capacity of the allocator!");

		size_t allocated = 0;
		MemoryHeader* header = allocate_memory_internal<MemoryHeader>(size, alignment, allocated);

		header->m_allocator = this;
		header->m_size = allocated;

		return static_cast<void*>(reinterpret_cast<uint8*>(header) + sizeof(MemoryHeader));
	}

	/**
	* WARNING:
	* Use at own discretion!
	* Releasing a block of memory that is not the last item being allocated will cause fragmentation and possibly corrupt memory.
	* If you still wish to continue, just remember that you have been warned and will likely encounter ground breaking bugs.
	*/
	void release_memory(void* pointer)
	{
		const uintptr_t current = reinterpret_cast<uintptr_t>(m_block + m_offset);
		const uintptr_t ptr = reinterpret_cast<uintptr_t>(Super::aligned_free(pointer));
		
		if (current > ptr)
		{
			uintptr_t root = reinterpret_cast<uintptr_t>(m_block);
			m_offset = static_cast<size_t>(ptr - root);
		}
	}

	template <typename T, typename... Arguments>
	T* allocator_new(Arguments&&... args)
	{
		T* ptr = reinterpret_cast<T*>(allocate_memory(sizeof(T)));
		new (ptr) T{ std::forward<Arguments>(args)... };
		return ptr;
	}

	/**
	* WARNING:
	* Use at own discretion!
	* Releasing a block of memory that is not the last item being allocated will cause fragmentation and possibly corrupt memory.
	* If you still wish to continue, just remember that you have been warned and will likely encounter ground breaking bugs.
	*/
	/*template <typename T>
	void allocator_free(T** pointer)
	{
		(*pointer)->~T();
		release_memory(pointer);
	}*/

	size_t	remaining_space	() const	{ return m_size - m_offset; }

	/**
	* WARNING:
	* Flushes the internal offset back to 0.
	* Use at own discretion!
	* Resetting the internal offset will invalidate previous pointers that were allocated.
	* This function does not call the destructor and will cause UB for non trivial/POD typed objects that were allocated from this allocator.
	*/
	void	flush			()			{ m_offset = 0; }

protected:
	uint8*	m_block;
	size_t	m_offset;
	size_t	m_size;

	friend class Deleter;

	LinearAllocator(const LinearAllocator&) = delete;
	LinearAllocator(LinearAllocator&&)		= delete;

	LinearAllocator& operator=(const LinearAllocator&)	= delete;
	LinearAllocator& operator=(LinearAllocator&&)		= delete;

	void request_memory_internal(std::source_location location)
	{
		m_block = reinterpret_cast<uint8*>(ftl::allocate_memory(m_size, __STDCPP_DEFAULT_NEW_ALIGNMENT__, location));
	}

	void release_memory_internal()
	{
		if (m_block)
		{
			ftl::release_memory(m_block);
			m_block = nullptr;
			m_offset = m_size = 0;
		}
	}

	template <typename MemoryHeader_t>
	MemoryHeader_t* allocate_memory_internal(size_t size, size_t alignment, size_t& allocated)
	{
		MemoryHeader_t* header = reinterpret_cast<MemoryHeader_t*>(aligned_alloc(m_block + m_offset, sizeof(MemoryHeader_t) + size, alignment, allocated));
		m_offset += allocated;

		return header;
	}
};

FTLEND

#endif // !FOUNDATION_ALLOCATOR_LINEAR_ALLOCATOR_H
