#pragma once
#ifndef LIB_SET_ONCE_HPP
#define LIB_SET_ONCE_HPP

#include <atomic>
#include "concepts.hpp"

namespace lib
{
template <typename T>
class set_once
{
public:
	using type = T;
	using value_type = std::decay_t<type>;

	set_once() = default;
	~set_once() = default;

	template <is_assignable_to<value_type> U>
	set_once(U&& value) :
		m_value{ value },
		m_flag{}
	{
		m_flag.test_and_set(std::memory_order_relaxed);
	}

	set_once(set_once const& rhs) = delete;
	set_once& operator=(set_once const& rhs) = delete;

	template <is_assignable_to<value_type> U>
	auto operator=(U&& value) -> set_once&
	{
		set(std::forward<T>(value));
		return *this;
	}

	template <is_assignable_to<value_type> U>
	auto set(U&& value) -> bool
	{
		if (!has_value())
		{
			new (&m_value) value_type{ std::forward<U>(value) };
		}
		return !m_flag.test_and_set(std::memory_order_acq_rel);
	}

	template <typename... Args>
	auto emplace(Args&&... args) -> bool
	{
		if (!has_value())
		{
			new (&m_value) value_type{ std::forward<Args>(args)... };
		}
		return !m_flag.test_and_set(std::memory_order_acq_rel);
	}

	auto reset() -> void
	{
		m_value.~value_type();
		m_flag.clear(std::memory_order_release);
	}

	auto operator*() -> type& { return m_value; }
	auto operator*() const -> type const& { return m_value; }

	explicit operator bool() const { return has_value(); }

	auto has_value() const -> bool { return m_flag.test(std::memory_order_acquire); }

	auto value() const -> type const& { return m_value; }
	auto value() -> type& { return m_value; }

private:
	type m_value = {};
	std::atomic_flag m_flag = {};
};
}

#endif // !LIB_SET_ONCE_HPP
