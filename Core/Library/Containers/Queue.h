#pragma once
#ifndef LEARNVK_QUEUE
#define LEARNVK_QUEUE

#include "Deque.h"


template <typename InElementType, size_t QueueSlack = 5>
class Queue
{
public:

	using ElementType	= InElementType;
	using Container		= Deque<ElementType, QueueSlack>;
	using Iterator		= typename Container::Iterator;
	//using ConstIterator = typename Container::ConstIterator;

private:

	Container Storage;

public:

	Queue() : Storage{} {}
	~Queue() { Release(); }

	Queue(const Queue& Rhs) { *this = Rhs; }
	Queue(Queue&& Rhs)		{ *this = Move(Rhs); }

	Queue& operator= (const Queue& Rhs)
	{
		if (this != &Rhs)
		{
			Storage = Rhs.Storage;
		}
		return *this;
	}

	Queue& operator= (Queue&& Rhs)
	{
		if (this != &Rhs)
		{
			Storage = Move(Rhs.Storage);
			new (&Rhs) Queue();
		}
		return *this;
	}

	ElementType& Front()	{ return Storage.Front(); }
	ElementType& Back()		{ return Storage.Back(); }

	bool Empty(bool Reconstruct = true) const { Storage.Empty(Reconstruct); }
	
	size_t Size()	const { return Storage.Size(); }
	size_t Length() const { return Storage.Length(); }

	void Enqueue(const ElementType& Object) { Storage.PushBack(Object); }
	void Enqueue(ElementType&& Object)		{ Storage.PushBack(Move(Object)); }

	void Deque	()	{ Storage.PopFront(); }
	void Empty	()	{ Storage.Empty(); }
	void Release()	{ Storage.Release(); }

	Iterator		begin()			{ return Storage.begin(); }
	//ConstIterator	begin() const	{ return Storage.begin(); }

	Iterator		end()			{ return Storage.end(); }
	//ConstIterator	end()	const	{ return Storage.end(); }
};

#endif // !LEANRVK_QUEUE