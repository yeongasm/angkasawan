#pragma once
#ifndef LEARNVK_BASE_ALLOCATIOR
#define LEARNVK_BASE_ALLOCATIOR

#include "Library/Memory/Memory.h"
#include "Library/Templates/Templates.h"

/**
* Allocator Interface abstract class.
*/
struct IAllocator
{
	IAllocator() {}
	virtual ~IAllocator() {};

	//virtual void	Initialize(size_t) = 0;
	virtual void*	Malloc	(size_t Size, size_t Alignment = 0x10) = 0;
	virtual void*	Realloc	(void* Block, size_t Size) = 0;
	virtual void	Free	(void* Block) = 0;
	//virtual void	Terminate() = 0;

	template <typename ResourceType, typename AllocatorType>
	static ResourceType* New(AllocatorType& Allocator)
	{
		ResourceType* resource = reinterpret_cast<ResourceType*>(Allocator.Malloc(sizeof(ResourceType)));
		new (resource) ResourceType();
		return resource;
	}

	template <typename ResourceType, typename AllocatorType, typename... Initializers>
	static ResourceType* New(AllocatorType& Allocator, Initializers&&... Args)
	{
		ResourceType* resource = reinterpret_cast<ResourceType*>(Allocator.Malloc(sizeof(ResourceType)));
		new (resource) ResourceType(Forward<Initializers>(Args)...);
		return resource;
	}

	template <typename ResourceType, typename AllocatorType>
	static void Delete(ResourceType* Resource, AllocatorType& Allocator)
	{
		Allocator.Free(Resource);
	}
};

#endif // !LEARNVK_BASE_ALLOCATIOR