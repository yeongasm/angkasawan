#pragma once
#ifndef FOUNDATION_CONTAINERS_UTILITY_H
#define FOUNDATION_CONTAINERS_UTILITY_H

#include "common.h"

FTLBEGIN

/**
* @brief Base class for all growth policy structs.
*/
struct ContainerGrowthPolicy {};
/**
* @brief Common interface for ShiftGrowthPolicy & PaddedGrowthPolicy.
*/
template <size_t Amount, bool Shift = false>
struct DefaultGrowthPolicy : public ContainerGrowthPolicy
{
    static constexpr size_t new_capacity(size_t num)
    {
        if constexpr (!Shift)
        {
            return num + Amount;
        }
        else
        {
            return num + (1 << Amount);
        }
    }
};

/**
* @brief Informs the container to grow the existing capacity by existing_capacity + (1 << Amount).
* @param Amount - Bits to left shift.
*/
template <size_t Amount>
using ShiftGrowthPolicy = DefaultGrowthPolicy<Amount, true>;
/**
* @brief Informs the container to grow the existing capacity by existing_capacity + Amount.
* @param Amount - Amount of padding to add to the existing capacity.
*/
template <size_t Amount>
using PaddedGrowthPolicy = DefaultGrowthPolicy<Amount, false>;
/**
* @brief Grows the container's capacity to the next power of two.
*/
struct PowerOfTwoGrowthPolicy : ContainerGrowthPolicy
{
    static constexpr size_t new_capacity(size_t num)
    {
        // Get the next power of two from num.
        ++num;
        for (size_t i = 0; i < sizeof(size_t) * CHAR_BIT; i *= 2)
        {
            num |= num >> i;
        }
        ++num;

        return num;
    }
};

struct OneTimeGrowthPolicy : ContainerGrowthPolicy
{
    static constexpr size_t new_capacity(size_t num)
    {
        return 0;
    }
};

/**
* TODO(Afiq):
* Introduce ExponentialGrowthPolicy, LogarithmicGrowthPolicy & PrimeGrowthPolicy.
*/

template <typename It>
class ReverseIterator
{
public:
    using value_type    = typename It::Type_t;
    using pointer       = typename It::Pointer_t;
    using reference     = typename It::Reference_t;

    constexpr ReverseIterator() = default;
    constexpr ReverseIterator(It it) : m_current{ std::move(it) } {}

    constexpr ~ReverseIterator() = default;

    constexpr ReverseIterator(const ReverseIterator& rhs) :
        m_current{ rhs.m_current }
    {}

    constexpr It base() const { return m_current; }

    constexpr ReverseIterator& operator=(const ReverseIterator& rhs)
    {
        m_current = rhs.m_current;
        return *this;
    }

    constexpr reference operator*() const
    {
        It tmp = m_current;
        return *--tmp;
    }

    constexpr pointer operator->() const
    {
        It tmp = m_current;
        --tmp;
        if constexpr (std::is_pointer_v<It>)
        {
            return tmp;
        }
        return tmp.operator->();
    }

    constexpr ReverseIterator& operator++()
    {
        --m_current;
        return *this;
    }

    constexpr ReverseIterator operator++(int)
    {
        ReverseIterator tmp = *this;
        --m_current;
        return tmp;
    }

    constexpr ReverseIterator& operator--()
    {
        ++m_current;
        return *this;
    }

    constexpr ReverseIterator operator--(int)
    {
        ReverseIterator tmp = *this;
        ++m_current;
        return tmp;
    }

    constexpr ReverseIterator operator+(const size_t offset) const
    {
        return ReverseIterator(m_current - offset);
    }

    constexpr ReverseIterator& operator+=(const size_t offset)
    {
        m_current -= offset;
        return *this;
    }

    constexpr ReverseIterator operator-(const size_t offset) const
    {
        return ReverseIterator(m_current + offset);
    }

    constexpr ReverseIterator& operator-=(const size_t offset)
    {
        m_current += offset;
        return *this;
    }

    constexpr reference operator[](const size_t offset) const
    {
        return m_current[static_cast<size_t>(-offset - 1)];
    }
private:
    It m_current;
};

template <typename ItRhs, typename ItLhs>
bool operator==(ReverseIterator<ItRhs> lhs, ReverseIterator<ItLhs> rhs)
{
    return lhs.base() == rhs.base();
}

template <typename ItRhs, typename ItLhs>
bool operator!=(ReverseIterator<ItRhs> lhs, ReverseIterator<ItLhs> rhs)
{
    return lhs.base() != rhs.base();
}

template <typename ItRhs, typename ItLhs>
bool operator<(ReverseIterator<ItRhs> lhs, ReverseIterator<ItLhs> rhs)
{
    return lhs.base() > rhs.base();
}

template <typename ItRhs, typename ItLhs>
bool operator<=(ReverseIterator<ItRhs> lhs, ReverseIterator<ItLhs> rhs)
{
    return lhs.base() >= rhs.base();
}

template <typename ItRhs, typename ItLhs>
bool operator>(ReverseIterator<ItRhs> lhs, ReverseIterator<ItLhs> rhs)
{
    return lhs.base() < rhs.base();
}

template <typename ItRhs, typename ItLhs>
bool operator>=(ReverseIterator<ItRhs> lhs, ReverseIterator<ItLhs> rhs)
{
    return lhs.base() <= rhs.base();
}

FTLEND

#endif // !FOUNDATION_CONTAINERS_UTILITY_H
