#pragma once
#ifndef LEARNVK_ITERATOR
#define LEARNVK_ITERATOR

#include "Library/Templates/Templates.h"

/**
* Forward Array Iterator.
*/
template <typename Type>
class ArrayIterator
{
private:

	using ConstType = const Type;
	Type* Pointer;

public:

	ArrayIterator() : 
		Pointer(nullptr) {}
		
	~ArrayIterator() 
	{ 
		Pointer = nullptr; 
	}

	ArrayIterator(Type* Rhs) :
		Pointer(Rhs) {}

	ArrayIterator(const ArrayIterator& Rhs)
	{
		*this = Rhs;
	}

	ArrayIterator(ArrayIterator&& Rhs)
	{
		*this = Move(Rhs);
	}

	ArrayIterator& operator= (const ArrayIterator& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs) 
			Pointer = Rhs.Pointer;
			
		return *this;
	}

	ArrayIterator& operator= (ArrayIterator&& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs) 
		{
			Pointer = Rhs.Pointer;
			new (&Rhs) ArrayIterator();
		}

		return *this;
	}

	ArrayIterator& operator++ () 
	{
		++Pointer;
		return *this;
	}

	ArrayIterator& operator-- () 
	{
		--Pointer;
		return *this;
	}

	bool operator!= (const ArrayIterator& Rhs) const 
	{
		return Pointer != Rhs.Pointer;
	}

	Type& operator* ()
	{
		return *Pointer;
	}

	ArrayIterator& operator+ (size_t Offset) 
	{
		Pointer += Offset;
		return *this;
	}

	ArrayIterator& operator+ (const ArrayIterator& Rhs) const
	{
		return Pointer + Rhs.Pointer;
	}

	ArrayIterator& operator- (size_t Offset) 
	{
		Pointer -= Offset;
		return *this;
	}

	ArrayIterator& operator- (const ArrayIterator& Rhs) const
	{
		return Pointer - Rhs.Pointer;
	}

	Type& operator[] (size_t Index) 
	{
		return *(Pointer + Index);
	}
};


/**
* Forward Array Const Iterator
*/
template <typename Type>
class ConstArrayIterator
{
private:
	using ConstType = const Type;
	ConstType* Pointer;

public:

	ConstArrayIterator() :
		Pointer(nullptr) {}

	~ConstArrayIterator()
	{
		Pointer = nullptr;
	}

	ConstArrayIterator(ConstType* Rhs) :
		Pointer(Rhs) {}

	ConstArrayIterator(const ConstArrayIterator& Rhs)	{ *this = Rhs; }
	ConstArrayIterator(ConstArrayIterator&& Rhs)		{ *this = Move(Rhs); }

	ConstArrayIterator& operator= (const ConstArrayIterator& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs)
			Pointer = Rhs.Pointer;

		return *this;
	}

	ConstArrayIterator& operator= (ConstArrayIterator&& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs)
		{
			Pointer = Rhs.Pointer;
			new (&Rhs) ConstArrayIterator();
		}

		return *this;
	}

	ConstArrayIterator& operator++ ()
	{
		++Pointer;
		return *this;
	}

	ConstArrayIterator& operator-- ()
	{
		--Pointer;
		return *this;
	}

	bool operator!= (const ConstArrayIterator& Rhs) const 
	{ 
		return Pointer != Rhs.Pointer; 
	}

	ConstType& operator* () const
	{
		return *Pointer; 
	}

	ConstArrayIterator& operator+ (size_t Offset)
	{
		Pointer += Offset;
		return *this;
	}

	ConstArrayIterator& operator+ (const ConstArrayIterator& Rhs) const
	{
		return Pointer + Rhs.Pointer;
	}

	ConstArrayIterator& operator- (size_t Offset)
	{
		Pointer -= Offset;
		return *this;
	}

	ConstArrayIterator& operator- (const ConstArrayIterator& Rhs) const
	{
		return Pointer - Rhs.Pointer;
	}

	ConstType& operator[] (size_t Index) const
	{
		return *(Pointer + Index);
	}
};


/**
* Node iterator for linked list.
* Supports bidirectional linked lists.
* 
* Should in theory support red black trees or any containers that uses Nodes.
*/
template <typename NodeType, typename ElementType>
class NodeIterator
{
private:

	NodeType* Node;

public:

	NodeIterator() :
		Node(nullptr) {}

	~NodeIterator()
	{
		Node = nullptr;
	}

	NodeIterator(NodeType* Rhs) :
		Node(Rhs) {}

	NodeIterator(const NodeIterator& Rhs)
	{
		*this = Rhs;
	}

	NodeIterator(NodeIterator&& Rhs)
	{
		*this = Move(Rhs);
	}

	NodeIterator& operator= (const NodeIterator& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs)
			Node = Rhs.Node;

		return *this;
	}

	NodeIterator& operator= (NodeIterator&& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs)
		{
			Node = Rhs.Node;
			new (&Rhs) NodeIterator();
		}

		return *this;
	}

	NodeIterator& operator++ ()
	{
		Node = Node->Next;
		return *this;
	}

	NodeIterator& operator-- ()
	{
		Node = Node->Previous;
		return *this;
	}

	bool operator!= (const NodeIterator& Rhs) const
	{
		return Node != Rhs.Node;
	}

	ElementType& operator* () const
	{
		return Node->Data;
	}

};


template <typename Type>
class DequeIterator
{
private:

	Type*	Pointer;
	size_t	Offset;
	size_t	TotalSize;

public:

	DequeIterator() :
		Pointer(nullptr), Offset(0), TotalSize(0)
	{}

	DequeIterator(Type* DataPointer) :
		Pointer(DataPointer), Offset(0), TotalSize(0)
	{}

	DequeIterator(Type* DataPointer, size_t FrontOffset, size_t TotalCapacity) :
		Pointer(DataPointer), Offset(FrontOffset), TotalSize(TotalCapacity)
	{}

	~DequeIterator()
	{}

	DequeIterator(const DequeIterator& Rhs) { *this = Rhs; }
	DequeIterator(DequeIterator&& Rhs) { *this = Move(Rhs); }

	DequeIterator& operator= (const DequeIterator& Rhs)
	{
		if (this != &Rhs)
		{
			Pointer = Rhs.Pointer;
			TotalSize = Rhs.TotalSize;
		}
		return *this;
	}

	DequeIterator& operator= (DequeIterator&& Rhs)
	{
		if (this != &Rhs)
		{
			Pointer = Move(Rhs.Pointer);
			TotalSize = Move(Rhs.TotalSize);
			new (&Rhs) DequeIterator();
		}
		return *this;
	}

	DequeIterator& operator++ ()
	{
		if (TotalSize)
		{
			Pointer++;
			Offset++;
			
			if (Offset == TotalSize)
			{
				Pointer -= TotalSize;
				Offset = 0;
			}
		}
		else
		{
			Pointer++;
		}

		return *this;
	}

	DequeIterator& operator-- ()
	{
		Pointer--;
		return *this;
	}

	bool operator!= (const DequeIterator& Rhs) const
	{
		return Pointer != Rhs.Pointer;
	}

	Type& operator* () const
	{
		return *Pointer;
	}
};


template <typename ElementNode, typename ElementType>
class HashmapIterator
{
private:

	ElementNode*	Data;
	size_t			NumToIterate;

public:

	HashmapIterator() :
		Data(nullptr), NumToIterate(0)
	{}

	HashmapIterator(ElementNode* ReferencePointer) :
		Data(ReferencePointer)
	{}

	HashmapIterator(ElementNode* ReferencePointer, size_t Iterations) :
		Data(ReferencePointer), NumToIterate(Iterations)
	{}

	~HashmapIterator()
	{
		Data = nullptr;
	}

	HashmapIterator(const HashmapIterator& Rhs) { *this = Rhs; }
	HashmapIterator(HashmapIterator&& Rhs)		{ *this = Move(Rhs); }

	HashmapIterator& operator= (const HashmapIterator& Rhs)
	{
		if (this != &Rhs)
		{
			Data = Rhs.Data;
		}
		return *this;
	}

	HashmapIterator& operator= (HashmapIterator&& Rhs)
	{
		if (this != &Rhs)
		{
			Data = Rhs.Data;
			new (&Rhs) ElementNode();
		}
		return *this;
	}

	HashmapIterator& operator++ ()
	{
		NumToIterate--;
		if (!NumToIterate)
		{
			return *this;
		}
		VKT_ASSERT(Data->Next);
		Data = Data->Next;
		return *this;
	}

	HashmapIterator& operator-- ()
	{
		VKT_ASSERT(Data->Previous);
		Data = Data->Previous;
		return *this;
	}

	bool operator!= (const HashmapIterator& Rhs) const
	{
		if (NumToIterate)
		{
			return true;
		}
		return Data != Rhs.Data;
	}

	ElementType& operator* () const
	{
		return *Data;
	}
};


#endif // !LEARNVK_ITERATOR