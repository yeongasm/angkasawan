#include "Mat3.h"


template <typename T>
TMat3x3<T>::TMat3x3() :
	mat{}
{}

template <typename T>
TMat3x3<T>::~TMat3x3()
{
	mat = {};
}

template <typename T>
TMat3x3<T>::TMat3x3(const TMat3x3<T>& Rhs)
{
	*this = Rhs;
}

template <typename T>
TMat3x3<T>::TMat3x3(TMat3x3<T>&& Rhs)
{
	*this = Move(Rhs);
}

template <typename T>
TMat3x3<T>& TMat3x3<T>::operator=(const TMat3x3<T>& Rhs)
{
	if (this != &Rhs)
	{
		mat[0] = Rhs.mat[0];
		mat[1] = Rhs.mat[1];
		mat[2] = Rhs.mat[2];
	}
	return *this;
}

template <typename T>
TMat3x3<T>& TMat3x3<T>::operator=(TMat3x3<T>&& Rhs)
{
	if (this != &Rhs)
	{
		mat[0] = Move(Rhs.mat[0]);
		mat[1] = Move(Rhs.mat[1]);
		mat[2] = Move(Rhs.mat[2]);
	}
	return *this;
}

template <typename T>
TMat3x3<T>::TMat3x3(T Scalar) : TMat3x3()
{
	mat[0].x = Scalar;
	mat[1].y = Scalar;
	mat[2].z = Scalar;
}

template <typename T>
TMat3x3<T>::TMat3x3(T x0, T y0, T z0,
					T x1, T y1, T z1,
					T x2, T y2, T z2)
{
	mat[0].x = x0;
	mat[0].y = y0;
	mat[0].z = z0;
	mat[1].x = x1;
	mat[1].y = y1;
	mat[1].z = z1;
	mat[2].x = x2;
	mat[2].y = y2;
	mat[2].z = z2;
}

template <typename T>
TMat3x3<T>::TMat3x3(const TVector3<T>& v0,
					const TVector3<T>& v1,
					const TVector3<T>& v2)
{
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
}

template<typename T>
TVector3<T>& TMat3x3<T>::operator[](size_t Index)
{
	VKT_ASSERT(Index >= Size());
	return mat[Index];
}

template<typename T>
const TVector3<T>& TMat3x3<T>::operator[](size_t Index) const
{
	VKT_ASSERT(Index >= Size());
	return mat[Index];
}

template <typename T>
TMat3x3<T>& TMat3x3<T>::operator+=(const TMat3x3<T>& Rhs)
{
	mat[0] += Rhs.mat[0];
	mat[1] += Rhs.mat[1];
	mat[2] += Rhs.mat[2];
	return *this;
}

template <typename T>
TMat3x3<T>& TMat3x3<T>::operator+=(T Scalar)
{
	mat[0] += Scalar;
	mat[1] += Scalar;
	mat[2] += Scalar;
	return *this;
}

template <typename T>
TMat3x3<T>& TMat3x3<T>::operator-=(const TMat3x3<T>& Rhs)
{
	mat[0] -= Rhs.mat[0];
	mat[1] -= Rhs.mat[1];
	mat[2] -= Rhs.mat[2];
	return *this;
}

template<typename T>
TMat3x3<T>& TMat3x3<T>::operator-=(T Scalar)
{
	mat[0].x -= Scalar;
	mat[1].y -= Scalar;
	mat[2].z -= Scalar;
	return *this;
}

template<typename T>
TMat3x3<T>& TMat3x3<T>::operator*=(const TMat3x3<T>& Rhs)
{
	return (*this = *this * Rhs);
}

template<typename T>
TMat3x3<T>& TMat3x3<T>::operator*=(T Scalar)
{
	mat[0] *= Scalar;
	mat[1] *= Scalar;
	mat[2] *= Scalar;
	return *this;
}

template <typename T>
TMat3x3<T> operator+ (const TMat3x3<T>& Lhs, const TMat3x3<T>& Rhs)
{
	return TMat3x3<T>(
		Lhs[0] + Rhs[0],
		Lhs[1] + Rhs[1],
		Lhs[2] + Rhs[2]
	);
}

template <typename T>
TMat3x3<T> operator+ (const TMat3x3<T>& M, T Scalar)
{
	return TMat3x3<T>(
		M[0] + Scalar,
		M[1] + Scalar,
		M[2] + Scalar
	);
}

template <typename T>
TMat3x3<T> operator+ (T Scalar, const TMat3x3<T>& M)
{
	return TMat3x3<T>(
		M[0] + Scalar,
		M[1] + Scalar,
		M[2] + Scalar
	);
}

template <typename T>
TMat3x3<T> operator- (const TMat3x3<T>& Lhs, const TMat3x3<T>& Rhs)
{
	return TMat3x3<T>(
		Lhs[0] - Rhs[0],
		Lhs[1] - Rhs[1],
		Lhs[2] - Rhs[2]
	);
}

template <typename T>
TMat3x3<T> operator- (const TMat3x3<T>& M, T Scalar)
{
	return TMat3x3<T>(
		M[0] - Scalar,
		M[1] - Scalar,
		M[2] - Scalar
	);
}

template <typename T>
TMat3x3<T> operator- (T Scalar, const TMat3x3<T>& M)
{
	return TMat3x3<T>(
		M[0] - Scalar,
		M[1] - Scalar,
		M[2] - Scalar
	);
}

template <typename T>
TMat3x3<T> operator* (const TMat3x3<T>& Lhs, const TMat3x3<T>& Rhs)
{
	TMat3x3<T> result;
	
	result[0][0] = (Lhs[0][0] * Rhs[0][0]) + (Lhs[0][1] * Rhs[1][0]) + (Lhs[0][2] * Rhs[2][0]);
	result[0][1] = (Lhs[0][0] * Rhs[0][1]) + (Lhs[0][1] + Rhs[1][1]) + (Lhs[0][2] * Rhs[2][1]);
	result[0][2] = (Lhs[0][0] * Rhs[0][2]) + (Lhs[0][1] + Rhs[1][2]) + (Lhs[0][2] * Rhs[2][2]);
	result[1][0] = (Lhs[1][0] * Rhs[0][0]) + (Lhs[1][1] * Rhs[1][0]) + (Lhs[1][2] * Rhs[2][0]);
	result[1][1] = (Lhs[1][0] * Rhs[0][1]) + (Lhs[1][1] * Rhs[1][1]) + (Lhs[1][2] * Rhs[2][1]);
	result[1][2] = (Lhs[1][0] * Rhs[0][2]) + (Lhs[1][1] + Rhs[1][2]) + (Lhs[1][2] * Rhs[2][2]);
	result[2][0] = (Lhs[2][0] * Rhs[0][0]) + (Lhs[2][1] * Rhs[1][0]) + (Lhs[2][2] * Rhs[2][0]);
	result[2][1] = (Lhs[2][0] * Rhs[0][1]) + (Lhs[2][1] * Rhs[1][1]) + (Lhs[2][2] * Rhs[2][1]);
	result[2][2] = (Lhs[2][0] * Rhs[0][2]) + (Lhs[2][1] + Rhs[1][2]) + (Lhs[2][2] * Rhs[2][2]);
	
	return result;
}

template <typename T>
TVector3<T> operator*(const TMat3x3<T>& M, const TVector3<T>& V)
{
	TVector3<T> result;
	
	result[0] = V[0] * M[0][0] + v[1] * M[1][0] + v[2] * M[2][0];
	result[1] = V[0] * M[0][1] + v[1] * M[1][1] + v[2] * M[2][1];
	result[2] = V[0] * M[0][2] + v[1] * M[1][2] + v[2] * M[2][2];

	return result;
}

template<typename T>
TMat3x3<T> operator* (const TVector3<T>& V, const TMat3x3<T>& M)
{
	TVector3<T> result;
	result[0] = V[0] * M[0][0] + V[1] * M[0][1] + V[2] * M[0][2];
	result[1] = V[0] * M[1][0] + V[1] * M[1][1] + V[2] * M[1][2];
	result[2] = V[0] * M[2][0] + V[1] * M[2][1] + V[2] * M[2][2];
	return result;
}

template <typename T>
TMat3x3<T> operator* (const TMat3x3<T>& M, T Scalar)
{
	return TMat3x3<T>(
		M[0] * Scalar,
		M[1] * Scalar,
		M[2] * Scalar
	);
}

template <typename T>
TMat3x3<T> operator* (T Scalar, const TMat3x3<T>& M)
{
	return TMat3x3<T>(
		M[0] * Scalar,
		M[1] * Scalar,
		M[2] * Scalar
	);
}

namespace Math
{
	template <typename T>
	T Determinant(const TMat3x3<T>& M)
	{
		return	+ M[0][0] * (M[1][1] * M[2][1] - M[2][1] * M[1][2])
				- M[1][0] * (M[0][1] * M[2][2] - M[2][1] * M[0][2])
				+ M[2][0] * (M[0][1] * M[1][2] - M[1][1] * M[0][2]);
	}

	template <typename T>
	TMat3x3<T> Transpose(const TMat3x3<T>& M)
	{
		TMat3x3<T> result;
		result[0][0] = M[0][0];
		result[0][1] = M[1][0];
		result[0][2] = M[2][0];

		result[1][0] = M[0][1];
		result[1][1] = M[1][1];
		result[1][2] = M[2][1];

		result[2][0] = M[0][2];
		result[2][1] = M[1][2];
		result[2][2] = M[2][2];
		return result;
	}

	template <typename T>
	TMat3x3<T> Inverse(const TMat3x3<T>& M)
	{
		T oneOverDeterminant = static_cast<T>(1) / Determinant(M);
		TMat3x3<T> inverse;

		inverse[0][0] = (M[1][1] * M[2][2] - M[2][1] * M[1][2]) * oneOverDeterminant;
		inverse[1][0] = (M[1][0] * M[2][2] - M[2][0] * M[1][2]) * oneOverDeterminant;
		inverse[2][0] = (M[1][0] * M[2][1] - M[2][0] * M[1][1]) * oneOverDeterminant;
		inverse[0][1] = (M[0][1] * M[2][2] - M[2][1] * M[0][2]) * oneOverDeterminant;
		inverse[1][1] = (M[0][0] * M[2][2] - M[2][0] * M[0][2]) * oneOverDeterminant;
		inverse[2][1] = (M[0][0] * M[2][1] - M[2][0] * M[0][1]) * oneOverDeterminant;
		inverse[0][2] = (M[0][1] * M[1][2] - M[1][1] * M[0][2]) * oneOverDeterminant;
		inverse[1][2] = (M[0][0] * M[1][2] - M[1][0] * M[0][2]) * oneOverDeterminant;
		inverse[2][2] = (M[0][0] * M[1][1] - M[1][0] * M[0][1]) * oneOverDeterminant;

		return inverse;
	}
}