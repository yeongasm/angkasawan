#pragma once
#ifndef STACK_ALLOCATOR_H
#define STACK_ALLOCATOR_H

#include "linear_allocator.h"

namespace lib
{

class stack_allocator;

template <typename T>
using stack_ptr = unique_ptr<T, typename stack_allocator::deleter>;

/**
* Cannot be used in containers like arrays, string / maps. Prefer using interfaces to those objects.
*/
class stack_allocator final : protected linear_allocator
{
public:
	class deleter
	{
	public:
		constexpr deleter(stack_allocator& allocator) :
			allocator{ allocator }
		{}

		constexpr ~deleter() {}

		constexpr deleter(deleter const& rhs) :
			allocator{ rhs.allocator }
		{}

		template <is_pointer pointer_type>
		constexpr void operator()(pointer_type pointer)
		{
			allocator.release_memory(pointer);
		}
	private:
		stack_allocator& allocator;
	};

	using super			= linear_allocator;
	using memory_header = typename super::super::memory_header<stack_allocator>;

	stack_allocator() : super{} {}
	stack_allocator(size_t capacity, std::source_location location = std::source_location::current()) : super{ capacity, location } {}
	
	using super::remaining_space;
	using super::is_good;

	template <typename T, typename... Arguments>
	stack_ptr<T> allocate_memory(allocate_type_info<T> const& info, Arguments&&... args)
	{
		constexpr size_t size = sizeof(T);

		ASSERTION(is_power_of_two(info.alignment) && "alignment needs to be a power of two!");
		ASSERTION(((super::m_offset + sizeof(memory_header) + (info.count * size) + info.alignment) < super::m_size) &&
					"Allocation will exceed the capacity of the allocator!");

		size_t allocated = 0;
		memory_header* header = static_cast<memory_header*>(aligned_alloc(m_block + m_offset, sizeof(memory_header) + (info.count * size), info.alignment, allocated));
		m_offset += allocated;

		header->allocator	= this;
		header->size		= allocated;

		T* result = reinterpret_cast<T*>(reinterpret_cast<uint8*>(header) + sizeof(memory_header));
		new (result) T{ std::forward<Arguments>(args)... };

		return stack_ptr<T>{ result, deleter{ this } };
	}

	//template <typename T, std::derived_from<T> U, typename... Arguments>
	//stack_ptr<T> allocate_memory(allocate_type_info<U> const& info, Arguments&&... args)
	//{
	//	constexpr size_t size = sizeof(U);

	//	ASSERTION(is_power_of_two(info.alignment) && "alignment needs to be a power of two!");
	//	ASSERTION(((super::m_offset + sizeof(memory_header) + (info.count * size) + info.alignment) < super::m_size) &&
	//		"Allocation will exceed the capacity of the allocator!");

	//	size_t allocated = 0;
	//	memory_header* header = static_cast<memory_header*>(aligned_alloc(m_block + m_offset, sizeof(memory_header) + (info.count * size), info.alignment, allocated));
	//	m_offset += allocated;

	//	header->allocator = this;
	//	header->size		= allocated;

	//	U* result = reinterpret_cast<U*>(reinterpret_cast<uint8*>(header) + sizeof(memory_header));
	//	new (result) U{ std::forward<Arguments>(args)... };

	//	return stack_ptr<T>{ static_cast<T*>(result) };
	//}

private:
	friend class deleter;

	stack_allocator(const stack_allocator&)				= delete;
	stack_allocator(stack_allocator&&)					= delete;
	stack_allocator& operator=(const stack_allocator&)	= delete;
	stack_allocator& operator=(stack_allocator&&)		= delete;

	template <is_pointer pointer_type>
	void release_memory(pointer_type pointer)
	{
		using element_type = std::remove_pointer_t<std::remove_cv_t<pointer_type>>;

		if (pointer)
		{
			pointer->~element_type();

			const uintptr_t current = reinterpret_cast<uintptr_t>(super::m_block + super::m_offset);
			const uintptr_t ptr = reinterpret_cast<uintptr_t>(super::super::aligned_free(pointer));

			if (current > ptr)
			{
				const memory_header* header = reinterpret_cast<const memory_header*>(reinterpret_cast<uint8*>(pointer) - sizeof(memory_header));
				super::m_offset -= header->m_size;
			}
		}
	}
};

}

#endif // !STACK_ALLOCATOR_H
