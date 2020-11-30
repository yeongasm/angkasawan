#pragma once
#ifndef LEARNVK_LIBRARY_MATH
#define LEARNVK_LIBRARY_MATH

#include "Platform/Minimal.h"

namespace Math
{
	template <typename A, typename B>
	struct TIsFloat;

	template <typename, typename>
	struct TIsFloat
	{
		enum { Value = false };
	};

	template <typename Type>
	struct TIsFloat<Type, float32>
	{
		enum { Value = true };
	};

	template <typename Type>
	struct TIsFloat<Type, float64>
	{
		enum { Value = true };
	};
}

struct FMath
{
	using Degree = float32;
	using Radian = float32;

	using Degree64 = float64;
	using Radian64 = float64;

	// Trigonometric functions.
	template <typename T> static T Cos	(const T Value);
	template <typename T> static T Sin	(const T Value);
	template <typename T> static T Tan	(const T Value);
	template <typename T> static T Acos	(const T Value);
	template <typename T> static T Asin	(const T Value);
	template <typename T> static T Atan	(const T Value);

	// Power functions.
	template <typename T> static T Sqrt	(const T Value);
	template <typename T> static T Sqr	(const T Value);
	template <typename T> static T Cbrt	(const T Value);
	template <typename T> static T Pow	(const T Value, T Exponent);

	// Other functions.
	template <typename T> static T Abs	(const T Value);

	static	float32 Pi		();
	static	Radian	Radians	(const Degree Val);
	static	Degree	Degrees	(const Radian Val);
};

#include "Math.inl"
#endif // !LEARNVK_LIBRARY_MATH