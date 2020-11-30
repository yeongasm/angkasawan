#pragma once
#ifndef LEARNVK_LIBRARY_MATH_MATRIX_FUNCS
#define LEARNVK_LIBRARY_MATH_MATRIX_FUNCS

#include "Mat4.h"

namespace Math
{
#define COORDINATE_SYSTEM_RH 1;

	template <typename T> void Perspective		(const FMath::Radian Fov, T Width, T Height, T Near, T Far, TMat4x4<T>& M);
	template <typename T> void PerspectiveRH	(const FMath::Radian Fov, T Width, T Height, T Near, T Far, TMat4x4<T>& M);
	template <typename T> void PerspectiveLH	(const FMath::Radian Fov, T Width, T Height, T Near, T Far, TMat4x4<T>& M);

	template <typename T> void Orthographic		(T Left, T Right, T Bottom, T Top, T Near, T Far, TMat4x4<T>& M);
	template <typename T> void OrthographicRH	(T Left, T Right, T Bottom, T Top, T Near, T Far, TMat4x4<T>& M);
	template <typename T> void OrthographicLH	(T Left, T Right, T Bottom, T Top, T Near, T Far, TMat4x4<T>& M);

	template <typename T> void LookAt			(const TVector3<T>& From, const TVector3<T>& To, const TVector3<T>& Up, TMat4x4<T>& M);
	template <typename T> void LookAtRH			(const TVector3<T>& From, const TVector3<T>& To, const TVector3<T>& Up, TMat4x4<T>& M);
	template <typename T> void LookAtLH			(const TVector3<T>& From, const TVector3<T>& To, const TVector3<T>& Up, TMat4x4<T>& M);
}

#include "MatrixFuncs.inl"
#endif // !LEARNVK_LIBRARY_MATH_MATRIX_FUNCS