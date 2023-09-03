#include <map>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include "memory.h"
//#include "fmt/format.h"

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

bool is_64bit_aligned(void* pointer)
{
	uintptr_t address = reinterpret_cast<uintptr_t>(pointer);
	return (address & 0x7) == 0;
}

class system_memory final
{
public:
	struct memory
	{
		void*	pointer;
		size_t	size;
	};

	system_memory() : m_allocated_size{ 0 }, m_memory_tracker{} {}

	~system_memory()
	{
#if _DEBUG

//#define MEMORY_LEAK_STR R"(
//[MEMORY LEAK DETECTED]
//Unreleased memory: {} Bytes.
//------------------------------------------------------------------------------------------------------------------------------------
//)"
//		if (m_allocated_size != 0)
//		{
//			fmt::print(MEMORY_LEAK_STR, m_allocated_size.load());
//			fmt::print("Address\t\t| Size\t\t| Function\t\t| Line\t\t| File\t\t\n");
//			for (auto const& [address, location] : m_memory_tracker)
//			{
//				size_t size = *static_cast<size_t*>(address);
//				fmt::print("------------------------------------------------------------------------------------------------------------------------------------\n");
//				fmt::print("{}\t| {} Bytes\t| {}\t\t| {}\t\t| {}\t\n", address, size, location.function_name(), location.line(), location.file_name());
//			}
//		}
#endif
		m_allocated_size = 0;
	}

	memory malloc(size_t size, size_t alignment = 16, std::source_location location = std::source_location::current())
	{
		size_t allocated = 0;

		void* pointer = aligned_alloc(size, alignment, allocated);

		if (!pointer)
		{
			ASSERTION(pointer != nullptr && "The impossible just happened!");
			return { nullptr, 0 };
		}

		ASSERTION(is_64bit_aligned(pointer) && "Address is not 64 bit aligned");

		m_allocated_size.fetch_add(allocated);

		m_memory_tracker.emplace(pointer, location);

		return { pointer, allocated };
	}

	void free(void* pointer)
	{
		ASSERTION(pointer != nullptr && "Pointer being released is null!");
		if (pointer)
		{
			m_memory_tracker.erase(pointer);
			aligned_free(pointer);
		}
	}

	size_t total_memory_allocated() const { return m_allocated_size.load(); }

private:
	friend void release_memory(void*);

	std::atomic_size_t						m_allocated_size;
	std::map<void*, std::source_location>	m_memory_tracker;

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

static system_memory global_memory{};

void* allocate_memory(allocate_info const& info)
{
	system_memory::memory memory = global_memory.malloc(info.size + sizeof(size_t), info.alignment, info.location);
	void* pointer = memory.pointer;

	new (pointer) size_t{ memory.size };

	pointer = static_cast<uint8*>(pointer) + sizeof(size_t);
	ASSERTION(is_64bit_aligned(pointer) && "Address is not aligned!");

	return pointer;
}

void release_memory(void* pointer)
{
	size_t* base = reinterpret_cast<size_t*>(static_cast<uint8*>(pointer) - sizeof(size_t));
	global_memory.m_allocated_size.fetch_sub(*base);
	global_memory.free(base);
}

size_t total_memory_allocated()
{
	return global_memory.total_memory_allocated();
}

}