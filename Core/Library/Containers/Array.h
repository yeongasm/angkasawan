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
        Release();
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
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array!" && Index < Len);
      //if (Len <= Index && Index < Capacity)
      //{
      //  Len = Index + 1;
      //}
      return Data[Index];
    }

    const Type& operator[] (width_t Index) const
    {
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array." && Index < Len);
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

    template <typename... ForwardType>
    void Resize(width_t Count, ForwardType&&... Args)
    {
      if (Count < Len)
      {
        Super::Destruct(Data, Count - 1, Len - 1);
        Len = Count;
      }
      else
      {
        for (width_t i = Len; i < Count; i++)
        {
          Emplace(Forward<ForwardType>(Args)...);
        }
      }
    }

    void Resize(width_t Count)
    {
      Resize(Count, Type());
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
      VKT_ASSERT((Index < Len) && "Index specified exceeded array's length");
      //if (Len == Capacity) { Grow(); }
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
      // The element should reside within the container's length.
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
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array!" && Index < Len);
      //if (Len <= Index && Index < Capacity)
      //{
      //  Len = Index + 1;
      //}
      return Data[Index];
    }

    const Type& operator[] (width_t Index) const
    {
      VKT_ASSERT("The index specified exceeded the internal buffer size of the Array." && Index < Len);
      return Data[Index];
    }

    template <typename... ForwardType>
    void Resize(width_t Count, ForwardType&&... Args)
    {
      VKT_ASSERT((Count < Capacity) && "Count can not exceed capacity for containers of this type");
      if (Count < Len)
      {
        Super::Destruct(Data, Count - 1, Len - 1);
        Len = Count;
      }
      else
      {
        for (width_t i = Len; i < Count; i++)
        {
          Emplace(Forward<ForwardType>(Args)...);
        }
      }
    }

    void Resize(width_t Count)
    {
      Resize(Count, Type());
    }

    template <typename... ForwardType>
    size_t Emplace(ForwardType&&... Args)
    {
      return Super::Emplace(Data, Forward<ForwardType>(Args)...);
    }

    template <typename... ForwardType>
    decltype(auto) EmplaceAt(width_t Index, ForwardType&&... Args)
    {
      VKT_ASSERT((Index < Len) && "Index specified exceeded array's length");
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
      // The element should reside within the container's length.
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

  /**
  * TODO(Ygsm):
  * Battle test this container.
  */
  template <typename T, typename UintWidth = size_t, UintWidth Capacity = 8>
  class SparseArray final : public IsContainerDynamicallyAllocated<true>
  {
  private:

    using Type = T;
    using width_t = UintWidth;

    struct SparseBlock
    {
      Type Data[Capacity];
      width_t Len;
      SparseBlock* Previous;
      SparseBlock* Next;
    };

    SparseBlock* Root;
    SparseBlock* Current;
    width_t Len;
    width_t NumOfBlocks;

    width_t GetTotalCapacity() const
    {
      return NumOfBlocks * Capacity;
    }

    void Grow(width_t Size = 0)
    {
      width_t blockCountBefore = NumOfBlocks;
      SparseBlock* previousBlock = Current;

      width_t allocCount = 1;
      if (Size)
      {
        allocCount = ((Size - GetTotalCapacity()) / Capacity) + 1;
      }

      while (allocCount)
      {
        SparseBlock* pBlock = new SparseBlock();
        if (!Root)
        {
          Root = pBlock;
        }

        // In the case where have to allocate more than 1 block.
        if (blockCountBefore == NumOfBlocks)
        {
          SparseBlock* currBlock = Current;
          Current = pBlock;
          if (currBlock && (currBlock->Len < Capacity))
          {
            Current = currBlock;
          }
        }

        if (previousBlock)
        {
          pBlock->Previous = previousBlock;
          previousBlock->Next = pBlock;
        }

        previousBlock = pBlock;
        NumOfBlocks++;
        allocCount--;
      }
    }

    width_t GetBlockIndex(width_t Index) const
    {
      return Index / Capacity;
    }

    width_t RealIndex(width_t Index, width_t BlockIndex) const
    {
      return Index - (BlockIndex * Capacity);
    }

    SparseBlock* GetBlockWithIndex(width_t Index)
    {
      width_t blockIndex = GetBlockIndex(Index);
      SparseBlock* pBlock = Root;
      for (width_t i = 0; i < blockIndex; i++)
      {
        pBlock = pBlock->Next;
      }
      return pBlock;
    }

    Type& GetElementAtIndex(width_t Index)
    {
      SparseBlock* pBlock = GetBlockWithIndex(Index);
      width_t realIndex = RealIndex(Index, GetBlockIndex(Index));
      return pBlock->Data[realIndex];
    }

    template <typename... ForwardType>
    width_t EmplaceInternal(ForwardType&&... Args)
    {
      new (Current->Data + Current->Len) Type(Forward<ForwardType>(Args)...);
      Current->Len++;
      return Len++;
    }

    void Destruct(width_t From, width_t To, bool Reconstruct = false)
    {
      SparseBlock* pSrcBlock = GetBlockWithIndex(From);
      SparseBlock* pDstBlock = GetBlockWithIndex(To);

      if (pSrcBlock == pDstBlock)
      {
        for (width_t i = From; i < To; i++)
        {
          pSrcBlock->Data[i].~T();
          if (Reconstruct)
          {
            new (pSrcBlock->Data + i) Type();
          }
        }
        return;
      }

      width_t index = RealIndex(To, GetBlockIndex(To));
      width_t count = To - From;

      while (count)
      {
        pDstBlock->Data[index].~T();
        if (Reconstruct)
        {
          new (pDstBlock->Data + index) T();
        }
        index--;
        if (index == -1)
        {
          pDstBlock = pDstBlock->Previous;
          index = Capacity - 1;
        }
        count--;
      }
    }

  public:

    SparseArray() :
      Root{}, Current{}, Len{ 0 }, NumOfBlocks{ 0 }
    {}

    ~SparseArray() { Release(); }

    SparseArray(const SparseArray& Rhs) { *this = Rhs; }
    SparseArray(SparseArray&& Rhs) { *this = Move(Rhs); }

    SparseArray& operator= (const SparseArray& Rhs)
    {
      if (this != &Rhs)
      {
        Empty();
        if (GetTotalCapacity() < Rhs.GetTotalCapacity())
        {
          Reserve(Rhs.GetTotalCapacity());
        }
        const width_t containerLength = Rhs.Length();
        for (width_t i = 0; i < containerLength; i++)
        {
          Emplace(Rhs[i]);
        }
      }
      return *this;
    }

    SparseArray& operator= (SparseArray&& Rhs)
    {
      if (this != &Rhs)
      {
        Release();

        Root = Rhs.Root;
        Current = Rhs.Current;
        Len = Rhs.Len;
        NumOfBlocks = Rhs.NumOfBlocks;

        new (&Rhs) SparseArray();
      }
      return *this;
    }

    void Release()
    {
      Empty();
      for (width_t i = 0; i < NumOfBlocks; i++)
      {
        SparseBlock* pPrevious = Root;
        Root = Root->Next;
        delete pPrevious;
      }
      new (this) SparseArray();
    }

    template <typename... ForwardType>
    width_t Emplace(ForwardType&&... Args)
    {
      if (Len == GetTotalCapacity()) { Grow(); }
      if (Current->Len == GetSlack()) { Current = Current->Next; }
      return EmplaceInternal(Forward<ForwardType>(Args)...);
    }

    template <typename... ForwardType>
    decltype(auto) EmplaceAt(width_t Index, ForwardType&&... Args)
    {
      VKT_ASSERT((Index < Len) && "Index specified exceeded Sparse Array's length");
      Destruct(Index, Index + 1);
      Type& element = GetElementAtIndex(Index);
      new (&element) Type(Forward<ForwardType>(Args)...);
      return element;
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
      return GetElementAtIndex(index);
    }

    decltype(auto) Insert(Type&& Element)
    {
      width_t index = Emplace(Move(Element));
      return GetElementAtIndex(index);
    }

    width_t GetSlack() const { return Capacity; }

    void Reserve(width_t Size)
    {
      if (Size < GetTotalCapacity())
      {
        return Destruct(Size, GetTotalCapacity() - 1);
      }
      Grow(Size);
    }

    template <typename... ForwardType>
    void Resize(width_t Count, ForwardType&&... Args)
    {
      if (Count < Len)
      {
        Destruct(Count - 1, Len - 1);
        Len = Count;
      }
      else
      {
        for (width_t i = Len; i < Count; i++)
        {
          Emplace(Forward<ForwardType>(Args)...);
        }
      }
    }

    T& operator[] (width_t Index)
    {
      VKT_ASSERT((Index < Len) && "Index specified exceeded Sparse Array's length");
      return GetElementAtIndex(Index);
    }

    const T& operator[] (width_t Index) const
    {
      VKT_ASSERT((Index < Len) && "Index specified exceeded Sparse Array's length");
      return GetElementAtIndex(Index);
    }

    void Empty()
    {
      Destruct(0, Len);
      Len = 0;
      Current = Root;
    }

    void Pop(width_t Count = 1)
    {
      Destruct(Len - Count, Len, true);
      Len -= Count;
      Len = (Len < 0) ? 0 : Len;
    }

    void PopAt(width_t Index)
    {
      VKT_ASSERT((Index < Len) && "Index specified exceeded Sparse Array's length.");
      Destruct(Index, Index + 1, true);
    }

    void Shrink()
    {
      width_t numBlocksToRelease = (GetTotalCapacity() - Len) / GetSlack();
      if (numBlocksToRelease)
      {
        Destruct(Len, GetTotalCapacity() - 1);

        // Get last block ... 
        SparseBlock* pLastBlock = Root;
        for (width_t i = 1; i < NumOfBlocks; i++)
        {
          pLastBlock = pLastBlock->Next;
        }

        while (numBlocksToRelease)
        {
          SparseBlock* pToDelete = pLastBlock;
          if (pLastBlock->Previous)
          {
            pLastBlock->Previous->Next = nullptr;
            pLastBlock = pLastBlock->Previous;
          }
          delete pToDelete;
          NumOfBlocks--;
          numBlocksToRelease--;
        }
      }
    }

    width_t Length() const { return Len; }
    width_t Size()	const { return GetTotalCapacity(); }
    Type* First() { return Root->Data; }
    const Type* First() const { return Root->Data; }
    Type* Last() { return &GetElementAtIndex(Len - 1); }
    const Type* Last() const { return &GetElementAtIndex(Len - 1); }
    Type& Front() { return *Root->Data; }
    const Type& Front() const { return *Root->Data; }
    Type& Back() { return GetElementAtIndex(Len - 1); }
    const Type& Back() const { return GetElementAtIndex(Len - 1); }

  };

}

#endif // !ANGKASA1_ARRAY
