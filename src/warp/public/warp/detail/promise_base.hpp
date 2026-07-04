#pragma once
#ifndef WARP_DETAIL_PROMISE_BASE_HPP
#define WARP_DETAIL_PROMISE_BASE_HPP

#include "result_container.hpp"

namespace warp
{
namespace detail
{
template <typename T>
class promise_result_container
{
public:
	auto set_result(result_container<T>& result) -> void
	{
		m_result = &result;
	}
protected:
	result_container<T>* m_result;
};

template <typename T>
class promise_base : public promise_result_container<T>
{
public:
	using return_type = T;

	template <typename ValueType>
	auto return_value(ValueType&& value) -> void requires std::constructible_from<return_type, ValueType>
	{
		return this->m_result->set_value(std::forward<ValueType>(value));
	}
};

template <>
class promise_base<void> : public promise_result_container<void>
{
public:
	using return_type = void;

	auto return_void() -> void
	{
		this->m_result->set_value(void_result{});
	}
};
}
}

#endif // !WARP_DETAIL_PROMISE_BASE_HPP