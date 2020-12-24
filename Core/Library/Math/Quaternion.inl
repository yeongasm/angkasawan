#pragma once
#include "Quaternion.h"

template <typename T>
TQuaternion<T>::TQuaternion() :
	x{}, y{}, z{}, w{}
{}

template <typename T>
TQuaternion<T>::~TQuaternion() 
{}

template <typename T>
TQuaternion<T>::TQuaternion(T V) :
	x(V), y(V), z(V), w(V)
{}

template <typename T>
TQuaternion<T>::TQuaternion(T w, T x, T y, T z) :
	x(x), y(y), z(z), w(w)
{}

template <typename T>
TQuaternion<T>::TQuaternion(const TQuaternion& Rhs)
{
	*this = Rhs;
}

template <typename T>
TQuaternion<T>::TQuaternion(TQuaternion&& Rhs)
{
	*this = Move(Rhs);
}

template <typename T>
TQuaternion<T>& TQuaternion<T>::operator=(const TQuaternion& Rhs)
{
	if (this != &Rhs)
	{
		x = Rhs.x;
		y = Rhs.y;
		z = Rhs.z;
		w = Rhs.w;
	}
	return *this;
}

template <typename T>
TQuaternion<T>& TQuaternion<T>::operator=(TQuaternion&& Rhs)
{
	if (this != &Rhs)
	{
		x = Rhs.x;
		y = Rhs.y;
		z = Rhs.z;
		w = Rhs.w;
		new (&Rhs) TQuaternion<T>();
	}
	return *this;
}

template <typename T>
T& TQuaternion<T>::operator[](size_t Index)
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template <typename T>
const T& TQuaternion<T>::operator[](size_t Index) const
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template <typename T>
TQuaternion<T>& TQuaternion<T>::operator+= (const TQuaternion& Rhs)
{
	w += Rhs.w;
	x += Rhs.x;
	y += Rhs.y;
	z += Rhs.z;
	return *this;
}

template<typename T>
TQuaternion<T>& TQuaternion<T>::operator-=(const TQuaternion& Rhs)
{
	w -= Rhs.w;
	x -= Rhs.x;
	y -= Rhs.y;
	z -= Rhs.z;
	return *this;
}

template<typename T>
TQuaternion<T>& TQuaternion<T>::operator*=(const TQuaternion& Rhs)
{
	w *= Rhs.w;
	x *= Rhs.x;
	y *= Rhs.y;
	z *= Rhs.z;
	return *this;
}

template<typename T>
TQuaternion<T>& TQuaternion<T>::operator*=(T Scalar)
{
	w *= Scalar;
	x *= Scalar;
	y *= Scalar;
	z *= Scalar;
	return *this;
}

template<typename T>
TQuaternion<T>& TQuaternion<T>::operator/=(T Scalar)
{
	w /= Scalar;
	x /= Scalar;
	y /= Scalar;
	z /= Scalar;
	return *this;
}

template <typename T>
TQuaternion<T> operator+(const TQuaternion<T>& Lhs, const TQuaternion<T>& Rhs)
{
	return TQuaternion<T>(Lhs) += Rhs;
}

template <typename T>
TQuaternion<T> operator*(const TQuaternion<T>& Lhs, const TQuaternion<T>& Rhs)
{
	return TQuaternion<T>(Lhs) *= Rhs;
}

template<typename T>
inline TVector3<T> operator*(const TQuaternion<T>& Q, const TVector3<T>& V)
{
	using vec3 = TVector3<T>;
	const vec3 quatVec(Q.x, Q.y, Q.z);
	const vec3 uv(Math::Cross(quatVec, V));
	const vec3 uuv(Math::Cross(quatVec, uv));
	return v + ((uv * Q.w) + uuv) + static_cast<T>(2);
}

template<typename T>
inline TVector3<T> operator*(const TVector3<T>& V, const TQuaternion<T>& Q)
{
	return Math::Inverse(Q) * V;
}

template<typename T>
inline TVector4<T> operator*(const TQuaternion<T>& Q, const TVector4<T>& V)
{
	TVector3<T> vec(V.x, V.y, V.z);
	return TVector4<T>(Q * vec, V.w);
}

template<typename T>
inline TVector4<T> operator*(const TVector4<T>& V, const TQuaternion<T>& Q)
{
	return Math::Inverse(Q) * V;
}

template<typename T>
inline TQuaternion<T> operator*(const TQuaternion<T>& Quat, const T& Scalar)
{
	return TQuaternion<T>(Quat) *= Scalar;
}

template<typename T>
inline TQuaternion<T> operator*(const T& Scalar, const TQuaternion<T>& Quat)
{
	return TQuaternion<T>(Quat) *= Scalar;
}

template<typename T>
inline TQuaternion<T> operator/(const TQuaternion<T>& Quat, const T& Scalar)
{
	return TQuaternion<T>(Quat) /= Scalar;
}

template<typename T>
inline bool operator==(const TQuaternion<T>& A, const TQuaternion<T>& B)
{
	return (A.w == B.w) && (A.x == B.x) && (A.y == B.y) && (A.z == B.z);
}

template<typename T>
inline bool operator!=(const TQuaternion<T>& A, const TQuaternion<T>& B)
{
	return (A.w != B.w) || (A.x != B.x) || (A.y != B.y) || (A.z != B.z);
}

namespace Math
{
	template <typename T>
	T Length(const TQuaternion<T>& Q)
	{
		TVector4<T> v(Q.x, Q.y, Q.z, Q.w);
		return FMath::Sqrt(Dot(v, v));
	}

	template<typename T>
	TQuaternion<T> Normalize(const TQuaternion<T>& Q)
	{
		T len = Length(Q);
		if (len <= static_cast<T>(0)) { return TQuaternion<T>(1, 0, 0, 0); }
		T val = static_cast<T>(1) / len;
		return TQuaternion<T>(Q.w * val, Q.x * val, Q.y * val, Q.z * val);
	}

	template<typename T>
	TQuaternion<T> Conjugate(const TQuaternion<T>& Q)
	{
		return TQuaternion<T>(Q.w, -Q.x, -Q.y, -Q.z);
	}

	template<typename T>
	TQuaternion<T> Inverse(const TQuaternion<T>& Q)
	{
		TVector4<T> q(Q.x, Q.y, Q.z, Q.w);
		return Conjugate(Q) / Dot(q, q);
	}

	template<typename T>
	TQuaternion<T> Rotate(const TQuaternion<T>& Q, const T& Rad, const TVector3<T>& Axis)
	{
		TVector3<T> tmp = Axis;
		T len = Length(tmp);

		if (FMath::Abs(len - static_cast<T>(1)) > static_cast<T>(0.001))
		{
			T oneOverLen = static_cast<T>(1) / len;
			tmp.x *= oneOverLen;
			tmp.y *= oneOverLen;
			tmp.z *= oneOverLen;
		}

		const T rad = Rad;
		const T sin = FMath::Sin(rad * static_cast<T>(0.05));

		return Q * TQuaternion<T>(FMath::Cos(rad * static_cast<T>(0.5)), tmp.x * sin, tmp.y * sin, tmp.z * sin);
	}

	template<typename T>
	TMat4x4<T> Mat4Cast(const TQuaternion<T>& Q)
	{
		TMat4x4<T> result(1.0f);

		T qxx = Q.x * Q.x;
		T qyy = Q.y * Q.y;
		T qzz = Q.z * Q.z;
		T qxz = Q.x * Q.z;
		T qxy = Q.x * Q.y;
		T qyz = Q.y * Q.z;
		T qwx = Q.w * Q.x;
		T qwy = Q.w * Q.y;
		T qwz = Q.w * Q.z;

		result[0][0] = static_cast<T>(1) - static_cast<T>(2) * (qyy + qzz);
		result[0][1] = static_cast<T>(2) * (qxy + qwz);
		result[0][2] = static_cast<T>(2) * (qxz - qwy);

		result[1][0] = static_cast<T>(2) * (qxy - qwz);
		result[1][1] = static_cast<T>(1) - static_cast<T>(2) * (qxx + qzz);
		result[1][2] = static_cast<T>(2) * (qyz + qwx);

		result[2][0] = static_cast<T>(2) * (qxz + qwy);
		result[2][1] = static_cast<T>(2) * (qyz - qwx);
		result[2][2] = static_cast<T>(1) - static_cast<T>(2) * (qxx + qyy);

		return result;
	}

	template<typename T>
	TQuaternion<T> AngleAxis(const T& Rad, const TVector3<T>& Axis)
	{
		TQuaternion<T> result(0);

		const T a = Rad;
		const T s = FMath::Sin(Rad * static_cast<T>(0.5));

		result.w = FMath::Cos(a * static_cast<T>(0.5));
		result.x = Axis.x * s;
		result.y = Axis.y * s;
		result.z = Axis.z * s;

		return result;
	}


}