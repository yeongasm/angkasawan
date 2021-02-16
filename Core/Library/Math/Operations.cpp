#include <math.h>
#include "Operations.h"

namespace math
{
	float Cos(const float num)
	{
		return cosf(num);
	}

	float Sin(const float num)
	{
		return sinf(num);
	}

	float Tan(const float num)
	{
		return tanf(num);
	}

	float Acos(const float num)
	{
		return acosf(num);
	}

	float Asin(const float num)
	{
		return asinf(num);
	}

	float Atan(const float num)
	{
		return atanf(num);
	}

	float Sqrt(const float num)
	{
		return sqrtf(num);
	}

	float Sqr(const float num)
	{
		return num * num;
	}

	float Cbrt(const float num)
	{
		return cbrtf(num);
	}

	float Pow(const float num, float exp)
	{
		return powf(num, exp);
	}

	int Abs(const int num)
	{
		return abs(num);
	}

	float FAbs(const float num)
	{
		return fabsf(num);
	}

	int Mod(const int num, const int denom)
	{
		return num % denom;
	}

	float FMod(const float num, const float denom)
	{
		return fmodf(num, denom);
	}

	float Pi()
	{
		return atanf(1.f) * 4.f;
	}

	float HalfPi()
	{
		return Pi() * 0.5f;
	}

	float Tau()
	{
		return Pi() * 2.0f;
	}

	float TwoPi()
	{
		return Tau();
	}

	float Radians(const float deg)
	{
		return deg * (Pi() / 180.f);
	}

	float Degrees(const float rad)
	{
		return rad * (180.f / Pi());
	}

}