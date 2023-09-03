#pragma once
#ifndef MEMORY_H
#define MEMORY_H

#include <source_location>
#include "common.h"
#include "concepts.h"
#include "api.h"

constexpr inline size_t operator""_KiB(size_t n) { return 1024 * n; }
constexpr inline size_t operator""_MiB(size_t n) { return 1024_KiB * n; }
constexpr inline size_t operator""_GiB(size_t n) { return 1024_MiB * n; }

namespace lib
{

// Allocator concept.
// An allocator is an object that can call allocate_memory() and release_memory().
template <typename T>
concept can_allocate_memory = requires(T v)
{
	{ v.allocate_memory(1) };
	{ v.release_memory(nullptr) };
};

template <typename T>
concept provides_memory = can_allocate_memory<T> || std::same_as<class system_memory, T>;

LIB_API void	memmove(void* dst, void* src, size_t size);
LIB_API void	memcopy(void* dst, const void* src, size_t size);
LIB_API size_t	memcmp(const void* src, const void* dst, size_t size);
LIB_API void	memset(void* dst, uint8 val, size_t size);
LIB_API void	memzero(void* dst, size_t size);

template <typename T>
void memswap(T* a, T* b)
{
	T temp{ *a };
	memcopy(a, b, sizeof(T));
	memcopy(b, &temp, sizeof(T));
}

LIB_API constexpr size_t	is_power_of_two(size_t num);
LIB_API constexpr size_t	pad_address(const uintptr_t address, const size_t alignment);

LIB_API bool is_64bit_aligned(void* pointer);

// All allocators must be derived from this class.
// Allocators can not be:
// 1. Copy constructable & assignable.
// 2. Move constructable & assignable.
class allocator_base
{
public:
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

/*template <typename allocator_type>
class deleter_interface
{
public:
	constexpr deleter_interface(allocator_type& allocator) : m_allocator{ allocator } {}
	constexpr ~deleter_interface() {}
	void operator()(void* pointer) { m_allocator.release_memory(pointer); }
private:
	allocator_type& m_allocator;
};*/

struct allocate_info
{
	size_t size;
	size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
	std::source_location location;
};

template <typename T>
struct allocate_type_info
{
	size_t count		= 1;
	size_t alignment	= __STDCPP_DEFAULT_NEW_ALIGNMENT__;
	std::source_location location;
};

// Allocates memory using the system default allocator.
LIB_API void*	allocate_memory(allocate_info const& info);

// Releases memory from the system default allocator.
LIB_API void	release_memory(void* pointer);

// Get total memory allocated system wide.
LIB_API size_t	total_memory_allocated();

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

// Unique Pointer type. Equivalent to std::unique_ptr.
// NOTE: If made unique with an allocator, you MUST specify the allocator's type as the template argument.
//template <typename T, provides_memory allocator_type = class system_memory>
/*template <typename T, typename deleter = default_deleter<T>>
class unique_ptr
{
public:
	using pointer = T*;
	using element_type = T;
	using deleter_type = deleter;

	constexpr unique_ptr() : m_data{}, m_deleter{} {}

	constexpr unique_ptr(T* source) : m_data{ source }, m_deleter{} {}
	constexpr unique_ptr(T* source, deleter_type const& deleter) : m_data{ source }, m_deleter{ deleter } {}

	constexpr ~unique_ptr()
	{
		if (m_data)
		{
			m_deleter(m_data);
			m_data = nullptr;
		}
	}

	constexpr unique_ptr(unique_ptr&& rhs) noexcept
	{
		*this = std::move(rhs);
	}

	constexpr unique_ptr& operator=(unique_ptr&& rhs) noexcept
	{
		if (this != &rhs)
		{
			m_data = rhs.m_data;
			new (&rhs) unique_ptr{};
		}
		return *this;
	}

	constexpr T&		operator*	()			{ return *get_pointer(); }
	constexpr const T&	operator*	() const	{ return *get_pointer(); }

	constexpr T*		operator->	()			{ return  get_pointer(); }
	constexpr const T*	operator->	() const	{ return  get_pointer(); }

	constexpr bool operator==(const unique_ptr& rhs) const { return m_data == rhs.m_data; }

	constexpr bool		is_null()	const	{ return m_data == nullptr; }
	constexpr T*		get()				{ return get_pointer(); }
	constexpr const T*	get()		const	{ return get_pointer(); }
private:
	pointer*		m_data;
	deleter_type	m_deleter;

	friend class ref<T>;

	constexpr pointer* get_pointer()
	{
		ASSERTION(!is_null() && "Pointer is null!");
		return m_data;
	}

	constexpr const pointer* get_pointer() const
	{
		ASSERTION(!is_null() && "Pointer is null!");
		return m_data;
	}

	//constexpr allocator_type* get_allocator(void* pointer) requires !std::same_as<allocator_type, class system_memory>
	//{
	//	using memory_header = typename allocator_type::memory_header;

	//	memory_header* header = reinterpret_cast<memory_header*>(reinterpret_cast<uint8*>(pointer) - sizeof(memory_header));
	//	allocator_type* allocator = reinterpret_cast<allocator_type*>(header->m_allocator);
	//	ASSERTION(allocator != nullptr && "Memory corruption!");
	//	return allocator;
	//}

	constexpr unique_ptr(const unique_ptr&) = delete;
	constexpr unique_ptr& operator=(const unique_ptr&) = delete;
};*/

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
	constexpr ref(std::unique_ptr<T> pointer) : m_data{ pointer.m_data } {}
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

	constexpr element_type& operator*()
	{
		ASSERTION(m_data != nullptr);
		return *m_data;
	}
	constexpr element_type* operator->()
	{
		ASSERTION(m_data != nullptr);
		return m_data;
	}
	constexpr element_type const& operator*() const
	{
		ASSERTION(m_data != nullptr);
		return *m_data;
	}
	constexpr element_type const* operator->() const
	{
		ASSERTION(m_data != nullptr);
		return m_data;
	}

	constexpr element_type*			get()		{ return m_data; }
	constexpr element_type const*	get() const { return m_data; }

	constexpr bool	operator== (ref const& rhs) const { return m_data == rhs.m_data; }
	constexpr bool	operator== (std::nullptr_t) const { return m_data == nullptr; }

	constexpr bool	is_null() const { return m_data == nullptr; }
};

template <typename T>
using const_ref = ref<std::remove_const_t<T> const>;

template <typename T, typename... Arguments>
constexpr unique_ptr<T> make_unique(Arguments&&... args)
{
	T* pointer = static_cast<T*>(allocate_memory(sizeof(T)));
	new (pointer) T{ std::forward<Arguments>(args)... };
	return unique_ptr<T>{ pointer };
}

//template <typename T, std::derived_from<T> U, typename... Arguments>
//constexpr unique_ptr<T> make_unique(Arguments&&... args)
//{
//	U* pointer = static_cast<U*>(allocate_memory(sizeof(U)));
//	new (pointer) U{ std::forward<Arguments>(args)... };
//	return unique_ptr<T>{ pointer };
//}

// --- Allocator interfaces ---

template <can_allocate_memory allocator>
class allocator_memory_interface
{
public:
	using allocator_type = allocator;

	constexpr allocator_memory_interface(allocator_type& allocator) :
		m_allocator{ &allocator }
	{}

	constexpr void* allocate_storage(allocate_info const& info)
	{
		return m_allocator->allocate_memory(info);
	}

	template <typename T>
	constexpr T* allocate_storage(allocate_type_info<T> const& info)
	{
		return static_cast<T*>(allocate_storage({ .size = info.count * sizeof(T), .alignemnt = info.alignment, .location = info.location }));
	}

	template <is_pointer pointer_type>
	constexpr void free_storage(pointer_type pointer)
	{
		using element_type = std::remove_pointer_t<std::remove_cv_t<pointer_type>>;
		if (pointer)
		{
			pointer->~element_type();
			m_allocator->release_memory(pointer);
		}
	}

	constexpr allocator_type* get_allocator() const
	{
		ASSERTION(m_allocator != nullptr);
		return m_allocator;
	}
private:
	allocator_type* m_allocator = nullptr;
};


struct system_memory_interface
{
	using allocator_type = system_memory;

	constexpr void* allocate_storage(allocate_info const& info)
	{
		return allocate_memory(info);
	}

	template <typename T>
	constexpr T* allocate_storage(allocate_type_info<T> const& info)
	{
		return static_cast<T*>(allocate_memory({ .size = info.count * sizeof(T), .alignment = info.alignment, .location = info.location }));
	}

	template <is_pointer pointer_type>
	constexpr void free_storage(pointer_type pointer)
	{
		using element_type = std::remove_pointer_t<std::remove_cv_t<pointer_type>>;
		if (pointer)
		{
			pointer->~element_type();
			release_memory(pointer);
		}
	}
};

}

#endif MEMORY_H
