#include "Math.h"

float32 FMath::Pi()
{
	return FMath::Atan(1.f) * 4.f;
}

FMath::Radian FMath::Radians(const FMath::Degree Val)
{
	return Val * (FMath::Pi() / 180.f);
}

FMath::Degree FMath::Degrees(const FMath::Radian Val)
{
	return Val * (180.f / FMath::Pi());
}