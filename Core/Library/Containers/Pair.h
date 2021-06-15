#pragma once
#ifndef LEARNVK_PAIR
#define LEARNVK_PAIR

#include "Library/Templates/Templates.h"

template <typename KeyType, typename ValueType>
struct Pair
{
	KeyType		Key;
	ValueType	Value;

	Pair() :
		Key(), Value() {}

	~Pair() { Free(); }

	Pair(const KeyType& Key, const ValueType& Value) :
		Key(Key), Value(Value) {}

	Pair(KeyType&& Key, ValueType&& Value) :
		Key(Move(Key)), Value(Move(Value)) {}

	Pair(const std::initializer_list<Pair<KeyType, ValueType>>& InitList) :
		Key(InitList[0]), Value(InitList[1]) {}

	Pair(std::initializer_list<Pair<KeyType, ValueType>>&& InitList) :
		Key(Move(InitList[0])), Value(Move(InitList[1])) {}

	Pair(const Pair& Rhs)	{ *this = Rhs; }
	Pair(Pair&& Rhs)		{ *this = Move(Rhs); }


	Pair& operator= (const Pair& Rhs)
	{
		if (this != &Rhs) {
			Key = Rhs.Key;
			Value = Rhs.Value;
		}
		return *this;
	}

	Pair& operator= (Pair&& Rhs)
	{
		// Will throw an assertion in debug if you are copying the object to itself.
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs) {
			Key = Move(Rhs.Key);
			Value = Move(Rhs.Value);
		}
		return *this;
	}

	bool operator== (const Pair& Rhs) const { return Key == Rhs.Key; }
	bool operator!= (const Pair& Rhs) const { return Key != Rhs.Key; }
	bool operator<	(const Pair& Rhs) const { return Key < Rhs.Key;	 }

	void Free()
	{
		Key.~KeyType();
		Value.~ValueType();
	}
};


#endif // !LEARNVK_PAIR
