#pragma once
#ifndef LEARNVK_MEMORY
#define LEARNVK_MEMORY

#include <new>
#include "Platform/EngineAPI.h"
#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"

#define BYTES(n)		(n)
#define KILOBYTES(n)	(BYTES(n)		* 1024)
#define MEGABYTES(n)	(KILOBYTES(n)	* 1024)
#define GIGABYTES(n)	(MEGABYTES(n)	* 1024)

struct ENGINE_API IMemory
{
	static void*	AllignedAlloc		(size_t Size, size_t Alignment);
	static void*	Malloc				(size_t Size);
	static void*	Realloc				(void* Block, size_t Size);
	static void		Free				(void* Block);

	static void		Memmove				(void* Destination, const void* Source, size_t Count);
	static void		Memcpy				(void* Destination, const void* Source, size_t Count);
	static void		Memzero				(void* Destination, size_t Count);
	static void		Memset				(void* Destination, uint8 Char, size_t Count);
	static size_t	Memcmp				(void* Buf1, void* Buf2, size_t Count);

	static bool		IsPowerOfTwo		(size_t Num);
	static size_t	CalculatePadding	(const uintptr_t BaseAddress, size_t Alignment);

	template <typename T>
	static T* New() { return new T(); }

	template <typename T>
	static T* New(size_t Count) { return new T[Count]; }

	template <typename T>
	static void Delete(T* Resource) { delete Resource; }

	template <typename T>
	static void	InitializeObject(T* Object) 
	{
		Memzero(Object, sizeof(T));
		new (Object) T();
	};

	template <typename T>
	static void InitializeObject(T& Object)
	{
		InitializeObject(&Object);
	}

	template <typename T, typename... Initializers>
	static void InitializeObject(T* Object, Initializers&&... Args)
	{
		Memzero(Object, sizeof(T));
		new (Object) T(Forward<Initializers>(Args)...);
	}

	template <typename T, typename... Initializers>
	static void InitializeObject(T& Object, Initializers&&... Args)
	{
		InitializeObject(&Object, Forward(Args)...);
	}
};

class alignas(void*) NullPointer
{
//public:
//  template <typename T>
//  operator T* () const { return nullptr; }
//  template <typename T, typename U>
//  operator T U::* () const { return nullptr; }
//  operator void* () const { return nullptr; }
//  bool operator==(const NullPointer&) const { return true; }
//  bool operator!=(const NullPointer&) const { return false; }
//  explicit operator bool() const { return false; }
//private:
//  void* _Padding;
};

#define NULLPTR NullPointer()

template <typename T>
class Ref;

template <typename T>
class UniquePtr final
{
private:
	friend class Ref<T>;
	T* _Ptr;
public:
	UniquePtr() : _Ptr(nullptr) {}
	UniquePtr(T* Object) : _Ptr(Object) {}
	~UniquePtr() { if (_Ptr) { IMemory::Delete(_Ptr); } }

	UniquePtr(const UniquePtr& Rhs) = delete;
	UniquePtr& operator=(const UniquePtr& Rhs) = delete;

	//UniquePtr(const UniquePtr& Rhs) { *this = Rhs; };
	//UniquePtr& operator=(const UniquePtr& Rhs)
	//{
	//	if (this != &Rhs) { _Ptr = Rhs._Ptr; }
	//	return *this;
	//}

	UniquePtr(UniquePtr&& Rhs) { *this = Move(Rhs); }
	UniquePtr& operator=(UniquePtr&& Rhs)
	{
		if (this != &Rhs) { _Ptr = Rhs._Ptr; new (&Rhs) UniquePtr(); }
		return *this;
	}
  T& operator*() { return *_Ptr; }
  const T& operator*() const { return *_Ptr; }
	T* operator->() { return _Ptr; }
	const T* operator->() const { return _Ptr; }
	bool operator==(const UniquePtr& Rhs) { return _Ptr == Rhs._Ptr; }
	bool operator!=(const UniquePtr& Rhs) { return _Ptr != Rhs._Ptr; }
	explicit operator bool() const { return _Ptr; }
};

template <typename T>
class Ref
{
protected:
	T* _Ptr;
public:
	Ref() : _Ptr(nullptr) {}
	~Ref() { _Ptr = nullptr; }
	Ref(NullPointer) : _Ptr(nullptr) {}
	Ref(T* Src) : _Ptr(Src) {} // Temporary solution ...
	Ref(UniquePtr<T>& Src) : _Ptr(Src._Ptr) {}
	Ref(const UniquePtr<T>& Src) : _Ptr(Src._Ptr) {}
	Ref(const Ref& Rhs) { *this = Rhs; }
	Ref(Ref&& Rhs) { *this = Move(Rhs); }
	Ref& operator=(const Ref& Rhs)
	{
		if (this != &Rhs) { _Ptr = Rhs._Ptr; }
		return *this;
	}
	Ref& operator=(Ref&& Rhs)
	{
		if (this != &Rhs) { _Ptr = Rhs._Ptr; new (&Rhs) Ref(); }
		return *this;
	}
	Ref& operator=(T* Src) { _Ptr = Src; return *this; }	// Temporary solution ...
	Ref& operator=(UniquePtr<T> Src) { _Ptr = Src._Ptr; return *this; }
	T* operator->() { return _Ptr; }
	const T* operator->() const { return _Ptr; }
	bool operator==(NullPointer) const { return  _Ptr == nullptr; }
	bool operator!=(NullPointer) const { return _Ptr != nullptr; }
	bool operator==(const Ref& Rhs) const { return _Ptr == Rhs._Ptr; }
	bool operator!=(const Ref& Rhs) const { return _Ptr != Rhs._Ptr; }
	explicit operator bool() const { return _Ptr; }
};

#endif // !LEARNVK_MEMORY
