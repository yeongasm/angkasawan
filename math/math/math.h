#pragma once
#ifndef MATH_LIBRARY_MATH_H
#define MATH_LIBRARY_MATH_H

#include <type_traits>
#include <concepts>
#include <cmath>

namespace math
{
    template <typename T>
    concept is_arithmetic = std::is_arithmetic_v<T>;

    auto cosf(std::floating_point auto num)
    {
        return std::cosf(num);
    }

    auto sinf(std::floating_point auto num)
    {
        return std::sinf(num);
    }

    auto tanf(std::floating_point auto num)
    {
        return std::tanf(num);
    }

    auto acosf(std::floating_point auto num)
    {
        return std::acosf(num);
    }

    auto asinf(std::floating_point auto num)
    {
        return std::asinf(num);
    }

    auto atanf(std::floating_point auto num)
    {
        return std::atanf(num);
    }

    auto sqrtf(std::floating_point auto num)
    {
        return std::sqrtf(num);
    }

    constexpr auto sqr(is_arithmetic auto num)
    {
        return num * num;
    }

    auto cbrtf(std::floating_point auto num)
    {
        return std::cbrtf(num);
    }

    auto powf(std::floating_point auto num, std::floating_point auto exp)
    {
        return std::powf(num, exp);
    }

    auto abs(std::integral auto num)
    {
        return std::abs(num);
    }

    auto fabsf(std::floating_point auto num)
    {
        return std::fabsf(num);
    }

    auto mod(std::integral auto num, std::integral auto denom)
    {
        return num % denom;
    }

    auto log2f(std::floating_point auto num)
    {
        return std::log2f(num);;
    }

    auto fmodf(std::floating_point auto num, std::floating_point auto denom)
    {
        return std::fmodf(num, denom);
    }

    auto floorf(std::floating_point auto num)
    {
        return std::floorf(num);
    }

    auto ceilf(std::floating_point auto num)
    {
        return std::ceilf(num);
    }

    auto roundf(std::floating_point auto num)
    {
        return std::roundf(num);
    }

    template <std::floating_point T>
    T pi()
    {
        return static_cast<T>(std::atanf(1.f) * 4.f);
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
    T radians(const T deg)
    {
        return deg * (pi<T>() / static_cast<T>(180.f));
    }

    template <std::floating_point T>
    T degrees(const T rad)
    {
        return rad * (static_cast<T>(180.f) / pi<T>());
    }

    bool compare_float(float32 a, float32 b, float32 epsilon = 0.0001f)
    {
        return ((a - b) <= epsilon);
    }

    bool compare_double(float64 a, float64 b, float64 epsilon = 0.0001)
    {
        return ((a - b) <= epsilon);
    }
}

#endif // MATH_LIBRARY_MATH_H
