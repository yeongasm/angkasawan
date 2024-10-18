#pragma once
#ifndef LIB_MEMORY_H
#define LIB_MEMORY_H

#include <source_location>
#include <bit>
#include "common.h"
#include "concepts.h"
#include "api.h"

constexpr inline size_t operator""_KiB(size_t n) { return 1024 * n; }
constexpr inline size_t operator""_MiB(size_t n) { return 1024_KiB * n; }
constexpr inline size_t operator""_GiB(size_t n) { return 1024_MiB * n; }

namespace lib
{

LIB_API void	memmove(void* dst, void* src, size_t size);
LIB_API void	memcopy(void* dst, const void* src, size_t size);
LIB_API size_t	memcmp(const void* src, const void* dst, size_t size);
LIB_API void	memset(void* dst, uint8 val, size_t size);
LIB_API void	memzero(void* dst, size_t size);

template <typename T>
void memswap(T* a, T* b)
{
	T temp;
	memcopy(&temp, a, sizeof(T));
	memcopy(a, b, sizeof(T));
	memcopy(b, &temp, sizeof(T));
}

LIB_API constexpr size_t	is_power_of_two(size_t num);
LIB_API constexpr size_t	pad_address(const uintptr_t address, const size_t alignment);

LIB_API constexpr bool		is_64bit_aligned(void* pointer);

struct allocate_info
{
	size_t size;
	size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
};

// Allocates memory using the system.
//[[deprecated("Every allocation needs to go through an allocator")]]
LIB_API void*	allocate_memory(allocate_info const& info);

// Releases memory from the system.
//[[deprecated("Every allocation needs to go through an allocator")]]
LIB_API void	release_memory(void* pointer);

// All allocators must be derived from this class.
// Allocators can not be:
// 1. Copy constructable & assignable.
// 2. Move constructable & assignable.
class allocator_base
{
public:
	using value_type	= std::byte;
	using pointer		= value_type*;
	using const_pointer = value_type const*;
	using void_pointer	= void*;
	using const_void_pointer = void const*;
	using difference_type = std::ptrdiff_t;
	using size_type		= size_t;

	constexpr allocator_base()	= default;
	constexpr ~allocator_base() = default;

	template <typename allocator_type>
	struct alignas(16) memory_header
	{
		allocator_type* allocator;
		size_t			size;
	};

	/**
	* This does not allocate, rather it returns the alligned address from the supplied pointer.
	*/
	void* aligned_alloc(void* pointer, size_t bytes, size_t alignment, size_t& allocated)
	{
		static auto aligned_pointer = [](void* ptr, size_t align) -> uint8*
			{
				const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
				const uintptr_t aligned = [](uintptr_t add, size_t al) -> uintptr_t
					{
						const size_t mask = al - 1;
						ASSERTION((al + mask) != 0);
						return(add + mask) & ~mask;
					}(address, align);

					return reinterpret_cast<uint8*>(aligned);
			};

		allocated += (bytes + alignment);
		uint8* ptr = aligned_pointer(pointer, alignment);
		ptrdiff_t shift = ptr - static_cast<uint8*>(pointer);

		ASSERTION(shift >= 0 && shift <= 256);

		ptr[-1] = static_cast<uint8>(shift & 0xff);

		return static_cast<void*>(ptr);
	}

	/**
	* This does not deallocate, rather it returns the root pointer that the block was allocated from.
	*/
	void* aligned_free(void* pointer)
	{
		uint8* block = static_cast<uint8*>(pointer);
		ptrdiff_t shift = block[-1];
		if (!shift)
		{
			shift = 256;
		}
		return block - shift;
	}

private:
	allocator_base(const allocator_base&)	= delete;
	allocator_base(allocator_base&&)		= delete;

	allocator_base& operator=(const allocator_base&)	= delete;
	allocator_base& operator=(allocator_base&&)			= delete;
};

struct default_allocator
{
	using value_type	= std::byte;
	using pointer		= value_type*;
	using const_pointer = value_type const*;
	using void_pointer	= void*;
	using const_void_pointer = void const*;
	using difference_type = std::ptrdiff_t;
	using size_type		= size_t;

	struct deleter
	{
		auto operator()(is_pointer auto const pointer) -> void
		{
			using type = std::remove_const_t<std::remove_pointer_t<decltype(pointer)>>;
			pointer->~type();
			default_allocator{}.deallocate(pointer);
		}
	};

	LIB_API auto allocate(size_t size, size_t alignment = alignof(std::max_align_t)) const -> void*;

	template <typename T, typename... Args>
	auto construct(T* p, Args&&... args) const -> void 
	{ 
		new (p) T{ std::forward<Args>(args)... }; 
	}

	auto destroy(is_pointer auto pointer) const -> void
	{
		using type = std::remove_pointer_t<decltype(pointer)>;
		pointer->~type();
	}

	LIB_API auto deallocate(void const* pointer) const -> void;
};

template <typename T>
concept provides_memory = std::same_as<T, default_allocator> /*|| can_allocate_memory<T>*/;

template <typename T>
struct default_delete
{
	constexpr default_delete() noexcept = default;

	constexpr void operator()(T* pointer) const
	{
		pointer->~T();
		release_memory(pointer);
	}
};

/**
* An extension to std::unique_ptr that integrates our memory tracking capabilities.
*/
template <typename T, typename deleter = default_delete<T>>
using unique_ptr = std::unique_ptr<T, deleter>;

/**
* References a unique_ptr or a pointer.
* Similar to std::shared_ptr without the reference counted control block.
* Unlike std::shared_ptr, Ref does not own the pointer or passes ownership. It merely references it.
*/
template <typename T>
class ref
{
private:
	using element_type = std::conditional_t<std::is_const_v<T>, const std::decay_t<T>, std::decay_t<T>>;
	element_type* m_data;

	constexpr void unreference() { m_data = nullptr; }
public:
	constexpr ref() : m_data{ nullptr } {}
	constexpr ref(element_type* data) : m_data{ data } {}
	constexpr ref(element_type& data) : ref{ &data } {}
	constexpr ref(unique_ptr<T>& pointer) : m_data{ pointer.get() } {}
	constexpr ~ref() { unreference(); }

	constexpr ref(ref const& rhs) : m_data{ rhs.m_data } {}
	constexpr ref(ref&& rhs) noexcept { *this = std::move(rhs); }

	constexpr ref& operator=(ref const& rhs) /*requires !std::is_const_v<element_type>*/
	{
		if (this != &rhs)
		{
			m_data = rhs.m_data;
		}
		return *this;
	}

	constexpr ref& operator=(ref&& rhs) noexcept
	{
		if (this != &rhs)
		{
			m_data = rhs.m_data;
			new (&rhs) ref{};
		}
		return *this;
	}

	constexpr element_type& operator*() const
	{
		ASSERTION(m_data != nullptr);
		return *m_data;
	}

	constexpr element_type* operator->() const
	{
		ASSERTION(m_data != nullptr);
		return m_data;
	}

	constexpr element_type*			get() const	{ return m_data; }

	constexpr bool	operator== (ref const& rhs) const { return m_data == rhs.m_data; }
	constexpr bool	operator== (std::nullptr_t) const { return m_data == nullptr; }

	constexpr bool	is_null() const { return m_data == nullptr; }
};

template <typename T>
using const_ref = ref<std::conditional_t<std::is_const_v<T>, T, T const>>;

template <typename T, typename... Arguments>
constexpr unique_ptr<T> make_unique(Arguments&&... args)
{
	T* pointer = static_cast<T*>(allocate_memory({ .size = sizeof(T) }));
	new (pointer) T{ std::forward<Arguments>(args)... };
	return unique_ptr<T>{ pointer };
}

// --- Allocator interfaces ---

template <typename allocator>
class allocator_memory_interface
{
public:
	using allocator_type = allocator;

	allocator_memory_interface(allocator_type& allocator) :
		m_allocator{ &allocator }
	{}

	void* allocate_storage(allocate_info const& info)
	{
		return m_allocator->allocate_memory(info);
	}

	template <typename T>
	T* allocate_storage(allocate_info const& info)
	{
		return static_cast<T*>(allocate_storage({ .size = info.size * sizeof(T), .alignment = info.alignment }));
	}

	template <is_pointer pointer_type>
	void free_storage(pointer_type pointer)
	{
		using element_type = std::remove_pointer_t<std::remove_cv_t<pointer_type>>;
		if (pointer)
		{
			m_allocator->release_memory(pointer);
		}
	}

	allocator_type* get_allocator() const
	{
		ASSERTION(m_allocator != nullptr);
		return m_allocator;
	}
private:
	allocator_type* m_allocator = nullptr;
};

struct system_memory_interface
{
	using allocator_type = default_allocator;

	void* allocate_storage(allocate_info const& info)
	{
		return allocate_memory(info);
	}

	template <typename T>
	T* allocate_storage(allocate_info const& info)
	{
		return static_cast<T*>(allocate_memory({ .size = info.size * sizeof(T), .alignment = info.alignment }));
	}

	template <is_pointer pointer_type>
	void free_storage(pointer_type pointer)
	{
		using element_type = std::remove_pointer_t<std::remove_cv_t<pointer_type>>;
		if (pointer)
		{
			release_memory(pointer);
		}
	}
};

template <typename allocator>
struct _conditional_allocator_base
{
	constexpr _conditional_allocator_base(allocator& alloc) :
		m_allocator{ &alloc }
	{}

	allocator* m_allocator;
};

template <>
struct _conditional_allocator_base<default_allocator>
{
	constexpr _conditional_allocator_base() = default;

	NO_UNIQUE_ADDRESS default_allocator m_allocator;
};

/**
 * Not the same type of Box from Rust.
 * Stores some value adjacent to an allocator.
 */
template <typename T, typename allocator = default_allocator>
class box : protected _conditional_allocator_base<allocator>
{
public:
	using type = T;
	using value_type	 = std::conditional_t<std::is_const_v<type>, std::remove_cvref_t<type>, std::decay_t<T>>;
	using allocator_type = std::decay_t<allocator>;

	constexpr box() = default;

	template <typename U = T>
	constexpr box(U&& resource) requires (std::same_as<allocator, default_allocator>) :
		_conditional_allocator_base<allocator>{},
		m_value{ std::forward<U>(resource) }
	{}

	template <typename U = T>
	constexpr box(U&& resource, allocator& alloc_) requires (!std::same_as<allocator, default_allocator>) :
		_conditional_allocator_base<allocator>{ alloc_ },
		m_value{ std::forward<U>(resource) }
	{}

	box(box const&) = delete;
	auto operator=(box const&) -> box& = delete;

	constexpr box(box&& rhs) :
		m_value{ std::move(rhs.m_value) }
	{
		if constexpr (!std::is_same_v<allocator, default_allocator>)
		{
			box::m_allocator = std::exchange(rhs.m_allocator, nullptr);
		}
	}

	constexpr auto operator=(box&& rhs) -> box&
	{
		if (this != &rhs)
		{
			~box();

			m_value = std::move(rhs.m_value);
			if constexpr (!std::is_same_v<allocator, default_allocator>)
			{
				box::m_allocator = std::exchange(rhs.m_allocator, nullptr);
			}
			new (&rhs) box{};
		}
		return *this;
	}

	constexpr ~box() = default;


	constexpr auto operator*() const -> value_type&
	{
		return m_value;
	}

	constexpr auto operator->() const -> value_type*
	{
		return &m_value;
	}

	constexpr auto value() const -> decltype(auto)
	{
		return m_value;
	}

	constexpr explicit operator allocator_type&()
	{
		if constexpr (std::is_same_v<allocator, default_allocator>)
		{
			return box::m_allocator;
		}
		else
		{
			return *box::m_allocator;
		}
	}

	constexpr explicit operator allocator_type const&() const
	{
		if constexpr (std::is_same_v<allocator, default_allocator>)
		{
			return box::m_allocator;
		}
		else
		{
			return *box::m_allocator;
		}
	}
private:
	value_type m_value;
};

template <typename T>
constexpr auto make_box(T&& obj) -> box<T>
{
	return box<T>{ std::forward<T>(obj) };
}

template <typename T, typename allocator_type>
constexpr auto make_box(T&& obj, allocator_type& allocator) -> box<T, allocator_type>
{
	return box<T, allocator_type>{ std::forward<T>(obj), allocator };
}

}

#endif LIB_MEMORY_H
