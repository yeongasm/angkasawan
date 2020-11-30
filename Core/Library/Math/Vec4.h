#pragma once
#ifndef LEARNVK_LIBRARY_MATH_VEC4
#define LEARNVK_LIBRARY_MATH_VEC4

#include "Vec3.h"

template <typename T>
struct TVector4
{
	union
	{
		struct { T x, y, z, w; };
		struct { T r, g, b, a; };
		struct { T s, t, p, q; };
	};

	TVector4();
	TVector4(T Scalar);
	TVector4(T S1, T S2, T S3, T S4);

	TVector4(const TVector2<T>& V);
	TVector4(const TVector2<T>& V, T S1, T S2);
	TVector4(const TVector2<T>& V1, const TVector2<T>& V2);
	TVector4(const TVector3<T>& V, T Scalar);
	TVector4(T Scalar, const TVector3<T>& V);

	~TVector4();

	TVector4(const TVector4& Rhs);
	TVector4(TVector4&& Rhs);

	TVector4& operator= (const TVector4& Rhs);
	TVector4& operator= (TVector4&& Rhs);

	T&			operator[] (size_t Index);
	const T&	operator[] (size_t Index) const;

	static size_t Size() { return 4; }

	TVector4& operator+= (const TVector4& Rhs);
	TVector4& operator+= (T Scalar);
	TVector4& operator-= (const TVector4& Rhs);
	TVector4& operator-= (T Scalar);
	TVector4& operator*= (const TVector4& Rhs);
	TVector4& operator*= (T Scalar);
	TVector4& operator/= (const TVector4& Rhs);
	TVector4& operator/= (T Scalar);
};


template <typename T> TVector4<T>& operator+ (const TVector4<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator+ (const TVector4<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector4<T>& operator+ (const TVector3<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator+ (const TVector4<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector4<T>& operator+ (const TVector2<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator+ (const TVector4<T>& V, T Scalar);
template <typename T> TVector4<T>& operator+ (T Scalar, const TVector4<T>& V);
template <typename T> TVector4<T>& operator- (const TVector4<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator- (const TVector4<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector4<T>& operator- (const TVector3<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator- (const TVector4<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector4<T>& operator- (const TVector2<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator- (const TVector4<T>& V, T Scalar);
template <typename T> TVector4<T>& operator- (T Scalar, const TVector4<T>& V);
template <typename T> TVector4<T>& operator* (const TVector4<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator* (const TVector4<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector4<T>& operator* (const TVector3<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator* (const TVector4<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector4<T>& operator* (const TVector2<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator* (const TVector4<T>& V, T Scalar);
template <typename T> TVector4<T>& operator* (T Scalar, const TVector4<T>& V);
template <typename T> TVector4<T>& operator/ (const TVector4<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator/ (const TVector4<T>& Lhs, const TVector3<T>& Rhs);
template <typename T> TVector4<T>& operator/ (const TVector3<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator/ (const TVector4<T>& Lhs, const TVector2<T>& Rhs);
template <typename T> TVector4<T>& operator/ (const TVector2<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> TVector4<T>& operator/ (const TVector4<T>& V, T Scalar);
template <typename T> TVector4<T>& operator/ (T Scalar, const TVector4<T>& V);

template <typename T> bool operator== (const TVector4<T>& Lhs, const TVector4<T>& Rhs);
template <typename T> bool operator!= (const TVector4<T>& Lhs, const TVector4<T>& Rhs);


namespace Math
{
	template <typename T> T Length		(const TVector4<T>& V);
	template <typename T> T Normalize	(const TVector4<T>& V);
	template <typename T> T Dot			(const TVector4<T>& V);
	template <typename T> T Dot			(const TVector4<T>& Lhs, const TVector4<T>& Rhs);

	/// <summary>
	/// Cross product is undefined in 4 dimensional space hence the w component is discarded during computation.
	///	Computes only on the subset; x, y, and z component of the 4d vector.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="Lhs"></param>
	/// <param name="Rhs"></param>
	/// <returns></returns>
	template <typename T> TVector4<T>	Cross(const TVector4<T>& Lhs, const TVector4<T>& Rhs);
	template <typename T> void			NormalizeVector(TVector4<T>& V);
}


using vec4	= TVector4<float32>;
using dvec4 = TVector4<float64>;

#include "Vec4.inl"
#endif // !LEARNVK_LIBRARY_MATH_VEC4