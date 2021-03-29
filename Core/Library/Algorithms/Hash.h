#pragma once
#ifndef LEARNVK_HASH
#define LEARNVK_HASH

#include "Platform/EngineAPI.h"
#include "Library/Templates/Types.h"


ENGINE_API void MurmurHash32(const void* Key, uint32 Len, void* Out, uint32 Seed = 0);
ENGINE_API void XXHash32(const void* Input, size_t Len, uint32* Out, uint32 Seed = 0);
ENGINE_API void XXHash64(const void* Input, size_t Len, uint64* Out, uint64 Seed = 0);

/**
* Default MurmurHash functor.
* Supplied type must contain these 2 functions:
*
* First()	- Returns a pointer to the to be hashed data.
* Length()	- Total number of data.
*/
template <typename Type>
struct MurmurHash
{
	uint32 operator() (const Type& Source) const
	{
		uint32 Hash = 0;
		MurmurHash32(Source.First(), static_cast<uint32>(Source.Length()), &Hash);
		return Hash;
	}
};


template <>
struct MurmurHash<int32>
{
	uint32 operator() (const int32 Source) const
	{
		uint32 Hash = 0;
		MurmurHash32(&Source, sizeof(int32), &Hash);
		return Hash;
	}
};


template <>
struct MurmurHash<uint32>
{
	uint32 operator() (const uint32 Source) const
	{
		uint32 Hash = 0;
		MurmurHash32(&Source, sizeof(uint32), &Hash);
		return Hash;
	}
};

template <>
struct MurmurHash<size_t>
{
	uint32 operator() (const size_t Source) const
	{
		uint32 Hash = 0;
		MurmurHash32(&Source, sizeof(size_t), &Hash);
		return Hash;
	}
};

template <>
struct MurmurHash<float32>
{
	uint32 operator() (const float32 Source) const
	{
		uint32 Hash = 0;
		MurmurHash32(&Source, sizeof(float32), &Hash);
		return Hash;
	}
};


template <>
struct MurmurHash<float64>
{
	uint32 operator() (const float64 Source) const
	{
		uint32 Hash = 0;
		MurmurHash32(&Source, sizeof(float64), &Hash);
		return Hash;
	}
};


#endif // !LEARNVK_HASH