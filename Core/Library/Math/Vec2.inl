#include "Vec2.h"
#include "Math.h"

template <typename T>
TVector2<T>::TVector2() : 
	x(0), y(0) 
{}

template <typename T>
TVector2<T>::TVector2(T Scalar) :
	x(Scalar), y(Scalar)
{}

template <typename T>
TVector2<T>::TVector2(T S1, T S2) :
	x(S1), y(S2)
{}

template <typename T>
TVector2<T>::~TVector2() 
{ 
	x = y = 0; 
}

template <typename T>
TVector2<T>::TVector2(const TVector2<T>& Rhs)
{
	*this = Rhs;
}

template <typename T>
TVector2<T>::TVector2(TVector2<T>&& Rhs)
{
	*this = Move(Rhs);
}

template <typename T>
T& TVector2<T>::operator[] (size_t Index)
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template <typename T>
const T& TVector2<T>::operator[] (size_t Index) const
{
	VKT_ASSERT(Index >= Size());
	return (&x)[Index];
}

template <typename T>
TVector2<T>& TVector2<T>::operator= (const TVector2<T>& Rhs)
{
	if (this != &Rhs)
	{
		x = Rhs.x;
		y = Rhs.y;
	}
	return *this;
}

template <typename T>
TVector2<T>& TVector2<T>::operator= (TVector2<T>&& Rhs)
{
	if (this != &Rhs)
	{
		x = Rhs.x;
		y = Rhs.y;
		new (&Rhs) TVector2<T>();
	}
	return *this;
}

template<typename T>
TVector2<T>& TVector2<T>::operator+=(const TVector2& Rhs)
{
	x += Rhs.x;
	y += Rhs.y;
	return *this;
}

template <typename T>
TVector2<T>& TVector2<T>::operator+= (T Scalar)
{
	x += Scalar;
	y += Scalar;
	return *this;
}

template<typename T>
TVector2<T>& TVector2<T>::operator-=(const TVector2& Rhs)
{
	x -= Rhs.x;
	y -= Rhs.y;
	return *this;
}

template<typename T>
TVector2<T>& TVector2<T>::operator-=(T Scalar)
{
	x -= Scalar;
	y -= Scalar;
	return *this;
}

template<typename T>
TVector2<T>& TVector2<T>::operator*=(const TVector2& Rhs)
{
	x *= Rhs.x;
	y *= Rhs.y;
	return *this;
}

template<typename T>
TVector2<T>& TVector2<T>::operator*=(T Scalar)
{
	x *= Scalar;
	y *= Scalar;
	return *this;
}

template<typename T>
TVector2<T>& TVector2<T>::operator/=(const TVector2& Rhs)
{
	x /= Rhs.x;
	y /= Rhs.y;
	return *this;
}

template<typename T>
TVector2<T>& TVector2<T>::operator/=(T Scalar)
{
	x /= Scalar;
	y /= Scalar;
	return *this;
}

template <typename T>
TVector2<T> operator+ (const TVector2<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector2<T>(Lhs.x + Rhs.x, Lhs.y + Rhs.y);
}

template<typename T>
TVector2<T> operator+(const TVector2<T>& V, T Scalar)
{
	return TVector2<T>(V.x + Scalar, V.y + Scalar);
}

template<typename T>
TVector2<T> operator+(T Scalar, const TVector2<T>& V)
{
	return TVector2<T>(V.x + Scalar, V.y + Scalar);
}

template<typename T>
TVector2<T> operator-(const TVector2<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector2<T>(Lhs.x - Rhs.x, Lhs.y - Rhs.y);
}

template<typename T>
TVector2<T> operator-(const TVector2<T>& V, T Scalar)
{
	return TVector2<T>(V.x - Scalar, V.y - Scalar);
}

template<typename T>
TVector2<T> operator-(T Scalar, const TVector2<T>& V)
{
	return TVector2<T>(Scalar - V.x, Scalar - V.y);
}

template<typename T>
TVector2<T> operator*(const TVector2<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector2<T>(Lhs.x * Rhs.x, Lhs.y * Rhs.y);
}

template<typename T>
TVector2<T> operator*(const TVector2<T>& V, T Scalar)
{
	return TVector2<T>(V.x * Scalar, V.y * Scalar);
}

template<typename T>
TVector2<T> operator*(T Scalar, const TVector2<T>& V)
{
	return TVector2<T>(V.x * Scalar, V.y * Scalar);
}

template<typename T>
TVector2<T> operator/(const TVector2<T>& Lhs, const TVector2<T>& Rhs)
{
	return TVector2<T>(Lhs.x / Rhs.x, Lhs.y / Rhs.y);
}

template<typename T>
TVector2<T> operator/(const TVector2<T>& V, T Scalar)
{
	return TVector2<T>(V.x / Scalar, V.y / Scalar);
}

template<typename T>
TVector2<T> operator/(T Scalar, const TVector2<T>& V)
{
	return TVector2<T>(Scalar / V.x, Scalar / V.y);
}

template<typename T>
bool operator==(const TVector2<T>& Lhs, const TVector2<T>& Rhs)
{
	return CompareFloat(Lhs.x, Rhs.x, FLOAT_EPSILON) && CompareFloat(Lhs.y, Rhs.y, FLOAT_EPSILON);
}

template<typename T>
bool operator!=(const TVector2<T>& Lhs, const TVector2<T>& Rhs)
{
	return !CompareFloat(Lhs.x, Rhs.x, FLOAT_EPSILON) || !CompareFloat(Lhs.y, Rhs.y, FLOAT_EPSILON);
}

namespace Math
{
	template <typename T>
	T Length(const TVector2<T>& V)
	{
		return FMath::Sqrt((V.x * V.x) + (V.y * V.y));
	}

	template <typename T>
	TVector2<T> Normalize(const TVector2<T>& V)
	{
		T length = Length(V);
		return TVector2<T>(V.x / length, V.y / length);
	}

	template <typename T>
	T Dot(const TVector2<T>& V)
	{
		return (V.x * V.x) + (V.y * V.y);
	}

	template <typename T>
	T Dot(const TVector2<T>& Lhs, const TVector2<T>& Rhs)
	{
		return (Lhs.x * Rhs.x) + (Lhs.y * Rhs.y);
	}

	template <typename T>
	T Cross(const TVector2<T>& Lhs, const TVector2<T>& Rhs)
	{
		return (Lhs.x * Rhs.y) - (Lhs.y * Rhs.x);
	}

	template <typename T>
	T Cross(const TVector2<T>& V)
	{
		return TVector2<T>(V.y, -V.x);
	}

	template <typename T>
	void NormalizeVector(TVector2<T>& V)
	{
		const T length = Length(V);
		V.x /= length;
		V.y /= length;
	}
}