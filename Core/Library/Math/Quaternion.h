#pragma once
#ifndef LEARNVK_LIBRARY_MATH_QUATERNION
#define LEARNVK_LIBRARY_MATH_QUATERNION

template <typename T>
class TQuaternion
{
public:

	TQuaternion();
	~TQuaternion();

	TQuaternion(const TQuaternion& Rhs);
	TQuaternion(TQuaternion&& Rhs);

	TQuaternion& operator=(const TQuaternion& Rhs);
	TQuaternion& operator=(TQuaternion&& Rhs);

private:
};

#include "Quaternion.inl"
#endif // !LEARNVK_LIBRARY_MATH_QUATERNION