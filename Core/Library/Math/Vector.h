#pragma once
#ifndef LEARNVK_LIBRARY_MATH_VECTOR_H
#define LEARNVK_LIBRARY_MATH_VECTOR_H

#include "Operations.h"

namespace astl
{
  namespace math
  {
    template <typename T>
    struct Vector3;

    template <typename T>
    struct Vector4;

    template <typename T>
    struct Vector2
    {
      union
      {
        struct { T x, y; };
        struct { T r, g; };
        struct { T s, t; };
      };

      Vector2() :
        x(0), y(0)
      {}

      Vector2(T s) :
        x(s), y(s)
      {}

      Vector2(T a, T b) :
        x(a), y(b)
      {}

      Vector2(const Vector3<T>& v) :
        x(v.x), y(v.y)
      {}

      Vector2(const Vector4<T>& v) :
        x(v.x), y(v.y)
      {}

      ~Vector2()
      {
        x = y = 0;
      }

      Vector2(const Vector2& v) { *this = v; }
      Vector2(Vector2&& v)      { *this = Move(v); }

      T& operator[]	(size_t i)
      {
        VKT_ASSERT(i < 2);
        return (&x)[i];
      }

      const T& operator[]	(size_t i) const
      {
        VKT_ASSERT(i < 2);
        return (&x)[i];
      }

      Vector2& operator= (const Vector2& v)
      {
        if (this != &v)
        {
          x = v.x;
          y = v.y;
        }
        return *this;
      }

      Vector2& operator= (Vector2&& v)
      {
        if (this != &v)
        {
          x = v.x;
          y = v.y;
          new (&v) Vector2();
        }
        return *this;
      }

      Vector2 operator-()
      {
        return Vector2(-x, -y);
      }

      const Vector2 operator-() const
      {
        return Vector2(-x, -y);
      }

      Vector2& operator+= (const Vector2& v)
      {
        x += v.x;
        y += v.y;
        return *this;
      }

      Vector2& operator+= (T s)
      {
        x += s;
        y += s;
        return *this;
      }

      Vector2& operator-= (const Vector2& v)
      {
        x -= v.x;
        y -= v.y;
        return *this;
      }

      Vector2& operator-= (T s)
      {
        x -= s;
        y -= s;
        return *this;
      }

      Vector2& operator*= (const Vector2& v)
      {
        x *= v.x;
        y *= v.y;
        return *this;
      }

      Vector2& operator*= (T s)
      {
        x *= s;
        y *= s;
        return *this;
      }

      Vector2& operator/= (const Vector2& v)
      {
        x /= v.x;
        y /= v.y;
        return *this;
      }

      Vector2& operator/= (T s)
      {
        x /= s;
        y /= s;
        return *this;
      }
    };

    template <typename T>
    Vector2<T> operator+ (const Vector2<T>& l, const Vector2<T>& r)
    {
      return Vector2<T>(l.x + r.x, l.y + r.y);
    }

    template <typename T>
    Vector2<T> operator+ (const Vector2<T>& v, T s)
    {
      return Vector2<T>(v.x + s, v.y + s);
    }

    template <typename T>
    Vector2<T> operator+ (T s, const Vector2<T>& v)
    {
      return v + s;
    }

    template <typename T>
    Vector2<T> operator- (const Vector2<T>& l, const Vector2<T>& r)
    {
      return Vector2<T>(l.x - r.x, l.y - r.y);
    }

    template <typename T>
    Vector2<T> operator- (const Vector2<T>& v, T s)
    {
      return Vector2<T>(v.x - s, v.y - s);
    }

    template <typename T>
    Vector2<T> operator- (T s, const Vector2<T>& v)
    {
      return Vector2<T>(s - v.x, s - v.y);
    }

    template <typename T>
    Vector2<T> operator* (const Vector2<T>& l, const Vector2<T>& r)
    {
      return Vector2<T>(l.x * r.x, l.y * r.y);
    }

    template <typename T>
    Vector2<T> operator* (const Vector2<T>& v, T s)
    {
      return Vector2<T>(v.x * s, v.y * s);
    }

    template <typename T>
    Vector2<T> operator* (T s, const Vector2<T>& v)
    {
      return Vector2<T>(s * v.x, s * v.y);
    }

    template <typename T>
    Vector2<T> operator/ (const Vector2<T>& l, const Vector2<T>& r)
    {
      return Vector2<T>(l.x / r.x, l.y / r.y);
    }

    template <typename T>
    Vector2<T> operator/ (const Vector2<T>& v, T s)
    {
      return Vector2<T>(v.x / s, v.y / s);
    }

    template <typename T>
    Vector2<T> operator/ (T s, const Vector2<T>& v)
    {
      return Vector2<T>(s / v.x, s / v.y);
    }

    template <typename T>
    bool operator== (const Vector2<T>& l, const Vector2<T>& r)
    {
      return (l.x == r.x) && (l.y == r.y);
    }

    template <typename T>
    bool operator!= (const Vector2<T>& l, const Vector2<T>& r)
    {
      return l.x != r.x || l.y != r.y;
    }

    template <typename T>
    T Length(const Vector2<T>& v)
    {
      return Sqrt(v.x * v.x + v.y * v.y);
    }

    template <typename T>
    void	Normalize(Vector2<T>& v)
    {
      const T len = Length(v);
      v /= len;
    }

    template <typename T>
    T Dot(const Vector2<T>& v)
    {
      return v.x * v.x + v.y * v.y;
    }

    template <typename T>
    T Dot(const Vector2<T>& l, const Vector2<T>& r)
    {
      return l.x * r.x + l.y * r.y;
    }

    template <typename T>
    Vector2<T>	Normalized(const Vector2<T>& v)
    {
      const T len = Length(v);
      return v / len;
    }

    template <typename T>
    bool	InBounds(const Vector2<T>& p, const Vector2<T>& min, const Vector2<T>& max)
    {
      return (p.x > min.x && p.x < max.x) && (p.y > min.y && p.y < max.y);
    }

    template <typename T>
    struct Vector3
    {
      union
      {
        struct { T x, y, z; };
        struct { T r, g, b; };
        struct { T s, t, p; };
      };

      Vector3() :
        x(0), y(0), z(0)
      {}

      Vector3(T s) :
        x(s), y(s), z(s)
      {}

      Vector3(T a, T b, T c) :
        x(a), y(b), z(c)
      {}

      Vector3(const Vector2<T>& v, T s) :
        x(v.x), y(v.y), z(s)
      {}

      Vector3(T s, const Vector2<T>& v) :
        x(s), y(v.x), z(v.y)
      {}

      Vector3(const Vector2<T>& v) :
        x(v.x), y(v.y), z(0)
      {}

      Vector3(const Vector4<T>& v) :
        x(v.x), y(v.y), z(v.z)
      {}

      Vector3(const Vector3& v)
      {
        *this = v;
      }

      Vector3(Vector3&& v)
      {
        *this = Move(v);
      }

      ~Vector3()
      {
        x = y = z = 0;
      }

      Vector3& operator= (const Vector3& v)
      {
        if (this != &v)
        {
          x = v.x;
          y = v.y;
          z = v.z;
        }
        return *this;
      }

      Vector3& operator= (Vector3&& v)
      {
        if (this != &v)
        {
          x = v.x;
          y = v.y;
          z = v.z;
          new (&v) Vector3();
        }
        return *this;
      }

      T& operator[] (size_t i)
      {
        VKT_ASSERT(i < 3);
        return (&x)[i];
      }

      const T& operator[] (size_t i) const
      {
        VKT_ASSERT(i < 3);
        return (&x)[i];
      }

      Vector3 operator-()
      {
        return Vector3(-x, -y, -z);
      }

      const Vector3 operator-() const
      {
        return Vector3(-x, -y, -z);
      }

      Vector3& operator+= (const Vector3& v)
      {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
      }

      Vector3& operator+= (T s)
      {
        x += s;
        y += s;
        z += s;
        return *this;
      }

      Vector3& operator-= (const Vector3& v)
      {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
      }

      Vector3& operator-= (T s)
      {
        x -= s;
        y -= s;
        z -= s;
        return *this;
      }

      Vector3& operator*= (const Vector3& v)
      {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
      }

      Vector3& operator*= (T s)
      {
        x *= s;
        y *= s;
        z *= s;
        return *this;
      }

      Vector3& operator/= (const Vector3& v)
      {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
      }

      Vector3& operator/= (T s)
      {
        x /= s;
        y /= s;
        z /= s;
        return *this;
      }
    };

    template <typename T>
    Vector3<T> operator+ (const Vector3<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x + r.x, l.y + r.y, l.z + r.z);
    }

    template <typename T>
    Vector3<T> operator+ (const Vector3<T>& l, const Vector2<T>& r)
    {
      return Vector3<T>(l.x + r.x, l.y + r.y, l.z);
    }

    template <typename T>
    Vector3<T> operator+ (const Vector2<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x + r.x, l.y + r.y, r.z);
    }

    template <typename T>
    Vector3<T> operator+ (const Vector3<T>& v, T s)
    {
      return Vector3<T>(v.x + s, v.y + s, v.z + s);
    }

    template <typename T>
    Vector3<T> operator+ (T s, const Vector3<T>& v)
    {
      return v + s;
    }

    template <typename T>
    Vector3<T> operator- (const Vector3<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x - r.x, l.y - r.y, l.z - r.z);
    }

    template <typename T>
    Vector3<T> operator- (const Vector3<T>& l, const Vector2<T>& r)
    {
      return Vector3<T>(l.x - r.x, l.y - r.y, l.z);
    }

    template <typename T>
    Vector3<T> operator- (const Vector2<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x - r.x, l.y - r.y, r.z);
    }

    template <typename T>
    Vector3<T> operator- (const Vector3<T>& v, T s)
    {
      return Vector3<T>(v.x - s, v.y - s, v.z - s);
    }

    template <typename T>
    Vector3<T> operator- (T s, const Vector3<T>& v)
    {
      return Vector3<T>(s - v.x, s - v.y, s - v.z);
    }

    template <typename T>
    Vector3<T> operator* (const Vector3<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x * r.x, l.y * r.y, l.z * r.z);
    }

    template <typename T>
    Vector3<T> operator* (const Vector3<T>& l, const Vector2<T>& r)
    {
      return Vector3<T>(l.x * r.x, l.y * r.y, l.z);
    }

    template <typename T>
    Vector3<T> operator* (const Vector2<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x * r.x, l.y * r.y, r.z);
    }

    template <typename T>
    Vector3<T> operator* (const Vector3<T>& v, T s)
    {
      return Vector3<T>(v.x * s, v.y * s, v.z * s);
    }

    template <typename T>
    Vector3<T> operator* (T s, const Vector3<T>& v)
    {
      return v * s;
    }

    template <typename T>
    Vector3<T> operator/ (const Vector3<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x / r.x, l.y / r.y, l.z / r.z);
    }

    template <typename T>
    Vector3<T> operator/ (const Vector3<T>& l, const Vector2<T>& r)
    {
      return Vector3<T>(l.x / r.x, l.y / r.y, l.z);
    }

    template <typename T>
    Vector3<T> operator/ (const Vector2<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(l.x / r.x, l.y / r.y, r.z);
    }

    template <typename T>
    Vector3<T> operator/ (const Vector3<T>& v, T s)
    {
      return Vector3<T>(v.x / s, v.y / s, v.z / s);
    }

    template <typename T>
    Vector3<T> operator/ (T s, const Vector3<T>& v)
    {
      return Vector3<T>(s / v.x, s / v.y, s / v.z);
    }

    template <typename T>
    bool operator== (const Vector3<T>& l, const Vector3<T>& r)
    {
      return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
    }

    template <typename T>
    bool operator!= (const Vector3<T>& l, const Vector3<T>& r)
    {
      return (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
    }

    template <typename T>
    T Length(const Vector3<T>& v)
    {
      return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    template <typename T>
    void	Normalize(Vector3<T>& v)
    {
      const T len = Length(v);
      v /= len;
    }

    template <typename T>
    T Dot(const Vector3<T>& v)
    {
      return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    template <typename T>
    T Dot(const Vector3<T>& l, const Vector3<T>& r)
    {
      return l.x * r.x + l.y * r.y + l.z * r.z;
    }

    template <typename T>
    Vector3<T>	Cross(const Vector3<T>& l, const Vector3<T>& r)
    {
      return Vector3<T>(
        l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x
      );
    }

    template <typename T>
    Vector3<T>	Normalized(const Vector3<T>& v)
    {
      const T len = Length(v);
      return v / len;
    }

    template <typename T>
    struct Vector4
    {
      union
      {
        struct { T x, y, z, w; };
        struct { T r, g, b, a; };
        struct { T s, t, p, q; };
      };

      Vector4() :
        x(0), y(0), z(0), w(0)
      {}

      Vector4(T s) :
        x(s), y(s), z(s), w(s)
      {}

      Vector4(T a, T b, T c, T d) :
        x(a), y(b), z(c), w(d)
      {}

      Vector4(const Vector2<T>& v) :
        x(v.x), y(v.y), z(0), w(0)
      {}

      Vector4(const Vector2<T>& v, T a, T b) :
        x(v.x), y(v.y), z(a), w(b)
      {}

      Vector4(T a, T b, const Vector2<T>& v) :
        x(a), y(b), z(v.x), w(v.y)
      {}

      Vector4(const Vector2<T>& a, const Vector2<T>& b) :
        x(a.x), y(a.y), z(b.x), w(b.y)
      {}

      Vector4(const Vector3<T>& v, T s) :
        x(v.x), y(v.y), z(v.z), w(s)
      {}

      Vector4(T s, const Vector3<T>& v) :
        x(s), y(v.x), z(v.y), w(v.z)
      {}

      Vector4(const Vector3<T>& v) :
        x(v.x), y(v.y), z(v.z), w(0)
      {}

      ~Vector4()
      {
        x = y = z = w = 0;
      }

      Vector4(const Vector4& v)
      {
        *this = v;
      }

      Vector4(Vector4&& v)
      {
        *this = Move(v);
      }

      Vector4& operator= (const Vector4& v)
      {
        if (this != &v)
        {
          x = v.x;
          y = v.y;
          z = v.z;
          w = v.w;
        }
        return *this;
      }

      Vector4& operator= (Vector4&& v)
      {
        if (this != &v)
        {
          x = v.x;
          y = v.y;
          z = v.z;
          w = v.w;
          new (&v) Vector4();
        }
        return *this;
      }

      T& operator[] (size_t i)
      {
        VKT_ASSERT(i < 4);
        return (&x)[i];
      }

      const T& operator[] (size_t i) const
      {
        VKT_ASSERT(i < 4);
        return (&x)[i];
      }

      Vector4 operator-()
      {
        return Vector4(-x, -y, -z, -w);
      }

      const Vector4 operator-() const
      {
        return Vector4(-x, -y, -z, -w);
      }

      Vector4& operator+= (const Vector4& v)
      {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
      }

      Vector4& operator+= (T s)
      {
        x += s;
        y += s;
        z += s;
        w += s;
        return *this;
      }

      Vector4& operator-= (const Vector4& v)
      {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
      }

      Vector4& operator-= (T s)
      {
        x -= s;
        y -= s;
        z -= s;
        w -= s;
        return *this;
      }

      Vector4& operator*= (const Vector4& v)
      {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
        return *this;
      }

      Vector4& operator*= (T s)
      {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
      }

      Vector4& operator/= (const Vector4& v)
      {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        w /= v.w;
        return *this;
      }

      Vector4& operator/= (T s)
      {
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
      }
    };

    template <typename T>
    Vector4<T> operator+ (const Vector4<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w);
    }

    template <typename T>
    Vector4<T> operator+ (const Vector4<T>& l, const Vector3<T>& r)
    {
      return Vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, l.w);
    }

    template <typename T>
    Vector4<T> operator+ (const Vector3<T>& l, const Vector4<T>& r)
    {
      return r + l;
      //return Vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, r.w);
    }

    template <typename T>
    Vector4<T> operator+ (const Vector4<T>& l, const Vector2<T>& r)
    {
      return Vector4<T>(l.x + r.x, l.y + r.y, l.z, l.w);
    }

    template <typename T>
    Vector4<T> operator+ (const Vector2<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x + r.x, l.y + r.y, r.z, r.w);
    }

    template <typename T>
    Vector4<T> operator+ (const Vector4<T>& v, T s)
    {
      return Vector4<T>(v.x + s, v.y + s, v.z + s, v.w + s);
    }

    template <typename T>
    Vector4<T> operator+ (T s, const Vector4<T>& v)
    {
      return v + s;
    }

    template <typename T>
    Vector4<T> operator- (const Vector4<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
    }

    template <typename T>
    Vector4<T> operator- (const Vector4<T>& l, const Vector3<T>& r)
    {
      return Vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, l.w);
    }

    template <typename T>
    Vector4<T> operator- (const Vector3<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, r.w);
    }

    template <typename T>
    Vector4<T> operator- (const Vector4<T>& l, const Vector2<T>& r)
    {
      return Vector4<T>(l.x - r.x, l.y - r.y, l.z, l.w);
    }

    template <typename T>
    Vector4<T> operator- (const Vector2<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x - r.x, l.y - r.y, r.z, r.w);
    }

    template <typename T>
    Vector4<T> operator- (const Vector4<T>& v, T s)
    {
      return Vector4<T>(v.x - s, v.y - s, v.z - s, v.w - s);
    }

    template <typename T>
    Vector4<T> operator- (T s, const Vector4<T>& v)
    {
      return Vector4<T>(s - v.x, s - v.y, s - v.z, s - v.w);
    }

    template <typename T>
    Vector4<T> operator* (const Vector4<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w);
    }

    template <typename T>
    Vector4<T> operator* (const Vector4<T>& l, const Vector3<T>& r)
    {
      return Vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, l.w);
    }

    template <typename T>
    Vector4<T> operator* (const Vector3<T>& l, const Vector4<T>& r)
    {
      return r * l;
      //return Vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, r.w);
    }

    template <typename T>
    Vector4<T> operator* (const Vector4<T>& l, const Vector2<T>& r)
    {
      return Vector4<T>(l.x * r.x, l.y * r.y, l.z, l.w);
    }

    template <typename T>
    Vector4<T> operator* (const Vector2<T>& l, const Vector4<T>& r)
    {
      return r * l;
    }

    template <typename T>
    Vector4<T> operator* (const Vector4<T>& v, T s)
    {
      return Vector4<T>(v.x * s, v.y * s, v.z * s, v.w * s);
    }

    template <typename T>
    Vector4<T> operator* (T s, const Vector4<T>& v)
    {
      return v * s;
    }

    template <typename T>
    Vector4<T> operator/ (const Vector4<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w);
    }

    template <typename T>
    Vector4<T> operator/ (const Vector4<T>& l, const Vector3<T>& r)
    {
      return Vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, l.w);
    }

    template <typename T>
    Vector4<T> operator/ (const Vector3<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, r.w);
    }

    template <typename T>
    Vector4<T> operator/ (const Vector4<T>& l, const Vector2<T>& r)
    {
      return Vector4<T>(l.x / r.x, l.y / r.y, l.z, l.w);
    }

    template <typename T>
    Vector4<T> operator/ (const Vector2<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(l.x / r.x, l.y / r.y, r.z, r.w);
    }

    template <typename T>
    Vector4<T> operator/ (const Vector4<T>& v, T s)
    {
      return Vector4<T>(v.x / s, v.y / s, v.z / s, v.w / s);
    }

    template <typename T>
    Vector4<T> operator/ (T s, const Vector4<T>& v)
    {
      return Vector4<T>(s / v.x, s / v.y, s / v.z, s / v.w);
    }

    template <typename T>
    bool operator== (const Vector4<T>& l, const Vector4<T>& r)
    {
      return (l.x == r.x) && (l.y == r.y) && (l.z == r.z) && (l.w == r.w);
    }

    template <typename T>
    bool operator!= (const Vector4<T>& l, const Vector4<T>& r)
    {
      return (l.x != r.x) || (l.y != r.y) || (l.z != r.z) || (l.w != r.w);
    }

    template <typename T>
    T Length(const Vector4<T>& v)
    {
      return Sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }

    template <typename T>
    void Normalize(Vector4<T>& v)
    {
      const T len = Length(v);
      v /= len;
    }

    template <typename T>
    T Dot(const Vector4<T>& v)
    {
      return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    }

    template <typename T>
    T Dot(const Vector4<T>& l, const Vector4<T>& r)
    {
      return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
    }

    template <typename T>
    Vector4<T> Cross(const Vector4<T>& l, const Vector4<T>& r)
    {
      return Vector4<T>(
        l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x,
        1
      );
    }

    template <typename T>
    Vector4<T> Normalized(const Vector4<T>& v)
    {
      const T len = Length(v);
      return v / len;
    }

    template <typename T>
    Vector2<T> Reflect(const Vector2<T>& i, const Vector2<T>& n)
    {
      return i - 2 * (Dot(i, n) * n);
    }

    template <typename T>
    Vector3<T> Reflect(const Vector3<T>& i, const Vector3<T>& n)
    {
      return i - 2 * (Dot(i, n) * n);;
    }

    template <typename T>
    Vector4<T> Reflect(const Vector4<T>& i, const Vector4<T>& n)
    {
      return i - 2 * (Dot(i, n) * n);
    }

    using vec2 = Vector2<float32>;
    using vec3 = Vector3<float32>;
    using vec4 = Vector4<float32>;

    using ivec2 = Vector2<int32>;
    using ivec3 = Vector3<int32>;
    using ivec4 = Vector4<int32>;

    using uvec2 = Vector2<uint32>;
    using uvec3 = Vector3<uint32>;
    using uvec4 = Vector4<uint32>;
  }
}

#endif // !LEARNVK_LIBRARY_MATH_VECTOR_H
