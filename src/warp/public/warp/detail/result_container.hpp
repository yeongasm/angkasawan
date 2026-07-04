#pragma once
#ifndef WARP_DETAIL_RESULT_TYPE_HPP
#define WARP_DETAIL_RESULT_TYPE_HPP

#include <concepts>
#include <type_traits>
#include <variant>

namespace warp
{
namespace detail
{
enum class void_result : unsigned char {};
/*
* TODO(afiq):
* [ ] - Extend result_container to store errors as well in the future. Probably needs a specialization for it.
*/
template <typename T>
class result_container
{
public:
	static_assert(!std::is_reference_v<T>, "Warp does not natively support storing references in task objects!");

	using value_type 	= std::conditional_t<std::same_as<T, void>, void_result, std::remove_const_t<T>>;
	using result_type 	= std::conditional_t<std::same_as<T, void>, void, value_type>;
private:
	std::variant<std::monostate, value_type> m_result;
public:

	template <typename... Args>
	constexpr auto set_value(Args&&... args) noexcept -> void requires std::constructible_from<value_type, Args...>
	{
		m_result.template emplace<1>(std::forward<Args>(args)...);
	}

	constexpr auto has_result_set() const noexcept -> bool
	{
		return std::holds_alternative<value_type>(m_result);
	}

	/*
	 * To be called inside an awaiter's await_resume().
	 */
	constexpr auto result() noexcept -> result_type
	{
		switch (m_result.index())
		{
		case 0:
			// Should never reach here! Something went wrong, abort!
			// TODO: Throw an assert with an overidable custom assert handler.
			std::unreachable();
			break;
		case 1:
		default:
			break;
		}

		if constexpr (std::same_as<value_type, void_result>)
		{
			return;
		}
		else
		{
			return std::move(std::get<value_type>(m_result));
		}
	}
};

namespace ___unit_test
{
consteval auto ___result_container_test_emplace_value() -> bool
{
	result_container<int> _intResultType;
	_intResultType.set_value(1);
	return _intResultType.has_result_set();
}

consteval auto ___result_container_test_result_resume() -> bool
{
	result_container<int> _intResultType;
	_intResultType.set_value(1);
	return _intResultType.result() == 1;
}
static_assert(___result_container_test_emplace_value(), "Unit test failing for ___result_type_test_emplace_value() in return_value.hpp");
static_assert(___result_container_test_result_resume(), "Unit test failing for ___result_type_test_result_resume() in return_value.hpp");
}
}
}

#endif // !WARP_DETAIL_RESULT_TYPE_HPP