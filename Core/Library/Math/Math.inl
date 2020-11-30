#pragma once
#include <cmath>
#include "Math.h"

template <typename T> T FMath::Cos(const T Value)
{
	return cos(Value);
}

template <typename T>
T FMath::Sin(const T Value)
{
	return sin(Value);
}

template <typename T>
T FMath::Tan(const T Value)
{
	return tan(Value);
}

template <typename T>
T FMath::Acos(const T Value)
{
	return acos(Value);
}

template <typename T>
T FMath::Asin(const T Value)
{
	return asin(Value);
}

template <typename T>
T FMath::Atan(const T Value)
{
	return atan(Value);
}

template <typename T>
T FMath::Sqrt(const T Value)
{
	return sqrt(Value);
}

template <typename T>
T FMath::Sqr(const T Value)
{
	return Value * Value;
}

template <typename T>
T FMath::Cbrt(const T Value)
{
	return cbrt(Value);
}

template <typename T>
T FMath::Pow(const T Value, T Exponent)
{
	return pow(Value, Exponent);
}

template <typename T>
T FMath::Abs(const T Value)
{
	return abs(Value);
}