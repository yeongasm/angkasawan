#pragma once
#ifndef LEARNVK_ARRAY
#define LEARNVK_ARRAY

#include "Library/Memory/Memory.h"
#include "Iterator.h"

/**
* Templated hybrid Array. Can be used as a static Array or dynamic Array.
*/
template <typename InElementType, size_t Slack = 5>
class Array 
{
public:

	using ElementType	= InElementType;
	using Iterator		= ArrayIterator<ElementType>;
	using ConstIterator = ConstArrayIterator<ElementType>;

protected:

	ElementType*		Data;
	size_t				Capacity;
	size_t				Len;

private:

	void Destruct(size_t FromIdx = 0, size_t ToIdx = Len, const bool Reconstruct = false) 
	{
		for (size_t i = FromIdx; i < ToIdx; i++) 
		{
			Data[i].~ElementType();

			if (Reconstruct)
			{
				new (Data + i) ElementType();
			}
		}
	}

	/*
	* Set the length of the Array. Meant for the use of static Arrays.
	* When using the Array dynamically, call Push() instead as the Array knows how to manage it's own space.
	*/
	void Grow(size_t Size = 0)
	{
		ElementType* Old = Data;
		ElementType* New = nullptr;

		Capacity = Capacity + (1 << Slack);
		
		if (Size)
		{
			Capacity = Size;
		}

		New = reinterpret_cast<ElementType*>(FMemory::Realloc(Old, sizeof(ElementType) * Capacity));
		
		for (size_t i = Len; i < Capacity; i++)
		{
			new (New + i) ElementType();
		}
		
		Data = New;
	}

	/**
	* We now use perfect forwarding to distinguish if the pushed in element a lvalue or rvalue.
	* This solves the problem of custom data types deleting copy assignment operator or move assignment operator.
	*/
	template <class... ForwardType>
	void StoreObject(ForwardType&&... Element)
	{
		if (Len == Capacity)
			Grow();

		Data[Len] = ElementType(Forward<ForwardType>(Element)...);
	}

public:

	/**
	* Default constructor used to initialise the empty Array.
	*/
	Array() : 
		Data(nullptr), Capacity(0), Len(0) {}


	~Array() 
	{
		Release();
	}

	Array(size_t Length) : Array()
	{
		Grow(Length);
	}


	/*
	* Constructor to initialise a 'static' Array of sorts.
	* The ability to Push() new elements into the Array is still do-able.
	*/
	//Array(size_t Length) : Array()
	//{
	//	Alloc(Length);
	//}


	/**
	* Copy constructor used to copy elements from another Array.
	*/
	Array(const Array& Rhs) : Array()
	{
		*this = Rhs;
	}


	/**
	* Move semantics.
	*/
	Array(Array&& Rhs) : Array()
	{
		*this = Move(Rhs);
	}


	/**
	* Constructor to initialise the Array with an initialiser list.
	*/
	Array(const std::initializer_list<ElementType>& InitList) : Array()
	{
		Append(InitList);
	}


	/**
	* Operator to copy the contents from another Array into this one.
	*/
	Array& operator= (const Array& Rhs)
	{
		if (this != &Rhs)
		{
			Destruct(0, Len);

			if (Capacity < Rhs.Capacity)
			{
				Grow(Rhs.Capacity);
			}
			
			Len = Rhs.Len;
			for (size_t i = 0; i < Rhs.Len; i++)
			{
				Data[i] = Rhs[i];
			}
		}

		return *this;
	}

	/**
	* Operator for move semantics.
	*/
	Array& operator= (Array&& Rhs)
	{
		// Gravity will throw an assertion if you are trying to perform a move semantic on itself in debug.
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs)
		{
			Destruct(0, Len);

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
	* 
	*/
	ElementType& operator[] (size_t Index)
	{
		VKT_ASSERT("The index specified exceeded the internal buffer size of the Array!" && Index < Capacity);
		if (Len < Index && Index < Capacity)
		{
			Len = Index + 1;
		}
		return Data[Index];
	}

	const ElementType& operator[] (size_t Index) const 
	{
		VKT_ASSERT("The index specified exceeded the internal buffer size of the Array." && Index < Capacity);
		return Data[Index];
	}

	/**
	* Allocate the specified amount of space for the Array increasing the internal capacity.
	* If the specified size is smaller than the array's existing capacity, it preserves it's previous capacity but removes elements beyond the specified size.
	*
	* @param Size - The desired newly allocated internal buffer size.
	*/
	void Reserve(size_t Size)
	{
		if (Size < Capacity)
		{
			Destruct(Size, Capacity);
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
		Destruct(0, Capacity);
		if (Data)
		{
			FMemory::Free(Data);
		}
		new (this) Array();
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
	size_t Push(const ElementType& Element)
	{
		StoreObject(Element);
		return Len++;
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
	size_t Push(ElementType&& Element)
	{
		StoreObject(Move(Element));
		return Len++;
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
	template <class Type>
	decltype(auto) Insert(Type&& Element) 
	{
		size_t Index = Push(Forward<Type>(Element));
		return Data[Index];
	}

	/**
	* Appends an existing Array and copy or move it's elements into the back of the Array.
	* Destroys contents from the copied Array and constructs a new one by default.
	* Returns the index of the last element after appending.
	*
	* @param Rhs   - The foreign Array that is being appended.
	*/
	size_t Append(const Array& Rhs)
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
	size_t Append(const std::initializer_list<ElementType>& InitList)
	{
		for (size_t i = 0; i < InitList.size(); i++)
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
	size_t Append(ConstIterator Begin, size_t Length)
	{
		for (size_t i = 0; i < Length; i++)
		{
			Push(Begin[i]);
		}

		return Len - 1;
	}

	/**
	* Moves all the element from the other Array into the existing one or simply copies from it.
	* Performs a move by default.
	*
	* @param Other      - The foreign Array that is being moved/copied.
	* @param RemLocal   - Clears all the existing elements in the local Array. [Default] true.
	* @param RemForeign - Clears all the existing element in the foreign Array. [Default] true.
	*/
	//void MoveOrCopy(Array& Rhs, bool RemoveLocal = true, bool RemoveForeign = true)
	//{
	//	if (RemoveLocal)
	//		Release();

	//	for (size_t i = 0; i < Rhs.Len; i++)
	//		Push(Rhs[i]);

	//	if (RemoveForeign)
	//		Rhs.Release();
	//}

	/**
	* Pop an element from the back of the Array.
	* Last-In-First-Out implementation.
	* Returns the last index in the element.
	*
	* @param Count	  - The amount of element(s) to be removed. [Default] 1.
	* @param Shrink	  - Reduce the size of the internal buffer allocator for the Array. [Default] false.
	*/
	size_t Pop(size_t Count = 1)
	{
		VKT_ASSERT(Len >= 0);

		Destruct(Len - Count, Len, true);
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
	void PopAt(size_t Index, bool Move = true)
	{
		// The element should reside in the container.
		VKT_ASSERT(Index < Len);

		Data[Index].~ElementType();

		if (Move) 
		{
			FMemory::Memmove(Data + Index, Data + Index + 1, sizeof(ElementType) * (--Len - Index));
			new (Data + Len) ElementType();
		} 
		else 
		{
			new (Data + Index) ElementType();
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
		Destruct(0, Len, Reconstruct);
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
	inline size_t IndexOf(const ElementType& Element) const 
	{
		return IndexOf(&Element);
	}

	/**
	* Returns the index of the specified object in the Array.
	* Not to be used for Arrays storing pointers. Check Find() function.
	*
	* @param Element - An element passed in by pointer.
	*/
	inline size_t IndexOf(const ElementType* Element) const 
	{
		// Object must reside within the boundaries of the Array.
		VKT_ASSERT(Element >= Data && Element < Data + Capacity);

		// The specified object should reside at an address that is an even multiple of of the Type's size.
		VKT_ASSERT(reinterpret_cast<uint8*>(Element) - reinterpret_cast<uint8*>(Data) % sizeof(ElementType) == 0);

		return reinterpret_cast<size_t>(Element - Data);
	}

	/**
	* Retrieves the index of the element in the Array.
	* Returns a -1 if not found.
	* If there are multiple similar elements
	*
	* @param Element - Object to be evaluated.
	*/
	//inline size_t Find(const ElementType& Element) const 
	//{
	//	for (size_t i = 0; i < Len; i++)
	//		if (Element == Data[i])
	//			return i;

	//	return -1;
	//}

	/**
	* Iterates the Array and checks if the specified element exists.
	* Returns true if exist, false if not.
	*
	* @param Element - Object to be evaluated.
	*/
	//inline bool Has(const ElementType& Element) const 
	//{
	//	return Find(Element) != -1;
	//}

	/**
	* Returns the index of the end-most object within the Array.
	*/
	inline size_t Length() const 
	{
		return Len;
	}

	/**
	* Returns the internal buffer size allocated inside the Array.
	*/
	inline size_t Size() const 
	{
		return Capacity;
	}

	/**
	* Returns a pointer to the first element in the Array.
	*/
	inline ElementType* First()
	{
		return Data;
	}

	inline const ElementType* First() const
	{
		return Data;
	}

	/**
	* Returns a pointer to the last element in the Array.
	* If length of array is 0, Last() returns the 0th element.
	*/
	inline ElementType* Last()
	{
		return &Data[Len - 1];
	}


	/**
	* Returns a reference to the first element in the Array.
	*/
	inline ElementType& Front() 
	{
		return *First();
	}

	/**
	* Returns a reference to the first element in the Array.
	* If the length of the Array is 0, Back() returns the 0th element.
	*/
	inline ElementType& Back() 
	{
		return *Last();
	}

	/**
	* Range-based loop initialiser.
	*/
	Iterator begin()
	{
		return Iterator(Data);
	}

	/**
	* Range-based loop const initialiser.
	*/
	ConstIterator begin() const
	{
		return ConstIterator(Data);
	}

	/**
	* Range-based loop terminator.
	*/
	Iterator end()
	{
		return Iterator(Data + Len);
	}

	/**
	* Range-based loop const terminator.
	*/
	ConstIterator end() const
	{
		return ConstIterator(Data + Len);
	}

	/**
	* Reverse iterator begin.
	*/
	//Iterator rbegin() 
	//{
	//	return Iterator(Data + Len);
	//}

	/**
	* Reverse iterator end.
	*/
	//Iterator rend()
	//{
	//	return Iterator(Data);
	//}
};


template <typename ElementType, size_t Capacity>
class StaticArray
{
private:

	using Type = ElementType;
	using Iterator = ArrayIterator<Type>;
	using ConstIterator = ConstArrayIterator<Type>;

	Type	Data[Capacity];
	size_t	Len;

	void Destruct(size_t From, size_t To, const bool Reconstruct = true)
	{
		for (size_t i = From; i < To; i++)
		{
			Data[i].~Type();
			if (Reconstruct)
			{
				new (Data + i) Type();
			}
		}
	}

	template <class... ForwardType>
	void StoreObject(ForwardType&&... Element)
	{
		VKT_ASSERT("No more space available in this array!" && (Len != Capacity));
		Data[Len] = ElementType(Forward<ForwardType>(Element)...);
	}

public:

	StaticArray() : 
		Data{}, Len(0)
	{}

	~StaticArray()
	{
		Destruct(0, Capacity);
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
			Destruct(0, Capacity);
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
			Destruct(0, Capacity);
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

	ElementType& operator[] (size_t Index)
	{
		VKT_ASSERT("The index specified exceeded the internal buffer size of the Array!" && Index < Capacity);
		if (Len < Index && Index < Capacity)
		{
			Len = Index + 1;
		}
		return Data[Index];
	}

	const ElementType& operator[] (size_t Index) const
	{
		VKT_ASSERT("The index specified exceeded the internal buffer size of the Array." && Index < Capacity);
		return Data[Index];
	}

	size_t Push(const ElementType& Element)
	{
		StoreObject(Element);
		return Len++;
	}

	size_t Push(ElementType&& Element)
	{
		StoreObject(Move(Element));
		return Len++;
	}

	template <class Type>
	decltype(auto) Insert(Type&& Element)
	{
		size_t Index = Push(Forward<Type>(Element));
		return Data[Index];
	}
	
	size_t Pop(size_t Count = 1)
	{
		VKT_ASSERT((Len >= 0) || (Len - Count > 0));
		Destruct(Len - Count, Len, true);
		Len -= Count;
		Len = (Len < 0) ? 0 : Len;

		return Len;
	}

	void PopAt(size_t Index, bool Move = true)
	{
		// The element should reside in the container.
		VKT_ASSERT(Index < Len);
		Data[Index].~ElementType();

		if (Move)
		{
			FMemory::Memmove(Data + Index, Data + Index + 1, sizeof(ElementType) * (--Len - Index));
			new (Data + Len) ElementType();
		}
		else
		{
			new (Data + Index) ElementType();
		}
	}

	void Empty(bool Reconstruct = true)
	{
		Destruct(0, Len, Reconstruct);
		Len = 0;
	}

	size_t Length() const
	{
		return Len;
	}

	size_t Size() const
	{
		return Capacity;
	}

	inline Type* First()
	{
		return Data;
	}

	inline Type* Last()
	{
		return &Data[Len - 1];
	}

	inline Type& Front()
	{
		return *First();
	}

	inline Type& Back()
	{
		return *Last();
	}

	Iterator begin()
	{
		return Iterator(Data);
	}

	ConstIterator begin() const
	{
		return ConstIterator(Data);
	}

	Iterator end()
	{
		return Iterator(Data + Len);
	}

	ConstIterator end() const
	{
		return ConstIterator(Data + Len);
	}
};


#endif // !LEARNVK_ARRAY