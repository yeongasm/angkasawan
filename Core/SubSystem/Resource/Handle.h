#pragma once
#ifndef LEARNVK_ASSETS_HANDLE
#define LEARNVK_ASSETS_HANDLE

#include "Library/Templates/Types.h"

#define INVALID_HANDLE static_cast<size_t>(-1)

template <typename Type>
class Handle
{
public:
	Handle()	= default;
	~Handle()	= default;
	Handle(const Handle&)	= default;
	Handle(Handle&&)		= default;
	Handle& operator=(const Handle&) = default;
	Handle& operator=(Handle&&)		 = default;

	Handle(size_t i) : OffsetToResource(i) {}

	operator size_t() const { return OffsetToResource; }
	//size_t operator+ (size_t i) const { return OffsetToResource + i; }
	operator uint32() const { return static_cast<uint32>(OffsetToResource); }

	bool operator== (size_t Val)	{ return OffsetToResource == Val; }
	bool operator== (uint32 Val)	{ return static_cast<uint32>(OffsetToResource) == Val; }
	bool operator== (int32	Val)	{ return static_cast<int32>(OffsetToResource) == Val; }
	bool operator!= (size_t Val)	{ return OffsetToResource != Val; }
	bool operator!= (uint32 Val)	{ return static_cast<uint32>(OffsetToResource) != Val; }
	bool operator!= (int32	Val)	{ return static_cast<int32>(OffsetToResource) != Val; }

	using ResourceType = Type;
private:
	size_t OffsetToResource;
};

#endif // !LEARNVK_ASSETS_HANDLE