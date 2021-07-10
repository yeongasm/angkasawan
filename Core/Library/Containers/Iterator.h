#pragma once
#ifndef ANGKASA1_ITERATOR
#define ANGKASA1_ITERATOR

#include "Library/Templates/Templates.h"

namespace astl
{

  template <typename T>
  class BaseArrayIterator
  {
  protected:
    using Type = T;
    using This = BaseArrayIterator<T>;
    mutable Type* Data;

    inline void Increment() { ++Data; }
    inline void Increment(size_t Offset) { Data += Offset; }
    inline void Decrement() { --Data; }
    inline void Decrement(size_t Offset) { Data -= Offset; }

    inline bool Compare(const This& Rhs) const { return Data == Rhs.Data; }

    Type* GetData() const { return Data; }

  public:
    BaseArrayIterator() :
      Data{ nullptr } {}

    BaseArrayIterator(Type* pData) :
      Data{ pData } {}

    ~BaseArrayIterator() { Data = nullptr; }

    BaseArrayIterator(const BaseArrayIterator& Rhs) { *this = Rhs; }
    BaseArrayIterator(BaseArrayIterator&& Rhs) { *this = Move(Rhs); }

    BaseArrayIterator& operator=(const BaseArrayIterator& Rhs)
    {
      VKT_ASSERT(this != &Rhs);
      if (this != &Rhs)
      {
        Data = Rhs.Data;
      }
      return *this;
    }

    BaseArrayIterator& operator=(BaseArrayIterator&& Rhs)
    {
      VKT_ASSERT(this != &Rhs);
      if (this != &Rhs)
      {
        Data = Rhs.Data;
        new (&Rhs) BaseArrayIterator();
      }
      return *this;
    }

    bool operator== (const This& Rhs) const { return  Compare(Rhs); }
    bool operator!= (const This& Rhs) const { return !Compare(Rhs); }

    Type& operator[] (size_t Index) const { return *(Data + Index); }
    Type& operator* () const { return *Data; }
    Type* operator->() const { return Data; }
  };

  template <typename T>
  class ArrayIterator : public BaseArrayIterator<T>
  {
  private:
    using Super = BaseArrayIterator<T>;
  public:
    using Super::BaseArrayIterator;

    ArrayIterator& operator++()
    {
      Super::Increment();
      return *this;
    }

    ArrayIterator& operator--()
    {
      Super::Decrement();
      return *this;
    }

    ArrayIterator& operator+ (size_t Offset)
    {
      Super::Increment(Offset);
      return *this;
    }

    ArrayIterator& operator- (size_t Offset)
    {
      Super::Decrement(Offset);
      return *this;
    }
  };

  template <typename T>
  class ReverseArrayIterator : public BaseArrayIterator<T>
  {
  private:
    using Super = BaseArrayIterator<T>;
  public:
    using Super::BaseArrayIterator;

    ReverseArrayIterator& operator++()
    {
      Super::Decrement();
      return *this;
    }

    ReverseArrayIterator& operator--()
    {
      Super::Increment();
      return *this;
    }

    ReverseArrayIterator& operator+ (size_t Offset)
    {
      Super::Decrement(Offset);
      return *this;
    }

    ReverseArrayIterator& operator- (size_t Offset)
    {
      Super::Increment(Offset);
      return *this;
    }

    bool operator>= (const ReverseArrayIterator& Rhs) const
    {
      return reinterpret_cast<size_t>(Super::GetData()) >= reinterpret_cast<size_t>(Rhs.GetData());
    }

  };

  template <typename Bucket>
  class BaseHashMapIterator
  {
  protected:
    using Type = typename Bucket::Type;
    mutable Bucket* Bckt;

    inline void Forward()  { Bckt = Bckt->Next; }
    inline void Backward() { Bckt = Bckt->Previous; }

  public:
    BaseHashMapIterator() :
      Bckt{ nullptr } {}

    ~BaseHashMapIterator() { Bckt = nullptr; }

    BaseHashMapIterator(Bucket* pBckt) :
      Bckt{ pBckt } {}

    BaseHashMapIterator(const BaseHashMapIterator& Rhs) { *this = Rhs; }
    BaseHashMapIterator(BaseHashMapIterator&& Rhs) { *this = Move(Rhs); }

    BaseHashMapIterator& operator=(const BaseHashMapIterator& Rhs)
    {
      if (this != &Rhs)
      {
        Bckt = Rhs.Bckt;
      }
      return *this;
    }

    BaseHashMapIterator& operator=(BaseHashMapIterator&& Rhs)
    {
      if (this != &Rhs)
      {
        Bckt = Rhs.Bckt;
        new (&Rhs) BaseHashMapIterator();
      }
      return *this;
    }

    bool operator==(const BaseHashMapIterator& Rhs) const
    {
      return Bckt == Rhs.Bckt;
    }
    bool operator!=(const BaseHashMapIterator& Rhs) const
    {
      return Bckt != Rhs.Bckt;
    }
    Type& operator*()   const { return *Bckt; }
    Type* operator->()  const { return Bckt; }
  };

  template <typename Bucket>
  class HashMapIterator final : public BaseHashMapIterator<Bucket>
  {
  private:
    using Super = BaseHashMapIterator<Bucket>;
  public:
    using Super::BaseHashMapIterator;

    HashMapIterator& operator++()
    {
      Super::Forward();
      return *this;
    }

    HashMapIterator& operator--()
    {
      Super::Backward();
      return *this;
    }
  };

  template <typename Bucket>
  class ReverseHashMapIterator final : public BaseHashMapIterator<Bucket>
  {
  private:
    using Super = BaseHashMapIterator<Bucket>;
  public:
    using Super::BaseHashMapIterator;

    ReverseHashMapIterator& operator++()
    {
      Super::Backward();
      return *this;
    }

    ReverseHashMapIterator& operator--()
    {
      Super::Forward();
      return *this;
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

    Type* Pointer;
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

}

#endif // !ANGKASA1_ITERATOR
