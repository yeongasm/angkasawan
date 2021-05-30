#pragma once
#ifndef ANGKASA1_CORE_LIBRARY_CONTAINER_REFERENCE_H
#define ANGKASA1_CORE_LIBRARY_CONTAINER_REFERENCE_H

#include "Library/Templates/Templates.h"

template <typename T>
class Ref
{
private:
	T* _Ptr;
public:

	Ref() : _Ptr(nullptr) {}
	~Ref() { _Ptr = nullptr; }

	Ref(T* Reference) :
		_Ptr(Reference)
	{}

	Ref(const T* Reference) :
		_Ptr(Reference)
	{}

	Ref(const Ref& Rhs) { *this = Rhs; }
	Ref(Ref&& Rhs) { *this = Move(Rhs); }

	Ref& operator=(const Ref& Rhs)
	{
		if (this != &Rhs)
		{
			_Ptr = Rhs._Ptr;
		}
		return *this;
	}

	Ref& operator=(Ref&& Rhs)
	{
		if (this != &Rhs)
		{
			_Ptr = Rhs._Ptr;
			new (&Rhs) Ref();
		}
		return *this;
	}

	Ref& operator=(T* Reference)
	{
		_Ptr = Reference;
		return *this;
	}

	Ref& operator=(const T* Reference)
	{
		_Ptr = Reference;
		return *this;
	}

	T* operator->() { return _Ptr; }
	const T* operator->() const { return _Ptr; }

	bool operator==(const Ref& Rhs) const { return _Ptr == Rhs._Ptr; }
	bool operator!=(const Ref& Rhs) const { return _Ptr != Rhs._Ptr; }
	explicit operator bool() const { return _Ptr; }

};

#endif // !ANGKASA1_CORE_LIBRARY_CONTAINER_REFERENCE_H