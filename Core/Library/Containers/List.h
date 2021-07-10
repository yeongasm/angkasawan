#pragma once
#ifndef LEARNVK_LINKED_LIST
#define LEARNVK_LINKED_LIST

#include "Library/Memory/Memory.h"
#include "Iterator.h"

namespace astl
{

  /**
  * Doubly Linked List.
  *
  * Insertions and deletions at front and back are O(1).
  * Insertions and deletions at random indices are O(n).
  */
  template <typename InElementType>
  class BaseList
  {
  public:

    using ElementType = InElementType;

  private:

    enum OperationDirection : size_t
    {
      List_Front = 0x00,
      List_Back = 0x01
    };

    struct Node
    {

      ElementType Data;
      Node* Previous;
      Node* Next;

      Node() :
        Data{}, Previous(nullptr), Next(nullptr)
      {}

      ~Node() {}

      Node(const Node& Rhs) { *this = Rhs; }
      Node(Node&& Rhs) { *this = Rhs; }

      Node& operator=(const Node& Rhs)
      {
        Data = Rhs.Data;
        Previous = Rhs.Previous;
        Next = Rhs.Next;
        return *this;
      }

      Node& operator=(Node&& Rhs)
      {
        Data = Move(Rhs.Data);
        Previous = Rhs.Previous;
        Next = Rhs.Next;
        new (&Rhs) Node();
        return *this;
      }
    };

  public:

    using Iterator = NodeIterator<Node, ElementType>;

  private:

    Node* Head;
    Node* Tail;
    size_t		Len;


    template <class... ForwardType>
    ElementType& StoreObject(OperationDirection Direction, ForwardType&&... Element)
    {
      Node* NewNode = reinterpret_cast<Node*>(IMemory::Malloc(sizeof(Node)));
      NewNode->Data = ElementType(Forward<ForwardType>(Element)...);

      if (!Head && !Tail)
      {
        Head = Tail = NewNode;
        Head->Next = Tail;
        Head->Previous = nullptr;
        Tail->Next = nullptr;
        Tail->Previous = Head;
      }
      else
      {
        if (Direction == OperationDirection::List_Back)
        {
          Tail->Next = NewNode;
          NewNode->Previous = Tail;
          Tail = NewNode;
          Tail->Next = nullptr;
        }

        if (Direction == OperationDirection::List_Front)
        {
          NewNode->Next = Head;
          Head->Previous = NewNode;
          Head = NewNode;
          Head->Previous = nullptr;
        }
      }

      Len++;

      return NewNode->Data;
    }


    template <class... ForwardType>
    ElementType& StoreObject(size_t Index, ForwardType&&... Element)
    {
      VKT_ASSERT(Index <= Len - 1);

      Node& It = Head;
      size_t Counter = 0;

      Node* Replaced = TraverseList(Index);

      VKT_ASSERT(Replaced);

      Node* NewNode = reinterpret_cast<Node*>(IMemory::Malloc(sizeof(Node)));
      NewNode->Data = ElementType(Forward<ForwardType>(Element)...);

      NewNode->Previous = Replaced->Previous;
      NewNode->Next = Replaced;
      Replaced->Previous = NewNode;

      Len++;

      return NewNode->Data;
    }


    Node* TraverseList(size_t Index)
    {
      VKT_ASSERT(Index < Len);

      Node* It = Head;
      size_t Counter = 0;

      while (Counter < Len)
      {
        if (Counter == Index)
        {
          break;
        }

        It = It->Next;
        Counter++;
      }

      return It;
    }


  public:

    BaseList() :
      Head(nullptr), Tail(nullptr), Len(0)
    {}

    ~BaseList()
    {
      Release();
    }

    BaseList(const BaseList& Rhs) { *this = Rhs; }
    BaseList(BaseList&& Rhs) { *this = Move(Rhs); }

    BaseList& operator=(const BaseList& Rhs)
    {
      if (this != &Rhs)
      {
        if (Len)
        {
          Release();
        }

        for (const ElementType& Element : Rhs)
        {
          PushBack(Element);
        }
      }

      return *this;
    }

    BaseList& operator=(BaseList&& Rhs)
    {
      if (this != Rhs)
      {
        if (Len)
        {
          Release();
        }

        Head = Move(Rhs.Head);
        Tail = Move(Rhs.Tail);
        Len = Rhs.Len;

        new (&Rhs) BaseList();
      }

      return *this;
    }


    ElementType& operator[] (size_t Index)
    {
      return GetAt(Index);
    }

    const ElementType& operator[] (size_t Index) const
    {
      return GetAt(Index);
    }


    ElementType& GetAt(size_t Index)
    {
      return TraverseList(Index)->Data;
    }

    const ElementType& GetAt(size_t Index) const
    {
      return TraverseList(Index)->Data;
    }


    ElementType& PushFront(const ElementType& Element)
    {
      return StoreObject(List_Front, Element);
    }


    ElementType& PushFront(ElementType&& Element)
    {
      return StoreObject(List_Front, Move(Element));
    }


    ElementType& PushBack(const ElementType& Element)
    {
      return StoreObject(List_Back, Element);
    }


    ElementType& PushBack(ElementType&& Element)
    {
      return StoreObject(List_Back, Move(Element));
    }


    ElementType& Insert(size_t Index, const ElementType& Element)
    {
      return StoreObject(Index, Element);
    }


    ElementType& Insert(size_t Index, ElementType&& Element)
    {
      return StoreObject(Index, Move(Element));
    }


    void PopBack()
    {
      VKT_ASSERT(Len);

      Node* BeforeTail = Tail->Previous;

      IMemory::Free(Tail);
      Tail = Move(BeforeTail);
      Tail->Next = nullptr;

      Len--;
    }


    void RemoveAt(size_t Index)
    {
      VKT_ASSERT(Len && Index <= Len - 1);

      Node* PoppedNode = TraverseList(Index);

      PoppedNode->Previous->Next = PoppedNode->Next;
      PoppedNode->Next->Previous = PoppedNode->Previous;

      IMemory::Free(PoppedNode);

      Len--;
    }


    void PopFront()
    {
      VKT_ASSERT(Len);

      Node* AfterHead = Head->Next;

      IMemory::Free(Head);
      Head = Move(AfterHead);
      Head->Previous = nullptr;

      Len--;
    }


    size_t Length() const
    {
      return Len;
    }


    void Release()
    {
      while (Len)
      {
        Node* BeforeTail = Tail->Previous;
        IMemory::Free(Tail);
        Tail = BeforeTail;

        Len--;
      }

      Head = Tail = nullptr;
    }


    ElementType& Front() const { return Head->Data; }
    ElementType& Back()  const { return Tail->Data; }

    Iterator begin() const { return Iterator(Head); }
    Iterator end()   const { return Iterator(Tail->Next); }

  };


  template <typename InElementType>
  using LinkedList = BaseList<InElementType>;

}

#endif // !LEARNVK_LINKED_LIST
