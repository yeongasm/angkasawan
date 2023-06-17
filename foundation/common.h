#pragma once
#ifndef FOUNDATION_COMMON_H
#define FOUNDATION_COMMON_H

#include <cassert>
#include <new>
#include <utility>
#include <optional>
#include <concepts>
#include <algorithm>
#include <functional>

using int8		= char;
using int16		= short;
using int32		= int;
using int64		= long long;

using byte		= unsigned char;

using uint8		= unsigned char;
using uint16	= unsigned short;
using uint32	= unsigned int;
using uint64	= unsigned long long;

using word		= unsigned short;
using dword		= unsigned long;

using float32	= float;
using float64	= double;

using literal_t			= const char*;
using wide_literal_t	= const wchar_t*;

#define FTLBEGIN namespace ftl {
#define FTLEND }

#define FLOAT_EPSILON	0.0001f
#define DOUBLE_EPSILON	0.0001

#if _DEBUG
#define ASSERTION(expr)	assert(expr)
#else
#define ASSERTION(expr)
#endif

#endif // !FOUNDATION_COMMON_H
