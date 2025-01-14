#pragma once
#ifndef MATH_LIBRARY_MATH_H
#define MATH_LIBRARY_MATH_H

#include <cassert>
#include <type_traits>
#include <concepts>
#include <cmath>

#if _DEBUG
#define ASSERTION(expr)	assert(expr)
#else
#define ASSERTION(expr)
#endif

using int8 = char;
using int16 = short;
using int32 = int;
using int64 = long long;

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using float32 = float;
using float64 = double;

namespace math
{
template <typename T>
concept is_arithmetic = std::is_arithmetic_v<T>;

inline auto cos(std::floating_point auto num)
{
    return std::cos(num);
}

inline auto sin(std::floating_point auto num)
{
    return std::sin(num);
}

inline auto tan(std::floating_point auto num)
{
    return std::tan(num);
}

inline auto acos(std::floating_point auto num)
{
    return std::acos(num);
}

inline auto asin(std::floating_point auto num)
{
    return std::asin(num);
}

inline auto atan(std::floating_point auto num)
{
    return std::atan(num);
}

inline auto sqrt(std::floating_point auto num)
{
    return std::sqrt(num);
}

inline constexpr auto sqr(is_arithmetic auto num)
{
    return num * num;
}

inline auto cbrt(std::floating_point auto num)
{
    return std::cbrt(num);
}

inline auto pow(std::floating_point auto num, std::floating_point auto exp)
{
    return std::pow(num, exp);
}

inline auto abs(std::integral auto num)
{
    return std::abs(num);
}

inline auto fabs(std::floating_point auto num)
{
    return std::fabs(num);
}

inline auto mod(std::integral auto num, std::integral auto denom)
{
    return num % denom;
}

inline auto log2(std::floating_point auto num)
{
    return std::log2(num);;
}

inline auto fmod(std::floating_point auto num, std::floating_point auto denom)
{
    return std::fmod(num, denom);
}

inline auto floor(std::floating_point auto num)
{
    return std::floor(num);
}

inline auto ceil(std::floating_point auto num)
{
    return std::ceil(num);
}

inline auto round(std::floating_point auto num)
{
    return std::round(num);
}

template <std::floating_point T>
T pi()
{
    return static_cast<T>(std::atan(T{ 1.f }) * T{ 4.f });
}

template <std::floating_point T>
T half_pi()
{
    return pi<T>() * static_cast<T>(.5f);
}

template <std::floating_point T>
T tau()
{
    return pi<T>() * static_cast<T>(2.0f);
}

template <std::floating_point T>
T two_pi()
{
    return tau<T>();
}

template <std::floating_point T>
T radians(T const deg)
{
    return deg * (pi<T>() / static_cast<T>(180.f));
}

template <std::floating_point T>
T degrees(T const rad)
{
    return rad * (static_cast<T>(180.f) / pi<T>());
}

inline bool compare_float(float32 a, float32 b, float32 epsilon = 0.0001f)
{
    return ((a - b) <= epsilon);
}

inline bool compare_double(float64 a, float64 b, float64 epsilon = 0.0001)
{
    return ((a - b) <= epsilon);
}
}

#endif // MATH_LIBRARY_MATH_H
