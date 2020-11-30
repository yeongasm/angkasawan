#pragma once
#ifndef LEARNVK_LINEAR_ALLOCATOR
#define LEARNVK_LINEAR_ALLOCATOR

#include "BaseAllocator.h"

/**
* Linear allocator implementation.
* 
* Currently does not allow memory re-allocation.
*/
class LinearAllocator final : protected AllocatorInterface
{
public:

	LinearAllocator() : 
		Block(nullptr), Size(0), Offset(0) 
	{}

	~LinearAllocator() 
	{}

	DELETE_COPY_AND_MOVE(LinearAllocator)

	void Initialize(size_t BlockSize)
	{
		if (Block) return;
		Block = reinterpret_cast<uint8*>(FMemory::Malloc(BlockSize));
		Size = BlockSize;
	}

	void Terminate()
	{
		if (!Block) return;
		FMemory::Free(Block);
		new (this) LinearAllocator();
	}

	void* Malloc(size_t Size, size_t Alignment = 0x10) override
	{
		VKT_ASSERT("Not enough space for allocation" && Offset + Size <= this->Size);

		size_t padding = 0;
		const uintptr_t currentAddress = reinterpret_cast<uintptr_t>(Block + Offset);

		if (Alignment != 0 && Offset % Alignment != 0)
		{
			padding = FMemory::CalculatePadding(currentAddress, Alignment);
		}

		Offset += padding;
		void* result = reinterpret_cast<void*>(currentAddress + padding);
		Offset += Size;

		FMemory::Memzero(result, Size);
		return result;
	}

	void FlushMemory()
	{
		FMemory::Memzero(Block, Size);
		Offset = 0;
	}

	template <typename Type>
	Type* New(bool Construct = true)
	{
		Type* p = reinterpret_cast<Type*>(Malloc(sizeof(Type), alignof(Type)));
		if (Construct)
		{
			new (p) Type();
		}
		return p;
	}

	template <typename Type>
	Type* New(size_t Num, bool Construct = true)
	{
		Type* p = reinterpret_cast<Type*>(Malloc(sizeof(Type) * Num, alignof(Type)));
		if (Construct)
		{
			for (size_t i = 0; i < Num; i++)
			{
				new (p[i]) Type();
			}
		}
		return p;
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