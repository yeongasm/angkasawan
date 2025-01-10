#pragma once
#ifndef LIB_MEMORY_HPP
#define LIB_MEMORY_HPP

#include <bit>
#include "common.hpp"
#include "concepts.hpp"
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

class memory_resource : non_copyable_non_movable
{
public:
	virtual ~memory_resource() noexcept = default;

	auto allocate(size_t size, size_t alignment = alignof(std::max_align_t)) -> void*
	{
		return do_allocate(size, alignment);
	}

	auto deallocate(void* p, size_t bytes, size_t alignment = alignof(std::max_align_t)) -> void
	{
		return do_deallocate(p, bytes, alignment);
	}

	auto is_equal(memory_resource const& other) -> bool
	{
		return do_is_equal(other);
	}

private:
	virtual auto do_allocate(size_t size, size_t alignment) -> void* = 0;
	virtual auto do_deallocate(void* p, size_t bytes, size_t alignment) -> void = 0;
	virtual auto do_is_equal(memory_resource const& other) -> bool = 0;
};

class default_memory_resource : public memory_resource
{
private:
	LIB_API virtual auto do_allocate(size_t size, size_t alignment) -> void* override;
	LIB_API virtual auto do_deallocate(void* p, size_t bytes, size_t alignment) -> void override;
	LIB_API virtual auto do_is_equal(memory_resource const& other) -> bool override;
};

LIB_API auto get_default_resource() noexcept -> memory_resource*;

template <typename T>
concept can_allocate_memory = requires (T allocator)
{
	{ allocator.allocate(0) } -> std::same_as<typename T::pointer>;
};

template <typename T>
concept can_deallocate_memory = requires (T allocator)
{
	{ allocator.deallocate(nullptr, 0) } -> std::same_as<void>;
};

template <typename T>
concept provides_memory = can_allocate_memory<T> && can_deallocate_memory<T>;

template <typename T = std::byte>
class allocator : not_copy_assignable
{
public:
	using value_type = T;
	using pointer = value_type*;
	using const_pointer = value_type const*;
	using void_pointer = void*;
	using const_void_pointer = void const*;
	using difference_type = ptrdiff_t;
	using size_type = size_t;

	template <typename U>
	struct rebind
	{
		using other = allocator<U>;
	};

	allocator() noexcept = default;
	~allocator() = default;

	allocator(allocator const& other) :
		m_memoryResource{ other.m_memoryResource }
	{}

	template <typename U>
	allocator(allocator<U> const& other) :
		m_memoryResource{ other.m_memoryResource }
	{}

	allocator(memory_resource* resource) :
		m_memoryResource{ resource }
	{}

	template <typename A>
	auto operator==(allocator<A> const& rhs) const -> bool
	{
		return m_memoryResource == rhs.m_memoryResource;
	}

	friend auto operator==(allocator const& lhs, allocator const& rhs) -> bool
	{
		return lhs.m_memoryResource == rhs.m_memoryResource;
	}

	auto allocate(size_t n = 1) -> pointer
	{
		return static_cast<pointer>(m_memoryResource->allocate(sizeof(value_type) * n));
	}

	auto deallocate(pointer p, size_t n = 1) -> void
	{
		return m_memoryResource->deallocate(p, sizeof(value_type) * n);
	}

	template <typename U, typename... Args>
	auto construct(U* p, Args&&... args) -> void
	{
		new (p) std::decay_t<U>{ std::forward<Args>(args)... };
	}

	template <typename U>
	auto destroy(U* p) -> void
	{
		p->~U();
	};

	auto allocate_bytes(size_t nbytes, size_t alignment = alignof(std::max_align_t)) -> void*
	{
		return m_memoryResource->allocate(nbytes, alignment);
	}

	auto deallocate_bytes(void* p, size_t nbytes, size_t alignment = alignof(std::max_align_t)) -> void
	{
		m_memoryResource->deallocate(p, nbytes, alignment);
	}

	template <typename U>
	auto allocate_object(size_t n = 1) -> U*
	{
		return static_cast<U*>(m_memoryResource->allocate(sizeof(U) * n, alignof(U)));
	}

	template <typename U>
	auto deallocate_object(U* p, size_t n = 1) -> void
	{
		m_memoryResource->deallocate(p, sizeof(U) * n, alignof(U));
	}

	auto select_on_container_copy_construction() const -> allocator
	{
		return allocator{};
	}

	auto resource() const -> memory_resource* { return m_memoryResource; }

private:
	memory_resource* m_memoryResource = get_default_resource();
};

// All allocators must be derived from this class.
// Allocators can not be:
// 1. Copy constructable & assignable.
// 2. Move constructable & assignable.
class allocator_base : non_copyable_non_movable
{
public:
	using value_type = std::byte;
	using pointer = void*;
	using const_pointer = void const*;
	using void_pointer = void*;
	using const_void_pointer = void const*;
	using difference_type = std::ptrdiff_t;
	using size_type = size_t;

	constexpr allocator_base() = default;
	constexpr ~allocator_base() = default;

protected:

	struct memory_header
	{
		memory_resource* pAllocator;
		size_t bytesAllocated;
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
};

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
public:
	constexpr ref() : m_data{ nullptr } {}
	constexpr ref(element_type* data) : m_data{ data } {}
	constexpr ref(element_type& data) : ref{ &data } {}
	constexpr ref(std::unique_ptr<T> const& pointer) : m_data{ pointer.get() } {}
	constexpr ~ref() { m_data = nullptr; }

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

	constexpr element_type*	get() const	{ return m_data; }

	constexpr bool	operator== (ref const& rhs) const { return m_data == rhs.m_data; }
	constexpr bool	operator== (std::nullptr_t) const { return m_data == nullptr; }
};

template <typename T>
using const_ref = ref<std::conditional_t<!std::is_const_v<T>, T, T const>>;

/**
* Not the same type of Box from Rust.
* It is a compressed pair that stores some value adjacent to an allocator.
*/
template <typename T, provides_memory allocator = allocator<std::decay_t<T>>>
struct box : public allocator, non_copyable
{
	using value_type	 = std::decay_t<T>;
	using allocator_type = allocator;

	value_type data = {};

	constexpr box()	= default;

	template <typename U = T>
	constexpr box(U&& resource, const allocator_type& alloc_ = allocator_type{}) :
		allocator_type{ alloc_ },
		data{ std::forward<U>(resource) }
	{}

	constexpr box(box&& rhs) noexcept :
		data{ std::move(rhs.data) }
	{}

	constexpr auto operator=(box&& rhs) noexcept -> box&
	{
		if (this != &rhs)
		{
			this->~box();

			data = std::move(rhs.data);
		}
		return *this;
	}

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
};

template <typename T>
constexpr auto make_box(T&& obj, allocator<std::decay_t<T>> const& allocator_ = allocator<std::decay_t<T>>{}) -> box<T>
{
	return box<T>{ std::forward<T>(obj), allocator_ };
}
}

#endif LIB_MEMORY_HPP
