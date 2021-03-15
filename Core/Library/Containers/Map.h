#pragma once
#ifndef LEARNVK_HASH_MAP
#define LEARNVK_HASH_MAP

#include "Library/Algorithms/Hash.h"
#include "Pair.h"
#include "Array.h"


template <typename KeyType, typename ValueType, typename HashAlgorithm = MurmurHash<KeyType>, size_t BucketSlack = 5>
class Map
{
public:

	enum BucketStatus : uint8
	{
		Bucket_IsEmpty		= 0x00,
		Bucket_IsOccupied	= 0x01,
		Bucket_WasDeleted	= 0x02
	};

	using ElementType = Pair<KeyType, ValueType>;

private:

	struct PairNode : public Pair<KeyType, ValueType>
	{
		using Super = Pair<KeyType, ValueType>;

		BucketStatus	Status;
		PairNode*		Previous;
		PairNode*		Next;

		PairNode() :
			Super(), Status(Bucket_IsEmpty), Previous(nullptr), Next(nullptr)
		{}

		~PairNode()
		{}

		PairNode(const KeyType& Key, const ValueType& Value) :
			Super(Key, Value), Status(Bucket_IsEmpty), Previous(nullptr), Next(nullptr)
		{}

		PairNode(KeyType&& Key, ValueType&& Value) :
			Super(Move(Key), Move(Value)), Status(Bucket_IsEmpty), Previous(nullptr), Next(nullptr)
		{}

		PairNode(const Super& Pair) :
			Super(Pair), Status(Bucket_IsEmpty), Previous(nullptr), Next(nullptr)
		{}

		PairNode(Super&& Pair) :
			Super(Move(Pair)), Status(Bucket_IsEmpty), Previous(nullptr), Next(nullptr)
		{}

		PairNode(const PairNode& Rhs)	{ *this = Rhs; }
		PairNode(PairNode&& Rhs)		{ *this = Move(Rhs); }

		PairNode& operator= (const PairNode& Rhs)
		{
			if (this != &Rhs)
			{
				Super::operator=(Rhs);
				Status		= Rhs.Status;
				Previous	= Rhs.Previous;
				Next		= Rhs.Next;
			}

			return *this;
		}

		PairNode& operator= (PairNode&& Rhs)
		{
			if (this != &Rhs)
			{
				Super::operator=(Move(Rhs));
				Status		= Rhs.Status;
				Previous	= Rhs.Previous;
				Next		= Rhs.Next;
				new (&Rhs) PairNode();
			}

			return *this;
		}
	};

	using This			= Map<KeyType, ValueType, HashAlgorithm, BucketSlack>;
	using ElementNode	= PairNode;
	using ContainerType = Array<ElementNode, BucketSlack>;
	using HashFunc		= HashAlgorithm;

	ContainerType	Entries;
	size_t			NumBuckets;
	ElementNode*	First;
	ElementNode*	Last;

public:

	using Iterator		= HashmapIterator<ElementNode, ElementType>;
	using ConstIterator = const Iterator;

	Map() :
		Entries(), NumBuckets(0), First(nullptr), Last(nullptr)
	{}

	~Map()
	{
		Release();
	}

	Map(const Map& Rhs) { *this = Rhs; }
	Map(Map&& Rhs)		{ *this = Move(Rhs); }

	Map& operator= (const Map& Rhs)
	{
		if (this != &Rhs)
		{
			Entries		= Rhs.Entries;
			NumBuckets	= Rhs.NumBuckets;
		}

		return *this;
	}

	Map& operator= (Map&& Rhs)
	{
		if (this != &Rhs)
		{
			Entries		= Move(Rhs.Entries);
			NumBuckets	= Rhs.NumBuckets;

			new (&Rhs) Map();
		}

		return *this;
	}

private:

		
	size_t Probe(size_t x, size_t b = 1, size_t a = 0)
	{
		return a * (x * x) + b * x;
	}


	size_t ProbeForIndex(const size_t Hash, const size_t X)
	{
		return (Hash + Probe(X)) % Entries.Size();
	}


	template <class... ForwardType>
	ElementNode& StoreObjectInMap(ForwardType&&... Element)
	{
		if (!NumBuckets)
		{
			NumBuckets = 1 << BucketSlack;
			size_t NewCapacity = Entries.Size() + NumBuckets;
			Entries.Reserve(NewCapacity);
		}

		HashFunc Func;
		size_t Constant = 0;
		size_t Hash		= static_cast<size_t>(Func(KeyType(Element.Key)...));
		size_t Index	= ProbeForIndex(Hash, Constant);
			
		ElementNode* Entry = &Entries[Index];

		while (Entry->Status == Bucket_IsOccupied)
		{
			Index = ProbeForIndex(Hash, ++Constant);
			Entry = &Entries[Index];
		}
		
		//*Entry = ElementNode(Forward<ForwardType>(Element)...);
		new (Entry) ElementNode(Element...);
		Entry->Status = Bucket_IsOccupied;
		NumBuckets--;

		if (!First && !Last)
		{
			First = Last = Entry;
		}
		else
		{
			Last->Next = Entry;
			Entry->Previous = Last;
			Last = Entry;
		}

		return *Entry;
	}


	ElementNode& FindObjectWithKey(const KeyType& Key, bool ShiftToTombStone = true)
	{
		HashFunc Func;
		size_t TombStone = -1;
		size_t Constant = 0;
		size_t Hash		= static_cast<size_t>(Func(Key));
		size_t Index	= ProbeForIndex(Hash, Constant);

		ElementNode* Element = &Entries[Index];

		//VKT_ASSERT(Element->Status != Bucket_IsEmpty && Element->Status != Bucket_WasDeleted);

		while (Element->Key != Key)
		{
			if (Element->Status == Bucket_WasDeleted && TombStone == -1)
			{
				TombStone = Index;
			}

			Index = ProbeForIndex(Hash, ++Constant);
			Element = &Entries[Index];

			//VKT_ASSERT(Element->Status != Bucket_IsEmpty && Element->Status != Bucket_WasDeleted);
		}

		if (TombStone != -1 && ShiftToTombStone)
		{
			Entries[TombStone] = Move(*Element);
			Element = &Entries[TombStone];
		}

		return *Element;
	}


	void RemoveElementWithKey(const KeyType& Key)
	{
		ElementNode& Element = FindObjectWithKey(Key, false);
		
		if (Last == &Element)
		{
			Last = Last->Previous;
		}
		
		if (First == &Element)
		{
			First = First->Next;
		}

		Element.Key.~KeyType();
		Element.Value.~ValueType();

		new (&Element) ElementNode();

		Element.Status = Bucket_WasDeleted;

		NumBuckets++;
	}


public:

	void Release()
	{
		Entries.Release();
		NumBuckets = 0;
	}

	void Empty()
	{
		Entries.Empty();
		NumBuckets = Entries.Size();
	}

	ValueType& operator[] (const KeyType& Key)
	{
		return Get(Key);
	}


	const ValueType& operator[] (const KeyType& Key) const
	{
		return Get(Key);
	}


	ElementType& Add(const Pair<KeyType, ValueType>& Element)
	{
		ElementType& Result = StoreObjectInMap(Element);
		return Result;
	}


	ElementType& Add(ElementType&& Element)
	{
		ElementType& Result = StoreObjectInMap(Move(Element));
		return Result;
	}


	ElementType& Add(const KeyType& Key, const ValueType& Value)
	{
		ElementType& Result = StoreObjectInMap(ElementType(Key, Value));
		return Result;
	}


	ElementType& Add(KeyType&& Key, ValueType&& Value)
	{
		ElementType& Result = StoreObjectInMap(ElementNode(Move(Key), Move(Value)));
		return Result;
	}


	ValueType& Insert(const ElementType& Element)
	{
		return Add(Element).Value;
	}


	ValueType& Insert(ElementType&& Element)
	{
		return Add(Move(Element)).Value;
	}

	ValueType& Insert(const KeyType& Key, const ValueType& Value)
	{
		return Add(Key, Value).Value;
	}

	ValueType& Insert(KeyType&& Key, ValueType&& Value)
	{
		return Add(Move(Key), Move(Value)).Value;
	}

	/**
	* Removes an element with the specified key.
	* Does nothing if the key does not exist.
	*/
	void Remove(const KeyType& Key)
	{
		RemoveElementWithKey(Key);
	}

	/**
	* Removes an element with the specified key.
	* Does nothing if the key does not exist.
	*/
	void Remove(KeyType&& Key)
	{
		RemoveElementWithKey(Move(Key));
	}

	ValueType& Get(const KeyType& Key)
	{
		ElementType& Element = FindObjectWithKey(Key);
		return Element.Value;
	}


	ElementType& GetPair(const KeyType& Key)
	{
		ElementType& Element = FindObjectWithKey(Key);
		return Element;
	}


	void Reserve(size_t Size)
	{
		Entries.Reserve(Size);
		NumBuckets = Entries.Size();
	}

	/**
	* Checks if the map is empty.
	*/
	bool IsEmpty() const
	{
		return NumBuckets == Entries.Size();
	}

	/**
	* Returns the total number of elements in the map.
	*/
	size_t Length() const
	{
		return Entries.Size() - NumBuckets;
	}

	Iterator		begin()			{ return Iterator(First, Length()); }
	ConstIterator	begin() const	{ return Iterator(First, Length()); }

	Iterator		end()			{ return Iterator(Last); }
	ConstIterator	end()	const	{ return Iterator(Last); }

};

#endif // !LEARNVK_HASH_MAP