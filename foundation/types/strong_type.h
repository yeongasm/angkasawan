#pragma once
#ifndef FOUNDATION_TYPE_STRONG_TYPE_H
#define FOUNDATION_TYPE_STRONG_TYPE_H

#include "common.h"

FTLBEGIN

template <typename T, typename Parameter>
class NamedType
{
public:
	using value_type = std::decay_t<T>;

	constexpr NamedType() : m_value{} {};
	constexpr explicit NamedType(T value) : m_value{ value } {}

	constexpr ~NamedType() { m_value.~T(); }

	constexpr NamedType(const NamedType& rhs) : m_value{ rhs.m_value } {}
	constexpr NamedType(NamedType&& rhs) : m_value{ rhs.m_value } {}

	constexpr NamedType& operator=(const NamedType& rhs)
	{
		if (this != &rhs)
		{
			m_value = rhs.m_value;
		}
		return *this;
	}

	constexpr NamedType& operator=(NamedType&& rhs)
	{
		if (this != &rhs)
		{
			m_value = std::move(rhs.m_value);
			new (&rhs) NamedType{};
		}
		return *this;
	}

	constexpr explicit operator T() { return m_value; }
	constexpr explicit operator const T() const { return m_value; }

	//constexpr const bool operator==(const NamedType& rhs) const { return m_value == rhs.m_value; }
	constexpr bool operator==(value_type rhs) const { return m_value == rhs; }
	//constexpr const bool operator!=(const NamedType& rhs) const { return m_value != rhs.m_value; }

	constexpr bool operator==(NamedType const& rhs) const { return m_value == rhs.m_value; }
	//constexpr bool operator!=(const NamedType& rhs) { return m_value != rhs.m_value; }

	constexpr T& get() { return m_value; }
	constexpr const T& get() const { return m_value; }
private:
	T m_value;
};

FTLEND

#endif // !FOUNDATION_TYPE_STRONG_TYPE_H
