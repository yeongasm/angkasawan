#pragma once
#ifndef LIB_FUNCTION_H
#define LIB_FUNCTION_H

#include <functional>
#include "memory.h"

namespace lib
{

struct function_properties
{
	size_t capacity;
	size_t alignment;
};

template <typename T>
struct function_trait;

namespace fn
{
namespace detail
{
enum class Op
{
	Move,
	Destroy
};

union data_accessor
{
	void* ptr;
	std::uintptr_t insitu_ptr;
};

template <function_properties properties>
union internal_storage
{
	static_assert(properties.capacity >= sizeof(std::uintptr_t), "Function capacity too small. Needs to be at least the size of a std::uintptr_t");

	alignas(properties.alignment) std::byte _buf[properties.capacity] = {};
	data_accessor accessor;
};

template <typename T>
auto data(std::true_type /*insitu*/, data_accessor& accessor) -> T*
{
	return static_cast<T*>(static_cast<void*>(&accessor.insitu_ptr));
};

template <typename T>
auto data(std::false_type /*insitu*/, data_accessor& accessor) -> T*
{
	return static_cast<T*>(accessor.ptr);
};

template <typename T>
using forward_arg = T&&;
}
}

template <typename Ret, typename... Args>
struct function_trait<Ret(Args...)>
{
	using type			= Ret(Args...);
	using const_type	= Ret(Args...) const;
	using return_type	= Ret;

	using invoke_fn_t = Ret(*)(fn::detail::data_accessor&, Args...);

	template <typename FnType>
	struct operator_impl
	{
		template <typename... Arguments>
		auto operator()(Arguments&&... args) -> return_type requires (... && std::same_as<Arguments, Args>)
		{
			auto&& fn = *(static_cast<FnType*>(this));
			if (!fn.m_vtbl.invocable)
			{
				ASSERTION(false && "Calling a vtable with an empty callable");
				std::abort();
			}

			return fn.m_vtbl.invocable(fn.m_storage.accessor, std::forward<Arguments>(args)...);
		}
	};

	template <typename T, bool insitu>
	static auto invoke(fn::detail::data_accessor& accessor, Args... args) -> return_type
	{
		if constexpr (insitu)
		{
			return std::invoke((*fn::detail::data<T>(std::true_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
		else
		{
			return std::invoke((*fn::detail::data<T>(std::false_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
	}
};

template <typename Ret, typename... Args>
struct function_trait<Ret(Args...) const>
{
	using type			= Ret(Args...) const;
	using const_type	= Ret(Args...) const;
	using return_type	= Ret;

	using invoke_fn_t = Ret(*)(fn::detail::data_accessor&, Args...);

	template <typename FnType>
	struct operator_impl
	{
		template <typename... Arguments>
		auto operator()(Arguments&&... args) const -> return_type requires (... && std::same_as<Arguments, Args>)
		{
			auto&& fn = *(static_cast<FnType*>(this));
			if (!fn.m_vtbl.invocable)
			{
				ASSERTION(false && "Calling a vtable with an empty callable");
				std::abort();
			}

			return fn.m_vtbl.invocable(fn.m_storage.accessor, std::forward<Arguments>(args)...);
		}
	};

	template <typename T, bool insitu>
	static auto invoke(fn::detail::data_accessor& accessor, Args... args) -> return_type
	{
		if constexpr (insitu)
		{
			return std::invoke((*fn::detail::data<T>(std::true_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
		else
		{
			return std::invoke((*fn::detail::data<T>(std::false_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
	}
};

template <typename Ret, typename... Args>
struct function_trait<Ret(Args...) noexcept>
{
	using type			= Ret(Args...) noexcept;
	using const_type	= Ret(Args...) const noexcept;
	using return_type	= Ret;

	using invoke_fn_t = Ret(*)(fn::detail::data_accessor&, Args...) noexcept;

	template <typename FnType>
	struct operator_impl
	{
		template <typename... Arguments>
		auto operator()(Arguments&&... args) noexcept -> return_type requires (... && std::same_as<Arguments, Args>)
		{
			auto&& fn = *(static_cast<FnType*>(this));
			if (!fn.m_vtbl.invocable)
			{
				ASSERTION(false && "Calling a vtable with an empty callable");
				std::abort();
			}

			return fn.m_vtbl.invocable(fn.m_storage.accessor, std::forward<Arguments>(args)...);
		}
	};

	template <typename T, bool insitu>
	static auto invoke(fn::detail::data_accessor& accessor, Args... args) noexcept -> return_type
	{
		if constexpr (insitu)
		{
			return std::invoke((*fn::detail::data<T>(std::true_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
		else
		{
			return std::invoke((*fn::detail::data<T>(std::false_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
	}
};

template <typename Ret, typename... Args>
struct function_trait<Ret(Args...) const noexcept>
{
	using type			= Ret(Args...) const noexcept;
	using const_type	= Ret(Args...) const noexcept;
	using return_type	= Ret;

	using invoke_fn_t = Ret(*)(fn::detail::data_accessor&, Args...) noexcept;

	template <typename FnType>
	struct operator_impl
	{
		template <typename... Arguments>
		auto operator()(Arguments&&... args) const noexcept -> return_type requires (... && std::same_as<Arguments, Args>)
		{
			auto&& fn = *(static_cast<FnType*>(this));
			if (!fn.m_vtbl.invocable)
			{
				ASSERTION(false && "Calling a vtable with an empty invocable");
				std::abort();
			}

			return fn.m_vtbl.invocable(fn.m_storage.accessor, std::forward<Arguments>(args)...);
		}
	};

	template <typename T, bool insitu>
	static auto invoke(fn::detail::data_accessor& accessor, Args... args) noexcept -> return_type
	{
		if constexpr (insitu)
		{
			return std::invoke((*fn::detail::data<T>(std::true_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
		else
		{
			return std::invoke((*fn::detail::data<T>(std::false_type{}, accessor)).data, std::forward<fn::detail::forward_arg<Args>>(args)...);
		}
	}
};

template <typename T>
struct vtable
{
	using invoke_fn_t = function_trait<T>::invoke_fn_t;
	using process_fn_t = void(*)(fn::detail::Op, fn::detail::data_accessor*, fn::detail::data_accessor*);

	invoke_fn_t invocable = {};
	process_fn_t op = {};

	template <bool insitu, typename Type>
	static auto construct(fn::detail::data_accessor& accessor, Type&& box)
	{
		using box_type			= std::decay_t<Type>;
		using allocator_type	= box_type::allocator_type;
		using emplace_insitu	= std::integral_constant<bool, insitu>;

		auto boxptr = fn::detail::data<box_type>(emplace_insitu{}, accessor);
		if (boxptr == nullptr)
		{
			boxptr = allocator_bind<allocator_type, box_type>::allocate(box.allocator);
			accessor.ptr = boxptr;
		}
		new (boxptr) box_type{ std::forward<Type>(box) };
	}

	template <typename Type, bool insitu>
	static auto process_op(fn::detail::Op op, fn::detail::data_accessor* dst, fn::detail::data_accessor* src) -> void
	{
		using box_type = std::decay_t<Type>;

		if (op == fn::detail::Op::Move)
		{
			if constexpr (insitu)
			{
				auto source = fn::detail::data<Type>(std::true_type{}, *src);
				construct<insitu>(*dst, std::move(*source));
				source->~box_type();
			}
			else
			{
				dst->ptr = std::exchange(src->ptr, nullptr);
			}
		}
		else if (op == fn::detail::Op::Destroy)
		{
			if constexpr (insitu)
			{
				auto box = fn::detail::data<Type>(std::true_type{}, *dst);
				box->~box_type();
			}
			else
			{
				using allocator_type = typename Type::allocator_type;
				auto box = fn::detail::data<box_type>(std::false_type{}, *dst);

				allocator_bind<allocator_type, box_type>::destroy(box);
				allocator_bind<allocator_type, box_type>::deallocate(box->allocator, box);
			}
		}
	}
};

/**
* \brief Move only function type with small buffer optimization.
* \brief Default small buffer size is 24 bytes to accommodate function object size produced from std::bind().
*/
template <typename T, function_properties properties = { .capacity = sizeof(std::uintptr_t) * 3, .alignment = alignof(std::max_align_t)}>
class function : public function_trait<T>::template operator_impl<function<T, properties>>
{
private:
	using super = function_trait<T>::template operator_impl<function>;

	friend super;

	using vtable_t	= vtable<T>;
	using storage_t = fn::detail::internal_storage<properties>;

	using internal_disable_constructor_on_move = void;

	storage_t   m_storage;
	vtable_t    m_vtbl;

	constexpr auto _destroy() -> void
	{
		if (m_vtbl.op)
		{
			m_vtbl.op(fn::detail::Op::Destroy, &m_storage.accessor, nullptr);
		}
		m_vtbl.invocable = nullptr;
		m_vtbl.op = nullptr;
	}
public:
	using trait			= function_trait<T>;
	using return_type	= trait::return_type;

	constexpr function() = default;
	constexpr ~function()
	{
		_destroy();
	}

	template <typename Functor>
	constexpr function(Functor&& functor) requires (!requires { typename Functor::internal_disable_constructor_on_move; }) :
		m_storage{},
		m_vtbl{}
	{
		auto box = make_box(std::forward<Functor>(functor));
		constexpr bool emplace_insitu = sizeof(box) <= properties.capacity;

		using box_type = decltype(box);

		vtable_t::template construct<emplace_insitu>(m_storage.accessor, std::move(box));

		m_vtbl.invocable = trait::template invoke<box_type, emplace_insitu>;
		m_vtbl.op = vtable_t::template process_op<box_type, emplace_insitu>;
	}

	template <typename Functor, typename Allocator>
	constexpr function(Functor&& functor, Allocator allocator) requires (!requires { typename Functor::internal_disable_constructor_on_move; }) :
		m_storage{},
		m_vtbl{}
	{
		auto box = make_box(std::forward<Functor>(functor), allocator);
		constexpr bool emplace_insitu = sizeof(box) <= properties.capacity;

		using box_type = decltype(box);

		vtable_t::template construct<emplace_insitu>(m_storage.accessor, std::move(box));

		m_vtbl.invocable = trait::template invoke<box_type, emplace_insitu>;
		m_vtbl.op = vtable_t::template process_op<box_type, emplace_insitu>;
	}

	constexpr function(function const&) = delete;
	constexpr auto operator=(function const&) -> function& = delete;

	constexpr function(function&& rhs) :
		m_storage{},
		m_vtbl{ std::move(rhs.m_vtbl) }
	{
		if (m_vtbl.op)
		{
			m_vtbl.op(fn::detail::Op::Move, &m_storage.accessor, &rhs.m_storage.accessor);
		}
		new (&rhs) function{};
	}

	constexpr auto operator=(function&& rhs) noexcept -> function&
	{
		if (this != &rhs)
		{
			_destroy();

			m_vtbl = std::move(rhs.m_vtbl);
			m_storage = std::exchange(rhs.m_storage, {});

			new (&rhs) function{};
		}
		return *this;
	}

	constexpr auto operator=(std::nullptr_t) noexcept -> function&
	{
		_destroy();

		return *this;
	}

	template <typename Functor>
	constexpr auto operator=(Functor&& functor) -> function&
	{
		_destroy();

		function{ std::forward<Functor>(functor) }.swap(*this);

		return *this;
	}

	constexpr auto swap(function& rhs) -> void
	{
		std::swap(m_vtbl, rhs.m_vtbl);
		std::swap(m_storage, rhs.m_storage);
	}

	constexpr explicit operator bool() const noexcept
	{
		return m_vtbl.invocable != nullptr;
	}

	friend constexpr auto operator==(function const& fn, std::nullptr_t) noexcept -> bool
	{
		return fn.m_vtbl.invocable == nullptr;
	}
};

}

#endif // !LIB_FUNCTION_H