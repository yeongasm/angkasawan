#pragma once
#ifndef LIB_HPPANDLE_HPP
#define LIB_HPPANDLE_HPP

#include "type.hpp"
#include "named_type.hpp"
#include "concepts.hpp"

namespace lib
{
/**
* A handle object.
* 
* Allows access to the value stored in the handle.
*/
template <typename T, is_handle_value_type U, U invalid_value>
class handle : public named_type<U, T>
{
protected:
	using super			= named_type<U, T>;
	using value_type	= typename super::value_type;	// This is the type of the value stored by the handle.
public:
	constexpr static handle invalid_handle() { return handle{}; }

	constexpr handle() : super{ invalid_value } {}
	constexpr explicit handle(value_type value) : super{ value } {}
	constexpr ~handle() = default;

	constexpr bool valid		() const	{ return super::get() != invalid_value; }
	constexpr void invalidate	()			{ *this = invalid_handle(); }
};

/**
* An opaque handle object.
* 
* Value stored in handle only accessible by Owners specified in the template parameter.
*/
template <typename T, is_handle_value_type U, U invalid_value, typename... Owners>
class opaque_handle : public handle<T, U, invalid_value>
{
private:
	using super			= handle<T, U, invalid_value>;
	using value_type	= typename super::value_type;
	using owner_types	= type_tuple<Owners...>;
public:
	using super::handle;

	constexpr static opaque_handle invalid_handle() { return opaque_handle{}; }

	constexpr value_type get() const = delete;

	template <typename Accessor>
	constexpr value_type access([[maybe_unused]] Accessor const& foo) const requires (exist_in_type_tuple<Accessor, owner_types>())
	{
		return super::get();
	}
};
}

#endif // !LIB_HPPANDLE_HPP
