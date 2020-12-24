#pragma once
#ifndef LEARNVK_CORE_LIBRARY_CONTAINERS_BITSET_H
#define LEARNVK_CORE_LIBRARY_CONTAINERS_BITSET_H

#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"

template <typename Type>
class BitSet
{
private:

	Type Data;

public:

	BitSet() :
		Data(0)
	{}

	BitSet(Type Val) :
		Data(Val)
	{}

	~BitSet()
	{
		Data = 0;
	}

	BitSet(const BitSet& Rhs)
	{
		*this = Rhs;
	}

	BitSet(BitSet&& Rhs)
	{
		*this = Move(Rhs);
	}

	BitSet& operator=(const BitSet& Rhs)
	{
		if (this != &Rhs)
		{
			Data = Rhs.Data;
		}
		return *this;
	}

	BitSet& operator=(BitSet&& Rhs)
	{
		if (this != &Rhs)
		{
			Data = Move(Rhs.Data);
			new (&Rhs) BitSet();
		}
		return *this;
	}

	void Set(Type Bit)
	{
		Data |= (1 << Bit);
	}

	void Remove(Type Bit)
	{
		Data &= ~(1 << Bit);
	}

	bool Has(Type Bit)
	{
		return (Data >> Bit) & 1U;
	}

	void Toggle(Type Bit)
	{
		Data ^= (1 << Bit);
	}
};

#endif // !LEARNVK_CORE_LIBRARY_CONTAINERS_BITSET_H