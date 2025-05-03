#pragma once
#ifndef MATH_LIBRARY_MATRIX_H
#define MATH_LIBRARY_MATRIX_H

#include "vector.h"

namespace math
{

template <is_arithmetic T>
struct matrix3x3
{
    T m00, m01, m02;
    T m10, m11, m12;
    T m20, m21, m22;

    constexpr matrix3x3() :
        m00(0), m01(0), m02(0),
        m10(0), m11(0), m12(0),
        m20(0), m21(0), m22(0)
    {}

    constexpr ~matrix3x3()
    {
        m00 = m01 = m02 = 0;
        m10 = m11 = m12 = 0;
        m20 = m21 = m22 = 0;
    }

    constexpr matrix3x3(T v) :
        matrix3x3()
    {
        m00 = m11 = m22 = v;
    }

    constexpr matrix3x3(
        T x0, T y0, T z0,
        T x1, T y1, T z1,
        T x2, T y2, T z2
    ) :
        m00(x0), m01(y0), m02(z0),
        m10(x1), m11(y1), m12(z1),
        m20(x2), m21(y2), m22(z2)
    {}

    constexpr matrix3x3(vector3<T> const& a, vector3<T> const& b, vector3<T> const& c) :
        m00(a.x), m01(a.y), m02(a.z),
        m10(b.x), m11(b.y), m12(b.z),
        m20(c.x), m21(c.y), m22(c.z)
    {}

    constexpr matrix3x3(matrix3x3 const& m)
    {
        *this = m;
    }

    constexpr matrix3x3(matrix3x3&& m)
    {
        *this = std::move(m);
    }

    constexpr T operator[] (size_t i)
    {
        return (&m00)[i];
    }

    constexpr const T operator[] (size_t i) const
    {
        return (&m00)[i];;
    }

    constexpr matrix3x3& operator=(matrix3x3 const& m)
    {
        if (this != &m)
        {
            m00 = m.m00;
            m01 = m.m01;
            m02 = m.m02;
            m10 = m.m10;
            m11 = m.m11;
            m12 = m.m12;
            m20 = m.m20;
            m21 = m.m21;
            m22 = m.m22;
        }
        return *this;
    }

    constexpr matrix3x3& operator=(matrix3x3&& m)
    {
        if (this != &m)
        {
            m00 = m.m00;
            m01 = m.m01;
            m02 = m.m02;
            m10 = m.m10;
            m11 = m.m11;
            m12 = m.m12;
            m20 = m.m20;
            m21 = m.m21;
            m22 = m.m22;
            new (&m) matrix3x3();
        }
        return *this;
    }

    constexpr matrix3x3& operator+= (matrix3x3 const& m)
    {
        *this = *this + m;
        return *this;
    }

    constexpr matrix3x3& operator+= (T s)
    {
        *this = *this + s;
        return *this;
    }

    constexpr matrix3x3& operator-= (matrix3x3 const& m)
    {
        *this = *this - m;
        return *this;
    }

    constexpr matrix3x3& operator-= (T s)
    {
        *this = *this - s;
        return *this;
    }

    constexpr matrix3x3& operator*= (matrix3x3 const& m)
    {
        *this = *this * m;
        return *this;
    }

    constexpr matrix3x3& operator*= (T s)
    {
        *this = *this * s;
        return *this;
    }

    static constexpr vector3<T> get_x_vector(matrix3x3 const& m)
    {
        return vector3<T>(m.m00, m.m01, m.m02);
    }

    static constexpr vector3<T> get_y_vector(matrix3x3 const& m)
    {
        return vector3<T>(m.m10, m.m11, m.m12);
    }

    static constexpr vector3<T> get_z_vector(matrix3x3 const& m)

    {
        return vector3<T>(m.m20, m.m21, m.m22);
    }
};

template <is_arithmetic T>
constexpr matrix3x3<T> operator+ (matrix3x3<T> const& l, matrix3x3<T> const& r)
{
    matrix3x3<T> res;
    res.m00 = l.m00 + r.m00;
    res.m01 = l.m01 + r.m01;
    res.m02 = l.m02 + r.m02;
    res.m10 = l.m10 + r.m10;
    res.m11 = l.m11 + r.m11;
    res.m12 = l.m12 + r.m12;
    res.m20 = l.m20 + r.m20;
    res.m21 = l.m21 + r.m21;
    res.m22 = l.m22 + r.m22;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator+ (matrix3x3<T> const& m, T s)
{
    matrix3x3<T> res;
    res.m00 = m.m00 + s;
    res.m01 = m.m01 + s;
    res.m02 = m.m02 + s;
    res.m10 = m.m10 + s;
    res.m11 = m.m11 + s;
    res.m12 = m.m12 + s;
    res.m20 = m.m20 + s;
    res.m21 = m.m21 + s;
    res.m22 = m.m22 + s;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator+ (T s, matrix3x3<T> const& m)
{
    return m + s;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator- (matrix3x3<T> const& l, matrix3x3<T> const& r)
{
    matrix3x3<T> res;
    res.m00 = l.m00 - r.m00;
    res.m01 = l.m01 - r.m01;
    res.m02 = l.m02 - r.m02;
    res.m10 = l.m10 - r.m10;
    res.m11 = l.m11 - r.m11;
    res.m12 = l.m12 - r.m12;
    res.m20 = l.m20 - r.m20;
    res.m21 = l.m21 - r.m21;
    res.m22 = l.m22 - r.m22;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator- (matrix3x3<T> const& m, T s)
{
    matrix3x3<T> res;
    res.m00 = m.m00 - s;
    res.m01 = m.m01 - s;
    res.m02 = m.m02 - s;
    res.m10 = m.m10 - s;
    res.m11 = m.m11 - s;
    res.m12 = m.m12 - s;
    res.m20 = m.m20 - s;
    res.m21 = m.m21 - s;
    res.m22 = m.m22 - s;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator- (T s, matrix3x3<T> const& m)
{
    matrix3x3<T> res;
    res.m00 = s - m.m00;
    res.m01 = s - m.m01;
    res.m02 = s - m.m02;
    res.m10 = s - m.m10;
    res.m11 = s - m.m11;
    res.m12 = s - m.m12;
    res.m20 = s - m.m20;
    res.m21 = s - m.m21;
    res.m22 = s - m.m22;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator* (matrix3x3<T> const& l, matrix3x3<T> const& r)
{
    matrix3x3<T> res;
    res.m00 = r.m00 * l.m00 + r.m01 * l.m10 + r.m02 * l.m20;
    res.m01 = r.m00 * l.m01 + r.m01 * l.m11 + r.m02 * l.m21;
    res.m02 = r.m00 * l.m02 + r.m01 * l.m12 + r.m02 * l.m22;
    res.m10 = r.m10 * l.m00 + r.m11 * l.m10 + r.m12 * l.m20;
    res.m11 = r.m10 * l.m01 + r.m11 * l.m11 + r.m12 * l.m21;
    res.m12 = r.m10 * l.m02 + r.m11 * l.m12 + r.m12 * l.m22;
    res.m20 = r.m20 * l.m00 + r.m21 * l.m10 + r.m22 * l.m20;
    res.m21 = r.m20 * l.m01 + r.m21 * l.m11 + r.m22 * l.m21;
    res.m22 = r.m20 * l.m02 + r.m21 * l.m12 + r.m22 * l.m22;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator* (matrix3x3<T> const& m, T s)
{
    matrix3x3<T> res;
    res.m00 = m.m00 * s;
    res.m01 = m.m01 * s;
    res.m02 = m.m02 * s;
    res.m10 = m.m10 * s;
    res.m11 = m.m11 * s;
    res.m12 = m.m12 * s;
    res.m20 = m.m20 * s;
    res.m21 = m.m21 * s;
    res.m22 = m.m22 * s;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> operator* (T s, matrix3x3<T> const& m)
{
    return m * s;
}

template <is_arithmetic T>
constexpr vector3<T> operator* (matrix3x3<T> const& m, vector3<T> const& v)
{
    vector3<T> res;
    res.x = v.x * m.m00 + v.y * m.m10 + v.z * m.m20;
    res.y = v.x * m.m01 + v.y * m.m11 + v.z * m.m21;
    res.z = v.x * m.m02 + v.y * m.m12 + v.z * m.m22;
    return res;
}

template <is_arithmetic T>
constexpr vector3<T> operator* (vector3<T> const& v, matrix3x3<T> const& m)
{
    const matrix3x3<T> transposed = transpose(m);
    return transposed * v;
}

template <is_arithmetic T>
constexpr T determinant(matrix3x3<T> const& m)
{
    T a = m.m00 * ((m.m11 * m.m22) - (m.m12 * m.m21));
    T b = m.m10 * ((m.m01 * m.m22) - (m.m02 * m.m21));
    T c = m.m20 * ((m.m01 * m.m12) - (m.m02 * m.m11));
    return a - b + c;
}

template <is_arithmetic T>
constexpr matrix3x3<T> transpose(matrix3x3<T> const& m)
{
    matrix3x3<T> res;
    res.m00 = m.m00;
    res.m01 = m.m10;
    res.m02 = m.m20;
    res.m10 = m.m01;
    res.m11 = m.m11;
    res.m12 = m.m21;
    res.m20 = m.m02;
    res.m21 = m.m12;
    res.m22 = m.m22;
    return res;
}

template <is_arithmetic T>
constexpr matrix3x3<T> inverse(matrix3x3<T> const& m)
{
    matrix3x3<T> res;
    T d = 1 / determinant(m);
    res.m00 = +(m.m11 * m.m22 - m.m12 * m.m21) * d;
    res.m01 = -(m.m01 * m.m22 - m.m02 * m.m21) * d;
    res.m02 = +(m.m01 * m.m12 - m.m02 * m.m11) * d;
    res.m10 = -(m.m10 * m.m22 - m.m12 * m.m20) * d;
    res.m11 = +(m.m00 * m.m22 - m.m20 * m.m02) * d;
    res.m12 = -(m.m00 * m.m12 - m.m02 * m.m10) * d;
    res.m20 = +(m.m21 * m.m10 - m.m11 * m.m20) * d;
    res.m21 = -(m.m21 * m.m00 - m.m02 * m.m20) * d;
    res.m22 = +(m.m11 * m.m00 - m.m01 * m.m10) * d;
    return res;
}

template <is_arithmetic T>
constexpr void transposed(matrix3x3<T>& m)
{
    m = transpose(m);
}

template <is_arithmetic T>
constexpr void inversed(matrix3x3<T>& m)
{
    m = inverse(m);
}

template <is_arithmetic T>
struct matrix4x4
{
    T m00, m01, m02, m03;
    T m10, m11, m12, m13;
    T m20, m21, m22, m23;
    T m30, m31, m32, m33;

    constexpr matrix4x4() :
        m00(0), m01(0), m02(0), m03(0),
        m10(0), m11(0), m12(0), m13(0),
        m20(0), m21(0), m22(0), m23(0),
        m30(0), m31(0), m32(0), m33(0)
    {}

    constexpr ~matrix4x4()
    {
        m00 = m01 = m02 = m03 = 0;
        m10 = m11 = m12 = m13 = 0;
        m20 = m21 = m22 = m23 = 0;
        m30 = m31 = m32 = m33 = 0;
    }

    constexpr matrix4x4(T v) :
        matrix4x4()
    {
        m00 = m11 = m22 = m33 = v;
    }

    constexpr matrix4x4(T x0, T y0, T z0, T w0,
                        T x1, T y1, T z1, T w1,
                        T x2, T y2, T z2, T w2,
                        T x3, T y3, T z3, T w3) :
        m00(x0), m01(y0), m02(z0), m03(w0),
        m10(x1), m11(y1), m12(z1), m13(w1),
        m20(x2), m21(y2), m22(z2), m23(w2),
        m30(x3), m31(y3), m32(z3), m33(w3)
    {}

    constexpr matrix4x4(vector4<T> const& a, vector4<T> const& b, vector4<T> const& c, vector4<T> const& d) :
        m00(a.x), m01(a.y), m02(a.z), m03(a.w),
        m10(b.x), m11(b.y), m12(b.z), m13(b.w),
        m20(c.x), m21(c.y), m22(c.z), m23(c.w),
        m30(d.x), m31(d.y), m32(d.z), m33(d.w)
    {}

    constexpr matrix4x4(matrix3x3<T> const& m) :
        m00(m.m00), m01(m.m01), m02(m.m02), m03(0),
        m10(m.m10), m11(m.m11), m12(m.m12), m13(0),
        m20(m.m20), m21(m.m21), m22(m.m22), m23(0),
        m30(0), m31(0), m32(0), m33(1)
    {}

    constexpr matrix4x4(matrix4x4 const& m)
    {
        *this = m;
    }

    constexpr matrix4x4(matrix4x4&& m)
    {
        *this = std::move(m);
    }

    constexpr T operator[] (size_t i)
    {
        return (&m00)[i];
    }

    constexpr const T operator[] (size_t i) const
    {
        return (&m00)[i];;
    }

    constexpr matrix4x4& operator=(matrix4x4 const& m)
    {
        if (this != &m)
        {
            m00 = m.m00;
            m01 = m.m01;
            m02 = m.m02;
            m03 = m.m03;
            m10 = m.m10;
            m11 = m.m11;
            m12 = m.m12;
            m13 = m.m13;
            m20 = m.m20;
            m21 = m.m21;
            m22 = m.m22;
            m23 = m.m23;
            m30 = m.m30;
            m31 = m.m31;
            m32 = m.m32;
            m33 = m.m33;
        }
        return *this;
    }

    constexpr matrix4x4& operator=(matrix4x4&& m)
    {
        if (this != &m)
        {
            m00 = m.m00;
            m01 = m.m01;
            m02 = m.m02;
            m03 = m.m03;
            m10 = m.m10;
            m11 = m.m11;
            m12 = m.m12;
            m13 = m.m13;
            m20 = m.m20;
            m21 = m.m21;
            m22 = m.m22;
            m23 = m.m23;
            m30 = m.m30;
            m31 = m.m31;
            m32 = m.m32;
            m33 = m.m33;
            new (&m) matrix4x4();
        }
        return *this;
    }

    constexpr matrix4x4& operator+= (matrix4x4 const& m)
    {
        *this = *this + m;
        return *this;
    }

    constexpr matrix4x4& operator+= (T s)
    {
        *this = *this + s;
        return *this;
    }

    constexpr matrix4x4& operator-= (matrix4x4 const& m)
    {
        *this = *this - m;
        return *this;
    }

    constexpr matrix4x4& operator-= (T s)
    {
        *this = *this - s;
        return *this;
    }

    constexpr matrix4x4& operator*= (matrix4x4 const& m)
    {
        *this = *this * m;
        return *this;
    }

    constexpr matrix4x4& operator*= (T s)
    {
        *this = *this * s;
        return *this;
    }

    static constexpr vector4<T> get_x_vector(matrix4x4 const& m)
    {
        return vector4<T>(m.m00, m.m01, m.m02, m.m03);
    }

    static constexpr vector4<T> get_y_vector(matrix4x4 const& m)
    {
        return vector4<T>(m.m10, m.m11, m.m12, m.m13);
    }

    static constexpr vector4<T> get_z_vector(matrix4x4 const& m)
    {
        return vector4<T>(m.m20, m.m21, m.m22, m.m23);
    }

    static constexpr vector4<T> get_w_vector(matrix4x4 const& m)
    {
        return vector4<T>(m.m30, m.m31, m.m32, m.m33);
    }

};

template <is_arithmetic T>
constexpr matrix4x4<T> operator+ (matrix4x4<T> const& l, matrix4x4<T> const& r)
{
    matrix4x4<T> res;
    res.m00 = l.m00 + r.m00;
    res.m01 = l.m01 + r.m01;
    res.m02 = l.m02 + r.m02;
    res.m03 = l.m03 + r.m03;
    res.m10 = l.m10 + r.m10;
    res.m11 = l.m11 + r.m11;
    res.m12 = l.m12 + r.m12;
    res.m13 = l.m13 + r.m13;
    res.m20 = l.m20 + r.m20;
    res.m21 = l.m21 + r.m21;
    res.m22 = l.m22 + r.m22;
    res.m23 = l.m23 + r.m23;
    res.m30 = l.m30 + r.m30;
    res.m31 = l.m31 + r.m31;
    res.m32 = l.m32 + r.m32;
    res.m33 = l.m33 + r.m33;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator+ (matrix4x4<T> const& m, T s)
{
    matrix4x4<T> res;
    res.m00 = m.m00 + s;
    res.m01 = m.m01 + s;
    res.m02 = m.m02 + s;
    res.m03 = m.m03 + s;
    res.m10 = m.m10 + s;
    res.m11 = m.m11 + s;
    res.m12 = m.m12 + s;
    res.m13 = m.m13 + s;
    res.m20 = m.m20 + s;
    res.m21 = m.m21 + s;
    res.m22 = m.m22 + s;
    res.m23 = m.m23 + s;
    res.m30 = m.m30 + s;
    res.m31 = m.m31 + s;
    res.m32 = m.m32 + s;
    res.m33 = m.m33 + s;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator+ (T s, matrix4x4<T> const& m)
{
    return m + s;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator- (matrix4x4<T> const& l, matrix4x4<T> const& r)
{
    matrix4x4<T> res;
    res.m00 = l.m00 - r.m00;
    res.m01 = l.m01 - r.m01;
    res.m02 = l.m02 - r.m02;
    res.m03 = l.m03 - r.m03;
    res.m10 = l.m10 - r.m10;
    res.m11 = l.m11 - r.m11;
    res.m12 = l.m12 - r.m12;
    res.m13 = l.m13 - r.m13;
    res.m20 = l.m20 - r.m20;
    res.m21 = l.m21 - r.m21;
    res.m22 = l.m22 - r.m22;
    res.m23 = l.m23 - r.m23;
    res.m30 = l.m30 - r.m30;
    res.m31 = l.m31 - r.m31;
    res.m32 = l.m32 - r.m32;
    res.m33 = l.m33 - r.m33;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator- (matrix4x4<T> const& m, T s)
{
    matrix4x4<T> res;
    res.m00 = m.m00 - s;
    res.m01 = m.m01 - s;
    res.m02 = m.m02 - s;
    res.m03 = m.m03 - s;
    res.m10 = m.m10 - s;
    res.m11 = m.m11 - s;
    res.m12 = m.m12 - s;
    res.m13 = m.m13 - s;
    res.m20 = m.m20 - s;
    res.m21 = m.m21 - s;
    res.m22 = m.m22 - s;
    res.m23 = m.m23 - s;
    res.m30 = m.m30 - s;
    res.m31 = m.m31 - s;
    res.m32 = m.m32 - s;
    res.m33 = m.m33 - s;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator- (T s, matrix4x4<T> const& m)
{
    matrix4x4<T> res;
    res.m00 = s - m.m00;
    res.m01 = s - m.m01;
    res.m02 = s - m.m02;
    res.m03 = s - m.m03;
    res.m10 = s - m.m10;
    res.m11 = s - m.m11;
    res.m12 = s - m.m12;
    res.m13 = s - m.m13;
    res.m20 = s - m.m20;
    res.m21 = s - m.m21;
    res.m22 = s - m.m22;
    res.m23 = s - m.m23;
    res.m30 = s - m.m30;
    res.m31 = s - m.m31;
    res.m32 = s - m.m32;
    res.m33 = s - m.m33;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator* (matrix4x4<T> const& l, matrix4x4<T> const& r)
{
    matrix4x4<T> res;
    res.m00 = r.m00 * l.m00 + r.m01 * l.m10 + r.m02 * l.m20 + r.m03 * l.m30;
    res.m01 = r.m00 * l.m01 + r.m01 * l.m11 + r.m02 * l.m21 + r.m03 * l.m31;
    res.m02 = r.m00 * l.m02 + r.m01 * l.m12 + r.m02 * l.m22 + r.m03 * l.m32;
    res.m03 = r.m00 * l.m03 + r.m01 * l.m13 + r.m02 * l.m23 + r.m03 * l.m33;
    res.m10 = r.m10 * l.m00 + r.m11 * l.m10 + r.m12 * l.m20 + r.m13 * l.m30;
    res.m11 = r.m10 * l.m01 + r.m11 * l.m11 + r.m12 * l.m21 + r.m13 * l.m31;
    res.m12 = r.m10 * l.m02 + r.m11 * l.m12 + r.m12 * l.m22 + r.m13 * l.m32;
    res.m13 = r.m10 * l.m03 + r.m11 * l.m13 + r.m12 * l.m23 + r.m13 * l.m33;
    res.m20 = r.m20 * l.m00 + r.m21 * l.m10 + r.m22 * l.m20 + r.m23 * l.m30;
    res.m21 = r.m20 * l.m01 + r.m21 * l.m11 + r.m22 * l.m21 + r.m23 * l.m31;
    res.m22 = r.m20 * l.m02 + r.m21 * l.m12 + r.m22 * l.m22 + r.m23 * l.m32;
    res.m23 = r.m20 * l.m03 + r.m21 * l.m13 + r.m22 * l.m23 + r.m23 * l.m33;
    res.m30 = r.m30 * l.m00 + r.m31 * l.m10 + r.m32 * l.m20 + r.m33 * l.m30;
    res.m31 = r.m30 * l.m01 + r.m31 * l.m11 + r.m32 * l.m21 + r.m33 * l.m31;
    res.m32 = r.m30 * l.m02 + r.m31 * l.m12 + r.m32 * l.m22 + r.m33 * l.m32;
    res.m33 = r.m30 * l.m03 + r.m31 * l.m13 + r.m32 * l.m23 + r.m33 * l.m33;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator* (matrix4x4<T> const& m, T s)
{
    matrix4x4<T> res;
    res.m00 = m.m00 * s;
    res.m01 = m.m01 * s;
    res.m02 = m.m02 * s;
    res.m03 = m.m03 * s;
    res.m10 = m.m10 * s;
    res.m11 = m.m11 * s;
    res.m12 = m.m12 * s;
    res.m13 = m.m13 * s;
    res.m20 = m.m20 * s;
    res.m21 = m.m21 * s;
    res.m22 = m.m22 * s;
    res.m23 = m.m23 * s;
    res.m30 = m.m30 * s;
    res.m31 = m.m31 * s;
    res.m32 = m.m32 * s;
    res.m33 = m.m33 * s;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> operator* (T s, matrix4x4<T> const& m)
{
    return m * s;
}

template <is_arithmetic T>
constexpr vector4<T> operator* (matrix4x4<T> const& m, vector4<T> const& v)
{
    vector4<T> res;
    res.x = v.x * m.m00 + v.y * m.m10 + v.z * m.m20 + v.w * m.m30;
    res.y = v.x * m.m01 + v.y * m.m11 + v.z * m.m21 + v.w * m.m31;
    res.z = v.x * m.m02 + v.y * m.m12 + v.z * m.m22 + v.w * m.m32;
    res.w = v.x * m.m03 + v.y * m.m13 + v.z * m.m23 + v.w * m.m33;
    return res;
}

template <is_arithmetic T>
constexpr vector4<T> operator* (vector4<T> const& v, matrix4x4<T> const& m)
{
    const matrix4x4<T> transposed = transpose(m);
    return transposed * v;
}

template <is_arithmetic T>
constexpr vector3<T> operator* (matrix4x4<T> const& m, vector3<T> const& v)
{
    vector3<T> res;
    res.x = v.x * m.m00 + v.y * m.m10 + v.z * m.m20;
    res.y = v.x * m.m01 + v.y * m.m11 + v.z * m.m21;
    res.z = v.x * m.m02 + v.y * m.m12 + v.z * m.m22;
    return res;
}

template <is_arithmetic T>
constexpr vector3<T> operator* (vector3<T> const& v, matrix4x4<T> const& m)
{
    const matrix4x4<T> transposed = transpose(m);
    return m * v;
}

template <is_arithmetic T>
constexpr T determinant(matrix4x4<T> const& m)
{
    T sf00 = m.m22 * m.m33 - m.m32 * m.m23;
    T sf01 = m.m21 * m.m33 - m.m31 * m.m23;
    T sf02 = m.m21 * m.m32 - m.m31 * m.m22;
    T sf03 = m.m20 * m.m33 - m.m30 * m.m23;
    T sf04 = m.m20 * m.m32 - m.m30 * m.m22;
    T sf05 = m.m20 * m.m31 - m.m30 * m.m21;

    T dc0 = +m.m11 * sf00 - m.m12 * sf01 + m.m13 * sf02;
    T dc1 = -m.m10 * sf00 - m.m12 * sf03 + m.m13 * sf04;
    T dc2 = +m.m10 * sf01 - m.m11 * sf03 + m.m13 * sf05;
    T dc3 = -m.m10 * sf02 - m.m11 * sf04 + m.m12 * sf05;

    return m.m00 * dc0 + m.m01 * dc1 + m.m02 * dc2 + m.m03 * dc3;
}

template <is_arithmetic T>
constexpr matrix4x4<T> transpose(matrix4x4<T> const& m)
{
    matrix4x4<T> res;
    res.m00 = m.m00;
    res.m01 = m.m10;
    res.m02 = m.m20;
    res.m03 = m.m30;
    res.m10 = m.m01;
    res.m11 = m.m11;
    res.m12 = m.m21;
    res.m13 = m.m31;
    res.m20 = m.m02;
    res.m21 = m.m12;
    res.m22 = m.m22;
    res.m23 = m.m32;
    res.m30 = m.m03;
    res.m31 = m.m13;
    res.m32 = m.m23;
    res.m33 = m.m33;
    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> inverse(matrix4x4<T> const& m)
{
    matrix4x4<T> res;
    T d = 1 / determinant(m);

    res.m00 = d * (m.m12 * m.m23 * m.m31 - m.m13 * m.m22 * m.m31 + m.m13 * m.m21 * m.m32 - m.m11 * m.m23 * m.m32 - m.m12 * m.m21 * m.m33 + m.m11 * m.m22 * m.m33);
    res.m01 = d * (m.m03 * m.m22 * m.m31 - m.m02 * m.m23 * m.m31 - m.m03 * m.m21 * m.m32 + m.m01 * m.m23 * m.m32 + m.m02 * m.m21 * m.m33 - m.m01 * m.m22 * m.m33);
    res.m02 = d * (m.m02 * m.m13 * m.m31 - m.m03 * m.m12 * m.m31 + m.m03 * m.m11 * m.m32 - m.m01 * m.m13 * m.m32 - m.m02 * m.m11 * m.m33 + m.m01 * m.m12 * m.m33);
    res.m03 = d * (m.m03 * m.m12 * m.m21 - m.m02 * m.m13 * m.m21 - m.m03 * m.m11 * m.m22 + m.m01 * m.m13 * m.m22 + m.m02 * m.m11 * m.m23 - m.m01 * m.m12 * m.m23);
    res.m10 = d * (m.m13 * m.m22 * m.m30 - m.m12 * m.m23 * m.m30 - m.m13 * m.m20 * m.m32 + m.m10 * m.m23 * m.m32 + m.m12 * m.m20 * m.m33 - m.m10 * m.m22 * m.m33);
    res.m11 = d * (m.m02 * m.m23 * m.m30 - m.m03 * m.m22 * m.m30 + m.m03 * m.m20 * m.m32 - m.m00 * m.m23 * m.m32 - m.m02 * m.m20 * m.m33 + m.m00 * m.m22 * m.m33);
    res.m12 = d * (m.m03 * m.m12 * m.m30 - m.m02 * m.m13 * m.m30 - m.m03 * m.m10 * m.m32 + m.m00 * m.m13 * m.m32 + m.m02 * m.m10 * m.m33 - m.m00 * m.m12 * m.m33);
    res.m13 = d * (m.m02 * m.m13 * m.m20 - m.m03 * m.m12 * m.m20 + m.m03 * m.m10 * m.m22 - m.m00 * m.m13 * m.m22 - m.m02 * m.m10 * m.m23 + m.m00 * m.m12 * m.m23);
    res.m20 = d * (m.m11 * m.m23 * m.m30 - m.m13 * m.m21 * m.m30 + m.m13 * m.m20 * m.m31 - m.m10 * m.m23 * m.m31 - m.m11 * m.m20 * m.m33 + m.m10 * m.m21 * m.m33);
    res.m21 = d * (m.m03 * m.m21 * m.m30 - m.m01 * m.m23 * m.m30 - m.m03 * m.m20 * m.m31 + m.m00 * m.m23 * m.m31 + m.m01 * m.m20 * m.m33 - m.m00 * m.m21 * m.m33);
    res.m22 = d * (m.m01 * m.m13 * m.m30 - m.m03 * m.m11 * m.m30 + m.m03 * m.m10 * m.m31 - m.m00 * m.m13 * m.m31 - m.m01 * m.m10 * m.m33 + m.m00 * m.m11 * m.m33);
    res.m23 = d * (m.m03 * m.m11 * m.m20 - m.m01 * m.m13 * m.m20 - m.m03 * m.m10 * m.m21 + m.m00 * m.m13 * m.m21 + m.m01 * m.m10 * m.m23 - m.m00 * m.m11 * m.m23);
    res.m30 = d * (m.m12 * m.m21 * m.m30 - m.m11 * m.m22 * m.m30 - m.m12 * m.m20 * m.m31 + m.m10 * m.m22 * m.m31 + m.m11 * m.m20 * m.m32 - m.m10 * m.m21 * m.m32);
    res.m31 = d * (m.m01 * m.m22 * m.m30 - m.m02 * m.m21 * m.m30 + m.m02 * m.m20 * m.m31 - m.m00 * m.m22 * m.m31 - m.m01 * m.m20 * m.m32 + m.m00 * m.m21 * m.m32);
    res.m32 = d * (m.m02 * m.m11 * m.m30 - m.m01 * m.m12 * m.m30 - m.m02 * m.m10 * m.m31 + m.m00 * m.m12 * m.m31 + m.m01 * m.m10 * m.m32 - m.m00 * m.m11 * m.m32);
    res.m33 = d * (m.m01 * m.m12 * m.m20 - m.m02 * m.m11 * m.m20 + m.m02 * m.m10 * m.m21 - m.m00 * m.m12 * m.m21 - m.m01 * m.m10 * m.m22 + m.m00 * m.m11 * m.m22);

    return res;
}

template <is_arithmetic T>
constexpr void transposed(matrix4x4<T>& m)
{
    m = transpose(m);
}

template <is_arithmetic T>
constexpr void inversed(matrix4x4<T>& m)
{
    m = inverse(m);
}

template <is_arithmetic T>
constexpr void translate(matrix4x4<T>& m, vector3<T> const& v)
{
    m = translated(m, v);
}

template <is_arithmetic T>
constexpr void rotate(matrix4x4<T>& m, T a, vector3<T> const& v)
{
    m = rotated(m, a, v);
}

template <is_arithmetic T>
constexpr void scale(matrix4x4<T>& m, vector3<T> const& v)
{
    m = scaled(m, v);
}

template <is_arithmetic T>
constexpr matrix4x4<T> translated(matrix4x4<T> const& m, vector3<T> const& v)
{
    matrix4x4<T> t(1);
    t.m30 = v.x;
    t.m31 = v.y;
    t.m32 = v.z;

    return m * t;
}

template <is_arithmetic T>
constexpr matrix4x4<T> rotated(matrix4x4<T> const& m, T a, vector3<T> const& v)
{
    const T c = static_cast<T>(Cos(a));
    const T s = static_cast<T>(Sin(a));

    vector3<T> axis = normalized(v);
    vector3<T> temp = (1 - c) * axis;

    matrix4x4<T> rot(1);

    rot.m00 = c + temp.x * axis.x;
    rot.m01 = temp.x * axis.y + s * axis.z;
    rot.m02 = temp.x * axis.z - s * axis.y;

    rot.m10 = temp.y * axis.x - s * axis.z;
    rot.m11 = c + temp.y * axis.y;
    rot.m12 = temp.y * axis.z + s * axis.x;

    rot.m20 = temp.z * axis.x + s * axis.y;
    rot.m21 = temp.z * axis.y - s * axis.x;
    rot.m22 = c + temp.z * axis.z;

    return m * rot;
}

template <is_arithmetic T>
constexpr matrix4x4<T> scaled(matrix4x4<T> const& m, vector3<T> const& v)
{
    matrix4x4<T> s(1);
    s.m00 = v.x;
    s.m11 = v.y;
    s.m22 = v.z;
    return m * s;
}

// Vulkan specific implementation of an orthographic projection.
template <is_arithmetic T>
constexpr matrix4x4<T> orthographic_rh(T left, T right, T bottom, T top, T znear, T zfar)
{
    matrix4x4<T> res(1);
    res.m00 = 2 / (right - left);       // x-axis
    res.m11 = -2 / (top - bottom);      // y-axis. Flip this because in vulkan positive y is down.
    res.m22 = 1 / (zfar - znear);       // z-axis
    res.m30 = -(right + left) / (right - left);
    res.m31 = -(top + bottom) / (top - bottom);
    res.m32 = -znear / (zfar - znear);
    //res.m22 = -2 / (zfar - znear);
    //res.m32 = -(zfar + znear) / (zfar - znear);
    return res;
}

// Vulkan specific implementation of a perspective projection.
template <is_arithmetic T>
constexpr matrix4x4<T> perspective_rh(T fov, T aspect, T znear, T zfar)
{
    matrix4x4<T> res(0);
    const T f = 1 / static_cast<T>(tan(fov * 0.5f));

    res.m00 = f / aspect;
    res.m11 = -f;
    res.m23 = -1;
    res.m22 = zfar / (zfar - znear);
    res.m32 = -(zfar * znear) / (zfar - znear);
    //res.m22 = -(znear + zfar) / (zfar - znear);
    //res.m32 = -(2 * zfar * znear) / (zfar - znear);

    return res;
}

template <is_arithmetic T>
constexpr matrix4x4<T> look_at_rh(vector3<T> const& eye, vector3<T> const& center, vector3<T> const& up)
{
    const vector3<T> f(normalized(center - eye));
    const vector3<T> s(normalized(Cross(f, up)));
    const vector3<T> u(cross(s, f));

    matrix4x4<T> res(1);
    res.m00 = s.x;
    res.m10 = s.y;
    res.m20 = s.z;
    res.m01 = u.x;
    res.m11 = u.y;
    res.m21 = u.z;
    res.m02 = -f.x;
    res.m12 = -f.y;
    res.m22 = -f.z;
    res.m30 = -dot(s, eye);
    res.m31 = -dot(u, eye);
    res.m32 = dot(f, eye);

    return res;
}

/**
* Broken! Don't use ... Need to figure out why ...
*/
//template <IsArithmetic T>
//matrix4x4<T> PerspectiveLH(T fov, T aspect, T near, T far)
//{
//  matrix4x4<T> res(0);
//  const T f = 1 / static_cast<T>(Tan(fov * 0.5f));

//  res.m00 = f / aspect;
//  res.m11 = -f;
//  res.m23 = 1;
//  res.m22 = (far + near) / (far - near);
//  res.m32 = -(2 * far * near) / (far - near);

//  return res;
//}

using mat3 = matrix3x3<float32>;
using mat4 = matrix4x4<float32>;

}

#endif // !MATH_LIBRARY_MATRIX_H
