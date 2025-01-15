#include <cstdlib>
#include <cstring>
#include "lib/memory.hpp"

namespace lib
{

void memmove(void* dst, void* src, size_t size)
{
	std::memmove(dst, src, size);
}

void memcopy(void* dst, const void* src, size_t size)
{
	std::memcpy(dst, src, size);
}

size_t memcmp(void* src, void* dst, size_t size)
{
	return std::memcmp(src, dst, size);
}

void memset(void* dst, uint8 Val, size_t size)
{
	std::memset(dst, Val, size);
}

void memzero(void* dst, size_t size)
{
	std::memset(dst, 0, size);
}

constexpr size_t is_power_of_two(size_t num)
{
	return (num > 0) & ((num & (num - 1)) == 0);
}

constexpr size_t pad_address(const uintptr_t address, const size_t alignment)
{
	const size_t multiplier = (address / alignment) + 1;
	const uintptr_t alignedAddress = multiplier * alignment;
	return static_cast<size_t>(alignedAddress - address);
}

constexpr bool is_64bit_aligned(void* pointer)
{
	uintptr_t address = std::bit_cast<uintptr_t>(pointer);
	return (address & 0x7) == 0;
}

struct memory
{
	void*	pointer;
	size_t	size;
};

struct system_memory final
{
	static memory allocate(size_t size, size_t alignment = 16)
	{
		size_t allocated = 0;

		void* pointer = aligned_alloc(size, alignment, allocated);

		if (!pointer)
		{
			ASSERTION(pointer != nullptr && "The impossible just happened!");
			return { nullptr, 0 };
		}

		ASSERTION(is_64bit_aligned(pointer) && "Address is not 64 bit aligned");

		return { pointer, allocated };
	}

	static void deallocate(void* pointer)
	{
		ASSERTION(pointer != nullptr && "Pointer being released is null!");
		if (pointer)
		{
			aligned_free(pointer);
		}
	}

private:

	static void* aligned_alloc(size_t size, size_t& alignment, size_t& total)
	{
		// Code shamefully taken from, https://stackoverflow.com/q/38088732.
		ASSERTION(is_power_of_two(alignment) && "Alignment needs to be a power of 2!");

		if (alignment < sizeof(void*))
		{
			alignment = sizeof(void*);
		}

		void* p0 = nullptr;
		void* p1 = nullptr;

		size_t offset = alignment - 1 + sizeof(void*);
		total = size + offset;

		p0 = operator new(total);

		if (!p0)
		{
			return nullptr;
		}

		p1 = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(p0) + offset) & ~(alignment - 1));

		ASSERTION(is_64bit_aligned(p1) && "Address is not aligned!");
		*(static_cast<void**>(p1) - 1) = p0;

		return p1;
	}

	static void aligned_free(void* pointer)
	{
		void* root = *(static_cast<void**>(pointer) - 1);
		operator delete(root);
	}
};

auto default_memory_resource::do_allocate(size_t size, size_t alignment) -> void*
{
	memory memory = system_memory::allocate(size, alignment);
	void* pointer = memory.pointer;

	return pointer;
}

auto default_memory_resource::do_deallocate(void* p, [[maybe_unused]] size_t bytes, [[maybe_unused]] size_t alignment) -> void
{
	if (p != nullptr)
	{
		system_memory::deallocate(p);
	}
}

auto default_memory_resource::do_is_equal(memory_resource const& other) -> bool
{
	return this == std::addressof(other);
}

auto get_default_resource() noexcept -> memory_resource*
{
	static default_memory_resource _default = {};
	return &_default;
}
}