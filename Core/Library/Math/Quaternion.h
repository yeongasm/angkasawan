#pragma once
#ifndef LEARNVK_LIBRARY_MATH_QUATERNION
#define LEARNVK_LIBRARY_MATH_QUATERNION

#include "Mat4.h"

template <typename T>
struct TQuaternion
{
	struct { T x, y, z, w };

	TQuaternion();
	~TQuaternion();

	TQuaternion(T V);
	TQuaternion(T w, T x, T y, T z);

	TQuaternion(const TQuaternion& Rhs);
	TQuaternion(TQuaternion&& Rhs);

	TQuaternion& operator=(const TQuaternion& Rhs);
	TQuaternion& operator=(TQuaternion&& Rhs);

	T&			operator[]	(size_t Index);
	const T&	operator[]	(size_t Index) const;

	static size_t Size() const { return 4; }

	TQuaternion& operator+= (const TQuaternion& Rhs);
	TQuaternion& operator-= (const TQuaternion& Rhs);
	TQuaternion& operator*= (const TQuaternion& Rhs);
	TQuaternion& operator*= (T Scalar);
	TQuaternion& operator/= (T Scalar);
};

template <typename T> TQuaternion<T> operator+ (const TQuaternion<T>& Lhs, const TQuaternion<T>& Rhs);
template <typename T> TQuaternion<T> operator* (const TQuaternion<T>& Lhs, const TQuaternion<T>& Rhs);

template <typename T> TVector3<T>	operator* (const TQuaternion<T>& Q, const TVector3<T>& V);
template <typename T> TVector3<T>	operator* (const TVector3<T>& V, const TQuaternion<T>& Q);

template <typename T> TVector4<T>	operator* (const TQuaternion<T>& Q, const TVector4<T>& V);
template <typename T> TVector4<T>	operator* (const TVector4<T>& V, const TQuaternion<T>& Q);

template <typename T> TQuaternion<T> operator* (const TQuaternion<T>& Quat, const T& Scalar);
template <typename T> TQuaternion<T> operator* (const T& Scalar, const TQuaternion<T>& Quat);

template <typename T> TQuaternion<T> operator/ (const TQuaternion<T>& Quat, const T& Scalar);

template <typename T> bool operator== (const TQuaternion<T>& A, const TQuaternion<T>& B);
template <typename T> bool operator!= (const TQuaternion<T>& A, const TQuaternion<T>& B);

namespace Math
{
	template <typename T> T					Length			(const TQuaternion<T>& Q);
	template <typename T> TQuaternion<T>	Normalize		(const TQuaternion<T>& Q);
	template <typename T> TQuaternion<T>	Conjugate		(const TQuaternion<T>& Q);
	template <typename T> TQuaternion<T>	Inverse			(const TQuaternion<T>& Q);
	template <typename T> TQuaternion<T>	Rotate			(const TQuaternion<T>& Q, const T& Rad, const TVector3<T>& Axis);
	template <typename T> TMat4x4<T>		Mat4Cast		(const TQuaternion<T>& Q);
	template <typename T> TQuaternion<T>	AngleAxis		(const T& Rad, const TVector3<T>& Axis);
}

using quat	= TQuaternion<float32>;
using dquat = TQuaternion<float64>;

#include "Quaternion.inl"
#endif // !LEARNVK_LIBRARY_MATH_QUATERNION