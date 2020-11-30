#pragma once
#ifndef LEARNVK_LIBRARY_MATH_MAT4
#define LEARNVK_LIBRARY_MATH_MAT4

#include "Vec4.h"


/// <summary>
/// Row major 4x4 matrix.
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
class TMat4x4
{
public:

	using VecType = TVector4<T>;
	using MatType = TMat4x4<T>;

private:

	VecType mat[4];

public:

	TMat4x4();
	~TMat4x4();

	TMat4x4(const TMat4x4& Rhs);
	TMat4x4(TMat4x4&& Rhs);

	TMat4x4& operator= (const TMat4x4& Rhs);
	TMat4x4& operator= (TMat4x4&& Rhs);

	TMat4x4(T Scalar);

	TMat4x4(T x0, T y0, T z0, T w0,
			T x1, T y1, T z1, T w1,
			T x2, T y2, T z2, T w2,
			T x3, T y3, T z3, T w3);

	TMat4x4(
		const TVector4<T>& v0,
		const TVector4<T>& v1,
		const TVector4<T>& v2,
		const TVector4<T>& v3
	);

	VecType&		operator[] (size_t Index);
	const VecType&	operator[] (size_t Index) const;

	inline static size_t Size() { return 4; }

	TMat4x4& operator+= (const TMat4x4& Rhs);
	TMat4x4& operator+= (T Scalar);
	TMat4x4& operator-= (const TMat4x4& Rhs);
	TMat4x4& operator-= (T Scalar);
	TMat4x4& operator*= (const TMat4x4& Rhs);
	TMat4x4& operator*= (T Scalar);
};


template <typename T> TMat4x4<T> operator+ (const TMat4x4<T>& Lhs, const TMat4x4<T>& Rhs);
template <typename T> TMat4x4<T> operator+ (const TMat4x4<T>& M, T Scalar);
template <typename T> TMat4x4<T> operator+ (T Scalar, const TMat4x4<T>& M);

template <typename T> TMat4x4<T> operator- (const TMat4x4<T>& Lhs, const TMat4x4<T>& Rhs);
template <typename T> TMat4x4<T> operator- (const TMat4x4<T>& M, T Scalar);
template <typename T> TMat4x4<T> operator- (T Scalar, const TMat4x4<T>& M);

template <typename T> TMat4x4<T> operator* (const TMat4x4<T>& Lhs, const TMat4x4<T>& Rhs);
template <typename T> TMat4x4<T> operator* (const TMat4x4<T>& M, T Scalar);
template <typename T> TMat4x4<T> operator* (T Scalar, const TMat4x4<T>& M);

template <typename T> TVector4<T> operator* (const TVector4<T>& V, const TMat4x4<T>& M);
template <typename T> TVector4<T> operator* (const TMat4x4<T>& M, const TVector4<T>& V);


namespace Math
{
	template <typename T> T				Determinant	(const TMat4x4<T>& M);
	template <typename T> TMat4x4<T>	Transpose	(const TMat4x4<T>& M);
	template <typename T> TMat4x4<T>	Inverse		(const TMat4x4<T>& M);

	template <typename T> TMat4x4<T>	Translate	(const TMat4x4<T>& M, const TVector3<T>& V);
	template <typename T> TMat4x4<T>	Scale		(const TMat4x4<T>& M, const TVector3<T>& V);
	template <typename T> TMat4x4<T>	Rotate		(const TMat4x4<T>& M, T Angle, const TVector3<T>& V);

	template <typename T> TMat4x4<T>	Translate	(const TMat4x4<T>& M, const TVector4<T>& V);
	template <typename T> TMat4x4<T>	Scale		(const TMat4x4<T>& M, const TVector4<T>& V);
	template <typename T> TMat4x4<T>	Rotate		(const TMat4x4<T>& M, T Angle, const TVector4<T>& V);
}

using mat4	= TMat4x4<float32>;
using dmat4 = TMat4x4<float64>;

#include "Mat4.inl"
#endif // !LEARNVK_LIBRARY_MATH_MAT4