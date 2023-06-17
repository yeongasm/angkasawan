#pragma once
#ifndef FOUNDATION_BITSET_H
#define FOUNDATION_BITSET_H

#include "memory/memory.h"

FTLBEGIN

template <typename T>
concept IsBitType = std::is_enum_v<T> || std::is_integral_v<T>;

template <typename T>
struct EnumTypeBase
{
	using Type = std::underlying_type_t<std::remove_cvref_t<T>>;
};

template <typename T>
struct IntegralTypeBase
{
	using Type = std::remove_cvref_t<T>;
};

template <IsBitType T, size_t Bits = sizeof(T) * CHAR_BIT>
class Bitset : public std::conditional_t<std::is_enum_v<T>, EnumTypeBase<T>, IntegralTypeBase<T>>
{
public:
	using Type = typename std::conditional_t<std::is_enum_v<T>, EnumTypeBase<T>, IntegralTypeBase<T>>::Type;

	constexpr Bitset() :
		m_data{}
	{}

	constexpr Bitset(Type value, size_t atWord = 0)
	{
		assign(value, atWord);
	}

	constexpr ~Bitset() { reset(); }

	constexpr Bitset(const Bitset& rhs) { *this = rhs; }
	constexpr Bitset(Bitset&& rhs) { *this = std::move(rhs); }

	constexpr Bitset& operator=(const Bitset& rhs)
	{
		if (this != &rhs)
		{
			ftl::memcopy(m_data, rhs.m_data, sizeof(T) * (_word + 1));
		}
		return *this;
	}

	constexpr Bitset& operator=(Bitset&& rhs)
	{
		if (this != &rhs)
		{
			ftl::memmove(m_data, rhs.m_data, sizeof(T) * (_word + 1));
			new (&rhs) Bitset{};
		}
		return *this;
	}

	/*constexpr Bitset& operator=(Type value)
	{
		return *this;
	}*/

	constexpr bool operator[](T position) const
	{
		return has(position);
	}

	constexpr Bitset& set(T position, bool value = true)
	{
		const Type pos = static_cast<Type>(position);
		ASSERTION(pos <= Bits && "Position supplied exceeded the number of bits!");
		return set_internal(pos, value);
	}

	constexpr Bitset& reset(T position)
	{
		const Type pos = static_cast<Type>(position);
		ASSERTION(pos <= Bits && "Position supplied exceeded the number of bits!");
		return set_internal(pos, false);
	}

	constexpr bool has(T position) const
	{
		const Type pos = static_cast<Type>(position);
		ASSERTION(pos <= Bits && "Position supplied exceeded the number of bits!");
		return (m_data[pos / _bitsPerWord] & (Type{1} << pos % _bitsPerWord)) != 0;
	}

	constexpr bool test(T position) const
	{
		return has(position);
	}

	constexpr Bitset& flip()
	{
		for (size_t i = 0; i <= _word; i++)
		{
			m_data[i] = ~m_data[i];
		}
		return *this;
	}

	constexpr Bitset& flip(T position)
	{
		const Type pos = static_cast<Type>(position);
		ASSERTION(pos <= Bits && "Position supplied exceeded the number of bits!");
		m_data[pos / _bitsPerWord] ^= Type{1} << (pos % _bitsPerWord);
		return *this;
	}

	constexpr Bitset& reset()
	{
		ftl::memzero(m_data, sizeof(Type) * (_word + 1));
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

	/*constexpr Bitset& assign(Type value, size_t atWord = 0)
	{
		ASSERTION(atWord <= _word && "Word provided exceeds bitset's capacity!");
		m_data[atWord] = value;
		return *this;
	}*/
private:
	static constexpr size_t _bitsPerWord = CHAR_BIT * sizeof(Type);
	static constexpr size_t _word = Bits == 0 ? 0 : (Bits - 1) / _bitsPerWord;

	Type m_data[_word + 1];

	constexpr Bitset& set_internal(Type position, bool value)
	{
		Type& selectedWord = m_data[position / _bitsPerWord];
		const Type bit = Type{ 1 } << (position % _bitsPerWord);

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


FTLEND

#endif // !FOUNDATION_BITSET_H
