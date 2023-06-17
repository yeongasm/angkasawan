#pragma once
#ifndef CORE_RESOURCE_HANDLE_H
#define CORE_RESOURCE_HANDLE_H

#include "types/strong_type.h"
#include "memory/memory.h"
#include <limits>

FTLBEGIN

using handle_value_t = size_t;
inline constexpr handle_value_t INVALID_HANDLE = std::numeric_limits<handle_value_t>::max();

/**
* A handle object.
*/
template <typename T, typename U = handle_value_t>
class Handle : protected NamedType<U, T>
{
private:
	using strong_type_value_t = U;
	using Super = ftl::NamedType<U, T>;
public:
	constexpr Handle() : Super{ static_cast<U>(INVALID_HANDLE) } {}
	constexpr explicit Handle(strong_type_value_t value) : Super{ value } {}
	constexpr ~Handle() = default;

	constexpr Handle(const Handle& rhs) { *this = rhs; }
	constexpr Handle(Handle&& rhs) { *this = std::move(rhs); }

	constexpr Handle& operator=(const Handle& rhs)
	{
		Super::operator=(rhs);
		return *this;
	}

	constexpr Handle& operator=(Handle&& rhs)
	{
		Super::operator=(std::move(rhs));
		return *this;
	}

	constexpr explicit operator strong_type_value_t() { return Super::get(); }
	constexpr explicit operator const strong_type_value_t const() const { return Super::get(); }

	constexpr strong_type_value_t		get()		{ return Super::get(); }
	constexpr strong_type_value_t const	get() const	{ return Super::get(); }

	constexpr bool is_valid() const { return Super::get() != INVALID_HANDLE; }

	constexpr bool const operator==(Handle rhs) const { return Super::operator==(rhs); }
	//constexpr bool const operator!=(Handle rhs) const { return Super::operator!=(rhs); }

	constexpr bool operator==(Handle rhs) { return Super::operator==(rhs); }
	//constexpr bool operator!=(Handle rhs) { return Super::operator!=(rhs); }
};

template <typename T>
inline constexpr Handle<T> invalid_handle_v{ INVALID_HANDLE };

/**
* A reference and handle object combined.
*/
/*template <typename T>
class RefHnd : public Handle<T>, public Ref<T>
{
public:
	RefHnd() : Handle<T>{}, ftl::Ref<T>{} {}
	~RefHnd() = default;

	RefHnd(handle_value_t value, T* object) :
		Handle<T>{ value }, ftl::Ref<T>{ object }
	{}

	RefHnd(const RefHnd& rhs) { *this = rhs; }
	RefHnd(RefHnd&& rhs) { *this = std::move(rhs); }

	RefHnd& operator=(const RefHnd& rhs)
	{
		Handle<T>::operator=(rhs);
		ftl::Ref<T>::operator=(rhs);
		return *this;
	}

	RefHnd& operator=(RefHnd&& rhs)
	{
		Handle<T>::operator=(std::move(rhs));
		ftl::Ref<T>::operator=(std::move(rhs));
		return *this;
	}

	using Handle<T>::operator==;
	using Handle<T>::operator!=;
	using ftl::Ref<T>::operator==;
	using ftl::Ref<T>::operator!=;
	using ftl::Ref<T>::operator->;
};*/

template <typename Flag_t>
concept IsEnum = std::is_enum_v<Flag_t>;

template <IsEnum Flag_t, typename Payload_t>
struct Result
{
	Flag_t		status;
	Payload_t	payload;
};

FTLEND

#endif // !CORE_RESOURCE_HANDLE_H
