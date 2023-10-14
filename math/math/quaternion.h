#pragma once
#ifndef MATH_LIBRARY_QUATERNION_H
#define MATH_LIBRARY_QUATERNION_H

#include "matrix.h"

namespace math
{
    template <is_arithmetic T>
    struct Quaternion
    {
        T w, x, y, z;

        constexpr Quaternion() :
            w(1), x(0), y(0), z(0)
        {}

        constexpr ~Quaternion()
        {
            x = y = z = 0;
            w = 1;
        }

        constexpr Quaternion(const Vector3<T>& v, T s) :
            w(s), x(v.x), y(v.y), z(v.z)
        {}

        constexpr Quaternion(T s, const Vector3<T>& v) :
            w(s), x(v.x), y(v.y), z(v.z)
        {}

        constexpr Quaternion(T w, T x, T y, T z) :
            w(w), x(x), y(y), z(z)
        {}

        constexpr Quaternion(const Quaternion& q)
        {
            *this = q;
        }

        constexpr Quaternion(Quaternion&& q)
        {
            *this = std::move(q);
        }

        constexpr Quaternion& operator=(const Quaternion& q)
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

        constexpr Quaternion& operator=(Quaternion&& q)
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

        constexpr Quaternion& operator+= (const Quaternion& q)
        {
            *this = *this + q;
            return *this;
        }

        constexpr Quaternion& operator-= (const Quaternion& q)
        {
            *this = *this - q;
            return *this;
        }

        constexpr Quaternion& operator*= (const Quaternion& q)
        {
            *this = *this * q;
            return *this;
        }

        constexpr Quaternion& operator*= (T s)
        {
            *this = *this * s;
            return *this;
        }

        constexpr Quaternion& operator/= (T s)
        {
            *this = *this / s;
            return *this;
        }
    };

    template <typename T>
    constexpr Quaternion<T> operator+ (const Quaternion<T>& l, const Quaternion<T>& r)
    {
        return Quaternion<T>(l.w + r.w, l.x + r.x, l.y + r.y, l.z + r.z);
    }

    template <typename T>
    constexpr Quaternion<T> operator- (const Quaternion<T>& l, const Quaternion<T>& r)
    {
        return Quaternion<T>(l.w - r.w, l.x - r.x, l.y - r.y, l.z - r.z);
    }

    template <typename T>
    constexpr Quaternion<T> operator* (const Quaternion<T>& l, const Quaternion<T>& r)
    {
        Vector3<T> a(l.x, l.y, l.z);
        Vector3<T> b(r.x, r.y, r.z);
        T s0 = l.w;
        T s1 = r.w;

        return Quaternion<T>(s0 * s1 - dot(a, b), (s0 * b) + (s1 * a) + cross(a, b));
        //Quaternion<T> res;
        //res.w = l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z;
        //res.x = l.w * r.x + r.w * l.x + l.y * r.z - l.z * r.y;
        //res.y = l.w * r.y + r.w * l.y + l.z * r.x - r.z * l.x;
        //res.z = l.w * r.z + r.w * l.z + l.w * r.y - r.x * l.y;
        //return res;
    }

    template <typename T>
    constexpr Vector3<T> operator* (const Quaternion<T>& l, const Vector3<T>& r)
    {
        const Vector3<T> qvec(l.x, l.y, l.z);
        const Vector3<T> uv(cross(qvec, r));
        const Vector3<T> uuv(cross(qvec, uv));

        return r + ((uv * l.w) + uuv) * static_cast<T>(2);
    }

    template <typename T>
    constexpr Vector3<T> operator* (const Vector3<T>& l, const Quaternion<T>& r)
    {
        return inversed(r) * l;
    }

    template <typename T>
    constexpr Quaternion<T> operator* (const Quaternion<T>& q, T s)
    {
        return Quaternion<T>(q.w * s, q.x * s, q.y * s, q.z * s);
    }

    template <typename T>
    constexpr Quaternion<T> operator* (T s, const Quaternion<T>& q)
    {
        return q * s;
    }

    template <typename T>
    constexpr Quaternion<T> operator/ (const Quaternion<T>& q, T s)
    {
        return Quaternion<T>(q.w / s, q.x / s, q.y / s, q.z / s);
    }

    template <typename T>
    constexpr bool operator== (const Quaternion<T>& l, const Quaternion<T>& r)
    {
        return (l.w == r.w) && (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
    }

    template <typename T>
    constexpr bool operator!= (const Quaternion<T>& l, const Quaternion<T>& r)
    {
        return (l.w != r.w) || (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
    }

    template <typename T>
    constexpr T length(const Quaternion<T>& q)
    {
        return sqrt(dot(q));
    }

    template <typename T>
    constexpr void normalize(Quaternion<T>& q)
    {
        q = normalized(q);
    }

    template <typename T>
    constexpr void conjugate(Quaternion<T>& q)
    {
        q = conjugated(q);
    }

    template <typename T>
    constexpr void inverse(Quaternion<T>& q)
    {
        q = inversed(q);
    }

    template <typename T>
    constexpr Quaternion<T> normalized(const Quaternion<T>& q)
    {
        T len = length(q);
        T denom = 1 / len;
        if (len <= 0)
        {
            return Quaternion<T>(1, 0, 0, 0);
        }
        return q * denom;
    }

    template <typename T>
    constexpr Quaternion<T> conjugated(const Quaternion<T>& q)
    {
        return Quaternion<T>(q.w, -q.x, -q.y, -q.z);
    }

    template <typename T>
    constexpr Quaternion<T> inversed(const Quaternion<T>& q)
    {
        return conjugated(q) / dot(q);
    }

    template <typename T>
    constexpr Matrix3x3<T> mat3_cast(const Quaternion<T>& q)
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
    constexpr Matrix4x4<T> mat4_cast(const Quaternion<T>& q)
    {
        return Matrix4x4<T>(mat3_cast(q));
    }

    template <typename T>
    constexpr Quaternion<T> angle_axis(T angle, const Vector3<T>& axis)
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
    constexpr T dot(const Quaternion<T>& q)
    {
        return dot(q, q);
    }

    template <typename T>
    constexpr T dot(const Quaternion<T>& a, const Quaternion<T>& b)
    {
        //vec4 tmp(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
        //return (tmp.x + tmp.y) + (tmp.z + tmp.w);
        return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    }

    using quat  = Quaternion<float32>;
    using dquat = Quaternion<float64>;

}

#endif // !MATH_LIBRARY_QUATERNION_H
