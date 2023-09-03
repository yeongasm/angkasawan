#pragma once
#ifndef NAMED_TYPE_H
#define NAMED_TYPE_H

namespace lib
{

template <typename T, typename param_name>
class named_type
{
public:
	using value_type = T;

	constexpr named_type() : m_value{} {};
	constexpr explicit named_type(T value) : m_value{ value } {}

	constexpr ~named_type() { m_value.~T(); }

	constexpr named_type(const named_type& rhs) : m_value{ rhs.m_value } {}
	constexpr named_type(named_type&& rhs) : m_value{ rhs.m_value } {}

	constexpr named_type& operator=(const named_type& rhs)
	{
		if (this != &rhs)
		{
			m_value = rhs.m_value;
		}
		return *this;
	}

	constexpr named_type& operator=(named_type&& rhs)
	{
		if (this != &rhs)
		{
			m_value = std::move(rhs.m_value);
			new (&rhs) named_type{};
		}
		return *this;
	}

	/*constexpr explicit operator T() { return m_value; }
	constexpr explicit operator const T() const { return m_value; }*/

	constexpr bool operator==(value_type rhs)			const { return m_value == rhs; }
	constexpr bool operator==(named_type const& rhs)	const { return m_value == rhs.m_value; }

	constexpr value_type&		get()		{ return m_value; }
	constexpr const value_type& get() const { return m_value; }
private:
	T m_value;
};

}

#endif // !NAMED_TYPE_H
