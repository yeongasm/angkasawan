#include <cstdlib>
#include <cstring>
#include "Memory.h"


void* FMemory::AllignedAlloc(size_t Size, size_t Alignment)
{
	return _aligned_malloc(Size, Alignment);
}

void* FMemory::Malloc(size_t Size)
{
	return malloc(Size);
}


void* FMemory::Realloc(void* Block, size_t Size)
{
	return realloc(Block, Size);
}


void FMemory::Free(void* Block)
{
	free(Block);
}


void FMemory::Memmove(void* Destination, const void* Source, size_t Count)
{
	memmove(Destination, Source, Count);
}

	
void FMemory::Memcpy(void* Destination, const void* Source, size_t Count)
{
	memcpy(Destination, Source, Count);
}


void FMemory::Memzero(void* Destination, size_t Count)
{
	memset(Destination, 0, Count);
}


void FMemory::Memset(void* Destination, uint8 Char, size_t Count)
{
	memset(Destination, Char, Count);	
}


size_t FMemory::Memcmp(void* Buf1, void* Buf2, size_t Count)
{
	return memcmp(Buf1, Buf2, Count);
}


bool FMemory::IsPowerOfTwo(size_t Num)
{
	return Num && !(Num & (Num - 1));
}


size_t FMemory::CalculatePadding(const uintptr_t BaseAddress, size_t Alignment)
{
	const size_t multiplier = (BaseAddress / Alignment) + 1;
	const uintptr_t alignedAddress = multiplier * Alignment;

	return static_cast<size_t>(alignedAddress - BaseAddress);
}
