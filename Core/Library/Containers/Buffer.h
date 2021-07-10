#pragma once
#ifndef LEARNVK_BUFFER_H
#define LEARNVK_BUFFER_H

#include "Library/Templates/Templates.h"
#include "Library/Templates/Types.h"
#include "Library/Memory/Memory.h"

namespace astl
{

  template <typename ElementType>
  class Buffer
  {
  public:
    using Type = ElementType;
    using ConstType = const Type;
  private:

    Type* Buf;
    size_t Len;

    void Destruct(size_t From, size_t To, bool Reconstruct)
    {
      for (size_t i = From; i < To; i++)
      {
        Buf[i].~Type();
        if (Reconstruct)
        {
          new (Buf + i) Type();
        }
      }
    }

  public:

    Buffer() :
      Buf(nullptr), Len(0)
    {}

    Buffer(size_t Capacity) :
      Buf(nullptr), Len(0)
    {
      Alloc(Capacity);
    }

    ~Buffer()
    {
      Release();
    }

    Buffer(const Buffer& Rhs)
    {
      *this = Rhs;
    }

    Buffer(Buffer&& Rhs)
    {
      *this = Move(Rhs);
    }

    Buffer& operator=(const Buffer& Rhs)
    {
      if (this != &Rhs)
      {
        if (Rhs.Count() > Count())
        {
          Alloc(Rhs.Count());
        }
        Len = Rhs.Len;
        for (size_t i = 0; i < Count(); i++)
        {
          *this[i] = Rhs[i];
        }
      }
      return *this;
    }

    Buffer& operator=(Buffer&& Rhs)
    {
      if (this != &Rhs)
      {
        Buf = Rhs.Buf;
        Len = Rhs.Len;
        new (&Rhs) Buffer();
      }
      return *this;
    }

    Type& operator[](size_t Index)
    {
      VKT_ASSERT(Index < Len);
      return Buf[Index];
    }

    ConstType& operator[](size_t Index) const
    {
      VKT_ASSERT(Index < Len);
      return Buf[Index];
    }

    operator void* ()
    {
      return Buf;
    }

    operator Type* ()
    {
      return Buf;
    }

    operator ConstType* () const
    {
      return Buf;
    }

    void Alloc(size_t Capacity)
    {
      if (Capacity < Len)
      {
        Destruct(Capacity, Len, false);
      }

      Type* Old = Buf;
      size_t capacity = Capacity * sizeof(Type);
      Buf = reinterpret_cast<Type*>(IMemory::Realloc(Old, capacity));

      if (!Buf) { return; }

      for (size_t i = Len; i < Capacity; i++)
      {
        new (Buf + i) Type();
      }

      Len = Capacity;
    }

    void Flush()
    {
      IMemory::Memzero(Buf, Size());
    }

    void Release()
    {
      if (!Buf) { return; }
      IMemory::Free(Buf);
      new (this) Buffer();
    }

    size_t Size() const
    {
      return Len * sizeof(Type);
    }

    size_t Count() const
    {
      return Len;
    }

    Type* Data()
    {
      return Buf;
    }

    ConstType* ConstData() const
    {
      return Buf;
    }
  };

  using BinaryBuffer = Buffer<uint8>;
  using DWordBuffer = Buffer<uint32>;

}

#endif // !LEARNVK_BUFFER_H
