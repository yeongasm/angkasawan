#pragma once
#ifndef LIB_LINEAR_ALLOCATOR_HPP
#define LIB_LINEAR_ALLOCATOR_HPP

#include "memory.hpp"

namespace lib
{

class linear_allocator : protected allocator_base
{
public:

	class deleter
	{
	public:
		deleter(linear_allocator& allocator) :
			allocator{ allocator }
		{}

		~deleter() {}

		template <is_pointer pointer_type>
		void operator()(pointer_type pointer) 
		{ 
			allocator.release_memory(pointer); 
		}
	private:
		linear_allocator& allocator;
	};

	using super			= allocator_base;
	using memory_header = typename super::memory_header<linear_allocator>;

	linear_allocator() :
		m_block{}, m_offset{}, m_size{}
	{}

	linear_allocator(size_t capacity, std::source_location location = std::source_location::current()) : 
		m_block{}, m_offset{}, m_size{ capacity }
	{
		m_block = static_cast<uint8*>(lib::allocate_memory({ .size = capacity, .location = location }));
	}

	~linear_allocator()
	{
		if (m_block)
		{
			lib::release_memory(m_block);
			m_block = nullptr;
			m_offset = m_size = 0;
		}
	}

	/**
	* Indicates if the allocator is good for use.
	*/
	bool is_good() const { return (m_block != nullptr) && (m_offset < m_size); }

	void* allocate_memory(allocate_info const& info)
	{
		ASSERTION(is_power_of_two(info.alignment) && "alignment needs to be a power of two!");
		ASSERTION(((m_offset + sizeof(memory_header) + info.size + info.alignment) < m_size) &&
					"Allocation will exceed the capacity of the allocator!");

		size_t allocated = 0;
		memory_header* header = static_cast<memory_header*>(aligned_alloc(m_block + m_offset, sizeof(memory_header) + info.size, info.alignment, allocated));
		m_offset += allocated;

		header->allocator	= this;
		header->size		= allocated;

		return static_cast<void*>(reinterpret_cast<uint8*>(header) + sizeof(memory_header));
	}

	template <typename T>
	T* allocate_memory(allocate_info const& info)
	{
		return static_cast<T*>(allocate_memory({ .size = info.size = sizeof(T), .alignment = info.alignment, .location = info.location }));
	}

	template <typename T, typename... Args>
	T* allocate_memory(allocate_info const& info, Args&&... args)
	{
		T* ptr = allocate_memory<T>(info);
		new (ptr) T{ std::forward<Args>(args)... };
		return ptr;
	}

	/**
	* WARNING:
	* Use at own discretion!
	* Releasing a block of memory that is not the last item being allocated will cause fragmentation and possibly corrupt memory.
	* If you still wish to continue, just remember that you have been warned and will likely encounter ground breaking bugs.
	*/
	template <is_pointer pointer_type>
	void release_memory(pointer_type pointer)
	{
		using element_type = std::remove_pointer_t<std::remove_cv_t<pointer_type>>;
		
		if (pointer)
		{
			pointer->~element_type();

			const uintptr_t current = reinterpret_cast<uintptr_t>(m_block + m_offset);
			const uintptr_t ptr = reinterpret_cast<uintptr_t>(super::aligned_free(pointer));

			if (current > ptr)
			{
				uintptr_t root = reinterpret_cast<uintptr_t>(m_block);
				m_offset = static_cast<size_t>(ptr - root);
			}
		}
	}

	size_t remaining_space	() const	{ return m_size - m_offset; }

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

	friend class deleter;

	linear_allocator(const linear_allocator&)	= delete;
	linear_allocator(linear_allocator&&)		= delete;

	linear_allocator& operator=(const linear_allocator&)	= delete;
	linear_allocator& operator=(linear_allocator&&)			= delete;
};

}

#endif // !LIB_LINEAR_ALLOCATOR_HPP
