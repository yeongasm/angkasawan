#pragma once
#ifndef FOUNDATION_ALLOCATOR_STACK_ALLOCATOR_H
#define FOUNDATION_ALLOCATOR_STACK_ALLOCATOR_H

#include "linear_allocator.h"

FTLBEGIN

/**
* Cannot be used in containers like arrays, string / maps. Prefer using interfaces to those objects.
*/
class StackAllocator final : protected LinearAllocator
{
public:
	using Deleter	= DeleterInterface<StackAllocator>;
	using Super		= LinearAllocator;
	using MemoryHeader = typename Super::Super::MemoryHeader<StackAllocator>;

	template <typename T>
	using StackPtr = UniquePtr<T, StackAllocator>;

	StackAllocator() : Super{} {}
	StackAllocator(size_t capacity) : Super{ capacity } {}
	
	using Super::remaining_space;
	using Super::is_good;

	template <typename T, typename... Arguments>
	StackPtr<T> allocate_memory(size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__, Arguments&&... args)
	{
		constexpr size_t size = sizeof(T);

		ASSERTION(is_power_of_two(alignment) && "alignment needs to be a power of two!");
		ASSERTION(((Super::m_offset + sizeof(MemoryHeader) + size + alignment) < Super::m_size) &&
					"Allocation will exceed the capacity of the allocator!");

		size_t allocated = 0;
		MemoryHeader* header = Super::allocate_memory_internal<MemoryHeader>(size, alignment, allocated);

		header->m_allocator = this;
		header->m_size = allocated;

		T* result = reinterpret_cast<T*>(reinterpret_cast<uint8*>(header) + sizeof(MemoryHeader));
		new (result) T{ std::forward<Arguments>(args)... };

		return StackPtr<T>{ result };
	}

	template <typename T, std::derived_from<T> U, typename... Arguments>
	StackPtr<T> allocate_memory(size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__, Arguments&&... args)
	{
		constexpr size_t size = sizeof(U);

		ASSERTION(is_power_of_two(alignment) && "alignment needs to be a power of two!");
		ASSERTION(((Super::m_offset + sizeof(MemoryHeader) + size + alignment) < Super::m_size) &&
			"Allocation will exceed the capacity of the allocator!");

		size_t allocated = 0;
		MemoryHeader* header = Super::allocate_memory_internal<MemoryHeader>(size, alignment, allocated);

		header->m_allocator = this;
		header->m_size = allocated;

		U* result = reinterpret_cast<U*>(reinterpret_cast<uint8*>(header) + sizeof(MemoryHeader));
		new (result) U{ std::forward<Arguments>(args)... };

		return StackPtr<T>{ static_cast<T*>(result) };
	}

private:
	friend class Deleter;

	StackAllocator(const StackAllocator&) = delete;
	StackAllocator(StackAllocator&&) = delete;

	StackAllocator& operator=(const StackAllocator&) = delete;
	StackAllocator& operator=(StackAllocator&&) = delete;

	void release_memory(void* pointer)
	{
		using Super = LinearAllocator;
		using MemoryHeader = typename Super::Super::MemoryHeader<StackAllocator>;

		const uintptr_t current = reinterpret_cast<uintptr_t>(Super::m_block + Super::m_offset);
		const uintptr_t ptr = reinterpret_cast<uintptr_t>(Super::Super::aligned_free(pointer));

		if (current > ptr)
		{
			const MemoryHeader* header = reinterpret_cast<const MemoryHeader*>(reinterpret_cast<uint8*>(pointer) - sizeof(MemoryHeader));
			Super::m_offset -= header->m_size;
		}
	}
};

template <typename T>
constexpr StackAllocator::StackPtr<T> make_stack(StackAllocator& allocator)
{
	auto pointer = allocator.allocate_memory<T>();
	new (pointer.get()) T{};

	return pointer;
}

template <typename T, typename... Arguments>
constexpr StackAllocator::StackPtr<T> make_stack(StackAllocator& allocator, Arguments&&... args)
{
	auto pointer = allocator.allocate_memory<T>();
	new (pointer.get()) T{ std::forward<Arguments>(args)... };

	return pointer;
}

FTLEND

#endif // !FOUNDATION_ALLOCATOR_STACK_ALLOCATOR_H
