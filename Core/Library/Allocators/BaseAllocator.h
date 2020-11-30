#pragma once
#ifndef LEARNVK_BASE_ALLOCATIOR
#define LEARNVK_BASE_ALLOCATIOR

#include "Platform/EngineAPI.h"
#include "Library/Memory/Memory.h"
#include "Library/Templates/Templates.h"

/**
* Allocator Interface abstract class.
*/
struct AllocatorInterface
{
	virtual void*	Malloc	(size_t Size, size_t Alignment = 0x10)	= 0;
	virtual void*	Realloc	(void* Block, size_t Size)				= 0;
	virtual void	Free	(void* Block)							= 0;
};

/**
* Default new and delete allocator.
*/
struct DefaultAllocator final : public AllocatorInterface
{
	void* Malloc(size_t Size, size_t Alignment = 0x10) override
	{
		return FMemory::AllignedAlloc(Size, Alignment);
	}

	void* Realloc(void* Block, size_t Size) override
	{
		return FMemory::Realloc(Block, Size);
	}

	void Free(void* Block) override
	{
		FMemory::Free(Block);
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

	template <typename Type>
	void Delete(Type* Block)
	{
		Free(Block);
	}
};

#endif // !LEARNVK_BASE_ALLOCATIOR