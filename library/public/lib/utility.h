#pragma once
#ifndef LIB_UTILITY_H
#define LIB_UTILITY_H

#include "common.h"

namespace lib
{

/**
* @brief Base class for all growth policy structs.
*/
struct container_growth_policy {};
/**
* @brief Common interface for ShiftGrowthPolicy & PaddedGrowthPolicy.
*/
template <size_t Amount, bool Shift = false>
struct default_growth_policy : public container_growth_policy
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
using shift_growth_policy = default_growth_policy<Amount, true>;
/**
* @brief Informs the container to grow the existing capacity by existing_capacity + Amount.
* @param Amount - Amount of padding to add to the existing capacity.
*/
template <size_t Amount>
using padded_growth_policy = default_growth_policy<Amount, false>;
/**
* @brief Grows the container's capacity to the next power of two.
*/
struct power_of_two_growth_policy : container_growth_policy
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

template <typename iterator>
class reverse_iterator
{
public:
    using value_type        = typename iterator::value_type;
    using difference_type   = typename iterator::diference_type;
    using pointer           = typename iterator::pointer;
    using reference         = typename iterator::reference;

    constexpr reverse_iterator() = default;
    constexpr reverse_iterator(iterator const& it) : m_current{ it } {}

    constexpr ~reverse_iterator() = default;

    constexpr reverse_iterator(reverse_iterator const& rhs) :
        m_current{ rhs.m_current }
    {}

    constexpr iterator base() const { return m_current; }

    constexpr reverse_iterator& operator=(const reverse_iterator& rhs)
    {
        m_current = rhs.m_current;
        return *this;
    }

    constexpr reference operator*() const
    {
        iterator tmp = m_current;
        return *--tmp;
    }

    constexpr pointer operator->() const
    {
        iterator tmp = m_current;
        --tmp;
        if constexpr (std::is_pointer_v<iterator>)
        {
            return tmp;
        }
        return tmp.operator->();
    }

    constexpr reverse_iterator& operator++()
    {
        --m_current;
        return *this;
    }

    constexpr reverse_iterator operator++(int)
    {
        reverse_iterator tmp = *this;
        --m_current;
        return tmp;
    }

    constexpr reverse_iterator& operator--()
    {
        ++m_current;
        return *this;
    }

    constexpr reverse_iterator operator--(int)
    {
        reverse_iterator tmp = *this;
        ++m_current;
        return tmp;
    }

    constexpr reverse_iterator operator+(difference_type const offset) const
    {
        return reverse_iterator(m_current - offset);
    }

    constexpr reverse_iterator& operator+=(difference_type const offset)
    {
        m_current -= offset;
        return *this;
    }

    constexpr reverse_iterator operator-(difference_type const offset) const
    {
        return reverse_iterator(m_current + offset);
    }

    constexpr reverse_iterator& operator-=(difference_type const offset)
    {
        m_current += offset;
        return *this;
    }

    constexpr reference operator[](difference_type const offset) const
    {
        return m_current[static_cast<difference_type>(-offset - 1)];
    }

    constexpr bool operator==(reverse_iterator rhs) const noexcept
    {
        return m_current == rhs.m_current;
    }

    constexpr std::strong_ordering operator<=>(reverse_iterator rhs) const noexcept
    {
        return m_current <=> rhs.m_current;
    }

private:
    iterator m_current;
};

}

#endif // !LIB_UTILITY_H
