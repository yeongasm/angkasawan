#pragma once
#ifndef LEARNVK_LIBRARY_MATH_OPERATIONS_H
#define LEARNVK_LIBRARY_MATH_OPERATIONS_H

#include "Platform/EngineAPI.h"

namespace math
{
	ENGINE_API float Cos(const float num);
	ENGINE_API float Sin(const float num);
	ENGINE_API float Tan(const float num);
	ENGINE_API float Acos(const float num);
	ENGINE_API float Asin(const float num);
	ENGINE_API float Atan(const float num);
	ENGINE_API float Sqrt(const float num);
	ENGINE_API float Sqr(const float num);
	ENGINE_API float Cbrt(const float num);
	ENGINE_API float Pow(const float num, float exp);
	ENGINE_API int Abs(const int num);
	ENGINE_API float FAbs(const float num);
	ENGINE_API int Mod(const int num, const int denom);
	ENGINE_API float FMod(const float num, const float denom);
	ENGINE_API float Pi();
	ENGINE_API float HalfPi();
	ENGINE_API float Tau();
	ENGINE_API float TwoPi();
	ENGINE_API float Radians(const float deg);
	ENGINE_API float Degrees(const float rad);
}

#endif // !LEARNVK_LIBRARY_MATH_OPERATIONS_H