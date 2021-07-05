#pragma once
#ifndef LEARNVK_LIBRARY_MATH_VECTOR_H
#define LEARNVK_LIBRARY_MATH_VECTOR_H

#include "Platform/EngineAPI.h"

namespace astl
{
  namespace math
  {
    struct vec3;
    struct vec4;

    struct ENGINE_API vec2
    {
      union
      {
        struct { float x, y; };
        struct { float r, g; };
        struct { float s, t; };
      };

      vec2();
      vec2(float s);
      vec2(float a, float b);

      vec2(const vec3& v);
      vec2(const vec4& v);

      ~vec2();

      vec2(const vec2& v);
      vec2(vec2&& v);

      float& operator[]	(size_t i);
      const float& operator[]	(size_t i) const;

      vec2& operator= (const vec2& v);
      vec2& operator= (vec2&& v);

      vec2 operator-();
      const vec2 operator-() const;

      vec2& operator+= (const vec2& v);
      vec2& operator+= (float s);
      vec2& operator-= (const vec2& v);
      vec2& operator-= (float s);
      vec2& operator*= (const vec2& v);
      vec2& operator*= (float s);
      vec2& operator/= (const vec2& v);
      vec2& operator/= (float s);
    };

    ENGINE_API vec2 operator+ (const vec2& l, const vec2& r);
    ENGINE_API vec2 operator+ (const vec2& v, float s);
    ENGINE_API vec2 operator+ (float s, const vec2& v);
    ENGINE_API vec2 operator- (const vec2& l, const vec2& r);
    ENGINE_API vec2 operator- (const vec2& v, float s);
    ENGINE_API vec2 operator- (float s, const vec2& v);
    ENGINE_API vec2 operator* (const vec2& l, const vec2& r);
    ENGINE_API vec2 operator* (const vec2& v, float s);
    ENGINE_API vec2 operator* (float s, const vec2& v);
    ENGINE_API vec2 operator/ (const vec2& l, const vec2& r);
    ENGINE_API vec2 operator/ (const vec2& v, float s);
    ENGINE_API vec2 operator/ (float s, const vec2& v);

    ENGINE_API bool operator== (const vec2& l, const vec2& r);
    ENGINE_API bool operator!= (const vec2& l, const vec2& r);

    ENGINE_API float Length(const vec2& v);
    ENGINE_API void	Normalize(vec2& v);
    ENGINE_API float Dot(const vec2& v);
    ENGINE_API float Dot(const vec2& l, const vec2& r);
    ENGINE_API vec2	Normalized(const vec2& v);
    ENGINE_API bool	InBounds(const vec2& p, const vec2& min, const vec2& max);

    struct ENGINE_API vec3
    {
      union
      {
        struct { float x, y, z; };
        struct { float r, g, b; };
        struct { float s, t, p; };
      };

      vec3();
      vec3(float s);
      vec3(float a, float b, float c);

      vec3(const vec2& v, float s);
      vec3(float s, const vec2& v);

      vec3(const vec2& v);
      vec3(const vec4& v);

      vec3(const vec3& v);
      vec3(vec3&& v);

      ~vec3();

      vec3& operator= (const vec3& v);
      vec3& operator= (vec3&& v);

      float& operator[] (size_t i);
      const float& operator[] (size_t i) const;

      vec3 operator-();
      const vec3 operator-() const;

      vec3& operator+= (const vec3& v);
      vec3& operator+= (float s);
      vec3& operator-= (const vec3& v);
      vec3& operator-= (float s);
      vec3& operator*= (const vec3& v);
      vec3& operator*= (float s);
      vec3& operator/= (const vec3& v);
      vec3& operator/= (float s);
    };

    ENGINE_API vec3 operator+ (const vec3& l, const vec3& r);
    ENGINE_API vec3 operator+ (const vec3& l, const vec2& r);
    ENGINE_API vec3 operator+ (const vec2& l, const vec3& r);
    ENGINE_API vec3 operator+ (const vec3& v, float s);
    ENGINE_API vec3 operator+ (float s, const vec3& v);
    ENGINE_API vec3 operator- (const vec3& l, const vec3& r);
    ENGINE_API vec3 operator- (const vec3& l, const vec2& r);
    ENGINE_API vec3 operator- (const vec2& l, const vec3& r);
    ENGINE_API vec3 operator- (const vec3& v, float s);
    ENGINE_API vec3 operator- (float s, const vec3& v);
    ENGINE_API vec3 operator* (const vec3& l, const vec3& r);
    ENGINE_API vec3 operator* (const vec3& l, const vec2& r);
    ENGINE_API vec3 operator* (const vec2& l, const vec3& r);
    ENGINE_API vec3 operator* (const vec3& v, float s);
    ENGINE_API vec3 operator* (float s, const vec3& v);
    ENGINE_API vec3 operator/ (const vec3& l, const vec3& r);
    ENGINE_API vec3 operator/ (const vec3& l, const vec2& r);
    ENGINE_API vec3 operator/ (const vec2& l, const vec3& r);
    ENGINE_API vec3 operator/ (const vec3& v, float s);
    ENGINE_API vec3 operator/ (float s, const vec3& v);

    ENGINE_API bool operator== (const vec3& l, const vec3& r);
    ENGINE_API bool operator!= (const vec3& l, const vec3& r);

    ENGINE_API float Length(const vec3& v);
    ENGINE_API void	Normalize(vec3& v);
    ENGINE_API float Dot(const vec3& v);
    ENGINE_API float Dot(const vec3& l, const vec3& r);
    ENGINE_API vec3	Cross(const vec3& l, const vec3& r);
    ENGINE_API vec3	Normalized(const vec3& v);

    struct ENGINE_API vec4
    {
      union
      {
        struct { float x, y, z, w; };
        struct { float r, g, b, a; };
        struct { float s, t, p, q; };
      };

      vec4();
      vec4(float s);
      vec4(float a, float b, float c, float d);

      vec4(const vec2& v);
      vec4(const vec2& v, float a, float b);
      vec4(float a, float b, const vec2& v);
      vec4(const vec2& a, const vec2& b);
      vec4(const vec3& v, float s);
      vec4(float s, const vec3& v);

      vec4(const vec3& v);

      ~vec4();

      vec4(const vec4& v);
      vec4(vec4&& v);

      vec4& operator= (const vec4& v);
      vec4& operator= (vec4&& v);

      float& operator[] (size_t i);
      const float& operator[] (size_t i) const;

      vec4 operator-();
      const vec4 operator-() const;

      vec4& operator+= (const vec4& v);
      vec4& operator+= (float s);
      vec4& operator-= (const vec4& v);
      vec4& operator-= (float s);
      vec4& operator*= (const vec4& v);
      vec4& operator*= (float s);
      vec4& operator/= (const vec4& v);
      vec4& operator/= (float s);
    };

    ENGINE_API vec4 operator+ (const vec4& l, const vec4& r);
    ENGINE_API vec4 operator+ (const vec4& l, const vec3& r);
    ENGINE_API vec4 operator+ (const vec3& l, const vec4& r);
    ENGINE_API vec4 operator+ (const vec4& l, const vec2& r);
    ENGINE_API vec4 operator+ (const vec2& l, const vec4& r);
    ENGINE_API vec4 operator+ (const vec4& v, float s);
    ENGINE_API vec4 operator+ (float s, const vec4& v);
    ENGINE_API vec4 operator- (const vec4& l, const vec4& r);
    ENGINE_API vec4 operator- (const vec4& l, const vec3& r);
    ENGINE_API vec4 operator- (const vec3& l, const vec4& r);
    ENGINE_API vec4 operator- (const vec4& l, const vec2& r);
    ENGINE_API vec4 operator- (const vec2& l, const vec4& r);
    ENGINE_API vec4 operator- (const vec4& v, float s);
    ENGINE_API vec4 operator- (float s, const vec4& v);
    ENGINE_API vec4 operator* (const vec4& l, const vec4& r);
    ENGINE_API vec4 operator* (const vec4& l, const vec3& r);
    ENGINE_API vec4 operator* (const vec3& l, const vec4& r);
    ENGINE_API vec4 operator* (const vec4& l, const vec2& r);
    ENGINE_API vec4 operator* (const vec2& l, const vec4& r);
    ENGINE_API vec4 operator* (const vec4& v, float s);
    ENGINE_API vec4 operator* (float s, const vec4& v);
    ENGINE_API vec4 operator/ (const vec4& l, const vec4& r);
    ENGINE_API vec4 operator/ (const vec4& l, const vec3& r);
    ENGINE_API vec4 operator/ (const vec3& l, const vec4& r);
    ENGINE_API vec4 operator/ (const vec4& l, const vec2& r);
    ENGINE_API vec4 operator/ (const vec2& l, const vec4& r);
    ENGINE_API vec4 operator/ (const vec4& v, float s);
    ENGINE_API vec4 operator/ (float s, const vec4& v);

    ENGINE_API bool operator== (const vec4& l, const vec4& r);
    ENGINE_API bool operator!= (const vec4& l, const vec4& r);

    ENGINE_API float Length(const vec4& v);
    ENGINE_API void Normalize(vec4& v);
    ENGINE_API float Dot(const vec4& v);
    ENGINE_API float Dot(const vec4& l, const vec4& r);
    ENGINE_API vec4 Cross(const vec4& l, const vec4& r);
    ENGINE_API vec4 Normalized(const vec4& v);

    ENGINE_API vec2 Reflect(const vec2& i, const vec2& n);
    ENGINE_API vec3 Reflect(const vec3& i, const vec3& n);
    ENGINE_API vec4 Reflect(const vec4& i, const vec4& n);
  }
}

#endif // !LEARNVK_LIBRARY_MATH_VECTOR_H
