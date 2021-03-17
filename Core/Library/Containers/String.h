#pragma once
#ifndef LEARNVK_STRING
#define LEARNVK_STRING

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include "Library/Memory/Memory.h"
#include "Library/Algorithms/Hash.h"
#include "Iterator.h"


/**
* Small String Optimized BasicString class.
*
* Fast, lightweight and semi-fully featured. 
* Any string that is less than 24 bytes (including the null terminator) would be placed on the stack instead of the heap.
* A decision was made to not revert the string class to a small string optimised version if the string is somehow altered to be shorter than before and less than 24 bytes.
* Reason being the cost of allocating memory on the heap outweighs the pros of putting the string on the stack.
*/
template <typename StringType, size_t SlackMultiplier = 2>
class BasicString
{
public:

	using Type		= StringType;
	using Iterator	= ArrayIterator<Type>;
	using ConstType	= const Type;
	//using ConstIterator = const Iterator;

protected:

	static constexpr size_t SSO_MAX = 24 / sizeof(Type) < 1 ? 1 : 24 / sizeof(Type);

	union
	{
		Type*	Data;
		Type	Buffer[SSO_MAX];
	};

	uint32 Capacity;
	uint32 Len;

	/**
	* Takes in a string literal as an argument and stores that literal into the buffer.
	*/
	size_t WriteFv(const char* Literal)
	{
		size_t Length = strlen(Literal);

		if (Length >= Capacity) 
			Alloc(Length + (1 << SlackMultiplier), false);

		if (Len > Length)
			Destruct(Length - 1, Len - 1);

		Len = 0;

		while (Len != Length) 
		{
			StoreInBuffer(Len, Literal[Len]);
			Len++;
		}

		Type* Ptr = PointerToString();
		Ptr[Len] = '\0';

		return Len;
	}

	/**
	* Takes in a formated string literal along with the format's arguments and stores it into the buffer.
	* Only 256 bytes of memory are allowed for formatted strings.
	*/
	size_t WriteFv(const char* Format, va_list Arguments)
	{
		size_t Length = -1;

		// No one should ever need more than 256 Bytes of memory to write a formatted string.
		// NOTE(Afiq):
		// The size of this temporary buffer will change over time.
		char Buf[256];

		Length = vsnprintf(Buf, 256, Format, Arguments);

		// Encoding error!
		VKT_ASSERT(Length >= 0);

		return WriteFv(Buf);
	}

private:

	/**
	* Checks if the current string is or is not a small string.
	*/
	bool IsSmallString() const
	{
		return Capacity <= SSO_MAX;
	}

	/**
	* Returns a pointer to the current buffer that is in use for the string. 
	*/
	Type* PointerToString()
	{
		return IsSmallString() ? Buffer : Data;
	}

	/**
	* Returns a const pointer to the current buffer that is in use for the string.
	*/
	const Type* PointerToString() const 
	{
		return IsSmallString() ? Buffer : Data;
	}

	/**
	* Stores the character into the buffer / pointer with perfect forwarding.
	*/
	template <typename... ForwardType>
	void StoreInBuffer(size_t Index, ForwardType&&... Char)
	{
		Type* Ptr = PointerToString();
		Ptr[Index] = Type(Forward<ForwardType>(Char)...);
	}

	/**
	* Destructs the buffer from and to the specified indexes.
	* Optionally reconstruct the characters after it has been destructed.
	*/
	void Destruct(size_t From = 0, size_t To = Len, bool Reconstruct = false)
	{
		Type* Ptr = PointerToString();
		size_t Count = To - From;
		FMemory::Memzero(&Ptr[From], Count);

		if (Count != Capacity)
		{
			FMemory::Memmove(Ptr, &Ptr[To + 1], Count);
		}

		Len -= static_cast<uint32>(Count);
		//FMemory::Memzero(Ptr, To);
	}

	/**
	* Allocates memory to fill the specified length.
	*/
	void Alloc(size_t Length, bool Append)
	{
		if (Length < Capacity)
			Destruct(Length, Capacity);

		Capacity = Append ? Capacity + (uint32)Length : (uint32)Length;

		Type* New = reinterpret_cast<Type*>(FMemory::Realloc(Data, Capacity));
			
		VKT_ASSERT("Unable to allocate memory!" && New);
		
		Data = New;
	}

	/**
	* Compares to see if the strings are the same.
	* Returns true if they are the same.
	*/
	bool Compare(const BasicString& Rhs) const
	{
		// If the object is being compared with itself, then it definitely will always be the same.
		if (this != &Rhs) 
		{
			// If the length of both strings are not the same, then they definitely will not be the same.
			if (Len != Rhs.Len)
				return false;

			const Type* Ptr = PointerToString();
			const Type* RhsPtr = Rhs.PointerToString();

			// If the length are the same, we have to check if the contents of the string matches one another.
			for (size_t i = 0; i < Len; i++)
			{
				if (Ptr[i] != RhsPtr[i])
					return false;
			}
		}
				
		return true;
	}


	bool Compare(const char* Literal) const
	{
		size_t Length = strlen(Literal);

		if (Len != Length)
			return false;

		const Type* Ptr = PointerToString();

		for (size_t i = 0; i < Len; i++)
		{
			if (Ptr[i] != Literal[i])
				return false;
		}

		return true;
	}


public:


	/**
	* Default constructor.
	*/
	BasicString() :
		Buffer{'\0'}, Capacity(SSO_MAX), Len(0) {}


	/**
	* Default destructor.
	*/
	~BasicString()
	{
		Release();
	}


	/**
	* Pre-allocates enough memory as specified in the argument for the string object.
	*/
	BasicString(size_t Size) : BasicString()
	{
		Alloc(Size, false);
	}


	/**
	* Assigns the string literal into the string object.
	*/
	BasicString(const char* Literal) : BasicString()
	{
		*this = Literal;
	}

		
	/**
	* Assigns the string literal into the string object.
	*/
	BasicString& operator= (const char* Literal)
	{
		WriteFv(Literal);

		return *this;
	}


	/**
	* Copy constructor.
	* Performs a deep copy.
	*/
	BasicString(const BasicString& Rhs)
	{
		*this = Rhs;
	}


	/**
	* Copy assignment operator.
	* Performs a deep copy.
	*/
	BasicString& operator= (const BasicString& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs)
		{
			Type* Ptr = Buffer;
			const Type* RhsPtr = Rhs.PointerToString();

			Capacity = Rhs.Capacity;
			Len		 = Rhs.Len;

			// Perform a deep copy if the copied string is not a small string.
			if (!Rhs.IsSmallString())
			{
				Alloc(Rhs.Capacity, false);
				Ptr = Data;
			}

			//memcpy(Ptr, RhsPtr, Len);
			for (size_t i = 0; i < Len; i++)
			{
				Ptr[i] = RhsPtr[i];
			}

			Ptr[Len] = '\0';
		}

		return *this;
	}


	/**
	* Move constructor.
	*/
	BasicString(BasicString&& Rhs)
	{
		*this = Move(Rhs);
	}


	/**
	* Move assignment operator.
	*/
	BasicString& operator= (BasicString&& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		if (this != &Rhs)
		{
			Type* RhsPtr = Rhs.PointerToString();

			Capacity = Rhs.Capacity;
			Len		 = Rhs.Len;

			if (!Rhs.IsSmallString()) 
			{
				Data = RhsPtr;

				RhsPtr			= nullptr;
				Rhs.Capacity	= 0;
				Rhs.Len			= 0;
			}
			else
			{
				for (size_t i = 0; i < Len; i++) 
				{
					Buffer[i] = RhsPtr[i];
				}

				Buffer[Len] = '\0';
				new (&Rhs) BasicString();
			}

		}

		return *this;
	}


	bool operator== (const BasicString& Rhs) const
	{
		return Compare(Rhs);
	}


	bool operator!= (const BasicString& Rhs) const
	{
		return !(*this == Rhs);
	}


	bool operator== (const char* Literal) const
	{
		return Compare(Literal);
	}


	bool operator!= (const char* Literal) const
	{
		return !(*this == Literal);
	}


	/**
	* Square brace overloaded operator.
	* Retrieves an element from the specified index.
	*/
	Type& operator[] (size_t Index)
	{
		if (Index >= Capacity)
			Alloc(Index + (1 << SlackMultiplier), true);

		// NOTE(Afiq):
		// Not too sure about this implementation. Will try out for a while.
		if (Len < Index)
			Len = (uint32)Index + 1;

		Type* Ptr = PointerToString();
		return Ptr[Index];
	}


	/**
	* Square brace overloaded operator.
	* Retrieves an element from the specified index.
	*/
	const Type& operator[] (size_t Index) const
	{
		VKT_ASSERT(Index < Len);
		Type* Ptr = PointerToString();
		return Ptr[Index];
	}


	/**
	* Clears all data from the string and releases it from memory.
	*/
	void Release()
	{
		Destruct(0, Capacity);

		if (!IsSmallString())
		{
			if (Data)
			{
				FMemory::Free(Data);
			}
		}

		new (this) BasicString();
	}


	/**
	* Reserves a fix amount of memory for the string object.
	* Recommended to be called before assigning a very large string.
	*/
	void Reserve(size_t Length, bool Append = false)
	{
		if (Length <= SSO_MAX)
		{
			return;
		}
		Alloc(Length, Append);
	}


	/**
	* Assign a formatted string into the string object.
	*/
	size_t Format(const char* Format, ...)
	{
		va_list Args;
		va_start(Args, Format);
		size_t Length = WriteFv(Format, Args);
		va_end(Args);

		return Length;
	}


	/**
	* Assign a string literal into the string object.
	* It is the same as using the assignment operator to assign the string.
	*/
	size_t Write(const char* Literal) { return WriteFv(Literal); }


	/**
	* Shrinks the total size capacity of the string to tightly fit the length of string +1 for the null terminator.
	*/
	void ShrinkToFit() { Alloc(Len + 1, false); }


	/**
	* Clears all data from the string object but does not release it from memory.
	*/
	void Empty() { Destruct(0, Len, true); }


	/**
	* Pushes a character to the back of the string.
	*/
	size_t Push(const Type& Char)
	{
		if (Len == Capacity)
		{
			Alloc(Capacity + (1 << SlackMultiplier));
		}

		StoreInBuffer(Len++, Char);
		Type* Ptr = PointerToString();
		Ptr[Len] = '\0';

		return Len++;
	}


	/**
	* Pushes a character to the back of the string.
	*/
	size_t Push(Type&& Char)
	{
		if (Len == Capacity)
		{
			Alloc(Capacity + (1 << SlackMultiplier));
		}

		StoreInBuffer(Len++, Move(Char));
		Type* Ptr = PointerToString();
		Ptr[Len] = '\0';

		return Len++;
	}

	ConstType*	C_Str	()	const	{ return PointerToString(); }
		
	size_t		Length	()	const	{ return static_cast<size_t>(Len); }
	size_t		Size	()	const	{ return static_cast<size_t>(Capacity); }

	Type*		First	()			{ return PointerToString(); }
	ConstType*	First	()	const	{ return PointerToString(); }
	Type*		Last	()			{ return PointerToString() + Len; }
	ConstType*	Last	()	const	{ return PointerToString() + Len; }
	Type&		Front	()			{ return *PointerToString(); }
	ConstType&	Front	()	const	{ return *PointerToString(); }
	Type&		Back	()			{ return *(PointerToString() + Len); }
	ConstType&	Back	()	const	{ return *(PointerToString() + Len); }

	Iterator		begin()			{ return Iterator(PointerToString()); }
	//ConstIterator	begin() const	{ return Iterator(PointerToString()); }

	Iterator		end()			{ return Iterator(PointerToString() + Len); }
	//ConstIterator	end()	const	{ return Iterator(PointerToString() + Len); }

};


/**
* BasicHashString data structure.
*
* Derived from BasicString.
* Strings contained within this class will be hashed on every single update.
*
* HashAlgorithm needs to be a functor object that accepts a const reference String or WString (depending on what was specified in the template argument) as an argument.
* The functor would then need to return the hash.
*/
template <typename StringType, typename HashAlgorithm = MurmurHash<BasicString<StringType>>, size_t SlackMultiplier = 2>
class BasicHashString : public BasicString<StringType, SlackMultiplier>
{
private:

	using Type	= StringType;
	using Super = BasicString<Type, SlackMultiplier>;

	void UpdateHash()
	{
		HashAlgorithm Algorithm;
		Hash = static_cast<uint32>(Algorithm(*this));
	}

public:

	size_t Push(const Type& Char)	= delete;
	size_t Push(Type&& Char)		= delete;

	/**
	* Default constructor.
	*/
	BasicHashString() : 
		Super(), Hash(0) {}

		
	/**
	* Default destructor.
	*/
	~BasicHashString()
	{
		Release();
	}


	/**
	* Constructor to take in a string literal.
	*/
	BasicHashString(const char* Literal) : Hash(0)
	{
		*this = Literal;
	}


	/**
	* Assigns the string literal into the string object and hashes it.
	*/
	BasicHashString& operator= (const char* Literal)
	{
		Super::operator=(Literal);
		UpdateHash();

		return *this;
	}


	/**
	* Constructor to take in a String object.
	*/
	BasicHashString(const Super& Source) : Super(), Hash(0)
	{
		*this = Source;
	}


	/**
	* Overloaded assignment operator that assigns a String object into BasicHashString and updates it's Hash.
	*/
	BasicHashString& operator= (const Super& Source)
	{
		Super::operator=(Source);
		UpdateHash();

		return *this;
	}


	/**
	* Default copy constructor.
	*/
	BasicHashString(const BasicHashString& Rhs)
	{
		*this = Rhs;
	}


	/**
	* Default copy assignment operator.
	*/
	BasicHashString& operator= (const BasicHashString& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		Hash = Rhs.Hash;
		Super::operator=(Rhs);

		return *this;
	}


	/**
	* Default move constructor.
	*/
	BasicHashString(BasicHashString&& Rhs)
	{
		*this = Move(Rhs);
	}


	/**
	* Default move assignment operator.
	*/
	BasicHashString& operator= (BasicHashString&& Rhs)
	{
		VKT_ASSERT(this != &Rhs);

		Hash = Rhs.Hash;
		BasicString<Type, SlackMultiplier>::operator=(Move(Rhs));
		Rhs.Hash = 0;
			
		return *this;
	}


	/**
	* Overloaded equality operator.
	*/
	bool operator== (const BasicHashString& Rhs) const
	{
		if (Hash != Rhs.Hash)
		{
			return Super::operator==(Rhs);
		}

		return true;
	}


	/**
	* Overloaded inequality operator.
	*/
	bool operator!= (const BasicHashString& Rhs) const
	{
		return !(*this == Rhs);
	}


	bool operator< (const BasicHashString& Rhs) const
	{
		return Hash < Rhs.Hash;
	}


	bool operator< (const Super& Rhs) const
	{
		BasicHashString<Type> Temp = Rhs;	
		return Hash < Temp.Hash;
	}


	/**
	* Clears all data and the hash from the string and releases it from memory.
	*/
	void Release()
	{
		Hash = 0;
		Super::Release();
	}


	/**
	* Clears all data and the hash from the string but does not release it from memory.
	*/
	void Empty()
	{
		Hash = 0;
		Super::Empty();
	}


	/**
	* Assign a formatted string into the string object and updates it's hash.
	*/
	size_t Format(const char* Format, ...)
	{
		va_list Args;
		va_start(Args, Format);
		size_t Length = Super::WriteFv(Format, Args);
		va_end(Args);

		UpdateHash();

		return Length;
	}

	/**
	* Assign a string literal into the string object and updates it's hash.
	* It is the same as using the assignment operator to assign a string.
	*/
	size_t Write(const char* Literal)
	{
		Super::Write(Literal);
		UpdateHash();

		return Super::Len;
	}

private:
	uint32 Hash;
};

template <typename Type, size_t Capacity>
class BaseStaticString
{
public:
	using StringType = Type;
	using ConstType  = const StringType;

	BaseStaticString() :
		Buf{}, Len(0)
	{}

	~BaseStaticString() { Flush(); }

	BaseStaticString(const BaseStaticString& Rhs) { *this = Rhs; }
	BaseStaticString(BaseStaticString&& Rhs) { *this = Move(Rhs); }

	BaseStaticString& operator=(const BaseStaticString& Rhs)
	{
		if (this != &Rhs)
		{
			FMemory::Memcpy(Buf, Rhs.Buf, Rhs.Len);
			Len = Rhs.Len;
		}
		return *this;
	}

	BaseStaticString& operator=(BaseStaticString&& Rhs)
	{
		if (this != &Rhs)
		{
			FMemory::Memmove(Buf, Rhs.Buf, Rhs.Len);
			Len = Rhs.Len;
			new (&Rhs) BaseStaticString();
		}
		return *this;
	}

	BaseStaticString(const Type* Text) : BaseStaticString()
	{
		Write(Text);
	}

	bool operator==(const BaseStaticString& Rhs)	{ return Compare(Rhs); }
	bool operator!=(const BaseStaticString& Rhs)	{ return !Compare(Rhs); }
	bool operator==(const Type* Literal)			{ return Compare(Literal); }
	bool operator!=(const Type* Literal)			{ return !Compare(Literal); }

	BaseStaticString& operator=(const Type* Literal)
	{
		Write(Literal);
		return *this;
	}

	void Flush()
	{
		FMemory::Memzero(Buf, Len);
		Len = 0;
	}

	size_t Format(const Type* Format, ...)
	{
		va_list Args;
		va_start(Args, Format);
		WriteFv(Format, Args);
		va_end(Args);
		return Len;
	}

	void Write(const Type* Literal)
	{
		size_t len = strlen(Literal);
		VKT_ASSERT((len + 1) <= Capacity);
		Flush();
		while (Len <= len)
		{
			Buf[Len] = Literal[Len];
			Len++;
		}
		Buf[Len] = '\0';
	}

	size_t		Diff	(const BaseStaticString& Rhs)	const { return static_cast<size_t>(strcmp(Buf, Rhs)); }
	size_t		Diff	(const char* Literal)			const { return static_cast<size_t>(strcmp(Buf, Literal)); }
	bool		Compare	(const BaseStaticString& Rhs)	const { return strcmp(Buf, Rhs.Buf) == 0; }
	bool		Compare	(const char* Literal)			const { return strcmp(Buf, Literal) == 0; }
	bool		IsEmpty	()								const { return !Len; }
	size_t		Length	()								const { return Len; }
	ConstType*	C_Str	()								const { return Buf; }
	ConstType*	First	()								const { return Buf; }
	ConstType*	Last	()								const { return Buf + Len; }
	StringType* First	()									  { return Buf; }
	StringType* Last	()									  { return Buf + Len; }

	template <typename... ForwardType>
	void AddChar(ForwardType&&... Char)
	{
		VKT_ASSERT(Len < Capacity);
		Buf[Len++] = Type(Forward<ForwardType>(Char)...);
		Buf[Len] = '\0';
	}

private:
	Type	Buf[Capacity];
	size_t	Len;

	size_t WriteFv(const Type* Format, va_list Arguments)
	{
		Len = vsnprintf(Buf, Capacity, Format, Arguments);
		// Encoding error!
		VKT_ASSERT(Len >= 0);
		return Len;
	}
};


//template <typename>
using String = BasicString<char>;

template <typename MemoryAllocator, typename HashFunc = MurmurHash<BasicString<char>>, size_t SlackMultiplier = 2>
using HashString = BasicHashString<char, HashFunc, SlackMultiplier>;

template <size_t Capacity>
using StaticString = BaseStaticString<char, Capacity>;

using String16 = StaticString<16>;
using String32 = StaticString<32>;
using String64 = StaticString<64>;
using String128 = StaticString<128>;
using String256 = StaticString<256>;


/**
* BasicStringView data structure.
*
* A wrapper around const (char types)* and much safer to use as alteration of the source data is strictly not allowed in BasicStringView.
*/
//template <typename Type>
//class BasicStringView
//{
//private:

//	using String = BasicString<Type>;
//	using Iterator = Iterator<Type>;

//	const Type* Ptr;
//	size_t		Len;

//public:


//	/**
//	* BasicStringView's default constructor.
//	*/
//	BasicStringView() :
//		Ptr(nullptr), Len(0) {}


//	constexpr BasicStringView(const BasicStringView& Rhs) noexcept = default;
//	constexpr BasicStringView& operator= (const BasicStringView& Rhs) noexcept = default;


//	/**
//	* Overloaded constructor that takes in a source String object.
//	*/
//	BasicStringView(const String& Source) :
//		Ptr(Source.C_Str()), Len(Source.Length()) {}


//	/**
//	* Overloaded constructor that takes in a pointer to a string and the string's length.
//	*/
//	BasicStringView(const Type* Pointer, size_t Length) :
//		Ptr(Pointer), Len(Length) {}


//	/**
//	* Retrieves a const reference to the character of the string at the specified index.
//	*/
//	const Type& operator[] (size_t Index) const
//	{
//		VKT_ASSERT(Index > Len);

//		return Ptr[Index];
//	}


//	/**
//	* Retrieves a const pointer to the string pointer. Equivalent to calling C_Str() from the String / WString class.
//	*/
//	constexpr const Type* Data() const
//	{
//		return Ptr;
//	}


//	/**
//	* Returns the length of the string view excluding the null terminator.
//	*/
//	constexpr size_t Length() const
//	{
//		return Len;
//	}


//	/**
//	* Retrieves a const pointer to the start of the string.
//	*/
//	constexpr const Type* First() const
//	{
//		return Ptr;
//	}


//	/**
//	* Retrieves a const pointer to the end of the string.
//	*/
//	constexpr const Type* Last() const
//	{
//		return Ptr + Len;
//	}


//	/**
//	* Retrieves a const reference to the start of the string.
//	*/
//	constexpr const Type& Front() const
//	{
//		return *Ptr;
//	}


//	/**
//	* Retrieves a const reference to the end of the string.
//	*/
//	constexpr const Type& Back() const
//	{
//		return *(Ptr + Len);
//	}


//	/**
//	* Const begin() iterator.
//	*/
//	constexpr Iterator begin() const
//	{
//		return Iterator(Ptr);
//	}


//	/**
//	* Const end() iterator.
//	*/
//	constexpr Iterator end() const
//	{
//		return Iterator(Ptr + Len);
//	}

//};


#endif LEARNVK_STRING