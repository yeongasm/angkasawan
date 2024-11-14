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
	using pointer		= void*;
	using const_pointer = void const*;
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
	using difference_type = ptrdiff_t;
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

	template <typename T>
	class bind_to
	{
	public:
		using value_type	= std::decay_t<T>;
		using pointer		= value_type*;
		using const_pointer = value_type const*;
		using void_pointer	= void*;
		using const_void_pointer = void const*;
		using difference_type = ptrdiff_t;
		using size_type		= size_t;

		bind_to([[maybe_unused]] default_allocator&) :
			m_allocator{}
		{};

		~bind_to() = default;

		auto allocate(size_t count) const -> pointer
		{
			return static_cast<pointer>(m_allocator.allocate(sizeof(value_type) * count, alignof(value_type)));
		}

		auto deallocate(is_pointer auto const* pointer, [[maybe_unused]] size_t) const -> void
		{
			m_allocator.deallocate(pointer);
		}

	private:
		NO_UNIQUE_ADDRESS default_allocator m_allocator;
	};

	template <typename T>
	struct rebind
	{
		using other = bind_to<T>;
	};

	LIB_API auto allocate(size_t size, size_t alignment = alignof(std::max_align_t)) const -> void*;
	LIB_API auto allocate_bytes(size_t bytes, size_t alignment = alignof(std::max_align_t)) const -> void*;
	LIB_API auto deallocate(void const* pointer) const -> void;
	LIB_API auto deallocate(void const* pointer, [[maybe_unused]] size_t) const -> void;
	LIB_API auto deallocate_bytes(void const* pointer, [[maybe_unused]] size_t, [[maybe_unused]] size_t) const -> void;
};

template <typename T>
concept can_allocate_memory = requires (T allocator, size_t sz)
{
	{ allocator.allocate(sz) } -> std::same_as<void*>;
};

template <typename T>
concept can_deallocate_memory = requires (T allocator)
{
	allocator.deallocate(nullptr);
};

template <typename T>
concept provides_memory = can_allocate_memory<T> && can_deallocate_memory<T>;

template <provides_memory AllocatorType, typename T = std::byte>
struct allocator_bind
{
	using allocator_type		= AllocatorType;
	using value_type			= T;
	using pointer				= value_type*;
	using const_pointer			= value_type const*;
	using void_pointer			= void*;
	using const_void_pointer	= void*;
	using difference_type		= ptrdiff_t;
	using size_type				= size_t;

	template <typename Other>
	using rebind = allocator_bind<AllocatorType, Other>;

	static constexpr auto allocate(allocator_type const& allocator, size_t count = 1, size_t alignment = alignof(value_type)) -> pointer
	{
		return static_cast<pointer>(allocator.allocate(sizeof(value_type) * count, alignment));
	}

	static constexpr auto deallocate(allocator_type const& allocator, is_pointer auto const pointer) -> void
	{
		allocator.deallocate(pointer);
	}

	static constexpr auto deallocate(allocator_type const& allocator, is_pointer auto const pointer, [[maybe_unused]] size_t) -> void
	{
		allocator.deallocate(pointer);
	}

	template <typename... Args>
	static constexpr auto construct(pointer p, Args&&... args) -> void
	{
		new (p) T{ std::forward<Args>(args)... };
	}

	static constexpr auto destroy(is_pointer auto const pointer) -> void
	{
		using type = std::remove_pointer_t<std::decay_t<decltype(pointer)>>;
		pointer->~type();
	}
};

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

template <typename allocator>
struct _conditional_allocator_base
{
	constexpr _conditional_allocator_base(allocator& alloc) :
		allocator{ &alloc }
	{}

	allocator* allocator;
};

template <>
struct _conditional_allocator_base<default_allocator>
{
	constexpr _conditional_allocator_base() = default;

	NO_UNIQUE_ADDRESS default_allocator allocator;
};

/**
 * Not the same type of Box from Rust.
 * Stores some value adjacent to an allocator.
 */
template <typename T, typename allocator = default_allocator>
struct box : public _conditional_allocator_base<allocator>
{
	using type = T;
	using value_type	 = std::conditional_t<std::is_const_v<type>, std::remove_cvref_t<type>, std::decay_t<T>>;
	using allocator_type = std::decay_t<allocator>;

	value_type data;

	constexpr box() = default;

	template <typename U = T>
	constexpr box(U&& resource) requires (std::same_as<allocator, default_allocator>) :
		_conditional_allocator_base<allocator>{},
		data{ std::forward<U>(resource) }
	{}

	template <typename U = T>
	constexpr box(U&& resource, allocator& alloc_) requires (!std::same_as<allocator, default_allocator>) :
		_conditional_allocator_base<allocator>{ alloc_ },
		data{ std::forward<U>(resource) }
	{}

	box(box const&) = delete;
	auto operator=(box const&) -> box& = delete;

	constexpr box(box&& rhs) :
		data{ std::move(rhs.data) }
	{
		if constexpr (!std::is_same_v<allocator, default_allocator>)
		{
			box::allocator = std::exchange(rhs.allocator, nullptr);
		}
		new (&rhs) box{};
	}

	constexpr auto operator=(box&& rhs) -> box&
	{
		if (this != &rhs)
		{
			this->~box();

			data = std::move(rhs.data);
			if constexpr (!std::is_same_v<allocator, default_allocator>)
			{
				box::allocator = std::exchange(rhs.allocator, nullptr);
			}
			new (&rhs) box{};
		}
		return *this;
	}

	constexpr ~box() = default;


	constexpr auto operator*() const -> value_type const&
	{
		return data;
	}

	constexpr auto operator*() -> value_type&
	{
		return data;
	}

	constexpr auto operator->() const -> value_type const*
	{
		return &data;
	}

	constexpr auto operator->() -> value_type*
	{
		return &data;
	}

	constexpr explicit operator allocator_type&()
	{
		if constexpr (std::is_same_v<allocator, default_allocator>)
		{
			return box::allocator;
		}
		else
		{
			return *box::allocator;
		}
	}

	constexpr explicit operator allocator_type const&() const
	{
		if constexpr (std::is_same_v<allocator, default_allocator>)
		{
			return box::allocator;
		}
		else
		{
			return *box::allocator;
		}
	}
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
