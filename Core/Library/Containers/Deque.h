#pragma once
#ifndef LEARNVK_DEQUEUE
#define LEARNVK_DEQUEUE

#include "Iterator.h"
#include "Library/Allocators/BaseAllocator.h"
#include "Library/Templates/Templates.h"


enum DequeStoreDirection : size_t
{
	Deque_Store_Front = 0x00,
	Deque_Store_Back  = 0x01
};


template <typename InElementType, size_t DequeSlack = 5>
class Deque
{
public:

	using ElementType	= InElementType;
	using Iterator		= DequeIterator<ElementType>;
	using ConstIterator = const Iterator;

private:

	ElementType*		Data;
	size_t				Capacity;
	size_t				QFront;
	size_t				QBack;


	void Grow(size_t Size = 0)
	{
		const size_t PrevCapacity = Capacity;
		ElementType* OldCopy = Data;

		Capacity = Capacity + (1 << DequeSlack);

		if (Size)
		{
			Capacity = Size;
		}

		ElementType* NewBlock = reinterpret_cast<ElementType*>(IMemory::Malloc(sizeof(ElementType) * Capacity));

		size_t PreviousMiddle = PrevCapacity / 2;
		size_t NewMiddle = Capacity / 2;

		size_t NumFrontData = PreviousMiddle - QFront;
		size_t NumBackData = QBack - PreviousMiddle;
		size_t FrontIndexFromNewMid = NewMiddle;
		size_t BackIndexFromNewMid = NewMiddle;

		// Unique case when there is a the pointer circles ....
		if (QBack == QFront && Capacity)
		{
			size_t TotalData = PrevCapacity - QFront + QBack;
			size_t DataMidBreakPoint = TotalData / 2;

			BackIndexFromNewMid = NewMiddle + (TotalData - DataMidBreakPoint);
			FrontIndexFromNewMid = NewMiddle - DataMidBreakPoint;
		}

		// We need to rearrange the data back linearly if the front index circles to the back.
		if (OldCopy)
		{
			if (QFront < QBack)
			{
				IMemory::Memmove(NewBlock + FrontIndexFromNewMid, OldCopy + QFront, QBack - QFront);
			}
			else
			{
				IMemory::Memmove(NewBlock + FrontIndexFromNewMid, OldCopy + QFront, (PrevCapacity - QFront) * sizeof(ElementType));
				IMemory::Memmove(NewBlock + FrontIndexFromNewMid + (PrevCapacity - QFront), OldCopy, QBack * sizeof(ElementType));
			}

			IMemory::Free(OldCopy);
		}

		QFront = FrontIndexFromNewMid;
		QBack = BackIndexFromNewMid;

		for (size_t i = 0; i < Capacity; i++)
		{
			if ((i >= QFront && i <= QBack - 1) && (QFront != QBack))
				continue;
			
			new (NewBlock + i) ElementType();
		}

		Data = NewBlock;
	}


	void Destruct(size_t From, size_t To, bool Reconstruct = false)
	{
		for (size_t i = From; i < To; i++)
		{
			Data[i].~ElementType();
			if (Reconstruct)
			{
				new (Data + i) ElementType();
			}
		}
	}

	template <typename... ForwardType>
	void StoreObject(DequeStoreDirection Direction, ForwardType&&... Element)
	{
		if (!Capacity || QFront == QBack)
		{
			Grow();
		}

		size_t Index = 0;
		if (Direction == Deque_Store_Front)
		{
			if (!QFront)
				QFront = Capacity;

			Index = --QFront;
		}

		if (Direction == Deque_Store_Back)
		{
			if (QBack == Capacity)
				QBack = 0;

			Index = QBack++;
		}

		Data[Index] = ElementType(Forward<ForwardType>(Element)...);
	}


public:

	Deque() :
		Data(nullptr), Capacity(0), QFront(0), QBack(0)
	{}

	Deque(size_t Length) : Deque()
	{
		Grow(Length);
	}

	~Deque()
	{
		Release();
	}

	Deque(const Deque& Rhs) { *this = Rhs; }
	Deque(Deque&& Rhs)		{ *this = Move(Rhs); }

	ElementType& operator[] (size_t Index)
	{
		return Data[QFront + Index];
	}

	const ElementType& operator[] (size_t Index) const
	{
		return Data[QFront + Index];
	}

	Deque& operator= (const Deque& Rhs)
	{
		if (this != &Rhs)
		{
			Destruct(0, Capacity);
			if (Capacity < Rhs.Capacity)
			{
				Grow(Rhs.Capacity);
			}
			QFront = Rhs.QFront;
			QBack = Rhs.QBack;
			for (size_t i = 0; i < Rhs.Capacity; i++)
			{
				Data[i] = Rhs.Data[i];
			}
		}

		return *this;
	}

	Deque& operator= (Deque&& Rhs)
	{
		if (this != &Rhs)
		{
			Destruct(0, Capacity);
			if (Data)
			{
				IMemory::Free(Data);
			}
			Data = Rhs.Data;
			QFront = Rhs.QFront;
			QBack = Rhs.QBack;
			new (&Rhs) Deque();
		}

		return *this;
	}


	void Release()
	{
		Destruct(0, Capacity);
		if (Data)
		{
			IMemory::Free(Data);
		}
		new (this) Deque();
	}
	
	void PushFront(const ElementType& Element)
	{
		StoreObject(Deque_Store_Front, Element);
	}

	void PushFront(ElementType&& Element)
	{
		StoreObject(Deque_Store_Front, Move(Element));
	}

	template <typename Type>
	decltype(auto) InsertFront(Type&& Element)
	{
		PushFront(Forward<Type>(Element));
		return Data[QFront];
	}

	void PushBack(const ElementType& Element)
	{
		StoreObject(Deque_Store_Back, Element);
	}

	void PushBack(ElementType&& Element)
	{
		StoreObject(Deque_Store_Back, Move(Element));
	}

	template <typename Type>
	decltype(auto) InsertBack(Type&& Element)
	{
		PushBack(Forward<Type>(Element));
		return Data[QBack];
	}

	void PopFront(size_t Count = 1)
	{
		if (QBack < QFront && QFront + Count >= Capacity)
		{
			size_t PopRemainder = QFront + Count - Capacity;
			Destruct(QFront, Capacity, true);
			QFront = 0;
			Destruct(QFront, QFront + PopRemainder, true);
			QFront += PopRemainder;
		}
		else
		{
			Destruct(QFront, QFront + Count, true);
			QFront += Count;
		}
	}

	void PopBack(size_t Count = 1)
	{
		if (QBack < QFront && QBack - Count <= 0)
		{
			size_t PopRemainder = Count - QBack;
			Destruct(0, QBack, true);
			QBack = Capacity - 1;
			Destruct(QBack - PopRemainder, QBack, true);
			QBack -= PopRemainder;
		}
		else
		{
			Destruct(QBack - Count, QBack, true);
			QBack -= Count;
		}
	}

	// done ...
	ElementType& At(size_t Index)
	{
		size_t Idx = QFront + Index;
		if (Idx >= Capacity)
		{
			Idx -= Capacity;
		}
		return Data[Idx];
	}

	// done ...
	const ElementType& At(size_t Index) const
	{
		size_t Idx = QFront + Index;
		if (Idx >= Capacity)
		{
			Idx -= Capacity;
		}
		return Data[Idx];
	}

	size_t Size() const
	{
		return Capacity;
	}

	size_t Length() const
	{
		size_t Length = QBack - QFront;
		if (QBack < QFront)
		{
			Length = QBack + (Capacity - QFront);
		}
		return Length;
	}

	void Empty(bool Reconstruct = true)
	{
		Destruct(0, Capacity, Reconstruct);
		QFront = QBack = Capacity / 2;
	}

	//void Reserve(size_t Size)
	//{
	//	if (Size < Capacity)
	//	{
	//		Destruct()
	//	}
	//	else
	//	{
	//		Alloc(Size);
	//	}
	//}

	ElementType& Front()
	{
		return Data[QFront];
	}

	ElementType& Back()
	{
		return Data[QBack];
	}

	Iterator begin()
	{
		if (QFront > QBack)
		{
			return Iterator(Data + QFront, QFront, Capacity); 
		}
		else
		{
			return Iterator(Data + QFront, 0, 0);
		}
	}

	//ConstIterator begin() const	
	//{
	//	if (QFront > QBack)
	//	{
	//		return Iterator(Data + QFront, QFront, Capacity); 
	//	}
	//	else
	//	{
	//		return Iterator(Data + QFront, 0, 0);
	//	}
	//}

	Iterator end() 
	{
		return Iterator(Data + QBack, 0, 0);
	}

	//ConstIterator end()	const	
	//{ 
	//	return Iterator(Data + QBack, 0, 0);
	//}

};


#endif // !LEARNVK_DEQUEUE