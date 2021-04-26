#pragma once
#ifndef ANGKASA1_ASSETS_HANDLE
#define ANGKASA1_ASSETS_HANDLE

#include "Library/Templates/Types.h"

#define INVALID_HANDLE -1

template <typename Type>
class Handle
{
public:
	Handle() : Value(INVALID_HANDLE) {};
	~Handle()	= default;
	Handle(const Handle&)	= default;
	Handle(Handle&&)		= default;
	Handle& operator=(const Handle&) = default;
	Handle& operator=(Handle&&)		 = default;

	Handle(size_t i) : Value(i) {}

	operator size_t() const { return Value; }
	operator uint32() const { return static_cast<uint32>(Value); }

	const bool operator== (const Handle Hnd) const { return Value == Hnd.Value; }
	const bool operator== (const size_t Val) const { return Value == Val; }
	const bool operator== (const uint32 Val) const { return static_cast<uint32>(Value) == Val; }
	const bool operator== (const int32  Val) const { return static_cast<int32>(Value)  == Val; }
	const bool operator!= (const Handle Hnd) const { return Value != Hnd.Value; }
	const bool operator!= (const size_t Val) const { return Value != Val; }
	const bool operator!= (const uint32 Val) const { return static_cast<uint32>(Value) != Val; }
	const bool operator!= (const int32  Val) const { return static_cast<int32>(Value)  != Val; }

	bool operator== (Handle Hnd) { return Value == Hnd.Value; }
	bool operator== (size_t Val) { return Value == Val; }
	bool operator== (uint32 Val) { return static_cast<uint32>(Value) == Val; }
	bool operator== (int32	Val) { return static_cast<int32>(Value) == Val; }
	bool operator!= (Handle Hnd) { return Value != Hnd.Value; }
	bool operator!= (size_t Val) { return Value != Val; }
	bool operator!= (uint32 Val) { return static_cast<uint32>(Value) != Val; }
	bool operator!= (int32	Val) { return static_cast<int32>(Value) != Val; }
private:
	size_t Value;
};

#endif // !ANGKASA1_ASSETS_HANDLE