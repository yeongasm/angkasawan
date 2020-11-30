#pragma once
#ifndef LEARNVK_LIBRARY_MATH_VEC3
#define LEARNVK_LIBRARY_MATH_VEC3

#include "Vec2.h"


template <typename T>
struct TVector3
{
	union
	{
		struct { T x, y, z; };
		struct { T r, g, b; };
		struct { T s, t, p; };
	};

	TVector3();
	TVector3(T Scalar);
	TVector3(T S1, T S2, T S3);

	TVector3(const TVector2<T>& V, T Scalar);
	TVector3(T Scalar, const TVector2<T>& V);

	TVector3(const TVector3& Rhs);
	TVector3(TVector3&& Rhs);

	~TVector3();

	TVector3& operator= (const TVector3& Rhs);
	TVector3& operator= (TVector3&& Rhs);

	T&			operator[] (size_t Index);
	const T&	operator[] (size_t Index) const;

	static size_t Size() { return 3; }

	TVector3& operator+= (const TVector3& Rhs);
	TVector3& operator+= (T Scalar);
	TVector3& operator-= (const TVector3& Rhs);
	TVector3& operator-= (T Scalar);
	TVector3& operator*= (const TVector3& Rhs);
	TVector3& operator*= (T Scalar);
	TVector3& operator/= (const TVector3& Rhs);
	TVector3& operator/= (T Scalar);
};


template <typename T> TVector3<T> operator+ (const TVector3<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector3<T> operator+ (const TVector3<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector3<T> operator+ (const TVector3<T>& V, T Scalar);
template <typename T> TVector3<T> operator+ (T Scalar, const TVector3<T>& V);
template <typename T> TVector3<T> operator- (const TVector3<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector3<T> operator- (const TVector3<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector3<T> operator- (const TVector3<T>& V, T Scalar);
template <typename T> TVector3<T> operator- (T Scalar, const TVector3<T>& V);
template <typename T> TVector3<T> operator* (const TVector3<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector3<T> operator* (const TVector3<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector3<T> operator* (const TVector3<T>& V, T Scalar);
template <typename T> TVector3<T> operator* (T Scalar, const TVector3<T>& V);
template <typename T> TVector3<T> operator/ (const TVector3<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector3<T> operator/ (const TVector3<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector3<T> operator/ (const TVector3<T>& V, T Scalar);
template <typename T> TVector3<T> operator/ (T Scalar, const TVector3<T>& V);

template <typename T> bool operator== (const TVector3<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> bool operator!= (const TVector3<T>& Lhs, const TVector2<T>& Rhs);


namespace Math
{
	template <typename T> T				Length		(const TVector3<T>& V);
	template <typename T> TVector3<T>	Normalize	(const TVector3<T>& V);
	template <typename T> T				Dot			(const TVector3<T>& V);
	template <typename T> T				Dot			(const TVector3<T>& Lhs, const TVector3<T>& Rhs);
	template <typename T> TVector3<T>	Cross		(const TVector3<T>& Lhs, const TVector3<T>& Rhs);
	template <typename T> T				Area		(const TVector3<T>& Lhs, const TVector3<T>& Rhs);
	template <typename T> void			NormalizeVector(TVector3<T>& V);
}


using vec3	= TVector3<float32>;
using dvec3 = TVector3<float64>;

#include "Vec3.inl"
#endif // !LEARNVK_LIBRARY_MATH_VEC3