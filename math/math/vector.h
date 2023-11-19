#pragma once
#ifndef MATH_LIBRARY_VECTOR_H
#define MATH_LIBRARY_VECTOR_H

#include "common.h"

namespace math
{

template <is_arithmetic T>
struct vector3;

template <is_arithmetic T>
struct vector4;

/**
* Disable warning "nonstandard extension used: nameless struct/union"
*/

template <is_arithmetic T>
struct vector2
{
    T x, y;

    constexpr vector2() :
        x{ 0 }, y{ 0 }
    {}

    constexpr vector2(T val) :
        x{ val }, y{ val }
    {}

    constexpr vector2(T a, T b) :
        x{ a }, y{ b }
    {}

    constexpr vector2(vector3<T> const& v) :
        x{ v.x }, y{ v.y }
    {}

    constexpr vector2(vector4<T> const& v) :
        x{ v.x }, y{ v.y }
    {}

    constexpr ~vector2()
    {
        x = y = 0;
    }

    constexpr vector2(const vector2& vec) { *this = vec; }
    constexpr vector2(vector2&& vec) { *this = std::move(vec); }

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

    constexpr vector2& operator= (const vector2& vec)
    {
        if (this != &vec)
        {
            x = vec.x;
            y = vec.y;
        }
        return *this;
    }

    constexpr vector2& operator= (vector2&& vec)
    {
        if (this != &vec)
        {
            x = vec.x;
            y = vec.y;
            new (&vec) vector2{};
        }
        return *this;
    }

    constexpr vector2 operator-()
    {
        return vector2{ -x, -y };
    }

    constexpr const vector2 operator-() const
    {
        return vector2{ -x, -y };
    }

    constexpr vector2& operator+= (const vector2& v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    constexpr vector2& operator+= (is_arithmetic auto val)
    {
        x += T{ val };
        y += T{ val };
        return *this;
    }

    constexpr vector2& operator-= (const vector2& v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    constexpr vector2& operator-= (is_arithmetic auto val)
    {
        x -= T{ val };
        y -= T{ val };
        return *this;
    }

    constexpr vector2& operator*= (const vector2& v)
    {
        x *= v.x;
        y *= v.y;
        return *this;
    }

    constexpr vector2& operator*= (is_arithmetic auto val)
    {
        x *= T{ val };
        y *= T{ val };
        return *this;
    }

    constexpr vector2& operator/= (const vector2& v)
    {
        x /= v.x;
        y /= v.y;
        return *this;
    }

    constexpr vector2& operator/= (is_arithmetic auto val)
    {
        x /= T{ val };
        y /= T{ val };
        return *this;
    }
};

template <is_arithmetic T>
constexpr vector2<T> operator+ (const vector2<T>& l, const vector2<T>& r)
{
    return vector2<T>{ l.x + r.x, l.y + r.y };
}

template <is_arithmetic T>
constexpr vector2<T> operator+ (const vector2<T>& v, T val)
{
    return vector2<T>{ v.x + val, v.y + val };
}

template <is_arithmetic T>
constexpr vector2<T> operator+ (T val, const vector2<T>& v)
{
    return v + val;
}

template <is_arithmetic T>
constexpr vector2<T> operator- (const vector2<T>& l, const vector2<T>& r)
{
    return vector2<T>{ l.x - r.x, l.y - r.y };
}

template <is_arithmetic T>
constexpr vector2<T> operator- (const vector2<T>& v, T val)
{
    return vector2<T>{ v.x - val, v.y - val };
}

template <is_arithmetic T>
constexpr vector2<T> operator- (T val, const vector2<T>& v)
{
    return vector2<T>{ val - v.x, val - v.y };
}

template <is_arithmetic T>
constexpr vector2<T> operator* (const vector2<T>& l, const vector2<T>& r)
{
    return vector2<T>{ l.x * r.x, l.y * r.y };
}

template <is_arithmetic T>
constexpr vector2<T> operator* (const vector2<T>& v, T val)
{
    return vector2<T>(v.x * val, v.y * val);
}

template <is_arithmetic T>
constexpr vector2<T> operator* (T val, const vector2<T>& v)
{
    return vector2<T>(val * v.x, val * v.y);
}

template <is_arithmetic T>
constexpr vector2<T> operator/ (const vector2<T>& l, const vector2<T>& r)
{
    return vector2<T>(l.x / r.x, l.y / r.y);
}

template <is_arithmetic T>
constexpr vector2<T> operator/ (const vector2<T>& v, T val)
{
    return vector2<T>(v.x / val, v.y / val);
}

template <is_arithmetic T>
constexpr vector2<T> operator/ (T val, const vector2<T>& v)
{
    return vector2<T>(val / v.x, val / v.y);
}

template <is_arithmetic T>
constexpr bool operator== (const vector2<T>& l, const vector2<T>& r)
{
    return (l.x == r.x) && (l.y == r.y);
}

template <is_arithmetic T>
constexpr bool operator!= (const vector2<T>& l, const vector2<T>& r)
{
    return l.x != r.x || l.y != r.y;
}

template <is_arithmetic T>
T length(const vector2<T>& v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

template <is_arithmetic T>
constexpr void normalize(vector2<T>& v)
{
    const T len = length(v);
    v /= len;
}

template <is_arithmetic T>
constexpr T dot(const vector2<T>& v)
{
    return v.x * v.x + v.y * v.y;
}

template <is_arithmetic T>
constexpr T dot(const vector2<T>& l, const vector2<T>& r)
{
    return l.x * r.x + l.y * r.y;
}

template <is_arithmetic T>
constexpr vector2<T> normalized(const vector2<T>& v)
{
    const T len = length(v);
    return v / len;
}

template <is_arithmetic T>
constexpr bool in_bounds(const vector2<T>& p, const vector2<T>& min, const vector2<T>& max)
{
    return (p.x > min.x && p.x < max.x) && (p.y > min.y && p.y < max.y);
}


template <is_arithmetic T>
struct vector3
{
    T x, y, z;

    constexpr vector3() :
        x(0), y(0), z(0)
    {}

    constexpr vector3(T val) :
        x(val), y(val), z(val)
    {}

    constexpr vector3(T a, T b, T c) :
        x(a), y(b), z(c)
    {}

    constexpr vector3(const vector2<T>& v, T val) :
        x(v.x), y(v.y), z(val)
    {}

    constexpr vector3(T val, const vector2<T>& v) :
        x(val), y(v.x), z(v.y)
    {}

    constexpr vector3(const vector2<T>& v) :
        x(v.x), y(v.y), z(0)
    {}

    constexpr vector3(vector4<T> const& v) :
        x(v.x), y(v.y), z(v.z)
    {}

    constexpr vector3(const vector3& v)
    {
        *this = v;
    }

    constexpr vector3(vector3&& v)
    {
        *this = std::move(v);
    }

    constexpr ~vector3()
    {
        x = y = z = 0;
    }

    constexpr vector3& operator= (const vector3& v)
    {
        if (this != &v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
        }
        return *this;
    }

    constexpr vector3& operator= (vector3&& v)
    {
        if (this != &v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            new (&v) vector3();
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

    constexpr vector3 operator-()
    {
        return vector3(-x, -y, -z);
    }

    constexpr const vector3 operator-() const
    {
        return vector3(-x, -y, -z);
    }

    constexpr vector3& operator+= (const vector3& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    constexpr vector3& operator+= (T val)
    {
        x += val;
        y += val;
        z += val;
        return *this;
    }

    constexpr vector3& operator-= (const vector3& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    constexpr vector3& operator-= (T val)
    {
        x -= val;
        y -= val;
        z -= val;
        return *this;
    }

    constexpr vector3& operator*= (const vector3& v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }

    constexpr vector3& operator*= (T val)
    {
        x *= val;
        y *= val;
        z *= val;
        return *this;
    }

    constexpr vector3& operator/= (const vector3& v)
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }

    constexpr vector3& operator/= (T val)
    {
        x /= val;
        y /= val;
        z /= val;
        return *this;
    }
};

template <is_arithmetic T>
constexpr vector3<T> operator+ (vector3<T> const& l, vector3<T> const& r)
{
    return vector3<T>(l.x + r.x, l.y + r.y, l.z + r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator+ (vector3<T> const& l, const vector2<T>& r)
{
    return vector3<T>(l.x + r.x, l.y + r.y, l.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator+ (const vector2<T>& l, vector3<T> const& r)
{
    return vector3<T>(l.x + r.x, l.y + r.y, r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator+ (vector3<T> const& v, T val)
{
    return vector3<T>(v.x + val, v.y + val, v.z + val);
}

template <is_arithmetic T>
constexpr vector3<T> operator+ (T val, vector3<T> const& v)
{
    return v + val;
}

template <is_arithmetic T>
constexpr vector3<T> operator- (vector3<T> const& l, vector3<T> const& r)
{
    return vector3<T>(l.x - r.x, l.y - r.y, l.z - r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator- (vector3<T> const& l, const vector2<T>& r)
{
    return vector3<T>(l.x - r.x, l.y - r.y, l.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator- (const vector2<T>& l, vector3<T> const& r)
{
    return vector3<T>(l.x - r.x, l.y - r.y, r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator- (vector3<T> const& v, T val)
{
    return vector3<T>(v.x - val, v.y - val, v.z - val);
}

template <is_arithmetic T>
constexpr vector3<T> operator- (T val, vector3<T> const& v)
{
    return vector3<T>(val - v.x, val - v.y, val - v.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator* (vector3<T> const& l, vector3<T> const& r)
{
    return vector3<T>(l.x * r.x, l.y * r.y, l.z * r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator* (vector3<T> const& l, const vector2<T>& r)
{
    return vector3<T>(l.x * r.x, l.y * r.y, l.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator* (const vector2<T>& l, vector3<T> const& r)
{
    return vector3<T>(l.x * r.x, l.y * r.y, r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator* (vector3<T> const& v, T val)
{
    return vector3<T>(v.x * val, v.y * val, v.z * val);
}

template <is_arithmetic T>
constexpr vector3<T> operator* (T val, vector3<T> const& v)
{
    return v * val;
}

template <is_arithmetic T>
constexpr vector3<T> operator/ (vector3<T> const& l, vector3<T> const& r)
{
    return vector3<T>(l.x / r.x, l.y / r.y, l.z / r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator/ (vector3<T> const& l, const vector2<T>& r)
{
    return vector3<T>(l.x / r.x, l.y / r.y, l.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator/ (const vector2<T>& l, vector3<T> const& r)
{
    return vector3<T>(l.x / r.x, l.y / r.y, r.z);
}

template <is_arithmetic T>
constexpr vector3<T> operator/ (vector3<T> const& v, T val)
{
    return vector3<T>(v.x / val, v.y / val, v.z / val);
}

template <is_arithmetic T>
constexpr vector3<T> operator/ (T val, vector3<T> const& v)
{
    return vector3<T>(val / v.x, val / v.y, val / v.z);
}

template <is_arithmetic T>
constexpr bool operator== (vector3<T> const& l, vector3<T> const& r)
{
    return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
}

template <is_arithmetic T>
constexpr bool operator!= (vector3<T> const& l, vector3<T> const& r)
{
    return (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
}

template <is_arithmetic T>
T length(vector3<T> const& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

template <is_arithmetic T>
constexpr void normalize(vector3<T>& v)
{
    const T len = length(v);
    v /= len;
}

template <is_arithmetic T>
constexpr T dot(vector3<T> const& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

template <is_arithmetic T>
T dot(vector3<T> const& l, vector3<T> const& r)
{
    return l.x * r.x + l.y * r.y + l.z * r.z;
}

template <is_arithmetic T>
constexpr vector3<T> cross(vector3<T> const& l, vector3<T> const& r)
{
    return vector3<T>(
        l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x
    );
}

template <is_arithmetic T>
constexpr vector3<T> normalized(vector3<T> const& v)
{
    const T len = length(v);
    return v / len;
}


template <is_arithmetic T>
struct vector4
{
    T x, y, z, w;

    constexpr vector4() :
        x(0), y(0), z(0), w(0)
    {}

    constexpr vector4(T val) :
        x(val), y(val), z(val), w(val)
    {}

    constexpr vector4(T a, T b, T c, T d) :
        x(a), y(b), z(c), w(d)
    {}

    constexpr vector4(const vector2<T>& v) :
        x(v.x), y(v.y), z(0), w(0)
    {}

    constexpr vector4(const vector2<T>& v, T a, T b) :
        x(v.x), y(v.y), z(a), w(b)
    {}

    constexpr vector4(T a, T b, const vector2<T>& v) :
        x(a), y(b), z(v.x), w(v.y)
    {}

    constexpr vector4(const vector2<T>& a, const vector2<T>& b) :
        x(a.x), y(a.y), z(b.x), w(b.y)
    {}

    constexpr vector4(vector3<T> const& v, T val) :
        x(v.x), y(v.y), z(v.z), w(val)
    {}

    constexpr vector4(T val, vector3<T> const& v) :
        x(val), y(v.x), z(v.y), w(v.z)
    {}

    constexpr vector4(vector3<T> const& v) :
        x(v.x), y(v.y), z(v.z), w(0)
    {}

    constexpr ~vector4()
    {
        x = y = z = w = 0;
    }

    constexpr vector4(const vector4& v)
    {
        *this = v;
    }

    constexpr vector4(vector4&& v)
    {
        *this = std::move(v);
    }

    constexpr vector4& operator= (const vector4& v)
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

    constexpr vector4& operator= (vector4&& v)
    {
        if (this != &v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            w = v.w;
            new (&v) vector4();
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

    constexpr vector4 operator-()
    {
        return vector4(-x, -y, -z, -w);
    }

    constexpr const vector4 operator-() const
    {
        return vector4(-x, -y, -z, -w);
    }

    constexpr vector4& operator+= (const vector4& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }

    constexpr vector4& operator+= (T val)
    {
        x += val;
        y += val;
        z += val;
        w += val;
        return *this;
    }

    constexpr vector4& operator-= (const vector4& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }

    constexpr vector4& operator-= (T val)
    {
        x -= val;
        y -= val;
        z -= val;
        w -= val;
        return *this;
    }

    constexpr vector4& operator*= (const vector4& v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        w *= v.w;
        return *this;
    }

    constexpr vector4& operator*= (T val)
    {
        x *= val;
        y *= val;
        z *= val;
        w *= val;
        return *this;
    }

    constexpr vector4& operator/= (const vector4& v)
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        w /= v.w;
        return *this;
    }

    constexpr vector4& operator/= (T val)
    {
        x /= val;
        y /= val;
        z /= val;
        w /= val;
        return *this;
    }
};

template <is_arithmetic T>
constexpr vector4<T> operator+ (vector4<T> const& l, vector4<T> const& r)
{
    return vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator+ (vector4<T> const& l, vector3<T> const& r)
{
    return vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator+ (vector3<T> const& l, vector4<T> const& r)
{
    return r + l;
    //return vector4<T>(l.x + r.x, l.y + r.y, l.z + r.z, r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator+ (vector4<T> const& l, const vector2<T>& r)
{
    return vector4<T>(l.x + r.x, l.y + r.y, l.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator+ (const vector2<T>& l, vector4<T> const& r)
{
    return vector4<T>(l.x + r.x, l.y + r.y, r.z, r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator+ (vector4<T> const& v, T val)
{
    return vector4<T>(v.x + val, v.y + val, v.z + val, v.w + val);
}

template <is_arithmetic T>
constexpr vector4<T> operator+ (T val, vector4<T> const& v)
{
    return v + val;
}

template <is_arithmetic T>
constexpr vector4<T> operator- (vector4<T> const& l, vector4<T> const& r)
{
    return vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator- (vector4<T> const& l, vector3<T> const& r)
{
    return vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator- (vector3<T> const& l, vector4<T> const& r)
{
    return vector4<T>(l.x - r.x, l.y - r.y, l.z - r.z, r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator- (vector4<T> const& l, const vector2<T>& r)
{
    return vector4<T>(l.x - r.x, l.y - r.y, l.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator- (const vector2<T>& l, vector4<T> const& r)
{
    return vector4<T>(l.x - r.x, l.y - r.y, r.z, r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator- (vector4<T> const& v, T val)
{
    return vector4<T>(v.x - val, v.y - val, v.z - val, v.w - val);
}

template <is_arithmetic T>
constexpr vector4<T> operator- (T val, vector4<T> const& v)
{
    return vector4<T>(val - v.x, val - v.y, val - v.z, val - v.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator* (vector4<T> const& l, vector4<T> const& r)
{
    return vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator* (vector4<T> const& l, vector3<T> const& r)
{
    return vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator* (vector3<T> const& l, vector4<T> const& r)
{
    return r * l;
    //return vector4<T>(l.x * r.x, l.y * r.y, l.z * r.z, r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator* (vector4<T> const& l, const vector2<T>& r)
{
    return vector4<T>(l.x * r.x, l.y * r.y, l.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator* (const vector2<T>& l, vector4<T> const& r)
{
    return r * l;
}

template <is_arithmetic T>
constexpr vector4<T> operator* (vector4<T> const& v, T val)
{
    return vector4<T>(v.x * val, v.y * val, v.z * val, v.w * val);
}

template <is_arithmetic T>
constexpr vector4<T> operator* (T val, vector4<T> const& v)
{
    return v * val;
}

template <is_arithmetic T>
constexpr vector4<T> operator/ (vector4<T> const& l, vector4<T> const& r)
{
    return vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator/ (vector4<T> const& l, vector3<T> const& r)
{
    return vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator/ (vector3<T> const& l, vector4<T> const& r)
{
    return vector4<T>(l.x / r.x, l.y / r.y, l.z / r.z, r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator/ (vector4<T> const& l, const vector2<T>& r)
{
    return vector4<T>(l.x / r.x, l.y / r.y, l.z, l.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator/ (const vector2<T>& l, vector4<T> const& r)
{
    return vector4<T>(l.x / r.x, l.y / r.y, r.z, r.w);
}

template <is_arithmetic T>
constexpr vector4<T> operator/ (vector4<T> const& v, T val)
{
    return vector4<T>(v.x / val, v.y / val, v.z / val, v.w / val);
}

template <is_arithmetic T>
constexpr vector4<T> operator/ (T val, vector4<T> const& v)
{
    return vector4<T>(val / v.x, val / v.y, val / v.z, val / v.w);
}

template <is_arithmetic T>
constexpr bool operator== (vector4<T> const& l, vector4<T> const& r)
{
    return (l.x == r.x) && (l.y == r.y) && (l.z == r.z) && (l.w == r.w);
}

template <is_arithmetic T>
constexpr bool operator!= (vector4<T> const& l, vector4<T> const& r)
{
    return (l.x != r.x) || (l.y != r.y) || (l.z != r.z) || (l.w != r.w);
}

template <is_arithmetic T>
T length(vector4<T> const& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

template <is_arithmetic T>
constexpr void normalize(vector4<T>& v)
{
    const T len = length(v);
    v /= len;
}

template <is_arithmetic T>
constexpr T dot(vector4<T> const& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

template <is_arithmetic T>
constexpr T dot(vector4<T> const& l, vector4<T> const& r)
{
    return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w;
}

template <is_arithmetic T>
constexpr vector4<T> cross(vector4<T> const& l, vector4<T> const& r)
{
    return vector4<T>(
        l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x,
        1
    );
}

template <is_arithmetic T>
constexpr vector4<T> normalized(vector4<T> const& v)
{
    const T len = length(v);
    return v / len;
}

template <is_arithmetic T>
constexpr vector2<T> reflect(const vector2<T>& i, const vector2<T>& n)
{
    return i - 2 * (dot(i, n) * n);
}

template <is_arithmetic T>
constexpr vector3<T> reflect(vector3<T> const& i, vector3<T> const& n)
{
    return i - 2 * (dot(i, n) * n);;
}

template <is_arithmetic T>
constexpr vector4<T> reflect(vector4<T> const& i, vector4<T> const& n)
{
    return i - 2 * (dot(i, n) * n);
}

using vec2 = vector2<float32>;
using vec3 = vector3<float32>;
using vec4 = vector4<float32>;

using dvec2 = vector2<float64>;
using dvec3 = vector3<float64>;
using dvec4 = vector4<float64>;

using ivec2 = vector2<int32>;
using ivec3 = vector3<int32>;
using ivec4 = vector4<int32>;

using uvec2 = vector2<uint32>;
using uvec3 = vector3<uint32>;
using uvec4 = vector4<uint32>;
}

#endif // !MATH_LIBRARY_VECTOR_H
