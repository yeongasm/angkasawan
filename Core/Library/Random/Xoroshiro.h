#pragma once
#ifndef LEARNVK_LIBRARY_RANDOM_XOROSHIRO
#define LEARNVK_LIBRARY_RANDOM_XOROSHIRO

#include "Library/Memory/Memory.h"
#include "Library/Templates/Templates.h"

template <typename DataWidth>
class XoroshiroBase
{
private:

	inline uint32 Rotl(const uint32 x, int32 k)
	{
		return (x << k) | (x >> (32 - k));
	}

	class SplitMix64
	{
	private:

		uint64 Seed;

	public:

		SplitMix64(uint64 Seed) : 
			Seed(Seed)
		{}

		~SplitMix64() 
		{}

		DELETE_COPY_AND_MOVE(SplitMix64)

		uint64 operator() ()
		{
			uint64 Result = (Seed + 0x9e3779b97f4a7c15);
			Result = (Result ^ (Result >> 30)) * 0xbf58476d1ce4e5b9;
			Result = (Result ^ (Result >> 27)) * 0x94d049bb133111eb;

			return Result ^ (Result >> 31);
		}
	};

	uint32 s[4];

public:

	XoroshiroBase() : s{}
	{}

	XoroshiroBase(uint64 Seed)
	{
		SplitMix64 Seeder(Seed);
		s[0] = static_cast<uint32>(Seeder());
		s[1] = static_cast<uint32>(Seeder());
		s[2] = static_cast<uint32>(Seeder());
		s[3] = static_cast<uint32>(Seeder());
	}

	~XoroshiroBase()
	{}

	XoroshiroBase(const XoroshiroBase& Rhs) { *this = Rhs; }
	XoroshiroBase(XoroshiroBase&& Rhs)		{ *this = Move(Rhs); }

	XoroshiroBase& operator= (const XoroshiroBase& Rhs)
	{
		if (this != &Rhs)
		{
			FMemory::Memcpy(s, Rhs.s, 4);
		}
		return *this;
	}

	XoroshiroBase& operator= (XoroshiroBase&& Rhs)
	{
		if (this != &Rhs)
		{
			FMemory::Memcpy(s, Rhs.s, 4);
			new (&Rhs) XoroshiroBase();
		}
		return *this;
	}

	DataWidth operator() ()
	{
		uint32_t Result = s[0] + s[3];
		const uint32_t T = s[1] << 9;

		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];

		s[2] ^= T;

		s[3] = Rotl(s[3], 11);

		return static_cast<DataWidth>(Result);
	}

	/**
	* This is the jump function for the generator. It can be used to generate 2^64 non-overlapping subsequences for parallel computations.
	*/
	void Jump()
	{
		static const uint32_t JUMP[] = { 0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b };

		uint32_t s0 = 0;
		uint32_t s1 = 0;
		uint32_t s2 = 0;
		uint32_t s3 = 0;
		for (int i = 0; i < sizeof JUMP / sizeof * JUMP; i++)
			for (int b = 0; b < 32; b++) {
				if (JUMP[i] & UINT32_C(1) << b) {
					s0 ^= s[0];
					s1 ^= s[1];
					s2 ^= s[2];
					s3 ^= s[3];
				}
				operator()();
			}

		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
	}

	uint32_t Min()
	{
		return 0;
	}

	uint32_t Max()
	{
		return ~DataWidth(0);
	}
};


using Xoroshiro32 = XoroshiroBase<uint32>;
using Xoroshiro64 = XoroshiroBase<uint64>;


#endif // !LEARNVK_LIBRARY_RANDOM_XOROSHIRO