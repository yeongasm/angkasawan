//#include <memory>
#include <map>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include "memory.h"
#include "fmt/format.h"

FTLBEGIN

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

bool is_64bit_aligned(void* pointer)
{
	uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
	return (address & 0x7) == 0;
}

// --- System Memory ---

class SystemMemory final
{
public:
	struct Memory
	{
		void*	m_pointer;
		size_t	m_allocationSize;
	};
	
	SystemMemory() : m_allocatedSize{ 0 }, m_memoryTracker{} {}

	~SystemMemory()
	{
#if _DEBUG

#define MEMORY_LEAK_STR R"(
[MEMORY LEAK DETECTED]
Unreleased memory: {} Bytes.
------------------------------------------------------------------------------------------------------------------------------------
)"
		if (m_allocatedSize != 0)
		{
			fmt::print(MEMORY_LEAK_STR, m_allocatedSize.load());
			fmt::print("Address\t\t| Size\t\t| Function\t\t| Line\t\t| File\t\t\n");
			for (auto const& [address, location] : m_memoryTracker)
			{
				size_t size = *static_cast<size_t*>(address);
				fmt::print("------------------------------------------------------------------------------------------------------------------------------------\n");
				fmt::print("{}\t| {} Bytes\t| {}\t\t| {}\t\t| {}\t\n", address, size, location.function_name(), location.line(), location.file_name());
			}
		}
#endif
		m_allocatedSize = 0;
	}

	Memory malloc(size_t size, size_t alignment = 16, std::source_location _ = std::source_location::current())
	{
		size_t allocated = 0;

		void* pointer = aligned_alloc(size, alignment, allocated);

		if (!pointer)
		{
			ASSERTION(pointer != nullptr && "The impossible just happened!");
			return { nullptr, 0 };
		}

		ASSERTION(is_64bit_aligned(pointer) && "Address is not 64 bit aligned");

		m_allocatedSize.fetch_add(allocated);

		m_memoryTracker.emplace(pointer, _);

		return { pointer, allocated };
	}

	void free(void* pointer)
	{
		ASSERTION(pointer != nullptr && "Pointer being released is null!");
		if (pointer)
		{
			m_memoryTracker.erase(pointer);
			aligned_free(pointer);
		}
	}

	size_t total_memory_allocated() const { return m_allocatedSize.load(); }

private:
	friend void release_memory(void*);

	// Should be atomic.
	std::atomic_size_t						m_allocatedSize;
	std::map<void*, std::source_location>	m_memoryTracker;

	void* aligned_alloc(size_t size, size_t& alignment, size_t& total)
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

		p0 = std::malloc(total);
		if (!p0)
		{
			return nullptr;
		}

		p1 = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(p0) + offset) & ~(alignment - 1));

		ASSERTION(is_64bit_aligned(p1) && "Address is not aligned!");
		*(static_cast<void**>(p1) - 1) = p0;
				
		return p1;
	}

	void aligned_free(void* pointer)
	{
		void* root = *(static_cast<void**>(pointer) - 1);
		std::free(root);
	}
};

static SystemMemory systemMemory{};

void* allocate_memory(size_t size, size_t alignment, std::source_location location)
{
	SystemMemory::Memory memory = systemMemory.malloc(size + sizeof(size_t), alignment, location);
	void* pointer = memory.m_pointer;

	new (pointer) size_t{ memory.m_allocationSize };

	pointer = static_cast<uint8*>(pointer) + sizeof(size_t);
	ASSERTION(is_64bit_aligned(pointer) && "Address is not aligned!");

	return pointer;
}

void release_memory(void* pointer)
{
	size_t* base = reinterpret_cast<size_t*>(static_cast<uint8*>(pointer) - sizeof(size_t));
	systemMemory.m_allocatedSize.fetch_sub(*base);
	systemMemory.free(base);
}

size_t total_memory_allocated()
{
	return systemMemory.total_memory_allocated();
}

FTLEND