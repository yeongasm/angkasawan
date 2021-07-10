#pragma once
#ifndef LEARNVK_LIBRARY_MATH_QUATERNION
#define LEARNVK_LIBRARY_MATH_QUATERNION

#include "Matrix.h"

namespace astl
{
  namespace math
  {
    template <typename T>
    struct Quaternion
    {
      T w, x, y, z;

      Quaternion() :
        w(1), x(0), y(0), z(0)
      {}

      ~Quaternion()
      {
        x = y = z = 0;
        w = 1;
      }

      Quaternion(const Vector3<T>& v, T s) :
        w(s), x(v.x), y(v.y), z(v.z)
      {}

      Quaternion(T s, const Vector3<T>& v) :
        w(s), x(v.x), y(v.y), z(v.z)
      {}

      Quaternion(T w, T x, T y, T z) :
        w(w), x(x), y(y), z(z)
      {}

      Quaternion(const Quaternion& q)
      {
        *this = q;
      }

      Quaternion(Quaternion&& q)
      {
        *this = Move(q);
      }

      Quaternion& operator=(const Quaternion& q)
      {
        if (this != &q)
        {
          w = q.w;
          x = q.x;
          y = q.y;
          z = q.z;
        }
        return *this;
      }

      Quaternion& operator=(Quaternion&& q)
      {
        if (this != &q)
        {
          w = q.w;
          x = q.x;
          y = q.y;
          z = q.z;
          new (&q) Quaternion();
        }
        return *this;
      }

      Quaternion& operator+= (const Quaternion& q)
      {
        *this = *this + q;
        return *this;
      }

      Quaternion& operator-= (const Quaternion& q)
      {
        *this = *this - q;
        return *this;
      }

      Quaternion& operator*= (const Quaternion& q)
      {
        *this = *this * q;
        return *this;
      }

      Quaternion& operator*= (T s)
      {
        *this = *this * s;
        return *this;
      }

      Quaternion& operator/= (T s)
      {
        *this = *this / s;
        return *this;
      }
    };

    template <typename T>
    Quaternion<T> operator+ (const Quaternion<T>& l, const Quaternion<T>& r)
    {
      return Quaternion<T>(l.w + r.w, l.x + r.x, l.y + r.y, l.z + r.z);
    }

    template <typename T>
    Quaternion<T> operator- (const Quaternion<T>& l, const Quaternion<T>& r)
    {
      return Quaternion<T>(l.w - r.w, l.x - r.x, l.y - r.y, l.z - r.z);
    }

    template <typename T>
    Quaternion<T> operator* (const Quaternion<T>& l, const Quaternion<T>& r)
    {
      Vector3<T> a(l.x, l.y, l.z);
      Vector3<T> b(r.x, r.y, r.z);
      T s0 = l.w;
      T s1 = r.w;

      return Quaternion<T>(s0 * s1 - Dot(a, b), (s0 * b) + (s1 * a) + Cross(a, b));
      //Quaternion<T> res;
      //res.w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;
      //res.x = l.w * r.x + r.w * l.x + l.y * r.z - l.z * r.y;
      //res.y = l.w * r.y + r.w * l.y + l.z * r.x - r.z * l.x;
      //res.z = l.w * r.z + r.w * l.z + l.w * r.y - r.x * l.y;
      //return res;
    }

    template <typename T>
    Vector3<T> operator* (const Quaternion<T>& l, const Vector3<T>& r)
    {
      const Vector3<T> qvec(l.x, l.y, l.z);
      const Vector3<T> uv(Cross(qvec, r));
      const Vector3<T> uuv(Cross(qvec, uv));

      return r + ((uv * l.w) + uuv) * static_cast<T>(2);
    }

    template <typename T>
    Vector3<T> operator* (const Vector3<T>& l, const Quaternion<T>& r)
    {
      return Inversed(r) * l;
    }

    template <typename T>
    Quaternion<T> operator* (const Quaternion<T>& q, T s)
    {
      return Quaternion<T>(q.w * s, q.x * s, q.y * s, q.z * s);
    }

    template <typename T>
    Quaternion<T> operator* (T s, const Quaternion<T>& q)
    {
      return q * s;
    }

    template <typename T>
    Quaternion<T> operator/ (const Quaternion<T>& q, T s)
    {
      return Quaternion<T>(q.w / s, q.x / s, q.y / s, q.z / s);
    }

    template <typename T>
    bool operator== (const Quaternion<T>& l, const Quaternion<T>& r)
    {
      return (l.w == r.w) && (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
    }

    template <typename T>
    bool operator!= (const Quaternion<T>& l, const Quaternion<T>& r)
    {
      return (l.w != r.w) || (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
    }

    template <typename T>
    T Length(const Quaternion<T>& q)
    {
      return Sqrt(Dot(q));
    }

    template <typename T>
    void Normalize(Quaternion<T>& q)
    {
      q = Normalized(q);
    }

    template <typename T>
    void Conjugate(Quaternion<T>& q)
    {
      q = Conjugated(q);
    }

    template <typename T>
    void Inverse(Quaternion<T>& q)
    {
      q = Inversed(q);
    }

    template <typename T>
    Quaternion<T> Normalized(const Quaternion<T>& q)
    {
      T len = Length(q);
      T denom = 1 / len;
      if (len <= 0)
      {
        return Quaternion<T>(1, 0, 0, 0);
      }
      return q * denom;
    }

    template <typename T>
    Quaternion<T> Conjugated(const Quaternion<T>& q)
    {
      return Quaternion<T>(q.w, -q.x, -q.y, -q.z);
    }

    template <typename T>
    Quaternion<T> Inversed(const Quaternion<T>& q)
    {
      return Conjugated(q) / Dot(q);
    }

    template <typename T>
    Matrix3x3<T> Mat3Cast(const Quaternion<T>& q)
    {
      Matrix3x3<T> res(1);

      T qxx(q.x * q.x);
      T qyy(q.y * q.y);
      T qzz(q.z * q.z);
      T qxz(q.x * q.z);
      T qxy(q.x * q.y);
      T qyz(q.y * q.z);
      T qwx(q.w * q.x);
      T qwy(q.w * q.y);
      T qwz(q.w * q.z);

      res.m00 = static_cast<T>(1) - static_cast<T>(2) * (qyy + qzz);
      res.m01 = static_cast<T>(2) * (qxy + qwz);
      res.m02 = static_cast<T>(2) * (qxz - qwy);

      res.m10 = static_cast<T>(2) * (qxy - qwz);
      res.m11 = static_cast<T>(1) - static_cast<T>(2) * (qxx + qzz);
      res.m12 = static_cast<T>(2) * (qyz + qwx);

      res.m20 = static_cast<T>(2) * (qxz + qwy);
      res.m21 = static_cast<T>(2) * (qyz - qwx);
      res.m22 = static_cast<T>(1) - static_cast<T>(2) * (qxx + qyy);

      return res;
    }

    template <typename T>
    Matrix4x4<T> Mat4Cast(const Quaternion<T>& q)
    {
      return Matrix4x4<T>(Mat3Cast(q));
    }

    template <typename T>
    Quaternion<T> AngleAxis(T angle, const Vector3<T>& axis)
    {
      Quaternion<T> res;
      const T a = angle;
      const T s = Sin(a * static_cast<T>(0.5));

      res.w = Cos(a * static_cast<T>(0.5));
      res.x = axis.x * s;
      res.y = axis.y * s;
      res.z = axis.z * s;

      return res;
    }

    template <typename T>
    T Dot(const Quaternion<T>& q)
    {
      return Dot(q, q);
    }

    template <typename T>
    T Dot(const Quaternion<T>& a, const Quaternion<T>& b)
    {
      //vec4 tmp(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
      //return (tmp.x + tmp.y) + (tmp.z + tmp.w);
      return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    }

    using quat = Quaternion<float32>;

  }
}

#endif // !LEARNVK_LIBRARY_MATH_QUATERNION
