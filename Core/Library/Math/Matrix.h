#pragma once
#ifndef LEARNVK_LIBRARY_MATH_MATRIX_H
#define LEARNVK_LIBRARY_MATH_MATRIX_H

#include "Platform/EngineAPI.h"

namespace astl
{

  namespace math
  {
    struct vec3;
    struct vec4;

    struct ENGINE_API mat3
    {
      float m00, m01, m02;
      float m10, m11, m12;
      float m20, m21, m22;

      mat3();
      ~mat3();

      mat3(float v);

      mat3(float x0, float y0, float z0,
        float x1, float y1, float z1,
        float x2, float y2, float z2);

      mat3(const vec3& a, const vec3& b, const vec3& c);

      mat3(const mat3& m);
      mat3(mat3&& m);

      float operator[] (size_t i);
      const float operator[] (size_t i) const;

      mat3& operator=(const mat3& m);
      mat3& operator=(mat3&& m);

      mat3& operator+= (const mat3& m);
      mat3& operator+= (float s);
      mat3& operator-= (const mat3& m);
      mat3& operator-= (float s);
      mat3& operator*= (const mat3& m);
      mat3& operator*= (float s);

      static const mat3 Identity;
      static vec3 GetXVector(const mat3& m);
      static vec3 GetYVector(const mat3& m);
      static vec3 GetZVector(const mat3& m);
    };

    ENGINE_API mat3 operator+ (const mat3& l, const mat3& r);
    ENGINE_API mat3 operator+ (const mat3& m, float s);
    ENGINE_API mat3 operator+ (float s, const mat3& m);
    ENGINE_API mat3 operator- (const mat3& l, const mat3& r);
    ENGINE_API mat3 operator- (const mat3& m, float s);
    ENGINE_API mat3 operator- (float s, const mat3& m);
    ENGINE_API mat3 operator* (const mat3& l, const mat3& r);
    ENGINE_API mat3 operator* (const mat3& m, float s);
    ENGINE_API mat3 operator* (float s, const mat3& m);

    ENGINE_API vec3 operator* (const mat3& m, const vec3& v);
    ENGINE_API vec3 operator* (const vec3& v, const mat3& m);

    ENGINE_API float Determinant(const mat3& m);
    ENGINE_API mat3 Transpose(const mat3& m);
    ENGINE_API mat3 Inverse(const mat3& m);
    ENGINE_API void Transposed(mat3& m);
    ENGINE_API void Inversed(mat3& m);

    struct ENGINE_API mat4
    {
      float m00, m01, m02, m03;
      float m10, m11, m12, m13;
      float m20, m21, m22, m23;
      float m30, m31, m32, m33;

      mat4();
      ~mat4();

      mat4(float v);

      mat4(float x0, float y0, float z0, float w0,
        float x1, float y1, float z1, float w1,
        float x2, float y2, float z2, float w2,
        float x3, float y3, float z3, float w3);

      mat4(const vec4& a, const vec4& b, const vec4& c, const vec4& d);
      mat4(const mat3& m);

      mat4(const mat4& m);
      mat4(mat4&& m);

      float operator[] (size_t i);
      const float operator[] (size_t i) const;

      mat4& operator=(const mat4& m);
      mat4& operator=(mat4&& m);

      mat4& operator+= (const mat4& m);
      mat4& operator+= (float s);
      mat4& operator-= (const mat4& m);
      mat4& operator-= (float s);
      mat4& operator*= (const mat4& m);
      mat4& operator*= (float s);

      static const mat4 Identity;
      static vec4 GetXVector(const mat4& m);
      static vec4 GetYVector(const mat4& m);
      static vec4 GetZVector(const mat4& m);
      static vec4 GetWVector(const mat4& m);
    };

    ENGINE_API mat4 operator+ (const mat4& l, const mat4& r);
    ENGINE_API mat4 operator+ (const mat4& m, float s);
    ENGINE_API mat4 operator+ (float s, const mat4& m);
    ENGINE_API mat4 operator- (const mat4& l, const mat4& r);
    ENGINE_API mat4 operator- (const mat4& m, float s);
    ENGINE_API mat4 operator- (float s, const mat4& m);
    ENGINE_API mat4 operator* (const mat4& l, const mat4& r);
    ENGINE_API mat4 operator* (const mat4& m, float s);
    ENGINE_API mat4 operator* (float s, const mat4& m);

    ENGINE_API vec4 operator* (const mat4& m, const vec4& v);
    ENGINE_API vec4 operator* (const vec4& v, const mat4& m);
    ENGINE_API vec3 operator* (const mat4& m, const vec3& v);
    ENGINE_API vec3 operator* (const vec3& v, const mat4& m);

    ENGINE_API float Determinant(const mat4& m);
    ENGINE_API mat4 Transpose(const mat4& m);
    ENGINE_API mat4 Inverse(const mat4& m);
    ENGINE_API void Transposed(mat4& m);
    ENGINE_API void Inversed(mat4& m);

    ENGINE_API void Translate(mat4& m, const vec3& v);
    ENGINE_API void Rotate(mat4& m, float a, const vec3& v);
    ENGINE_API void Scale(mat4& m, const vec3& v);
    ENGINE_API mat4 Translated(const mat4& m, const vec3& v);
    ENGINE_API mat4 Rotated(const mat4& m, float a, const vec3& v);
    ENGINE_API mat4 Scaled(const mat4& m, const vec3& v);

    ENGINE_API mat4 OrthographicRH(float left, float right, float bottom, float top, float near, float far);
    ENGINE_API mat4 PerspectiveRH(float fov, float aspect, float near, float far);
    ENGINE_API mat4 LookAtRH(const vec3& eye, const vec3& center, const vec3& up);

    /**
    * Broken! Don't use ... Need to figure out why ...
    */
    ENGINE_API mat4 PerspectiveLH(float fov, float aspect, float near, float far);

  }

}

#endif // !LEARNVK_LIBRARY_MATH_MATRIX_H
