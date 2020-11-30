#include "Mat4.h"


template <typename T>
TMat4x4<T>::TMat4x4() :
	mat{}
{}

template <typename T>
TMat4x4<T>::~TMat4x4()
{
	FMemory::Memzero(&mat, sizeof(mat));
}

template <typename T>
TMat4x4<T>::TMat4x4(const TMat4x4<T>& Rhs)
{
	*this = Rhs;
}

template <typename T>
TMat4x4<T>::TMat4x4(TMat4x4<T>&& Rhs)
{
	*this = Move(Rhs);
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator=(const TMat4x4<T>& Rhs)
{
	if (this != &Rhs)
	{
		mat[0] = Rhs.mat[0];
		mat[1] = Rhs.mat[1];
		mat[2] = Rhs.mat[2];
		mat[3] = Rhs.mat[3];
	}
	return *this;
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator=(TMat4x4<T>&& Rhs)
{
	if (this != &Rhs)
	{
		mat[0] = Move(Rhs.mat[0]);
		mat[1] = Move(Rhs.mat[1]);
		mat[2] = Move(Rhs.mat[2]);
		mat[3] = Move(Rhs.mat[3]);
		new (&Rhs) TMat4x4<T>();
	}
	return *this;
}

template <typename T>
TMat4x4<T>::TMat4x4(T Scalar) :
	TMat4x4<T>()
{
	mat[0].x = Scalar;
	mat[1].y = Scalar;
	mat[2].z = Scalar;
	mat[3].w = Scalar;
}

template <typename T>
TMat4x4<T>::TMat4x4(T x0, T y0, T z0, T w0,
					T x1, T y1, T z1, T w1,
					T x2, T y2, T z2, T w2,
					T x3, T y3, T z3, T w3) : TMat4x4<T>()
{
	mat[0] = VecType(x0, y0, z0, w0);
	mat[1] = VecType(x1, y1, z1, w1);
	mat[2] = VecType(x2, y2, z2, w2);
	mat[3] = VecType(x3, y3, z3, w3);
}

template<typename T>
inline TMat4x4<T>::TMat4x4(	const TVector4<T>& v0, 
							const TVector4<T>& v1, 
							const TVector4<T>& v2, 
							const TVector4<T>& v3) : TMat4x4<T>()
{
	mat[0] = v0;
	mat[1] = v1;
	mat[2] = v2;
	mat[3] = v3;
}


template <typename T>
TVector4<T>& TMat4x4<T>::operator[](size_t Index)
{
	VKT_ASSERT(Index >= Size());
	return mat[Index];
}

template <typename T>
const TVector4<T>& TMat4x4<T>::operator[](size_t Index) const
{
	VKT_ASSERT(Index >= Size());
	return mat[Index];
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator+=(const TMat4x4<T>& Rhs)
{
	mat[0] += Rhs.mat[0];
	mat[1] += Rhs.mat[1];
	mat[2] += Rhs.mat[2];
	mat[3] += Rhs.mat[3];
	return *this;
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator+=(T Scalar)
{
	mat[0] += Scalar;
	mat[1] += Scalar;
	mat[2] += Scalar;
	mat[3] += Scalar;
	return *this;
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator-=(const TMat4x4<T>& Rhs)
{
	mat[0] -= Rhs.mat[0];
	mat[1] -= Rhs.mat[1];
	mat[2] -= Rhs.mat[2];
	mat[3] -= Rhs.mat[3];
	return *this;
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator-=(T Scalar)
{
	mat[0] -= Scalar;
	mat[1] -= Scalar;
	mat[2] -= Scalar;
	mat[3] -= Scalar;
	return *this;
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator*=(const TMat4x4<T>& Rhs)
{
	return (*this = *this * Rhs);
}

template <typename T>
TMat4x4<T>& TMat4x4<T>::operator*=(T Scalar)
{
	mat[0] *= Scalar;
	mat[1] *= Scalar;
	mat[2] *= Scalar;
	mat[3] *= Scalar;
	return *this;
}

template <typename T>
TMat4x4<T> operator+(const TMat4x4<T>& Lhs, const TMat4x4<T>& Rhs)
{
	return TMat4x4<T>(
		Lhs[0] + Rhs[0],
		Lhs[1] + Rhs[1],
		Lhs[2] + Rhs[2],
		Lhs[3] + Rhs[3]
	);
}

template <typename T>
TMat4x4<T> operator+(const TMat4x4<T>& M, T Scalar)
{
	return TMat4x4<T>(
		M[0] + Scalar,
		M[1] + Scalar,
		M[2] + Scalar,
		M[3] + Scalar
	);
}

template <typename T>
TMat4x4<T> operator+(T Scalar, const TMat4x4<T>& M)
{
	return TMat4x4<T>(
		M[0] + Scalar,
		M[1] + Scalar,
		M[2] + Scalar,
		M[3] + Scalar
	);
}

template <typename T>
TMat4x4<T> operator-(const TMat4x4<T>& Lhs, const TMat4x4<T>& Rhs)
{
	return TMat4x4<T>(
		Lhs[0] - Rhs[0],
		Lhs[1] - Rhs[1],
		Lhs[2] - Rhs[2],
		Lhs[3] - Rhs[3]
	);
}

template <typename T>
TMat4x4<T> operator-(const TMat4x4<T>& M, T Scalar)
{
	return TMat4x4<T>(
		M[0] - Scalar,
		M[1] - Scalar,
		M[2] - Scalar,
		M[3] - Scalar
	);
}

template <typename T>
TMat4x4<T> operator-(T Scalar, const TMat4x4<T>& M)
{
	return TMat4x4<T>(
		M[0] - Scalar,
		M[1] - Scalar,
		M[2] - Scalar,
		M[3] - Scalar
	);
}

template <typename T>
TMat4x4<T> operator*(const TMat4x4<T>& Lhs, const TMat4x4<T>& Rhs)
{
	using Vector = typename TMat4x4<T>::VecType;

	const Vector a0 = Lhs[0];
	const Vector a1 = Lhs[1];
	const Vector a2 = Lhs[2];
	const Vector a3 = Lhs[3];

	const Vector b0 = Rhs[0];
	const Vector b1 = Rhs[1];
	const Vector b2 = Rhs[2];
	const Vector b3 = Rhs[3];

	TMat4x4<T> result;
	result[0] = a0 * b0[0] + a1 * b0[1] + a2 * b0[2] + a3 * b0[3];
	result[1] = a0 * b1[0] + a1 * b1[1] + a2 * b1[2] + a3 * b1[3];
	result[2] = a0 * b2[0] + a1 * b2[1] + a2 * b2[2] + a3 * b2[3];
	result[3] = a0 * b3[0] + a1 * b3[1] + a2 * b3[2] + a3 * b3[3];
	return result;
}

template <typename T>
TMat4x4<T> operator*(const TMat4x4<T>& M, T Scalar)
{
	return TMat4x4<T>(
		M[0] * Scalar,
		M[1] * Scalar,
		M[2] * Scalar,
		M[3] * Scalar
	);
}

template <typename T>
TMat4x4<T> operator*(T Scalar, const TMat4x4<T>& M)
{
	return TMat4x4<T>(
		M[0] * Scalar,
		M[1] * Scalar,
		M[2] * Scalar,
		M[3] * Scalar
	);
}

template <typename T>
TVector4<T> operator*(const TMat4x4<T>& M, const TVector4<T>& V)
{
	TVector4<T> result;
	result[0] = V[0] * M[0][0] + V[1] * M[1][0] + V[2] * M[2][0] + V[3] * M[3][0];
	result[1] = V[0] * M[0][1] + V[1] * M[1][1] + V[2] * M[2][1] + V[3] * M[3][1];
	result[2] = V[0] * M[0][2] + V[1] * M[1][2] + V[2] * M[2][2] + V[3] * M[3][2];
	result[3] = V[0] * M[0][3] + V[1] * M[1][3] + V[2] * M[2][3] + V[3] * M[3][3];
	return result;
}

template<typename T>
TVector4<T> operator*(const TVector4<T>& V, const TMat4x4<T>& M)
{
	TVector4<T> result;
	result[0] = V[0] * M[0][0] + V[1] * M[0][1] + V[2] * M[0][2] + V[3] * M[0][3];
	result[1] = V[0] * M[1][0] + V[1] * M[1][1] + V[2] * M[1][2] + V[3] * M[1][3];
	result[2] = V[0] * M[2][0] + V[1] * M[2][1] + V[2] * M[2][2] + V[3] * M[2][3];
	result[3] = V[0] * M[3][0] + V[1] * M[3][1] + V[2] * M[3][2] + V[3] * M[3][3];
	return result;
}

namespace Math
{
	template <typename T>
	T Determinant(const TMat4x4<T>& M)
	{
		T subFactor00 = M[2][2] * M[3][3] - M[3][2] * M[2][3];
		T subFactor01 = M[2][1] * M[3][3] - M[3][1] * M[2][3];
		T subFactor02 = M[2][1] * M[3][2] - M[3][1] * M[2][2];
		T subFactor03 = M[2][0] * M[3][3] - M[3][0] * M[2][3];
		T subFactor04 = M[2][0] * M[3][2] - M[3][0] * M[2][2];
		T subFactor05 = M[2][0] * M[3][1] - M[3][0] * M[2][1];

		TVector4<T> detCof(
			 (M[1][1] * subFactor00 - M[1][2] * subFactor01 + M[1][3] * subFactor02),
			-(M[1][0] * subFactor00 - M[1][2] * subFactor01 + M[1][3] * subFactor04),
			 (M[1][0] * subFactor00 - M[1][1] * subFactor01 + M[1][3] * subFactor05),
			-(M[1][0] * subFactor00 - M[1][1] * subFactor01 + M[1][2] * subFactor05)
		);

		return M[0][0] * detCof[0] + M[0][1] * detCof[1] + M[0][2] * detCof[2] + M[0][3] * detCof[3];
	}

	template <typename T>
	TMat4x4<T> Transpose(const TMat4x4<T>& M)
	{
		TMat4x4<T> result;

		result[0][0] = M[0][0];
		result[0][1] = M[1][0];
		result[0][2] = M[2][0];
		result[0][3] = M[3][0];

		result[1][0] = M[0][1];
		result[1][1] = M[1][1];
		result[1][2] = M[2][1];
		result[1][3] = M[3][1];

		result[2][0] = M[0][2];
		result[2][1] = M[1][2];
		result[2][2] = M[2][2];
		result[2][3] = M[3][2];

		result[3][0] = M[0][3];
		result[3][1] = M[1][3];
		result[3][2] = M[2][3];
		result[3][3] = M[3][3];

		return result;
	}

	template <typename T>
	TMat4x4<T> Inverse(const TMat4x4<T>& M)
	{
		T coef00 = M[2][2] * M[3][3] - M[3][2] * M[2][3];
		T coef02 = M[1][2] * M[3][3] - M[3][2] * M[1][3];
		T coef03 = M[1][2] * M[2][3] - M[2][2] * M[1][3];

		T coef04 = M[2][1] * M[3][3] - M[3][1] * M[2][3];
		T coef06 = M[1][1] * M[3][3] - M[3][1] * M[1][3];
		T coef07 = M[1][1] * M[2][3] - M[2][1] * M[1][3];

		T coef08 = M[2][1] * M[3][2] - M[3][1] * M[2][2];
		T coef10 = M[1][1] * M[3][2] - M[3][1] * M[1][2];
		T coef11 = M[1][1] * M[2][2] - M[2][1] * M[1][2];

		T coef12 = M[2][0] * M[3][3] - M[3][0] * M[2][3];
		T coef14 = M[1][0] * M[3][3] - M[3][0] * M[1][3];
		T coef15 = M[1][0] * M[2][3] - M[2][0] * M[1][3];

		T coef16 = M[2][0] * M[3][2] - M[3][0] * M[2][2];
		T coef18 = M[1][0] * M[3][2] - M[3][0] * M[1][2];
		T coef19 = M[1][0] * M[2][2] - M[2][0] * M[1][2];

		T coef20 = M[2][0] * M[3][1] - M[3][0] * M[2][1];
		T coef22 = M[1][0] * M[3][1] - M[3][0] * M[1][1];
		T coef23 = M[1][0] * M[2][1] - M[2][0] * M[1][1];

		TVector4<T> fac0(coef00, coef00, coef02, coef03);
		TVector4<T> fac1(coef04, coef04, coef06, coef07);
		TVector4<T> fac2(coef08, coef08, coef10, coef11);
		TVector4<T> fac3(coef12, coef12, coef14, coef15);
		TVector4<T> fac4(coef16, coef16, coef18, coef19);
		TVector4<T> fac5(coef20, coef20, coef22, coef23);

		TVector4<T> v0(M[1][0], M[0][0], M[0][0], M[0][0]);
		TVector4<T> v1(M[1][1], M[0][1], M[0][1], M[0][1]);
		TVector4<T> v2(M[1][2], M[0][2], M[0][2], M[0][2]);
		TVector4<T> v3(M[1][3], M[0][3], M[0][3], M[0][3]);

		TVector4<T> inv0(v1 * fac0 - v2 * fac1 + v3 * fac2);
		TVector4<T> inv1(v0 * fac0 - v2 * fac3 + v3 * fac4);
		TVector4<T> inv2(v0 * fac1 - v1 * fac3 + v3 * fac5);
		TVector4<T> inv3(v0 * fac2 - v1 * fac4 + v2 * fac5);

		TVector4<T> signA(+1, -1, +1, -1);
		TVector4<T> signB(-1, +1, -1, +1);
		TMat4x4<T> inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

		TVector4<T> row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);
		TVector4<T> dot0(M[0] * row0);

		T dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);
		T oneOverDeterminant = static_cast<T>(1) / dot1;

		return inverse * oneOverDeterminant;
	}

	template <typename T>
	TMat4x4<T> Translate(const TMat4x4<T>& M, const TVector3<T>& V)
	{
		TMat4x4<T> result(M);
		result[3] = M[0] * V[0] + M[1] * V[1] + M[2] * V[2] * M[3];
		return result;
	}

	template <typename T>
	TMat4x4<T> Scale(const TMat4x4<T>& M, const TVector3<T>& V)
	{
		TMat4x4<T> result;
		result[0] = M[0] * V[0];
		result[1] = M[1] * V[1];
		result[2] = M[2] * V[2];
		result[3] = M[3];
		return result;
	}

	template <typename T>
	TMat4x4<T> Rotate(const TMat4x4<T>& M, T Angle, const TVector3<T>& V)
	{
		const T a = Angle;
		const T c = FMath::Cos(a);
		const T s = FMath::Sin(a);

		TVector3<T> axis(Normalize(V));
		TVector3<T> temp((static_cast<T>(1) - c) * axis);

		TMat4x4<T> rotate;
		rotate[0][0] = c + temp[0] * axis[0];
		rotate[0][1] = temp[0] * axis[1] + s * axis[2];
		rotate[0][2] = temp[0] * axis[2] - s * axis[1];

		rotate[1][0] = temp[1] * axis[0] - s * axis[2];
		rotate[1][1] = c + temp[1] * axis[1];
		rotate[1][2] = temp[1] * axis[2] + s * axis[0];

		rotate[2][0] = temp[2] * axis[0] + s * axis[1];
		rotate[2][1] = temp[2] * axis[1] - s * axis[0];
		rotate[2][2] = c + temp[2] * axis[2];

		TMat4x4<T> result;
		result[0] = M[0] * rotate[0][0] + M[1] * rotate[0][1] + M[2] * rotate[0][2];
		result[1] = M[0] * rotate[1][0] + M[1] * rotate[1][1] + M[2] * rotate[1][2];
		result[2] = M[0] * rotate[2][0] + M[1] * rotate[2][1] + M[2] * rotate[2][2];
		result[3] = M[3];
		return result;
	}

	template <typename T>
	TMat4x4<T> Translate(const TMat4x4<T>& M, const TVector4<T>& V)
	{
		return Translate(M, TVector3<T>(V.x, V.y, V.z));
	}

	template <typename T>
	TMat4x4<T> Scale(const TMat4x4<T>& M, const TVector4<T>& V)
	{
		return Scale(M, TVector3<T>(V.x, V.y, V.z));
	}

	template <typename T>
	TMat4x4<T> Rotate(const TMat4x4<T>& M, T Angle, const TVector4<T>& V)
	{
		return Rotate(M, Angle, TVector3<T>(V.x, V.y, V.z));
	}
}