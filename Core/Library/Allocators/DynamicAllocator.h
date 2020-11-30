#pragma once
#ifndef LEARNVK_DYNAMIC_ALLOCATOR
#define LEARNVK_DYNAMIC_ALLOCATOR

#include "BaseAllocator.h"

/**
* Dynamic allocator implementation.
*
* Current does not allow memory re-allocation.
*/
class DynamicAllocator final : protected AllocatorInterface
{
private:

	template <typename T>
	struct FreeLinkedList
	{
		struct Node
		{
			T		Data;
			Node*	Previous;
			Node*	Next;
		};

		Node* Head;

		FreeLinkedList() :
			Head(nullptr) {}

		~FreeLinkedList() {}

		DELETE_COPY_AND_MOVE(FreeLinkedList)

		void Insert(Node* NewNode)
		{
			if (!Head)
			{
				Head = NewNode;
				Head->Previous = nullptr;
				Head->Next = nullptr;

				return;
			}

			NewNode->Previous = Head;
			Head->Next = NewNode;
			Head = NewNode;
		}

		void Remove(Node* ToDeleteNode)
		{
			if (!ToDeleteNode->Previous)
			{
				if (!ToDeleteNode->Next)
				{
					Head = nullptr;
				}
				else
				{
					Head = ToDeleteNode->Next;
					Head->Previous = nullptr;
				}

				return;
			}

			if (!ToDeleteNode->Next)
			{
				ToDeleteNode->Previous->Next = nullptr;

				return;
			}

			ToDeleteNode->Previous->Next = ToDeleteNode->Next;
			ToDeleteNode->Next->Previous = ToDeleteNode->Previous;
		}

		void Reset()
		{
			Node* Temp = nullptr;

			while (Head)
			{
				Temp = Head->Next;
				Head = nullptr;
				Head = Temp;
				Head->Previous = nullptr;
			}
		}
	};

	struct FreeBlock							{ size_t BlockSize; };
	struct AllocationHeader : public FreeBlock	{ uint8 Padding; };

	using FreeList = FreeLinkedList<FreeBlock>;
	using Node = typename FreeList::Node;

	uint8*		Block;
	size_t		Size;
	FreeList	ListOfFreeBlocks;

	size_t CalculatePaddingWithHeader(const uintptr_t BaseAddress, const size_t Alignment)
	{
		size_t Padding = FMemory::CalculatePadding(BaseAddress, Alignment);
		size_t NeededSpace = sizeof(AllocationHeader);

		if (Padding < NeededSpace)
		{
			NeededSpace -= Padding;

			if (NeededSpace % Alignment > 0)
			{
				Padding += Alignment * (1 + (NeededSpace / Alignment));
			}
			else
			{
				Padding += Alignment * (NeededSpace / Alignment);
			}
		}

		return Padding;
	}

	void Coalescence(Node* FreeBlock)
	{
		if (FreeBlock->Next)
		{
			const uintptr_t TailAddress = reinterpret_cast<uintptr_t>(FreeBlock) + FreeBlock->Data.BlockSize;
			const uintptr_t NextAddress = reinterpret_cast<uintptr_t>(FreeBlock->Next);

			if (TailAddress == NextAddress)
			{
				ListOfFreeBlocks.Remove(FreeBlock->Next);
			}
		}

		if (FreeBlock->Previous)
		{
			const uintptr_t HeadAddress = reinterpret_cast<uintptr_t>(FreeBlock->Previous) + FreeBlock->Data.BlockSize;
			const uintptr_t BlockAddress = reinterpret_cast<uintptr_t>(FreeBlock);

			if (HeadAddress == BlockAddress)
			{
				FreeBlock->Previous->Data.BlockSize += FreeBlock->Data.BlockSize;
				ListOfFreeBlocks.Remove(FreeBlock);
			}
		}
	}

	void FindFirst(const size_t Size, const size_t Alignment, size_t& Padding, Node*& FoundNode)
	{
		Node* It = ListOfFreeBlocks.Head;

		while (It)
		{
			Padding = CalculatePaddingWithHeader(reinterpret_cast<uintptr_t>(It), Alignment);
			size_t requiredSpace = Size + Padding;

			if (It->Data.BlockSize >= requiredSpace)
			{
				break;
			}

			It = It->Previous;
		}

		FoundNode = It;
	}

	//void RequestMemory(size_t Size)
	//{
	//	const size_t Offset = this->Size;
	//	size_t Temp = this->Size;
	//		
	//	if (Size)
	//	{
	//		FMemory::Realloc(Block, Size);
	//		Temp = Size - Temp;
	//	}

	//	Node* FreeNode = reinterpret_cast<Node*>(Block + Offset);
	//	FreeNode->Data.BlockSize = Temp;
	//	FreeNode->Previous = nullptr;
	//	FreeNode->Next = nullptr;

	//	ListOfFreeBlocks.Insert(FreeNode);
	//	Coalescence(FreeNode);
	//}

public:

	DynamicAllocator() : Block(nullptr), Size(0), ListOfFreeBlocks{} {}
	~DynamicAllocator() {}

	DELETE_COPY_AND_MOVE(DynamicAllocator)

	//size_t UsedCapacity() const 
	//{
	//	size_t Unused = 0;
	//	Node* It = ListOfFreeBlocks.Head;
	//	while (It)
	//	{
	//		AllocationHeader* header = reinterpret_cast<AllocationHeader*>(It);
	//		Unused += header->BlockSize;
	//		It = It->Previous;
	//	}
	//	return Size - Unused;
	//}

	void Initialize(size_t BlockSize)
	{
		if (Block) return;
		Block = reinterpret_cast<uint8*>(FMemory::Malloc(BlockSize));
		Size = BlockSize;

		Node* FreeNode = reinterpret_cast<Node*>(Block);
		FreeNode->Data.BlockSize = Size;
		FreeNode->Previous = nullptr;
		FreeNode->Next = nullptr;

		ListOfFreeBlocks.Insert(FreeNode);
	}

	void Terminate()
	{
		if (!Block) return;
		FMemory::Free(Block);
		new (this) DynamicAllocator();
	}

	void* Malloc(size_t Size, size_t Alignment = 0x10) override
	{
		constexpr size_t headerSize = sizeof(AllocationHeader);
		constexpr size_t sizeOfNode = sizeof(Node);

		VKT_ASSERT(Size >= sizeOfNode);
		VKT_ASSERT(Alignment >= 8);

		size_t padding = 0;
		Node* freeNode = nullptr;

		FindFirst(Size, Alignment, padding, freeNode);

		VKT_ASSERT("Allocator ran out of space" && freeNode);

		const size_t alignmentPadding = padding - headerSize;
		const size_t totalSize = Size + padding;
		const size_t freeBlockSize = freeNode->Data.BlockSize - totalSize;

		if (freeBlockSize)
		{
			Node* newFreeNode = reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(freeNode) + totalSize);
			newFreeNode->Data.BlockSize = freeBlockSize;
			newFreeNode->Previous = nullptr;
			newFreeNode->Next = nullptr;
			ListOfFreeBlocks.Insert(newFreeNode);
		}

		ListOfFreeBlocks.Remove(freeNode);

		uintptr_t headerAddress = reinterpret_cast<uintptr_t>(freeNode) + alignmentPadding;
		uintptr_t DataAddress = headerAddress + headerSize;

		AllocationHeader* BlockHeader = reinterpret_cast<AllocationHeader*>(HeaderAddress);
		BlockHeader->BlockSize = TotalSize;
		BlockHeader->Padding = static_cast<uint8>(AlignmentPadding);

		void* Result = reinterpret_cast<void*>(DataAddress);
		FMemory::Memzero(Result, AllocationSize);

		return Result;
	}

	void* Alloc(size_t AllocationSize, size_t Alignment = 0x10)
	{
		constexpr size_t HeaderSize = sizeof(AllocationHeader);
		constexpr size_t SizeOfNode = sizeof(Node);

		/**
		* NOTE(Ygsm):
		* Not sure if I want to restrict allocations to being bigger than the size of the node.
		*/
		VKT_ASSERT(AllocationSize >= SizeOfNode);
		VKT_ASSERT(Alignment >= 8);

		size_t Padding = 0;
		Node* FreeNode = nullptr;

		FindFirst(AllocationSize, Alignment, Padding, FreeNode);

		while (!FreeNode)
		{
			RequestMemory(Memory.Size + Policy::Slack);
			FindFirst(AllocationSize, Alignment, Padding, FreeNode);
		}

		const size_t AlignmentPadding = Padding - HeaderSize;
		const size_t TotalSize = AllocationSize + Padding;
		const size_t FreeBlockSize = FreeNode->Data.BlockSize - TotalSize;

		if (FreeBlockSize)
		{
			Node* NewFreeNode = reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(FreeNode) + TotalSize);
			NewFreeNode->Data.BlockSize = FreeBlockSize;
			NewFreeNode->Previous = nullptr;
			NewFreeNode->Next = nullptr;
			FreeList.Insert(NewFreeNode);
		}

		FreeList.Remove(FreeNode);

		uintptr_t HeaderAddress = reinterpret_cast<uintptr_t>(FreeNode) + AlignmentPadding;
		uintptr_t DataAddress = HeaderAddress + HeaderSize;

		AllocationHeader* BlockHeader = reinterpret_cast<AllocationHeader*>(HeaderAddress);
		BlockHeader->BlockSize = TotalSize;
		BlockHeader->Padding = static_cast<uint8>(AlignmentPadding);

		void* Result = reinterpret_cast<void*>(DataAddress);
		FMemory::Memzero(Result, AllocationSize);

		return Result;
	}

	void* Realloc(void* Block, size_t Size)
	{
		void* Result = Alloc(Size);

		if (Block)
		{
			constexpr size_t HeaderSize = sizeof(AllocationHeader);
			const uintptr_t OldBlockAddress = reinterpret_cast<uintptr_t>(Block);
			const uintptr_t OldHeaderAddress = OldBlockAddress - HeaderSize;
			
			const AllocationHeader* OldHeader = reinterpret_cast<AllocationHeader*>(OldHeaderAddress);

			for (size_t i = 0; i < OldHeader->BlockSize; i++)
			{
				if (i > Size)
				{
					break;
				}

				Result[i] = Block[i];
			}

			Free(Block, OldHeader->BlockSize);
		}

		return Result;
	}

	void Free(void* Pointer, size_t Size)
	{
		if (!Pointer) return;

		constexpr size_t HeaderSize = sizeof(AllocationHeader);

		const uintptr_t CurrentAddress = reinterpret_cast<uintptr_t>(Pointer);
		const uintptr_t HeaderAddress = CurrentAddress - HeaderSize;

		const AllocationHeader* Header = reinterpret_cast<AllocationHeader*>(HeaderAddress);

		Node* FreeNode = reinterpret_cast<Node*>(HeaderAddress);
		FreeNode->Data.BlockSize = Header->BlockSize + static_cast<size_t>(Header->Padding);
		FreeNode->Next = nullptr;

		FreeList.Insert(FreeNode);
		Coalescence(FreeNode);

		FMemory::Memzero(Pointer, Size);
	}
		
	void Flush()
	{
		FreeList.Reset();
		FMemory::Memzero(Memory.Block, Memory.Size);
		RequestMemory(0);
	}

	template <typename T, size_t Alignment = 0x10>
	T* New()
	{
		T* Result = reinterpret_cast<T*>(Alloc(sizeof(T), Alignment));
		new (Result) T();
		return Result;
	}

	template <typename T, size_t Alignment = 0x10, typename... Args>
	T* New(Args&&... Arguments)
	{
		T* Result = reinterpret_cast<T*>(Alloc(sizeof(T), Alignment));
		new (Result) T(Forward<Args>(Arguments)...);
		return Result;
	}

	template <typename T>
	void Delete(T* Pointer)
	{
		Pointer->~T();
		Free(Pointer, sizeof(T));
	}

	void Release()
	{
		FreeList.Reset();
		Memory.Free();
	}
};


#endif // !LEARNVK_DYNAMIC_ALLOCATOR