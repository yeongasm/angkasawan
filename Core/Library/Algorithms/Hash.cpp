#include "Hash.h"

void MurmurHash32(const void* Key, uint32 Len, void* Out, uint32 Seed)
{
	const uint8* Data = reinterpret_cast<const uint8*>(Key);
	const int32 NumBlocks = Len / 4;

	const uint32* Blocks = reinterpret_cast<const uint32*>(Data + NumBlocks * 4);
	uint32 Hash = Seed;

	for (int32 i = -NumBlocks; i; i++)
	{
		uint32 Element = static_cast<uint32>(Blocks[i]);

		Element *= 0xCC9E2D51;
		Element = (Element << 15) | (Element >> (32 - 15));
		Element *= 0x1B873593;

		Hash ^= Element;
		Hash = (Hash << 13) | (Hash >> (32 - 13));
		Hash = Hash * 5 + 0xE6546b64;
	}

	Hash ^= Len;
	Hash ^= Hash >> 16;
	Hash *= 0x85EBCA6B;
	Hash ^= Hash >> 13;
	Hash *= 0xC2B2AE35;
	Hash ^= Hash >> 16;

	*reinterpret_cast<uint32*>(Out) = Hash;
}