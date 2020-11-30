#include "Vec3.h"


template <typename T>
TVector3<T>::TVector3() :
	x(0), y(0), z(0)
{}

template <typename T>
TVector3<T>::TVector3(T Scalar) :
	x(Scalar), y(Scalar), z(Scalar)
{}

template <typename T>
TVector3<T>::TVector3(T S1, T S2, T S3) :
	x(S1), y(S2), z(S3)
{}

template <typename T>
TVector3<T>::TVector3(const TVector2<T>& V, T Scalar) :
	x(V.x), y(V.y), z(Scalar)
{}

template <typename T>
TVector3<T>::TVector3(T Scalar, const TVector2<T>& V) :
	x(Scalar), y(V.x), z(V.y)
{}

template <typename T>
TVector3<T>::TVector3(const TVector3<T>& Rhs)
{
	*this = Rhs;
}

template <typename T>
TVector3<T>::TVector3(TVector3<T>&& Rhs)
{
	*this = Move(Rhs);
}

template<typename T>
inline TVector3<T>::~TVector3()
{
	x = y = z = 0;
}

template<typename T>
TVector3<T>& TVector3<T>::operator=(const TVector3<T>& Rhs)
{
	if (this != &Rhs)
	{
		x = Rhs.x;
		y = Rhs.y;
		z = Rhs.z;
	}
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator=(TVector3<T>&& Rhs)
{
	if (this != &Rhs)
	{
		x = Rhs.x;
		y = Rhs.y;
		z = Rhs.z;
		new (&Rhs) TVector3<T>();
	}
	return *this;
}

template<typename T>
T& TVector3<T>::operator[](size_t Index)
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template<typename T>
const T& TVector3<T>::operator[](size_t Index) const
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template<typename T>
TVector3<T>& TVector3<T>::operator+=(const TVector3& Rhs)
{
	x += Rhs.x;
	y += Rhs.y;
	z += Rhs.z;
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator+=(T Scalar)
{
	x += Scalar;
	y += Scalar;
	z += Scalar;
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator-=(const TVector3<T>& Rhs)
{
	x -= Rhs.x;
	y -= Rhs.y;
	z -= Rhs.z;
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator-=(T Scalar)
{
	x -= Scalar;
	y -= Scalar;
	z -= Scalar;
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator*=(const TVector3<T>& Rhs)
{
	x *= Rhs.x;
	y *= Rhs.y;
	z *= Rhs.z;
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator*=(T Scalar)
{
	x *= Scalar;
	y *= Scalar;
	z *= Scalar;
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator/=(const TVector3<T>& Rhs)
{
	x /= Rhs.x;
	y /= Rhs.y;
	z /= Rhs.z;
	return *this;
}

template<typename T>
TVector3<T>& TVector3<T>::operator/=(T Scalar)
{
	x /= Scalar;
	y /= Scalar;
	z /= Scalar;
	return *this;
}

template <typename T>
TVector3<T> operator+ (const TVector3<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector3<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y, Lhs.z + Rhs.z);
}

template<typename T>
TVector3<T> operator+(const TVector3<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector3<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y, Lhs.z);
}

template<typename T>
TVector3<T> operator+(const TVector3<T>& V, T Scalar)
{
	return TVector3<T>(V.x + Scalar, V.y + Scalar, V.z + Scalar);
}

template<typename T>
TVector3<T> operator+(T Scalar, const TVector3<T>& V)
{
	return TVector3<T>(V.x + Scalar, V.y + Scalar, V.z + Scalar);
}

template<typename T>
TVector3<T> operator-(const TVector3<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector3<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y, Lhs.z - Rhs.z);
}

template<typename T>
TVector3<T> operator-(const TVector3<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector3<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y, Lhs.z);
}

template<typename T>
TVector3<T> operator-(const TVector3<T>& V, T Scalar)
{
	return TVector3<T>(V.x - Scalar, V.y - Scalar, V.z - Scalar);
}

template<typename T>
TVector3<T> operator-(T Scalar, const TVector3<T>& V)
{
	return TVector3<T>(Scalar - V.x, Scalar - V.y, Scalar - V.z);
}

template<typename T>
TVector3<T> operator*(const TVector3<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector3<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y, Lhs.z * Rhs.z);
}

template<typename T>
TVector3<T> operator*(const TVector3<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector3<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y, Lhs.z);
}

template<typename T>
TVector3<T> operator*(const TVector3<T>& V, T Scalar)
{
	return TVector3<T>(V.x * Scalar, V.y * Scalar, V.z * Scalar);
}

template<typename T>
TVector3<T> operator*(T Scalar, const TVector3<T>& V)
{
	return TVector3<T>(V.x * Scalar, V.y * Scalar, V.z * Scalar);
}

template<typename T>
TVector3<T> operator/(const TVector3<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector3<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y, Lhs.z / Rhs.z);
}

template<typename T>
TVector3<T> operator/(const TVector3<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector3<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y, Lhs.z);
}

template<typename T>
TVector3<T> operator/(const TVector3<T>& V, T Scalar)
{
	return TVector3<T>(V.x / Scalar, V.y / Scalar, V.z / Scalar);
}

template<typename T>
TVector3<T> operator/(T Scalar, const TVector3<T>& V)
{
	return TVector3<T>(Scalar / V.x, Scalar / V.y, Scalar / V.z);
}

template<typename T>
bool operator==(const TVector3<T>& Lhs, const TVector3<T>& Rhs)
{
	return CompareFloat(Lhs.x, Rhs.x, FLOAT_EPSILON) && CompareFloat(Lhs.y, Rhs.y, FLOAT_EPSILON) && CompareFloat(Lhs.z, Rhs.z, FLOAT_EPSILON);
}

template<typename T>
bool operator!=(const TVector3<T>& Lhs, const TVector2<T>& Rhs)
{
	return !CompareFloat(Lhs.x, Rhs.x, FLOAT_EPSILON) || !CompareFloat(Lhs.y, Rhs.y, FLOAT_EPSILON) || !CompareFloat(Lhs.z, Rhs.z, FLOAT_EPSILON);
}

namespace Math
{
	template <typename T>
	T Length(const TVector3<T>& V)
	{
		return FMath::Sqrt((V.x * V.x) + (V.y * V.y) + (V.z * V.z));
	}

	template <typename T>
	TVector3<T> Normalize(const TVector3<T>& V)
	{
		T length = Length(V);
		return TVector3<T>(V.x / length, V.y / length, V.z / length);
	}

	template <typename T>
	T Dot(const TVector3<T>& V)
	{
		return (V.x * V.x) + (V.y * V.y) + (V.y * V.y);
	}

	template <typename T>
	T Dot(const TVector3<T>& Lhs, const TVector3<T>& Rhs)
	{
		return (Lhs.x * Rhs.x) + (Lhs.y * Rhs.y) + (Lhs.z * Rhs.z);
	}

	template <typename T>
	TVector3<T> Cross(const TVector3<T>& Lhs, const TVector3<T>& Rhs)
	{
		T x = (Lhs.y * Rhs.z) - (Lhs.z * Rhs.y);
		T y = (Lhs.z * Rhs.x) - (Lhs.x * Rhs.z);
		T z = (Lhs.x * Rhs.y) - (Lhs.y * Rhs.x);
		return TVector3<T>(x, y, z);
	}

	template <typename T>
	T Area(const TVector3<T>& Lhs, const TVector3<T>& Rhs)
	{
		TVector3<T> cross = Cross(Lhs, Rhs);
		return Length(cross);
	}

	template <typename T>
	void NormalizeVector(TVector3<T>& V)
	{
		const T length = Length(V);
		V.x /= length;
		V.y /= length;
		V.z /= length;
	}
}