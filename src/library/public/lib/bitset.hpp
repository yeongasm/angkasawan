#pragma once
#ifndef LIB_BITSET_HPP
#define LIB_BITSET_HPP

#include "memory.hpp"

namespace lib
{

template <typename T>
struct enum_type_base
{
	using type = std::underlying_type_t<std::remove_cvref_t<T>>;
};

template <typename T>
struct integral_type_base
{
	using type = std::remove_cvref_t<T>;
};

template <is_bit_compatible T, size_t Bits = sizeof(T) * CHAR_BIT>
class bitset : public std::conditional_t<std::is_enum_v<T>, enum_type_base<T>, integral_type_base<T>>
{
public:
	using bit_type		= T;
	using value_type	= typename std::conditional_t<std::is_enum_v<bit_type>, enum_type_base<bit_type>, integral_type_base<bit_type>>::type;

	constexpr bitset() :
		m_data{}
	{}

	constexpr bitset(value_type value, size_t atWord = 0)
	{
		assign(value, atWord);
	}

	constexpr ~bitset() { reset(); }

	constexpr bitset(bitset const& rhs) { *this = rhs; }
	constexpr bitset(bitset&& rhs) { *this = std::move(rhs); }

	constexpr bitset& operator=(bitset const& rhs)
	{
		if (this != &rhs)
		{
			std::memcpy(m_data, rhs.m_data, sizeof(value_type) * (_word + 1));
		}
		return *this;
	}

	constexpr bitset& operator=(bitset&& rhs)
	{
		if (this != &rhs)
		{
			std::memmove(m_data, rhs.m_data, sizeof(value_type) * (_word + 1));
			new (&rhs) bitset{};
		}
		return *this;
	}

	constexpr bool operator[](bit_type position) const
	{
		return has(position);
	}

	constexpr bitset& set(bit_type position, bool value = true)
	{
		const value_type pos = static_cast<value_type>(position);
		ASSERTION(std::cmp_less_equal(pos, Bits) && "Position supplied exceeded the number of bits!");
		return set_internal(pos, value);
	}

	constexpr bitset& reset(bit_type position)
	{
		const value_type pos = static_cast<value_type>(position);
		ASSERTION(std::cmp_less_equal(pos, Bits) && "Position supplied exceeded the number of bits!");
		return set_internal(pos, false);
	}

	constexpr bool has(bit_type position) const
	{
		const value_type pos = static_cast<value_type>(position);
		ASSERTION(std::cmp_less_equal(pos, Bits) && "Position supplied exceeded the number of bits!");
		return (m_data[pos / _bitsPerWord] & (value_type{1} << pos % _bitsPerWord)) != 0;
	}

	constexpr bool test(bit_type position) const
	{
		return has(position);
	}

	constexpr bitset& flip()
	{
		for (size_t i = 0; i <= _word; i++)
		{
			m_data[i] = ~m_data[i];
		}
		return *this;
	}

	constexpr bitset& flip(bit_type position)
	{
		const value_type pos = static_cast<value_type>(position);
		ASSERTION(pos <= Bits && "Position supplied exceeded the number of bits!");
		m_data[pos / _bitsPerWord] ^= value_type{1} << (pos % _bitsPerWord);
		return *this;
	}

	constexpr bitset& reset()
	{
		memzero(m_data, sizeof(value_type) * (_word + 1));
		return *this;
	}

	constexpr size_t size() const { return Bits; }

	/*template <typename Lambda>
	constexpr void for_each(Lambda lambda)
	{
		size_t bit = 0;
		size_t currentWord = 0;
		const size_t maxWord = _word + 1;

		while (currentWord != maxWord)
		{
			Type value = m_data[currentWord] & (1 << bit++);
			lambda(static_cast<T>(value));

			if (bit >= _bitsPerWord)
			{
				bit = 0;
				++currentWord;
			}
		}
	}*/

	/*constexpr Type value(size_t selectedWord = 0) const
	{
		ASSERTION(selectedWord <= _word && "Selected word exceeds bitset's capacity!");
		return m_data[selectedWord];
	}*/

	/*constexpr bitset& assign(Type value, size_t atWord = 0)
	{
		ASSERTION(atWord <= _word && "Word provided exceeds bitset's capacity!");
		m_data[atWord] = value;
		return *this;
	}*/
private:
	static constexpr size_t _bitsPerWord = CHAR_BIT * sizeof(value_type);
	static constexpr size_t _word = Bits == 0 ? 0 : (Bits - 1) / _bitsPerWord;

	value_type m_data[_word + 1];

	constexpr bitset& set_internal(value_type position, bool value)
	{
		value_type& selectedWord = m_data[position / _bitsPerWord];
		const value_type bit = value_type{ 1 } << (position % _bitsPerWord);

		if (value)
		{
			selectedWord |= bit;
		}
		else
		{
			selectedWord &= ~bit;
		}
		return *this;
	}
};

}

#endif // !LIB_BITSET_HPP
