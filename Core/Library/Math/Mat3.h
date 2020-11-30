#pragma once
#ifndef LEARNVK_LIBRARY_MATH_MAT2
#define LEARNVK_LIBRARY_MATH_MAT2

#include "Vec3.h"


/// <summary>
/// Row major 3x3 matrix.
/// </summary>
/// <typeparam name="T"></typeparam>
template <typename T>
class TMat3x3
{
public:

	using VecType = TVector3<T>;
	using RowType = VecType;
	using ColType = VecType;
	using MatType = TMat3x3<T>;

private:

	VecType mat[3];

public:

	TMat3x3();
	~TMat3x3();

	TMat3x3(const TMat3x3& Rhs);
	TMat3x3(TMat3x3&& Rhs);

	TMat3x3& operator= (const TMat3x3& Rhs);
	TMat3x3& operator= (TMat3x3&& Rhs);

	TMat3x3(T Scalar);

	TMat3x3(T x0, T y0, T z0, 
			T x1, T y1, T z1,
			T x2, T y2, T z2);

	TMat3x3(
		const TVector3<T>& v0,
		const TVector3<T>& v1,
		const TVector3<T>& v2
	);

	VecType&		operator[] (size_t Index);
	const VecType&	operator[] (size_t Index) const;

	inline static size_t Size() { return 3; }

	TMat3x3& operator+= (const TMat3x3& Rhs);
	TMat3x3& operator+= (T Scalar);
	TMat3x3& operator-= (const TMat3x3& Rhs);
	TMat3x3& operator-= (T Scalar);
	TMat3x3& operator*= (const TMat3x3& Rhs);
	TMat3x3& operator*= (T Scalar);
};


template <typename T> TMat3x3<T> operator+ (const TMat3x3<T>& Lhs, const TMat3x3<T>& Rhs);
template <typename T> TMat3x3<T> operator+ (const TMat3x3<T>& M, T Scalar);
template <typename T> TMat3x3<T> operator+ (T Scalar, const TMat3x3<T>& M);

template <typename T> TMat3x3<T> operator- (const TMat3x3<T>& Lhs, const TMat3x3<T>& Rhs);
template <typename T> TMat3x3<T> operator- (const TMat3x3<T>& M, T Scalar);
template <typename T> TMat3x3<T> operator- (T Scalar, const TMat3x3<T>& M);

// u = v * M^T = M * v

template <typename T> TMat3x3<T> operator* (const TMat3x3<T>& Lhs, const TMat3x3<T>& Rhs);
template <typename T> TMat3x3<T> operator* (const TMat3x3<T>& M, T Scalar);
template <typename T> TMat3x3<T> operator* (T Scalar, const TMat3x3<T>& M);

template <typename T> TVector3<T> operator* (const TVector3<T>& V, const TMat3x3<T>& M);
template <typename T> TVector3<T> operator* (const TMat3x3<T>& M, const TVector3<T>& V);


namespace Math
{
	template <typename T> T				Determinant	(const TMat3x3<T>& M);
	template <typename T> TMat3x3<T>	Transpose	(const TMat3x3<T>& M);
	template <typename T> TMat3x3<T>	Inverse		(const TMat3x3<T>& M);
}


using mat3	= TMat3x3<float32>;
using dmat3 = TMat3x3<float64>;


#include "Mat3.inl"
#endif // !LEARNVK_LIBRARY_MATH_MAT2