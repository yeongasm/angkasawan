#include "Vec4.h"


template <typename T>
TVector4<T>::TVector4() :
	x(0), y(0), z(0), w(0)
{}

template <typename T>
TVector4<T>::TVector4(T Scalar) :
	x(Scalar), y(Scalar), z(Scalar), w(Scalar)
{}

template<typename T>
TVector4<T>::TVector4(T S1, T S2, T S3, T S4) :
	x(S1), y(S2), z(S3), w(S4)
{}

template<typename T>
TVector4<T>::TVector4(const TVector2<T>& V) :
	x(V.x), y(V.y), z(0), w(0)
{}

template<typename T>
TVector4<T>::TVector4(const TVector2<T>& V, T S1, T S2) :
	x(V.x), y(V.y), z(S1), w(S2)
{}

template<typename T>
TVector4<T>::TVector4(const TVector2<T>& V1, const TVector2<T>& V2) :
	x(V1.x), y(V1.y), z(V2.x), w(V2.y)
{}

template<typename T>
TVector4<T>::TVector4(const TVector3<T>& V, T Scalar) :
	x(V.x), y(V.y), z(V.z), w(Scalar)
{}

template<typename T>
TVector4<T>::TVector4(T Scalar, const TVector3<T>& V) :
	x(Scalar), y(V.x), z(V.y), w(V.z)
{}

template<typename T>
TVector4<T>::~TVector4()
{
	x = y = z = w = 0;
}

template<typename T>
TVector4<T>::TVector4(const TVector4<T>& Rhs)
{
	*this = Rhs;
}

template<typename T>
TVector4<T>::TVector4(TVector4<T>&& Rhs)
{
	*this = Move(Rhs);
}

template<typename T>
TVector4<T>& TVector4<T>::operator=(const TVector4<T>& Rhs)
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

template<typename T>
TVector4<T>& TVector4<T>::operator=(TVector4<T>&& Rhs)
{
	if (this != &Rhs)
	{
		x = Rhs.x;
		y = Rhs.y;
		z = Rhs.z;
		w = Rhs.w;
		new (&Rhs) TVector4<T>();
	}
	return *this;
}

template<typename T>
T& TVector4<T>::operator[](size_t Index)
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template<typename T>
const T& TVector4<T>::operator[](size_t Index) const
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template<typename T>
TVector4<T>& TVector4<T>::operator+=(const TVector4<T>& Rhs)
{
	x += Rhs.x;
	y += Rhs.y;
	z += Rhs.z;
	w += Rhs.w;
	return *this;
}

template<typename T>
TVector4<T>& TVector4<T>::operator+=(T Scalar)
{
	x += Scalar;
	y += Scalar;
	z += Scalar;
	w += Scalar;
	return *this;
}

template<typename T>
TVector4<T>& TVector4<T>::operator-=(const TVector4<T>& Rhs)
{
	x -= Rhs.x;
	y -= Rhs.y;
	z -= Rhs.z;
	w -= Rhs.w;
	return *this;
}

template<typename T>
TVector4<T>& TVector4<T>::operator-=(T Scalar)
{
	x -= Scalar;
	y -= Scalar;
	z -= Scalar;
	w -= Scalar;
	return *this;
}

template<typename T>
TVector4<T>& TVector4<T>::operator*=(const TVector4<T>& Rhs)
{
	x *= Rhs.x;
	y *= Rhs.y;
	z *= Rhs.z;
	w *= Rhs.w;
	return *this;
}

template<typename T>
TVector4<T>& TVector4<T>::operator*=(T Scalar)
{
	x *= Scalar;
	y *= Scalar;
	z *= Scalar;
	w *= Scalar;
	return *this;
}

template<typename T>
TVector4<T>& TVector4<T>::operator/=(const TVector4<T>& Rhs)
{
	x /= Rhs.x;
	y /= Rhs.y;
	z /= Rhs.z;
	w /= Rhs.w;
	return *this;
}

template<typename T>
TVector4<T>& TVector4<T>::operator/=(T Scalar)
{
	x /= Scalar;
	y /= Scalar;
	z /= Scalar;
	w /= Scalar;
	return *this;
}

template <typename T>
TVector4<T> operator+ (const TVector4<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y, Lhs.z + Rhs.z, Lhs.w + Rhs.w);
}

template<typename T>
TVector4<T>& operator+(const TVector4<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector4<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y, Lhs.z + Rhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator+(const TVector3<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y, Lhs.z + Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator+(const TVector4<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector4<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y, Lhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator+(const TVector2<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y, Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator+(const TVector4<T>& V, T Scalar)
{
	return TVector4<T>(V.x + Scalar, V.y + Scalar, V.z + Scalar, V.w + Scalar);
}

template<typename T>
TVector4<T>& operator+(T Scalar, const TVector4<T>& V)
{
	return TVector4<T>(Scalar + V.x, Scalar + V.y, Scalar + V.z, Scalar + V.w);
}

template <typename T>
TVector4<T> operator- (const TVector4<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y, Lhs.z - Rhs.z, Lhs.w - Rhs.w);
}

template<typename T>
TVector4<T>& operator-(const TVector4<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector4<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y, Lhs.z - Rhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator-(const TVector3<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y, Lhs.z - Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator-(const TVector4<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector4<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y, Lhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator-(const TVector2<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y, Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator-(const TVector4<T>& V, T Scalar)
{
	return TVector4<T>(V.x - Scalar, V.y - Scalar, V.z - Scalar, V.w - Scalar);
}

template<typename T>
TVector4<T>& operator-(T Scalar, const TVector4<T>& V)
{
	return TVector4<T>(Scalar - V.x, Scalar - V.y, Scalar - V.z, Scalar - V.w);
}

template <typename T>
TVector4<T> operator* (const TVector4<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y, Lhs.z * Rhs.z, Lhs.w * Rhs.w);
}

template<typename T>
TVector4<T>& operator*(const TVector4<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector4<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y, Lhs.z * Rhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator*(const TVector3<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y, Lhs.z * Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator*(const TVector4<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector4<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y, Lhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator*(const TVector2<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y, Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator*(const TVector4<T>& V, T Scalar)
{
	return TVector4<T>(V.x * Scalar, V.y * Scalar, V.z * Scalar, V.w * Scalar);
}

template<typename T>
TVector4<T>& operator*(T Scalar, const TVector4<T>& V)
{
	return TVector4<T>(Scalar * V.x, Scalar * V.y, Scalar * V.z, Scalar * V.w);
}

template <typename T>
TVector4<T> operator/ (const TVector4<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y, Lhs.z / Rhs.z, Lhs.w / Rhs.w);
}

template<typename T>
TVector4<T>& operator/(const TVector4<T>& Lhs, const TVector3<T>& Rhs)
{
	return TVector4<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y, Lhs.z / Rhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator/(const TVector3<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y, Lhs.z / Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator/(const TVector4<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector4<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y, Lhs.z, Lhs.w);
}

template<typename T>
TVector4<T>& operator/(const TVector2<T>& Lhs, const TVector4<T>& Rhs)
{
	return TVector4<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y, Rhs.z, Rhs.w);
}

template<typename T>
TVector4<T>& operator/(const TVector4<T>& V, T Scalar)
{
	return TVector4<T>(V.x / Scalar, V.y / Scalar, V.z / Scalar, V.w / Scalar);
}

template<typename T>
TVector4<T>& operator/(T Scalar, const TVector4<T>& V)
{
	return TVector4<T>(Scalar / V.x, Scalar / V.y, Scalar / V.z, Scalar / V.w);
}

template<typename T>
bool operator==(const TVector4<T>& Lhs, const TVector4<T>& Rhs)
{
	return	CompareFloat(Lhs.x, Rhs.x, FLOAT_EPSILON) && CompareFloat(Lhs.y, Rhs.y, FLOAT_EPSILON) && 
			CompareFloat(Lhs.z, Rhs.z, FLOAT_EPSILON) && CompareFloat(Lhs.w, Rhs.w, FLOAT_EPSILON);
}

template<typename T>
bool operator!=(const TVector4<T>& Lhs, const TVector4<T>& Rhs)
{
	return	!CompareFloat(Lhs.x, Rhs.x, FLOAT_EPSILON) || !CompareFloat(Lhs.y, Rhs.y, FLOAT_EPSILON) ||
			!CompareFloat(Lhs.z, Rhs.z, FLOAT_EPSILON) || !CompareFloat(Lhs.w, Rhs.w, FLOAT_EPSILON);
}

namespace Math
{
	template <typename T>
	T Length(const TVector4<T>& V)
	{
		return FMath::Sqrt((V.x * V.x) + (V.y * V.y) + (V.z * V.z) + (V.w * V.w));
	}

	template <typename T>
	T Normalize(const TVector4<T>& V)
	{
		T length = Length(V);
		return TVector4<T>(V.x / length, V.y / length, V.z / length, V.w / length);
	}

	template <typename T>
	T Dot(const TVector4<T>& V)
	{
		return (V.x * V.x) + (V.y * V.y) + (V.z * V.z) + (V.w * V.w);
	}

	template <typename T>
	T Dot(const TVector4<T>& Lhs, const TVector4<T>& Rhs)
	{
		return (Lhs.x * Rhs.x) + (Lhs.y * Rhs.y) + (Lhs.z * Rhs.z) + (Lhs.w * Rhs.w);
	}

	template <typename T>
	TVector4<T> Cross(const TVector4<T>& Lhs, const TVector4<T>& Rhs)
	{
		T x = (Lhs.y * Rhs.z) - (Lhs.z * Rhs.y);
		T y = (Lhs.z * Rhs.x) - (Lhs.x * Rhs.z);
		T z = (Lhs.x * Rhs.y) - (Lhs.y * Rhs.z);
		return TVector4<T>(x, y, z, static_cast<T>(1));
	}

	template <typename T>
	void NormalizeVector(TVector4<T>& V)
	{
		const size_t length = Length(V);
		V.x /= length;
		V.y /= length;
		V.z /= length;
		V.w /= length;
	}
}