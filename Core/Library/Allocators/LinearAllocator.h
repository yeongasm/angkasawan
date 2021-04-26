#pragma once
#ifndef LEARNVK_LINEAR_ALLOCATOR
#define LEARNVK_LINEAR_ALLOCATOR

#include "BaseAllocator.h"

/**
* Linear allocator implementation.
* Currently does not allow memory re-allocation.
*/
class LinearAllocator final : protected IAllocator
{
public:

	LinearAllocator() : 
		Block(nullptr), Size(0), Offset(0) 
	{}

	LinearAllocator(size_t BlockSize) :
		LinearAllocator()
	{
		Initialize(BlockSize);
	}

	~LinearAllocator() 
	{
		Terminate();
	}

	DELETE_COPY_AND_MOVE(LinearAllocator)

	void Initialize(size_t BlockSize)
	{
		if (Block) return;
		Block = reinterpret_cast<uint8*>(IMemory::Malloc(BlockSize));
		Size = BlockSize;
	}

	void Terminate()
	{
		if (!Block) return;
		IMemory::Free(Block);
		new (this) LinearAllocator();
	}

	void* Malloc(size_t Size, size_t Alignment = 0x10) override
	{
		VKT_ASSERT("Not enough space for allocation" && Offset + Size <= this->Size);

		size_t padding = 0;
		const uintptr_t currentAddress = reinterpret_cast<uintptr_t>(Block + Offset);

		if (Alignment != 0 && Offset % Alignment != 0)
		{
			padding = IMemory::CalculatePadding(currentAddress, Alignment);
		}

		Offset += padding;
		void* result = reinterpret_cast<void*>(currentAddress + padding);
		Offset += Size;

		IMemory::Memzero(result, Size);
		return result;
	}

	void FlushMemory()
	{
		IMemory::Memzero(Block, Size);
		Offset = 0;
	}

private:

	uint8*	Block;
	size_t	Size;
	size_t	Offset;

	/**
	* DO NOT USE!!! Reallocation is not supported in LinearAllocators. This is to avoid fragmentation.
	*/
	void* Realloc(void* Block, size_t Size) override { return nullptr; }

	/**
	* DO NOT USE!!! Free-ing a block will cause fragmentation.
	*/
	void Free(void* Block) override {}
};


#endif // !LEARNVK_LINEAR_ALLOCATOR