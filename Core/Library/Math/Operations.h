#pragma once
#ifndef LEARNVK_LIBRARY_MATH_OPERATIONS_H
#define LEARNVK_LIBRARY_MATH_OPERATIONS_H

#include <cmath>
#include "Library/Templates/Templates.h"
#include "Library/Templates/Types.h"

namespace astl
{
  namespace math
  {
    inline float32 Cos(const float32 num)
    {
      return cosf(num);
    }

    inline float32 Sin(const float32 num)
    {
      return sinf(num);
    }

    inline float32 Tan(const float32 num)
    {
      return tanf(num);
    }

    inline float32 Acos(const float32 num)
    {
      return acosf(num);
    }

    inline float32 Asin(const float32 num)
    {
      return asinf(num);
    }

    inline float32 Atan(const float32 num)
    {
      return atanf(num);
    }

    inline float32 Sqrt(const float32 num)
    {
      return sqrtf(num);
    }

    inline float32 Sqr(const float32 num)
    {
      return num * num;
    }

    inline float32 Cbrt(const float32 num)
    {
      return cbrtf(num);
    }

    inline float32 Pow(const float32 num, float32 exp)
    {
      return powf(num, exp);
    }

    inline int32 Abs(const int32 num)
    {
      return abs(num);
    }

    inline float32 FAbs(const float32 num)
    {
      return fabsf(num);
    }

    inline int32 Mod(const int32 num, const int32 denom)
    {
      return num % denom;
    }

    inline float32 FLog2(const float32 num)
    {
      return log2f(num);;
    }

    inline float32 FMod(const float32 num, const float32 denom)
    {
      return fmodf(num, denom);
    }

    inline float32 FFloor(const float32 num)
    {
      return floorf(num);
    }

    inline float32 FCeil(const float32 num)
    {
      return ceilf(num);
    }

    inline float32 FRound(const float32 num)
    {
      return roundf(num);
    }

    inline float32 Pi()
    {
      return atanf(1.f) * 4.f;
    }

    inline float32 HalfPi()
    {
      return Pi() * 0.5f;
    }

    inline float32 Tau()
    {
      return Pi() * 2.0f;
    }

    inline float32 TwoPi()
    {
      return Tau();
    }

    inline float32 Radians(const float32 deg)
    {
      return deg * (Pi() / 180.f);
    }

    inline float32 Degrees(const float32 rad)
    {
      return rad * (180.f / Pi());
    }
  }
}

#endif // !LEARNVK_LIBRARY_MATH_OPERATIONS_H
