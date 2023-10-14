#pragma once
#ifndef MATH_LIBRARY_VECTOR_H
#define MATH_LIBRARY_VECTOR_H

#include "math.h"

namespace math
{

    template <is_arithmetic T>
    struct Vector3;

    template <is_arithmetic T>
    struct Vector4;

    template <is_arithmetic T>
    struct Vector2
    {
        union
        {
            struct { T x, y; };
            struct { T r, g; };
            struct { T s, t; };
        };

        constexpr Vector2() :
            x(0), y(0)
        {}

        constexpr Vector2(T s) :
            x(s), y(s)
        {}

        constexpr Vector2(T a, T b) :
            x(a), y(b)
        {}

        constexpr Vector2(const Vector3<T>& v) :
            x(v.x), y(v.y)
        {}

        constexpr Vector2(const Vector4<T>& v) :
            x(v.x), y(v.y)
        {}

        constexpr ~Vector2()
        {
            x = y = 0;
        }

        constexpr Vector2(const Vector2& v) { *this = v; }
        constexpr Vector2(Vector2&& v) { *this = std::move(v); }

        constexpr T& operator[]	(size_t i)
        {
            ASSERTION(i < 2);
            return (&x)[i];
        }

        constexpr const T& operator[]	(size_t i) const
        {
            ASSERTION(i < 2);
            return (&x)[i];
        }

        constexpr Vector2& operator= (const Vector2& v)
        {
            if (this != &v)
            {
                x = v.x;
                y = v.y;
            }
            return *this;
        }

        constexpr Vector2& operator= (Vector2&& v)
        {
            if (this != &v)
            {
                x = v.x;
                y = v.y;
                new (&v) Vector2();
            }
            return *this;
        }

        constexpr Vector2 operator-()
        {
            return Vector2(-x, -y);
        }

        constexpr const Vector2 operator-() const
        {
            return Vector2(-x, -y);
        }

        constexpr Vector2& operator+= (const Vector2& v)
        {
            x += v.x;
            y += v.y;
            return *this;
        }

        constexpr Vector2& operator+= (T s)
        {
            x += s;
            y += s;
            return *this;
        }

        constexpr Vector2& operator-= (const Vector2& v)
        {
            x -= v.x;
            y -= v.y;
            return *this;
        }

        constexpr Vector2& operator-= (T s)
        {
            x -= s;
            y -= s;
            return *this;
        }

        constexpr Vector2& operator*= (const Vector2& v)
        {
            x *= v.x;
            y *= v.y;
            return *this;
        }

        constexpr Vector2& operator*= (T s)
        {
            x *= s;
            y *= s;
            return *this;
        }

        constexpr Vector2& operator/= (const Vector2& v)
        {
            x /= v.x;
            y /= v.y;
            return *this;
        }

        constexpr Vector2& operator/= (T s)
        {
            x /= s;
            y /= s;
            return *this;
        }
    };

    template <is_arithmetic T>
    constexpr Vector2<T> operator+ (const Vector2<T>& l, const Vector2<T>& r)
    {
        return Vector2<T>(l.x + r.x, l.y + r.y);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator+ (const Vector2<T>& v, T s)
    {
        return Vector2<T>(v.x + s, v.y + s);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator+ (T s, const Vector2<T>& v)
    {
        return v + s;
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator- (const Vector2<T>& l, const Vector2<T>& r)
    {
        return Vector2<T>(l.x - r.x, l.y - r.y);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator- (const Vector2<T>& v, T s)
    {
        return Vector2<T>(v.x - s, v.y - s);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator- (T s, const Vector2<T>& v)
    {
        return Vector2<T>(s - v.x, s - v.y);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator* (const Vector2<T>& l, const Vector2<T>& r)
    {
        return Vector2<T>(l.x * r.x, l.y * r.y);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator* (const Vector2<T>& v, T s)
    {
        return Vector2<T>(v.x * s, v.y * s);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator* (T s, const Vector2<T>& v)
    {
        return Vector2<T>(s * v.x, s * v.y);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator/ (const Vector2<T>& l, const Vector2<T>& r)
    {
        return Vector2<T>(l.x / r.x, l.y / r.y);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator/ (const Vector2<T>& v, T s)
    {
        return Vector2<T>(v.x / s, v.y / s);
    }

    template <is_arithmetic T>
    constexpr Vector2<T> operator/ (T s, const Vector2<T>& v)
    {
        return Vector2<T>(s / v.x, s / v.y);
    }

    template <is_arithmetic T>
    constexpr bool operator== (const Vector2<T>& l, const Vector2<T>& r)
    {
        return (l.x == r.x) && (l.y == r.y);
    }

    template <is_arithmetic T>
    constexpr bool operator!= (const Vector2<T>& l, const Vector2<T>& r)
    {
        return l.x != r.x || l.y != r.y;
    }

    template <is_arithmetic T>
    T length(const Vector2<T>& v)
    {
        return sqrt(v.x * v.x + v.y * v.y);
    }

    template <is_arithmetic T>
    constexpr void normalize(Vector2<T>& v)
    {
        const T len = length(v);
        v /= len;
    }

    template <is_arithmetic T>
    constexpr T dot(const Vector2<T>& v)
    {
        return v.x * v.x + v.y * v.y;
    }

    template <is_arithmetic T>
    constexpr T dot(const Vector2<T>& l, const Vector2<T>& r)
    {
        return l.x * r.x + l.y * r.y;
    }

    template <is_arithmetic T>
    constexpr Vector2<T> normalized(const Vector2<T>& v)
    {
        const T len = length(v);
        return v / len;
    }

    template <is_arithmetic T>
    constexpr bool in_bounds(const Vector2<T>& p, const Vector2<T>& min, const Vector2<T>& max)
    {
        return (p.x > min.x && p.x < max.x) && (p.y > min.y && p.y < max.y);
    }

    template <is_arithmetic T>
    struct Vector3
    {
        union
        {
            struct { T x, y, z; };
            struct { T r, g, b; };
            struct { T s, t, p; };
        };

        constexpr Vector3() :
            x(0), y(0), z(0)
        {}

        constexpr Vector3(T s) :
            x(s), y(s), z(s)
        {}

        constexpr Vector3(T a, T b, T c) :
            x(a), y(b), z(c)
        {}

        constexpr Vector3(const Vector2<T>& v, T s) :
            x(v.x), y(v.y), z(s)
        {}

        constexpr Vector3(T s, const Vector2<T>& v) :
            x(s), y(v.x), z(v.y)
        {}

        constexpr Vector3(const Vector2<T>& v) :
            x(v.x), y(v.y), z(0)
        {}

        constexpr Vector3(const Vector4<T>& v) :
            x(v.x), y(v.y), z(v.z)
        {}

        constexpr Vector3(const Vector3& v)
        {
            *this = v;
        }

        constexpr Vector3(Vector3&& v)
        {
            *this = std::move(v);
        }

        constexpr ~Vector3()
        {
            x = y = z = 0;
        }

        constexpr Vector3& operator= (const Vector3& v)
        {
            if (this != &v)
            {
                x = v.x;
                y = v.y;
                z = v.z;
            }
            return *this;
        }

        constexpr Vector3& operator= (Vector3&& v)
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

        constexpr T& operator[] (size_t i)
        {
            ASSERTION(i < 3);
            return (&x)[i];
        }

        constexpr const T& operator[] (size_t i) const
        {
            ASSERTION(i < 3);
            return (&x)[i];
        }

        constexpr Vector3 operator-()
        {
            return Vector3(-x, -y, -z);
        }

        constexpr const Vector3 operator-() const
        {
            return Vector3(-x, -y, -z);
        }

        constexpr Vector3& operator+= (const Vector3& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;
            return *this;
        }

        constexpr Vector3& operator+= (T s)
        {
            x += s;
            y += s;
            z += s;
            return *this;
        }

        constexpr Vector3& operator-= (const Vector3& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            return *this;
        }

        constexpr Vector3& operator-= (T s)
        {
            x -= s;
            y -= s;
            z -= s;
            return *this;
        }

        constexpr Vector3& operator*= (const Vector3& v)
        {
            x *= v.x;
            y *= v.y;
            z *= v.z;
            return *this;
        }

        constexpr Vector3& operator*= (T s)
        {
            x *= s;
            y *= s;
            z *= s;
            return *this;
        }

        constexpr Vector3& operator/= (const Vector3& v)
        {
            x /= v.x;
            y /= v.y;
            z /= v.z;
            return *this;
        }

        constexpr Vector3& operator/= (T s)
        {
            x /= s;
            y /= s;
            z /= s;
            return *this;
        }
    };

    template <is_arithmetic T>
    constexpr Vector3<T> operator+ (const Vector3<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x + r.x, l.y + r.y, l.z + r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator+ (const Vector3<T>& l, const Vector2<T>& r)
    {
        return Vector3<T>(l.x + r.x, l.y + r.y, l.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator+ (const Vector2<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x + r.x, l.y + r.y, r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator+ (const Vector3<T>& v, T s)
    {
        return Vector3<T>(v.x + s, v.y + s, v.z + s);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator+ (T s, const Vector3<T>& v)
    {
        return v + s;
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator- (const Vector3<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x - r.x, l.y - r.y, l.z - r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator- (const Vector3<T>& l, const Vector2<T>& r)
    {
        return Vector3<T>(l.x - r.x, l.y - r.y, l.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator- (const Vector2<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x - r.x, l.y - r.y, r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator- (const Vector3<T>& v, T s)
    {
        return Vector3<T>(v.x - s, v.y - s, v.z - s);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator- (T s, const Vector3<T>& v)
    {
        return Vector3<T>(s - v.x, s - v.y, s - v.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator* (const Vector3<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x * r.x, l.y * r.y, l.z * r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator* (const Vector3<T>& l, const Vector2<T>& r)
    {
        return Vector3<T>(l.x * r.x, l.y * r.y, l.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator* (const Vector2<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x * r.x, l.y * r.y, r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator* (const Vector3<T>& v, T s)
    {
        return Vector3<T>(v.x * s, v.y * s, v.z * s);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator* (T s, const Vector3<T>& v)
    {
        return v * s;
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator/ (const Vector3<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x / r.x, l.y / r.y, l.z / r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator/ (const Vector3<T>& l, const Vector2<T>& r)
    {
        return Vector3<T>(l.x / r.x, l.y / r.y, l.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator/ (const Vector2<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(l.x / r.x, l.y / r.y, r.z);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator/ (const Vector3<T>& v, T s)
    {
        return Vector3<T>(v.x / s, v.y / s, v.z / s);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> operator/ (T s, const Vector3<T>& v)
    {
        return Vector3<T>(s / v.x, s / v.y, s / v.z);
    }

    template <is_arithmetic T>
    constexpr bool operator== (const Vector3<T>& l, const Vector3<T>& r)
    {
        return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
    }

    template <is_arithmetic T>
    constexpr bool operator!= (const Vector3<T>& l, const Vector3<T>& r)
    {
        return (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
    }

    template <is_arithmetic T>
    T length(const Vector3<T>& v)
    {
        return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    template <is_arithmetic T>
    constexpr void normalize(Vector3<T>& v)
    {
        const T len = length(v);
        v /= len;
    }

    template <is_arithmetic T>
    constexpr T dot(const Vector3<T>& v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    template <is_arithmetic T>
    T dot(const Vector3<T>& l, const Vector3<T>& r)
    {
        return l.x * r.x + l.y * r.y + l.z * r.z;
    }

    template <is_arithmetic T>
    constexpr Vector3<T> cross(const Vector3<T>& l, const Vector3<T>& r)
    {
        return Vector3<T>(
            l.y * r.z - l.z * r.y,
            l.z * r.x - l.x * r.z,
            l.x * r.y - l.y * r.x
        );
    }

    template <is_arithmetic T>
    constexpr Vector3<T> normalized(const Vector3<T>& v)
    {
        const T len = length(v);
        return v / len;
    }

    template <is_arithmetic T>
    struct Vector4
    {
        union
        {
            struct { T x, y, z, w; };
            struct { T r, g, b, a; };
            struct { T s, t, p, q; };
        };

        constexpr Vector4() :
            x(0), y(0), z(0), w(0)
        {}

        constexpr Vector4(T s) :
            x(s), y(s), z(s), w(s)
        {}

        constexpr Vector4(T a, T b, T c, T d) :
            x(a), y(b), z(c), w(d)
        {}

        constexpr Vector4(const Vector2<T>& v) :
            x(v.x), y(v.y), z(0), w(0)
        {}

        constexpr Vector4(const Vector2<T>& v, T a, T b) :
            x(v.x), y(v.y), z(a), w(b)
        {}

        constexpr Vector4(T a, T b, const Vector2<T>& v) :
            x(a), y(b), z(v.x), w(v.y)
        {}

        constexpr Vector4(const Vector2<T>& a, const Vector2<T>& b) :
            x(a.x), y(a.y), z(b.x), w(b.y)
        {}

        constexpr Vector4(const Vector3<T>& v, T s) :
            x(v.x), y(v.y), z(v.z), w(s)
        {}

        constexpr Vector4(T s, const Vector3<T>& v) :
            x(s), y(v.x), z(v.y), w(v.z)
        {}

        constexpr Vector4(const Vector3<T>& v) :
            x(v.x), y(v.y), z(v.z), w(0)
        {}

        constexpr ~Vector4()
        {
            x = y = z = w = 0;
        }

        constexpr Vector4(const Vector4& v)
        {
            *this = v;
        }

        constexpr Vector4(Vector4&& v)
        {
            *this = std::move(v);
        }

        constexpr Vector4& operator= (const Vector4& v)
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

        constexpr Vector4& operator= (Vector4&& v)
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

        constexpr T& operator[] (size_t i)
        {
            ASSERTION(i < 4);
            return (&x)[i];
        }

        constexpr const T& operator[] (size_t i) const
        {
            ASSERTION(i < 4);
            return (&x)[i];
        }

        constexpr Vector4 operator-()
        {
            return Vector4(-x, -y, -z, -w);
        }

        constexpr const Vector4 operator-() const
        {
            return Vector4(-x, -y, -z, -w);
        }

        constexpr Vector4& operator+= (const Vector4& v)
        {
            x += v.x;
            y += v.y;
            z += v.z;
            w += v.w;
            return *this;
        }

        constexpr Vector4& operator+= (T s)
        {
            x += s;
            y += s;
            z += s;
            w += s;
            return *this;
        }

        constexpr Vector4& operator-= (const Vector4& v)
        {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            w -= v.w;
            return *this;
        }

        constexpr Vector4& operator-= (T s)
        {
            x -= s;
            y -= s;
            z -= s;
            w -= s;
            return *this;
        }

        constexpr Vector4& operator*= (const Vector4& v)
        {
            x *= v.x;
            y *= v.y;
            z *= v.z;
            w *= v.w;
            return *this;
        }

        constexpr Vector4& operator*= (T s)
        {
            x *= s;
            y *= s;
            z *= s;
            w *= s;
            return *this;
        }

        constexpr Vector4& operator/= (const Vector4& v)
        {
            x /= v.x;
            y /= v.y;
            z /= v.z;
            w /= v.w;
            return *this;
        }

        constexpr Vector4& operator/= (T s)
        {
            x /= s;
            y /= s;
            z /= s;
            w /= s;
            return *this;
        }
    };

    template <is_arithmetic T>
    constexpr Vector4<T> operator+ (const Vector4<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator+ (const Vector4<T>& l, const Vector3<T>& r)
    {
        return Vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator+ (const Vector3<T>& l, const Vector4<T>& r)
    {
        return r + l;
        //return Vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator+ (const Vector4<T>& l, const Vector2<T>& r)
    {
        return Vector4<T>(l.x + r.x, l.y + r.y, l.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator+ (const Vector2<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x + r.x, l.y + r.y, r.z, r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator+ (const Vector4<T>& v, T s)
    {
        return Vector4<T>(v.x + s, v.y + s, v.z + s, v.w + s);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator+ (T s, const Vector4<T>& v)
    {
        return v + s;
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator- (const Vector4<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator- (const Vector4<T>& l, const Vector3<T>& r)
    {
        return Vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator- (const Vector3<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator- (const Vector4<T>& l, const Vector2<T>& r)
    {
        return Vector4<T>(l.x - r.x, l.y - r.y, l.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator- (const Vector2<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x - r.x, l.y - r.y, r.z, r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator- (const Vector4<T>& v, T s)
    {
        return Vector4<T>(v.x - s, v.y - s, v.z - s, v.w - s);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator- (T s, const Vector4<T>& v)
    {
        return Vector4<T>(s - v.x, s - v.y, s - v.z, s - v.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator* (const Vector4<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator* (const Vector4<T>& l, const Vector3<T>& r)
    {
        return Vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator* (const Vector3<T>& l, const Vector4<T>& r)
    {
        return r * l;
        //return Vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator* (const Vector4<T>& l, const Vector2<T>& r)
    {
        return Vector4<T>(l.x * r.x, l.y * r.y, l.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator* (const Vector2<T>& l, const Vector4<T>& r)
    {
        return r * l;
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator* (const Vector4<T>& v, T s)
    {
        return Vector4<T>(v.x * s, v.y * s, v.z * s, v.w * s);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator* (T s, const Vector4<T>& v)
    {
        return v * s;
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator/ (const Vector4<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator/ (const Vector4<T>& l, const Vector3<T>& r)
    {
        return Vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator/ (const Vector3<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator/ (const Vector4<T>& l, const Vector2<T>& r)
    {
        return Vector4<T>(l.x / r.x, l.y / r.y, l.z, l.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator/ (const Vector2<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(l.x / r.x, l.y / r.y, r.z, r.w);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator/ (const Vector4<T>& v, T s)
    {
        return Vector4<T>(v.x / s, v.y / s, v.z / s, v.w / s);
    }

    template <is_arithmetic T>
    constexpr Vector4<T> operator/ (T s, const Vector4<T>& v)
    {
        return Vector4<T>(s / v.x, s / v.y, s / v.z, s / v.w);
    }

    template <is_arithmetic T>
    constexpr bool operator== (const Vector4<T>& l, const Vector4<T>& r)
    {
        return (l.x == r.x) && (l.y == r.y) && (l.z == r.z) && (l.w == r.w);
    }

    template <is_arithmetic T>
    constexpr bool operator!= (const Vector4<T>& l, const Vector4<T>& r)
    {
        return (l.x != r.x) || (l.y != r.y) || (l.z != r.z) || (l.w != r.w);
    }

    template <is_arithmetic T>
    T length(const Vector4<T>& v)
    {
        return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }

    template <is_arithmetic T>
    constexpr void normalize(Vector4<T>& v)
    {
        const T len = length(v);
        v /= len;
    }

    template <is_arithmetic T>
    constexpr T dot(const Vector4<T>& v)
    {
        return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    }

    template <is_arithmetic T>
    constexpr T dot(const Vector4<T>& l, const Vector4<T>& r)
    {
        return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
    }

    template <is_arithmetic T>
    constexpr Vector4<T> cross(const Vector4<T>& l, const Vector4<T>& r)
    {
        return Vector4<T>(
            l.y * r.z - l.z * r.y,
            l.z * r.x - l.x * r.z,
            l.x * r.y - l.y * r.x,
            1
        );
    }

    template <is_arithmetic T>
    constexpr Vector4<T> normalized(const Vector4<T>& v)
    {
        const T len = length(v);
        return v / len;
    }

    template <is_arithmetic T>
    constexpr Vector2<T> reflect(const Vector2<T>& i, const Vector2<T>& n)
    {
        return i - 2 * (dot(i, n) * n);
    }

    template <is_arithmetic T>
    constexpr Vector3<T> reflect(const Vector3<T>& i, const Vector3<T>& n)
    {
        return i - 2 * (dot(i, n) * n);;
    }

    template <is_arithmetic T>
    constexpr Vector4<T> reflect(const Vector4<T>& i, const Vector4<T>& n)
    {
        return i - 2 * (dot(i, n) * n);
    }

    using vec2 = Vector2<float32>;
    using vec3 = Vector3<float32>;
    using vec4 = Vector4<float32>;

    using dvec2 = Vector2<float64>;
    using dvec3 = Vector3<float64>;
    using dvec4 = Vector4<float64>;

    using ivec2 = Vector2<int32>;
    using ivec3 = Vector3<int32>;
    using ivec4 = Vector4<int32>;

    using uvec2 = Vector2<uint32>;
    using uvec3 = Vector3<uint32>;
    using uvec4 = Vector4<uint32>;
}

#endif // !MATH_LIBRARY_VECTOR_H
