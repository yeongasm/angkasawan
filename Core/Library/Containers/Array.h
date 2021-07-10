#pragma once
#ifndef ANGKASA1_ARRAY
#define ANGKASA1_ARRAY

#include "Library/Memory/Memory.h"
#include "Library/Containers/Iterator.h"

namespace astl
{

  template <bool Expression>
  struct IsContainerDynamicallyAllocated
  {
    using IsDynamic = IntegralConstant<bool, Expression>;
  };

  template <typename T, typename UintWidth>
  class BaseArray
  {
  protected:

    using Type = T;
    using width_t = UintWidth;

    width_t Len;
    width_t Capacity;

    void Destruct(Type* pData, width_t FromIdx = 0, width_t ToIdx = Len, bool Reconstruct = false)
    {
      for (width_t i = FromIdx; i < ToIdx; i++)
      {
        pData[i].~Type();
        if (Reconstruct)
        {
          new (pData + i) Type();
        }
      }
    }

    template <typename... ForwardType>
    width_t Emplace(T* pData, ForwardType&&... Args)
    {
      new (pData + Len) Type(Forward<ForwardType>(Args)...);
      return Len++;
    }

    template <typename... ForwardType>
    decltype(auto) EmplaceAt(T* pData, width_t Index, ForwardType&&... Args)
    {
      Destruct(pData, Index, Index + 1);
      new (pData + Index) Type(Forward<ForwardType>(Args)...);
      if (Index > Len)
      {
        Len = Index + 1;
      }
      return pData[Index];
    }
  public:
    width_t GetSlack() const { return 0; }
    void Reserve(width_t) {}
  };

  template <typename T, typename UintWidth = size_t, UintWidth Slack = 5>
  class Array final : public BaseArray<T, UintWidth>, public IsContainerDynamicallyAllocated<true>
  {
  private:
    using Type = T;
    using width_t = UintWidth;
    using Super = BaseArray<T, width_t>;
    using Iterator = ArrayIterator<Type>;
    using ConstIterator = ArrayIterator<const Type>;
    using ReverseIterator = ReverseArrayIterator<Type>;
    using ConstReverseIterator = ReverseArrayIterator<const Type>;

    Type* Data;

    using Super::Capacity;
    using Super::Len;

    void Grow(width_t Size = 0)
    {
      Type* Old = Data;
      Type* New = nullptr;

      Capacity = Capacity + GetSlack();

      if (Size) { Capacity = Size; }

      New = reinterpret_cast<Type*>(IMemory::Realloc(Old, sizeof(Type) * Capacity));

      for (width_t i = Len; i < Capacity; i++)
      {
        new (New + i) Type();
      }

      Data = New;
    }

  public:

    Array() :
      Super{}, Data{ nullptr } {}

    ~Array()
    {
      Release();
    }

    Array(size_t Length) : Array()
    {
      Grow(Length);
    }

    Array(const std::initializer_list<Type>& InitList) :
      Array()
    {
      Append(InitList);
    }

    Array(const Array& Rhs) { *this = Rhs; }
    Array(Array&& Rhs) { *this = Move(Rhs); }

    Array& operator= (const Array& Rhs)
    {
      if (this != &Rhs)
      {
        Super::Destruct(Data, 0, Len);

        if (Capacity < Rhs.Capacity)
        {
          Grow(Rhs.Capacity);
        }

        Len = Rhs.Len;
        for (width_t i = 0; i < Rhs.Len; i++)
        {
          Data[i] = Rhs[i];
        }
      }

      return *this;
    }

    Array& operator= (Array&& Rhs)
    {
      if (this != &Rhs)
      {
        Super::Destruct(Data, 0, Len);

        if (Data)
        {
          Release();
        }

        Len = Rhs.Len;
        Capacity = Rhs.Capacity;
        Data = Rhs.Data;
        new (&Rhs) Array();
      }

      return *this;
    }

    /**
    * CHANGELOG:
    * 09.08.2020, 04:22AM	- The container no longer supports dynamic allocations through the [] operator.
    */
    Type& operator[] (width_t Index)
    {
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array!" && Index < Capacity);
      if (Len <= Index && Index < Capacity)
      {
        Len = Index + 1;
      }
      return Data[Index];
    }

    const Type& operator[] (width_t Index) const
    {
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array." && Index < Capacity);
      return Data[Index];
    }

    width_t GetSlack() const { return 1 << Slack; }

    /**
    * Allocate the specified amount of space for the Array increasing the internal capacity.
    * If the specified size is smaller than the array's existing capacity, it preserves it's previous capacity but removes elements beyond the specified size.
    *
    * @param Size - The desired newly allocated internal buffer size.
    */
    void Reserve(width_t Size)
    {
      if (Size < Capacity)
      {
        Super::Destruct(Data, Size, Capacity);
      }
      else
      {
        Grow(Size);
      }
    }

    /**
    * Free all the elements in the Array, releasing it's contents.
    */
    void Release()
    {
      Super::Destruct(Data, 0, Capacity);
      if (Data)
      {
        IMemory::Free(Data);
      }
      new (this) Array();
    }

    template <typename... ForwardType>
    size_t Emplace(ForwardType&&... Args)
    {
      if (Len == Capacity) { Grow(); }
      return Super::Emplace(Data, Forward<ForwardType>(Args)...);
    }

    template <typename... ForwardType>
    decltype(auto) EmplaceAt(width_t Index, ForwardType&&... Args)
    {
      VKT_ASSERT((Index < Capacity) && "Index specified exceeded array's capacity");
      if (Len == Capacity) { Grow(); }
      Super::Destruct(Data, Index, Index + 1);
      return Super::EmplaceAt(Data, Index, Forward<ForwardType>(Args)...);
    }

    width_t Push(const Type& Element)
    {
      return Emplace(Element);
    }

    /**
    * Push an element into the the back of the Array.
    * If elements were assigned via the static method and there are empty slots within the Array, the Array class would
    * ignore the index of the gap and push new elements to the back of the Array.
    * First-In-Last-Out implementation.
    * Returns the index of the newly allocated Element within the Array.
    *
    * @param Element - An element to be pushed into the back of the Array.
    */
    width_t Push(Type&& Element)
    {
      return Emplace(Move(Element));
    }

    decltype(auto) Insert(const Type& Element)
    {
      width_t index = Emplace(Element);
      return Data[index];
    }

    /**
    * Push an element into the the back of the Array.
    * If elements were assigned via the static method and there are empty slots within the Array, the Array class would
    * ignore the index of the gap and push new elements to the back of the Array.
    * First-In-Last-Out implementation.
    * Returns a reference / pointer to the newly allocated Element within the Array.
    *
    * @param Element - An element to be pushed into the back of the Array.
    */
    decltype(auto) Insert(Type&& Element)
    {
      width_t index = Emplace(Move(Element));
      return Data[index];
    }

    /**
    * Appends an existing Array and copy or move it's elements into the back of the Array.
    * Destroys contents from the copied Array and constructs a new one by default.
    * Returns the index of the last element after appending.
    *
    * @param Rhs   - The foreign Array that is being appended.
    */
    width_t Append(const Array& Rhs)
    {
      Append(Rhs.begin(), Rhs.Len);
      return Len;
    }

    /**
    * Appends new elements into the Array with an initializer list.
    * Returns the index of the last element after appending.
    *
    * @param InitList - A foreign initializer list.
    */
    width_t Append(const std::initializer_list<Type>& InitList)
    {
      for (width_t i = 0; i < InitList.size(); i++)
      {
        Push(*(InitList.begin() + i));
      }
      return Len - 1;
    }

    /**
    * Append mutiple elements into the Array by supplying a beginning pointer and the total elements from the foreign container.
    * If the amount inserted exceeds the the internal buffer of the Array, the Array memory reallocation is done.
    * Does not destroy the foreign container's contents.
    * Returns the index of the last element within the Array after appending.
    *
    * @param Begin	- Foreign container's beginning pointer.
    * @param Length	- Foreign container's end pointer.
    */
    width_t Append(ConstIterator It, width_t Length)
    {
      for (width_t i = 0; i < Length; i++)
      {
        Push(It[i]);
      }
      return Len - 1;
    }

    width_t Append(ConstIterator Begin, ConstIterator End)
    {
      for (ConstIterator it = Begin; it != End; ++it)
      {
        Emplace(*it);
      }
      return Len - 1;
    }

    /**
    * Pop an element from the back of the Array.
    * Last-In-First-Out implementation.
    * Returns the last index in the element.
    *
    * @param Count	  - The amount of element(s) to be removed. [Default] 1.
    * @param Shrink	  - Reduce the size of the internal buffer allocator for the Array. [Default] false.
    */
    width_t Pop(width_t Count = 1)
    {
      VKT_ASSERT(Len >= 0);
      if (!Len) { return 0; }
      Super::Destruct(Data, Len - Count, Len, true);
      Len -= Count;
      Len = (Len < 0) ? 0 : Len;

      return Len;
    }

    /**
    * Pops an element at the specified index.
    * Optionally moves the contents of the Array. Optionally shrinks the length of the Array.
    * Reconstructs a fresh state in that index by calling the object's constructor.
    *
    * @param Idx	- The variable's index within the container.
    * @param Move	- Moves the contents within the container closing any gaps in indices. [Default] true.
    */
    void PopAt(width_t Index, bool Move = true)
    {
      // The element should reside in the container.
      VKT_ASSERT(Index < Len);

      Data[Index].~Type();

      if (Move)
      {
        IMemory::Memmove(Data + Index, Data + Index + 1, sizeof(Type) * (--Len - Index));
        new (Data + Len) Type();
      }
      else
      {
        new (Data + Index) Type();
      }
    }

    /**
    * Pops all elements out of the container.
    * Unlike Release(), Empty() maintains the internal capacity of the Array.
    * Pushing a new element would insert it back at index 0.
    * Optionally, reconstruct the element to it's default state.
    *
    * @param Reconstruct - Reconstruct all elements inside the container to it's default state. [Default] true.
    */
    void Empty(bool Reconstruct = true)
    {
      Super::Destruct(Data, 0, Len, Reconstruct);
      Len = 0;
    }

    /**
    * Shrinks the size of the Array to perfectly fit it's contents.
    * This means storing only enough space to hold the total elements inside the Array.
    * Returns true on success, false on error.
    */
    //bool ShrinkToFit() 
    //{
    //	Alloc(Len);
    //
    //	return true;
    //}

    /**
    * Returns the index of the specified object in the Array by reference.
    * Not to be used for Arrays storing pointers. Check Find() function.
    *
    * @param Element - An element passed in as value.
    */
    inline width_t IndexOf(const Type& Element) const
    {
      return IndexOf(&Element);
    }

    /**
    * Returns the index of the specified object in the Array.
    * Not to be used for Arrays storing pointers. Check Find() function.
    *
    * @param Element - An element passed in by pointer.
    */
    inline width_t IndexOf(const Type* Element) const
    {
      // Object must reside within the boundaries of the Array.
      VKT_ASSERT(Element >= Data && Element < Data + Capacity);

      // The specified object should reside at an address that is an even multiple of of the Type's size.
      VKT_ASSERT(((uint8*)Element - (uint8*)Data) % sizeof(Type) == 0);

      return static_cast<size_t>(Element - Data);
    }

    /**
    * Returns the index of the end-most object within the Array.
    */
    inline width_t Length() const { return Len; }

    /**
    * Returns the internal buffer size allocated inside the Array.
    */
    inline width_t Size() const { return Capacity; }

    /**
    * Returns a pointer to the first element in the Array.
    */
    inline Type* First() { return Data; }
    inline const Type* First() const { return Data; }

    /**
    * Returns a pointer to the last element in the Array.
    * If length of array is 0, Last() returns the 0th element.
    */
    inline Type* Last() { return Data + (Len - 1); }
    inline const Type* Last() const { return Data + (Len - 1); }

    /**
    * Returns a reference to the first element in the Array.
    */
    inline Type& Front() { return *First(); }
    inline const Type& Front() const { return *First(); }
    /**
    * Returns a reference to the first element in the Array.
    * If the length of the Array is 0, Back() returns the 0th element.
    */
    inline Type& Back() { return *Last(); }
    inline const Type& Back() const { return *Last(); }

    Iterator              begin() { return Iterator(Data); }
    Iterator              end() { return Iterator(Data + Len); }
    ConstIterator         begin() const { return ConstIterator(Data); }
    ConstIterator         end() const { return ConstIterator(Data + Len); }
    ReverseIterator       rbegin() { return ReverseIterator(&Data[Len - 1]); }
    ReverseIterator       rend() { return ReverseIterator(Data); }
    ConstReverseIterator  rbegin() const { return ConstReverseIterator(&Data[Len - 1]); }
    ConstReverseIterator  rend() const { return ConstReverseIterator(Data); }
  };


  template <typename T, size_t Count, typename UintWidth = size_t>
  class StaticArray final : public BaseArray<T, UintWidth>, public IsContainerDynamicallyAllocated<false>
  {
  private:
    using Type = T;
    using width_t = UintWidth;
    using Super = BaseArray<T, width_t>;
    using Iterator = ArrayIterator<Type>;
    using ConstIterator = ArrayIterator<const Type>;
    using ReverseIterator = ReverseArrayIterator<Type>;
    using ConstReverseIterator = ReverseArrayIterator<const Type>;
    using Super::Capacity;
    using Super::Len;

    Type Data[Count];

  public:

    StaticArray() :
      BaseArray<T, width_t>{}, Data{}
    {
      Capacity = Count;
    }

    ~StaticArray()
    {
      Release();
    }

    StaticArray(const StaticArray& Rhs) :
      StaticArray()
    {
      *this = Rhs;
    }

    StaticArray(StaticArray&& Rhs) :
      StaticArray()
    {
      *this = Move(Rhs);
    }

    StaticArray& operator= (const StaticArray& Rhs)
    {
      if (this != &Rhs)
      {
        Super::Destruct(Data, 0, Capacity);
        Len = 0;
        for (size_t i = 0; i < Rhs.Len; i++)
        {
          if (Capacity == i)
          {
            break;
          }
          Data[i] = Rhs.Data[i];
          Len++;
        }
      }
      return *this;
    }

    StaticArray& operator= (StaticArray&& Rhs)
    {
      if (this != &Rhs)
      {
        Super::Destruct(Data, 0, Capacity);
        Len = 0;
        for (size_t i = 0; i < Rhs.Len; i++)
        {
          if (Capacity == i)
          {
            break;
          }
          Data[i] = Rhs.Data[i];
          Len++;
        }
        new (&Rhs) StaticArray();
      }
      return *this;
    }

    Type& operator[] (width_t Index)
    {
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array!" && Index < Capacity);
      if (Len <= Index && Index < Capacity)
      {
        Len = Index + 1;
      }
      return Data[Index];
    }

    const Type& operator[] (width_t Index) const
    {
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array." && Index < Capacity);
      return Data[Index];
    }

    template <typename... ForwardType>
    size_t Emplace(ForwardType&&... Args)
    {
      return Super::Emplace(Data, Forward<ForwardType>(Args)...);
    }

    template <typename... ForwardType>
    decltype(auto) EmplaceAt(width_t Index, ForwardType&&... Args)
    {
      VKT_ASSERT((Index < Capacity) && "Index specified exceeded array's capacity");
      Super::Destruct(Data, Index, Index + 1);
      return Super::EmplaceAt(Data, Index, Forward<ForwardType>(Args)...);
    }

    width_t Push(const Type& Element)
    {
      return Emplace(Element);
    }

    width_t Push(Type&& Element)
    {
      return Emplace(Move(Element));
    }

    decltype(auto) Insert(const Type& Element)
    {
      width_t index = Emplace(Element);
      return Data[index];
    }

    decltype(auto) Insert(Type&& Element)
    {
      width_t index = Emplace(Move(Element));
      return Data[index];
    }

    width_t Pop(width_t Count = 1)
    {
      VKT_ASSERT((Len >= 0) || (Len - Count > 0));
      Super::Destruct(Data, Len - Count, Len, true);
      Len -= Count;
      Len = (Len < 0) ? 0 : Len;

      return Len;
    }

    void PopAt(width_t Index, bool Move = true)
    {
      // The element should reside in the container.
      VKT_ASSERT(Index < Len);
      Data[Index].~Type();

      if (Move)
      {
        IMemory::Memmove(Data + Index, Data + Index + 1, sizeof(Type) * (--Len - Index));
        new (Data + Len) Type();
      }
      else
      {
        new (Data + Index) Type();
      }
    }

    void Empty(bool Reconstruct = true)
    {
      Super::Destruct(Data, 0, Len, Reconstruct);
      Len = 0;
    }

    void Release() { Empty(false); }

    inline width_t IndexOf(const Type& Element) const
    {
      return IndexOf(&Element);
    }

    inline width_t IndexOf(const Type* Element) const
    {
      const Type* Base = &Data[0];
      VKT_ASSERT(Element >= Base && Element < Base + Capacity);
      VKT_ASSERT(((uint8*)Element - (uint8*)Base) % sizeof(Type) == 0);
      return static_cast<width_t>(Element - Base);
    }

    width_t Length()  const { return Len; }
    width_t Size()    const { return Capacity; }

    inline Type* First() { return Data; }
    inline const Type* First() const { return Data; }

    inline Type* Last() { return &Data[Len - 1]; }
    inline const Type* Last() const { return &Data[Len - 1]; }

    inline Type& Front() { return *First(); }
    inline const Type& Front() const { return *First(); }
    inline Type& Back() { return *Last(); }
    inline const Type& Back() const { return *Last(); }

    Iterator              begin() { return Iterator(Data); }
    Iterator              end() { return Iterator(&Data[Len]); }
    ConstIterator         begin() const { return ConstIterator(Data); }
    ConstIterator         end() const { return ConstIterator(&Data[Len]); }
    ReverseIterator       rbegin() { return ReverseIterator(&Data[Len - 1]); }
    ReverseIterator       rend() { return ReverseIterator(Data); }
    ConstReverseIterator  rbegin() const { return ConstReverseIterator(&Data[Len - 1]); }
    ConstReverseIterator  rend() const { return ConstReverseIterator(Data); }
  };

}

#endif // !ANGKASA1_ARRAY
