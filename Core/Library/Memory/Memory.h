#pragma once
#ifndef LEARNVK_MEMORY
#define LEARNVK_MEMORY

#include "Platform/EngineAPI.h"
#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"

#define BYTES(n)		(n)
#define KILOBYTES(n)	(BYTES(n)		* 1024)
#define MEGABYTES(n)	(KILOBYTES(n)	* 1024)
#define GIGABYTES(n)	(MEGABYTES(n)	* 1024)

struct ENGINE_API FMemory
{
	static void*	AllignedAlloc		(size_t Size, size_t Alignment);
	static void*	Malloc				(size_t Size);
	static void*	Realloc				(void* Block, size_t Size);
	static void		Free				(void* Block);

	static void		Memmove				(void* Destination, const void* Source, size_t Count);
	static void		Memcpy				(void* Destination, const void* Source, size_t Count);
	static void		Memzero				(void* Destination, size_t Count);
	static void		Memset				(void* Destination, uint8 Char, size_t Count);
	static size_t	Memcmp				(void* Buf1, void* Buf2, size_t Count);

	static bool		IsPowerOfTwo		(size_t Num);
	static size_t	CalculatePadding	(const uintptr_t BaseAddress, size_t Alignment);

	template <typename T>
	static void	InitializeObject(T* Object) 
	{
		Memzero(Object, sizeof(T));
		new (Object) T();
	};

	template <typename T>
	static void InitializeObject(T& Object)
	{
		InitializeObject(&Object);
	}

	template <typename T, typename... Initializers>
	static void InitializeObject(T* Object, Initializers&&... Args)
	{
		Memzero(Object, sizeof(T));
		new (Object) T(Forward<Initializers>(Args)...);
	}

	template <typename T, typename... Initializers>
	static void InitializeObject(T& Object, Initializers&&... Args)
	{
		InitializeObject(&Object, Forward(Args)...);
	}
};

//struct ENGINE_API MemoryBlock
//{
//	uint8*	Block;
//	size_t	Size;
//
//	MemoryBlock() : Block(nullptr), Size(0) {}
//	~MemoryBlock() {}
//	MemoryBlock(const MemoryBlock& Rhs)				= delete;
//	MemoryBlock(MemoryBlock&& Rhs)					= delete;
//	MemoryBlock& operator=(const MemoryBlock& Rhs)	= delete;
//	MemoryBlock& operator=(MemoryBlock&& Rhs)		= delete;
//
//	void*	Malloc	(size_t Size);
//	void*	Realloc	(void* Block, size_t Size);
//	void	Free	();
//};

#endif // !LEARNVK_MEMORY