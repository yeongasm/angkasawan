#pragma once
#ifndef LEARNVK_LIBRARY_MATH_VEC2
#define LEARNVK_LIBRARY_MATH_VEC2

#include "Platform/Minimal.h"
#include "Library/Templates/Templates.h"

template <typename T>
struct TVector2
{
	union
	{
		struct { T x, y; };
		struct { T r, g; };
		struct { T s, t; };
	};

	TVector2();
	TVector2(T Scalar);
	TVector2(T S1, T S2);

	~TVector2();

	TVector2(const TVector2& Rhs);
	TVector2(TVector2&& Rhs);

	T&			operator[]	(size_t Index);
	const T&	operator[]	(size_t Index) const;

	static size_t Size() { return 2; }

	TVector2& operator= (const TVector2& Rhs);
	TVector2& operator= (TVector2&& Rhs);

	TVector2& operator+= (const TVector2& Rhs);
	TVector2& operator+= (T Scalar);
	TVector2& operator-= (const TVector2& Rhs);
	TVector2& operator-= (T Scalar);
	TVector2& operator*= (const TVector2& Rhs);
	TVector2& operator*= (T Scalar);
	TVector2& operator/= (const TVector2& Rhs);
	TVector2& operator/= (T Scalar);
};


template <typename T> TVector2<T> operator+ (const TVector2<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector2<T> operator+ (const TVector2<T>& V, T Scalar);
template <typename T> TVector2<T> operator+ (T Scalar, const TVector2<T>& V);
template <typename T> TVector2<T> operator- (const TVector2<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector2<T> operator- (const TVector2<T>& V, T Scalar);
template <typename T> TVector2<T> operator- (T Scalar, const TVector2<T>& V);
template <typename T> TVector2<T> operator* (const TVector2<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector2<T> operator* (const TVector2<T>& V, T Scalar);
template <typename T> TVector2<T> operator* (T Scalar, const TVector2<T>& V);
template <typename T> TVector2<T> operator/ (const TVector2<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector2<T> operator/ (const TVector2<T>& V, T Scalar);
template <typename T> TVector2<T> operator/ (T Scalar, const TVector2<T>& V);

template <typename T> bool operator== (const TVector2<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> bool operator!= (const TVector2<T>& Lhs, const TVector2<T>& Rhs);


namespace Math
{
	template <typename T> T				Length		(const TVector2<T>& V);
	template <typename T> TVector2<T>	Normalize	(const TVector2<T>& V);
	template <typename T> T				Dot			(const TVector2<T>& V);
	template <typename T> T				Dot			(const TVector2<T>& Lhs, const TVector2<T>& Rhs);
	template <typename T> T				Cross		(const TVector2<T>& Lhs, const TVector2<T>& Rhs);
	template <typename T> TVector2<T>	Cross		(const TVector2<T>& V);

	template <typename T> void			NormalizeVector(TVector2<T>& V);
}


using vec2	= TVector2<float32>;
using dvec2	= TVector2<float64>;


#include "Vec2.inl"
#endif // !LEARNVK_LIBRARY_MATH_VEC2