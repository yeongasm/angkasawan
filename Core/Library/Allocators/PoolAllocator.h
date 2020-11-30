#pragma once
#ifndef LEARNVK_POOL_ALLOCATOR
#define LEARNVK_POOL_ALLOCATOR

#include "Library/Templates/Templates.h"
#include "Library/Memory/Memory.h"

/**
* The PoolAllocator assumes that you will definitely allocate for only a single type.
* Will crash if used incorrectly ie: using it to allocate different type of objects.
*/
template <typename AllocationPolicy>
class PoolAllocator
{
public:

	struct SingleAlloc		{ enum { Value = 0x01 }; };
	struct StringCompatible { enum { Value = 0x00 }; };

private:

	using Policy = AllocationPolicy;

	template <typename ElementType>
	struct PAStackFreeList
	{
		struct Node
		{
			ElementType	Data;
			Node*		Next;
		};

		Node* StackHead;

		PAStackFreeList() :
			StackHead(nullptr) {}

		~PAStackFreeList() {}

		DELETE_COPY_AND_MOVE(PAStackFreeList)

		void Push(Node* InNode)
		{
			InNode->Next = StackHead;
			StackHead = InNode;
		}

		Node* Pop()
		{
			Node* Top = StackHead;
			StackHead = StackHead->Next;

			return Top;
		}

		void Reset()
		{
			Node* Temp = nullptr;

			while (StackHead != nullptr)
			{
				Temp = StackHead->Next;
				StackHead = nullptr;
				StackHead = Temp;
			}
		}
	};

	struct FreeHeader {};

	using PAFreeList = PAStackFreeList<FreeHeader>;
	using Node = typename PAFreeList::Node;
		
	MemoryBlock	Memory;
	PAFreeList	FreeList;
	size_t		FreeChunks;

	void RequestMemory(size_t Size, size_t SizeOf)
	{
		if (Size)
		{
			Memory.Realloc(Memory.Block, Size);
			Memory.Size = Size;
		}

		uint8* Base = reinterpret_cast<uint8*>(Memory.Block + Memory.Size);
		FreeChunks += Memory.Size / SizeOf;

		for (size_t i = 0; i < FreeChunks; i++)
		{
			FreeList.Push(reinterpret_cast<Node*>(Base + i * SizeOf));
		}
	}

public:

	PoolAllocator() :
		Memory(), FreeList(), FreeChunks(0)
	{}

	~PoolAllocator() 
	{
		Release();
	}

	DELETE_COPY_AND_MOVE(PoolAllocator)


	template <typename T>
	T* GetData()
	{
		return reinterpret_cast<T*>(Memory.Block);
	}


	template <typename T>
	const T* GetData() const
	{
		return reinterpret_cast<T*>(Memory.Block);
	}

	void* Alloc(size_t SizeOf, size_t Alignment = 0xDEAD)
	{
		if (!FreeChunks)
		{
			RequestMemory(Memory.Size + Policy::Slack, SizeOf);
		}

		Node* Result = FreeList.Pop();
		FreeChunks--;

		return reinterpret_cast<void*>(Result);
	}

	void Free(void* Pointer, size_t Size)
	{
		if (!Pointer) return;
		FMemory::Memzero(Pointer, Size);
		FreeList.Push(reinterpret_cast<Node*>(Pointer));
		FreeChunks++;
	}
		
	template <typename T>
	T* New()
	{
		T* Result = reinterpret_cast<T*>(Alloc(sizeof(T)));
		new (Result) T();
		return Result;
	}

	template <typename T, class... Args>
	T* New(Args&&... Arguments)
	{
		T* Result = reinterpret_cast<T*>(Alloc(sizeof(T)));
		new (Result) T(Forward<Args>(Arguments)...);
		return Result;
	}

	template <typename T>
	void Delete(T* Pointer)
	{
		Pointer->~T();
		Free(Pointer, sizeof(T));
	}

	void Flush()
	{
		FreeChunks = 0;
		FreeList.Reset();
		FMemory::Memzero(Memory.Block, Memory.Size);
		RequestMemory(0);
	}

	void Release()
	{
		Memory.Free();
		new (this) PoolAllocator();
	}

};

#endif // !LEARNVK_POOL_ALLOCATOR