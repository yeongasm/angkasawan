#pragma once
#ifndef HANDLE_H
#define HANDLE_H

#include "named_type.h"
#include "concepts.h"

namespace lib
{

/**
* A handle object.
*/
template <typename T, is_handle_value_type U, U invalid_value>
class handle : public named_type<U, T>
{
private:
	using super = named_type<U, T>;
	using value_type = typename super::value_type;
public:
	inline static constexpr handle invalid_handle_v = handle{ invalid_value };

	constexpr handle() : super{ invalid_value } {}
	constexpr explicit handle(value_type value) : super{ value } {}
	constexpr ~handle() = default;

	/*constexpr handle(handle const& rhs) { *this = rhs; }
	constexpr handle(handle&& rhs) { *this = std::move(rhs); }*/

	/*constexpr handle& operator=(handle const& rhs)
	{
		Super::operator=(rhs);
		return *this;
	}*/

	/*constexpr handle& operator=(handle&& rhs)
	{
		Super::operator=(std::move(rhs));
		return *this;
	}*/

	/*constexpr explicit operator strong_type_value_t() { return Super::get(); }
	constexpr explicit operator const strong_type_value_t const() const { return Super::get(); }*/

	constexpr bool valid() const { return super::get() != invalid_value; }

	/*constexpr bool const operator==(handle rhs) const { return Super::operator==(rhs); }*/
	//constexpr bool const operator!=(handle rhs) const { return Super::operator!=(rhs); }

	/*constexpr bool operator==(handle rhs) { return Super::operator==(rhs); }*/
	//constexpr bool operator!=(handle rhs) { return Super::operator!=(rhs); }
};

}

#endif // !HANDLE_H
