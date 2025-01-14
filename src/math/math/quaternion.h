#pragma once
#ifndef MATH_LIBRARY_QUATERNION_H
#define MATH_LIBRARY_QUATERNION_H

#include "matrix.h"

namespace math
{
template <is_arithmetic T>
struct quaternion
{
    T w, x, y, z;

    constexpr quaternion() :
        w(1), x(0), y(0), z(0)
    {}

    constexpr ~quaternion()
    {
        x = y = z = 0;
        w = 1;
    }

    constexpr quaternion(vector3<T> const& v, T s) :
        w(s), x(v.x), y(v.y), z(v.z)
    {}

    constexpr quaternion(T s, vector3<T> const& v) :
        w(s), x(v.x), y(v.y), z(v.z)
    {}

    constexpr quaternion(T w, T x, T y, T z) :
        w(w), x(x), y(y), z(z)
    {}

    constexpr quaternion(quaternion const& q)
    {
        *this = q;
    }

    constexpr quaternion(quaternion&& q)
    {
        *this = std::move(q);
    }

    constexpr quaternion& operator=(quaternion const& q)
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

    constexpr quaternion& operator=(quaternion&& q)
    {
        if (this != &q)
        {
            w = q.w;
            x = q.x;
            y = q.y;
            z = q.z;
            new (&q) quaternion();
        }
        return *this;
    }

    constexpr quaternion& operator+= (quaternion const& q)
    {
        *this = *this + q;
        return *this;
    }

    constexpr quaternion& operator-= (quaternion const& q)
    {
        *this = *this - q;
        return *this;
    }

    constexpr quaternion& operator*= (quaternion const& q)
    {
        *this = *this * q;
        return *this;
    }

    constexpr quaternion& operator*= (T s)
    {
        *this = *this * s;
        return *this;
    }

    constexpr quaternion& operator/= (T s)
    {
        *this = *this / s;
        return *this;
    }
};

template <typename T>
constexpr quaternion<T> operator+ (quaternion<T> const& l, quaternion<T> const& r)
{
    return quaternion<T>(l.w + r.w, l.x + r.x, l.y + r.y, l.z + r.z);
}

template <typename T>
constexpr quaternion<T> operator- (quaternion<T> const& l, quaternion<T> const& r)
{
    return quaternion<T>(l.w - r.w, l.x - r.x, l.y - r.y, l.z - r.z);
}

template <typename T>
constexpr quaternion<T> operator* (quaternion<T> const& l, quaternion<T> const& r)
{
    vector3<T> a(l.x, l.y, l.z);
    vector3<T> b(r.x, r.y, r.z);
    T s0 = l.w;
    T s1 = r.w;

    return quaternion<T>(s0 * s1 - dot(a, b), (s0 * b) + (s1 * a) + cross(a, b));
    //quaternion<T> res;
    //res.w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;
    //res.x = l.w * r.x + r.w * l.x + l.y * r.z - l.z * r.y;
    //res.y = l.w * r.y + r.w * l.y + l.z * r.x - r.z * l.x;
    //res.z = l.w * r.z + r.w * l.z + l.w * r.y - r.x * l.y;
    //return res;
}

template <typename T>
constexpr vector3<T> operator* (quaternion<T> const& l, vector3<T> const& r)
{
    const vector3<T> qvec(l.x, l.y, l.z);
    const vector3<T> uv(cross(qvec, r));
    const vector3<T> uuv(cross(qvec, uv));

    return r + ((uv * l.w) + uuv) * static_cast<T>(2);
}

template <typename T>
constexpr vector3<T> operator* (vector3<T> const& l, quaternion<T> const& r)
{
    return inversed(r) * l;
}

template <typename T>
constexpr quaternion<T> operator* (quaternion<T> const& q, T s)
{
    return quaternion<T>(q.w * s, q.x * s, q.y * s, q.z * s);
}

template <typename T>
constexpr quaternion<T> operator* (T s, quaternion<T> const& q)
{
    return q * s;
}

template <typename T>
constexpr quaternion<T> operator/ (quaternion<T> const& q, T s)
{
    return quaternion<T>(q.w / s, q.x / s, q.y / s, q.z / s);
}

template <typename T>
constexpr bool operator== (quaternion<T> const& l, quaternion<T> const& r)
{
    return (l.w == r.w) && (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
}

template <typename T>
constexpr bool operator!= (quaternion<T> const& l, quaternion<T> const& r)
{
    return (l.w != r.w) || (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
}

template <typename T>
constexpr T length(quaternion<T> const& q)
{
    return sqrt(dot(q));
}

template <typename T>
constexpr void normalize(quaternion<T>& q)
{
    q = normalized(q);
}

template <typename T>
constexpr void conjugate(quaternion<T>& q)
{
    q = conjugated(q);
}

template <typename T>
constexpr void inverse(quaternion<T>& q)
{
    q = inversed(q);
}

template <typename T>
constexpr quaternion<T> normalized(quaternion<T> const& q)
{
    T len = length(q);
    T denom = 1 / len;
    if (len <= 0)
    {
        return quaternion<T>(1, 0, 0, 0);
    }
    return q * denom;
}

template <typename T>
constexpr quaternion<T> conjugated(quaternion<T> const& q)
{
    return quaternion<T>(q.w, -q.x, -q.y, -q.z);
}

template <typename T>
constexpr quaternion<T> inversed(quaternion<T> const& q)
{
    return conjugated(q) / dot(q);
}

template <typename T>
constexpr matrix3x3<T> mat3_cast(quaternion<T> const& q)
{
    matrix3x3<T> res(1);

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
constexpr matrix4x4<T> mat4_cast(quaternion<T> const& q)
{
    return matrix4x4<T>(mat3_cast(q));
}

template <typename T>
constexpr quaternion<T> angle_axis(T angle, vector3<T> const& axis)
{
    quaternion<T> res;
    const T a = angle;
    const T s = sin(a * static_cast<T>(0.5));

    res.w = cos(a * static_cast<T>(0.5));
    res.x = axis.x * s;
    res.y = axis.y * s;
    res.z = axis.z * s;

    return res;
}

template <typename T>
constexpr T dot(quaternion<T> const& q)
{
    return dot(q, q);
}

template <typename T>
constexpr T dot(quaternion<T> const& a, quaternion<T> const& b)
{
    //vec4 tmp(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
    //return (tmp.x + tmp.y) + (tmp.z + tmp.w);
    return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

using quat  = quaternion<float32>;
using dquat = quaternion<float64>;

}

#endif // !MATH_LIBRARY_QUATERNION_H
