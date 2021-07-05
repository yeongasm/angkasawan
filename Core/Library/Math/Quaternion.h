#pragma once
#ifndef LEARNVK_LIBRARY_MATH_QUATERNION
#define LEARNVK_LIBRARY_MATH_QUATERNION

#include "Platform/EngineAPI.h"

namespace astl
{
  namespace math
  {
    struct vec3;
    struct mat3;
    struct mat4;

    struct ENGINE_API quat
    {
      float w, x, y, z;

      quat();
      ~quat();

      quat(const vec3& v, float s);
      quat(float s, const vec3& v);
      quat(float w, float x, float y, float z);

      quat(const quat& q);
      quat(quat&& q);

      quat& operator=(const quat& q);
      quat& operator=(quat&& q);

      quat& operator+= (const quat& q);
      quat& operator-= (const quat& q);
      quat& operator*= (const quat& q);
      quat& operator*= (float s);
      quat& operator/= (float s);
    };

    ENGINE_API quat operator+ (const quat& l, const quat& r);
    ENGINE_API quat operator- (const quat& l, const quat& r);
    ENGINE_API quat operator* (const quat& l, const quat& r);
    ENGINE_API vec3 operator* (const quat& l, const vec3& r);
    ENGINE_API vec3 operator* (const vec3& l, const quat& r);
    ENGINE_API quat operator* (const quat& q, float s);
    ENGINE_API quat operator* (float s, const quat& q);
    ENGINE_API quat operator/ (const quat& q, float s);

    ENGINE_API bool operator== (const quat& l, const quat& r);
    ENGINE_API bool operator!= (const quat& l, const quat& r);

    ENGINE_API float Length(const quat& q);
    ENGINE_API void Normalize(quat& q);
    ENGINE_API void Conjugate(quat& q);
    ENGINE_API void Inverse(quat& q);
    ENGINE_API quat Normalized(const quat& q);
    ENGINE_API quat Conjugated(const quat& q);
    ENGINE_API quat Inversed(const quat& q);

    ENGINE_API mat3 Mat3Cast(const quat& q);
    ENGINE_API mat4 Mat4Cast(const quat& q);

    ENGINE_API quat AngleAxis(float angle, const vec3& axis);

    ENGINE_API float Dot(const quat& q);
    ENGINE_API float Dot(const quat& a, const quat& b);
  }
}

#endif // !LEARNVK_LIBRARY_MATH_QUATERNION
