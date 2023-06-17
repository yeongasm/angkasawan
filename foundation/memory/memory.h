#pragma once
#ifndef FOUNDATION_MEMORY_H
#define FOUNDATION_MEMORY_H

#include <source_location>
#include "common.h"
#include "foundation_api.h"

#define BYTES(n) n
#define KILOBYTES(n) 1024 * BYTES(n)
#define MEGABYTES(n) 1024 * KILOBYTES(n)
#define GIGABYTES(n) 1024 * MEGABYTES(n)

constexpr inline size_t operator""_KiB(size_t n) { return 1024 * n; }
constexpr inline size_t operator""_MiB(size_t n) { return 1024_KiB * n; }
constexpr inline size_t operator""_GiB(size_t n) { return 1024_MiB * n; }

// ftl - Foundation template library
FTLBEGIN

extern FOUNDATION_API void		memmove	(void* dst, void* src, size_t size);
extern FOUNDATION_API void		memcopy	(void* dst, const void* src, size_t size);
extern FOUNDATION_API size_t	memcmp	(const void* src, const void* dst, size_t size);
extern FOUNDATION_API void		memset	(void* dst, uint8 val, size_t size);
extern FOUNDATION_API void		memzero	(void* dst, size_t size);

template <typename T>
void memswap(T* a, T* b)
{
	T temp{ *a };
	memcopy(a, b, sizeof(T));
	memcopy(b, &temp, sizeof(T));
}

extern FOUNDATION_API constexpr size_t	is_power_of_two		(size_t num);
extern FOUNDATION_API constexpr size_t	pad_address			(const uintptr_t address, const size_t alignment);

extern FOUNDATION_API bool is_64bit_aligned(void* pointer);

class SystemMemory;
class StackAllocator;

// All allocators must be derived from this class.
// Allocators can not be:
// 1. Copy constructable & assignable.
// 2. Move constructable & assignable.
class AllocatorBase
{
public:
	constexpr AllocatorBase() {};
	constexpr ~AllocatorBase() {};

	template <typename AllocatorType>
	struct alignas(16) MemoryHeader
	{
		AllocatorType*	m_allocator;
		size_t			m_size;
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
	AllocatorBase(const AllocatorBase&) = delete;
	AllocatorBase(AllocatorBase&&)		= delete;

	AllocatorBase& operator=(const AllocatorBase&)	= delete;
	AllocatorBase& operator=(AllocatorBase&&)		= delete;
};

template <typename Allocator_t>
class DeleterInterface
{
public:
	constexpr DeleterInterface(Allocator_t& allocator) : m_allocator{ allocator } {}
	constexpr ~DeleterInterface() {}
	void operator()(void* pointer) { m_allocator.release_memory(pointer); }
private:
	Allocator_t& m_allocator;
};

// Allocator concept.
// An allocator is an object that can call allocate_memory() and release_memory().
template <typename T>
concept CanAllocateMemory = requires(T v)
{
	{ v.allocate_memory(1) };
	{ v.release_memory(nullptr) };
};

template <typename T>
concept ProvidesMemory = CanAllocateMemory<T> || std::same_as<SystemMemory, T> || std::same_as<StackAllocator, T>;

// Allocates memory using the system default allocator.
extern FOUNDATION_API void* allocate_memory(size_t size, size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__, std::source_location location = std::source_location::current());

// Releases memory from the system default allocator.
extern FOUNDATION_API void	release_memory(void* pointer);

// Get total memory allocated system wide.
extern FOUNDATION_API size_t total_memory_allocated();

// Forward declare Ref class.
template <typename T> class Ref;


// Unique Pointer type. Equivalent to std::unique_ptr.
// NOTE: If made unique with an allocator, you MUST specify the allocator's type as the template argument.
template <typename T, ProvidesMemory Allocator = SystemMemory>
class UniquePtr
{
public:
	constexpr UniquePtr() : m_data{} {}

	constexpr UniquePtr(T* source) : m_data{ source } {}

	constexpr ~UniquePtr()
	{
		if (m_data)
		{
			m_data->~T();
			if constexpr (!std::is_same_v<Allocator, SystemMemory>)
			{
				Allocator* allocator = get_allocator(m_data);
				typename Allocator::Deleter deleter{ *allocator };
				deleter(m_data);
			}
			else
			{
				release_memory(m_data);
			}
			m_data = nullptr;
		}
	}

	constexpr UniquePtr(UniquePtr&& rhs) noexcept
	{
		*this = std::move(rhs);
	}

	constexpr UniquePtr& operator=(UniquePtr&& rhs) noexcept
	{
		if (this != &rhs)
		{
			m_data = rhs.m_data;
			new (&rhs) UniquePtr{};
		}
		return *this;
	}

	constexpr T&		operator*	() 			{ return *get_pointer(); }
	constexpr const T&	operator*	() const	{ return *get_pointer(); }

	constexpr T*		operator->	()			{ return  get_pointer(); }
	constexpr const T*	operator->	() const	{ return  get_pointer(); }

	constexpr bool operator==(const UniquePtr& rhs) const { return m_data == rhs.m_data; }
	constexpr bool operator!=(const UniquePtr& rhs) const { return m_data != rhs.m_data; }

	constexpr bool		is_null	() const	{ return m_data == nullptr; }
	constexpr T*		get		()			{ return get_pointer(); }
	constexpr const T*	get		() const	{ return get_pointer(); }
private:
	T* m_data;

	friend class Ref<T>;
			
	constexpr T* get_pointer()
	{
		ASSERTION(!is_null() && "Pointer is null!");
		return m_data;
	}

	constexpr const T* get_pointer() const
	{
		ASSERTION(!is_null() && "Pointer is null!");
		return m_data;
	}

	constexpr Allocator* get_allocator(void* pointer) requires !std::same_as<Allocator, void>
	{
		using MemoryHeader = typename Allocator::MemoryHeader;

		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(reinterpret_cast<uint8*>(pointer) - sizeof(MemoryHeader));
		Allocator* allocator = reinterpret_cast<Allocator*>(header->m_allocator);
		ASSERTION(allocator != nullptr && "Memory corruption!");
		return allocator;
	}

	constexpr UniquePtr(const UniquePtr&) = delete;
	constexpr UniquePtr& operator=(const UniquePtr&) = delete;
};

/**
* References a UniquePtr or a pointer.
* Similar to std::shared_ptr without the reference counted control block.
* Unlike std::shared_ptr, Ref does not own the pointer or passes ownership. It merely references it.
*/
template <typename T>
class Ref
{
private:
	using Type = std::conditional_t<std::is_const_v<T>, const std::decay_t<T>, std::decay_t<T>>;
	Type* m_data;

	constexpr void unreference() { m_data = nullptr; }
public:
	constexpr Ref() : m_data{ nullptr } {}
	constexpr Ref(Type* data) : m_data{ data } {}
	constexpr Ref(Type& data) : Ref{ &data } {}
	constexpr Ref(UniquePtr<T> pointer) : m_data{ pointer.m_data } {}
	constexpr ~Ref() { unreference(); }

	constexpr Ref(const Ref& rhs) : m_data{ rhs.m_data } {}
	constexpr Ref(Ref&& rhs) noexcept { *this = std::move(rhs); }

	constexpr Ref& operator=(const Ref& rhs) /*requires !std::is_const_v<Type>*/
	{
		if (this != &rhs)
		{
			m_data = rhs.m_data;
		}
		return *this;
	}

	constexpr Ref& operator=(Ref&& rhs) noexcept
	{
		if (this != &rhs)
		{
			m_data = rhs.m_data;
			new (&rhs) Ref{};
		}
		return *this;
	}

	//constexpr Type&			value()			{ return *m_data; }
	//constexpr const Type&	value() const	{ return *m_data; }

	//explicit constexpr operator Type&		()		 { return *m_data; }
	//explicit constexpr operator const Type&	() const { return *m_data; }

	constexpr Type&		operator*	() 
	{ 
		ASSERTION(m_data != nullptr); 
		return *m_data; 
	}
	constexpr Type*		operator->	() 
	{ 
		ASSERTION(m_data != nullptr); 
		return m_data; 
	}
	constexpr const Type&	operator*	() const 
	{ 
		ASSERTION(m_data != nullptr); 
		return *m_data; 
	}
	constexpr const Type*	operator->	() const 
	{ 
		ASSERTION(m_data != nullptr); 
		return m_data; 
	}

	constexpr Type*			get()		{ return m_data; }
	constexpr const Type*	get() const { return m_data; }

	constexpr bool		operator==	(const Ref& rhs) const { return m_data == rhs.m_data; }
	constexpr bool		operator!=	(const Ref& rhs) const { return m_data != rhs.m_data; }
	constexpr bool		operator!=	(std::nullptr_t) const { return m_data == nullptr; }
	constexpr bool		operator==	(std::nullptr_t) const { return m_data != nullptr; }

	constexpr bool		is_null() const { return m_data == nullptr; }
};

template <typename T>
inline constexpr Ref<T> null_ref{ nullptr };

template <typename T>
using ConstRef = Ref<std::remove_const_t<T> const>;

template <typename T, typename... Arguments>
constexpr UniquePtr<T> make_unique(Arguments&&... args)
{
	T* pointer = static_cast<T*>(allocate_memory(sizeof(T)));
	new (pointer) T{ std::forward<Arguments>(args)... };
	return UniquePtr<T>{ pointer };
}

template <typename T, std::derived_from<T> U, typename... Arguments>
constexpr UniquePtr<T> make_unique(Arguments&&... args)
{
	U* pointer = static_cast<U*>(allocate_memory(sizeof(U)));
	new (pointer) U{ std::forward<Arguments>(args)... };
	return UniquePtr<T>{ pointer };
}

// --- Allocator interfaces ---


template <typename T, typename AllocatorType>
class AllocatorMemoryInterface
{
public:
	using Type = AllocatorType;

	constexpr AllocatorMemoryInterface(AllocatorType* allocator) :
		m_allocator{ allocator }
	{}

	// Allocate memory for the type supplied to the FUNCTIONS's template argument.
	template <typename T>
	constexpr T* allocate_storage_for(size_t size, size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__)
	{
		return reinterpret_cast<T*>(allocate(size * sizeof(T), alignment));
	}

	// Allocate memory for the type supplied to the OBJECT's template argument.
	constexpr T* allocate_storage(size_t size, size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__)
	{
		return reinterpret_cast<T*>(allocate(size * sizeof(T), alignment));
	}

	template <typename T>
	constexpr void free_storage_for(T* pointer)
	{
		deallocate(pointer);
	}

	constexpr void free_storage(T* pointer)
	{
		deallocate(pointer);
	}

	constexpr void release_allocator	()		 { m_allocator = nullptr; }
	constexpr bool allocator_referenced	() const { return m_allocator != nullptr; }

	constexpr AllocatorType* get_allocator() const
	{
		ASSERTION(m_allocator != nullptr);
		return m_allocator;
	}
private:
	constexpr void* allocate(size_t size, size_t alignment)
	{
		return m_allocator->allocate_memory(size, alignment);
	}

	constexpr void deallocate(void* pointer)
	{
		if (pointer)
		{
			m_allocator->release_memory(pointer);
		}
	}

	AllocatorType* m_allocator = nullptr;
};


template <typename T>
struct SystemMemoryInterface
{
	using Type = SystemMemory;

	// Allocate memory for the type supplied to the FUNCTIONS's template argument.
	template <typename T>
	constexpr T* allocate_storage_for(size_t size, size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__, std::source_location location = std::source_location::current())
	{
		return reinterpret_cast<T*>(allocate_memory(size * sizeof(T), alignment, location));
	}

	// Allocate memory for the type supplied to the OBJECT's template argument.
	constexpr T* allocate_storage(size_t capacity, size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__, std::source_location location = std::source_location::current())
	{
		return reinterpret_cast<T*>(allocate_memory(capacity * sizeof(T), alignment, location));
	}

	template <typename T>
	constexpr void free_storage_for(T* pointer)
	{
		if (pointer)
		{
			release_memory(pointer);
		}
	}

	constexpr void free_storage(T* pointer)
	{
		if (pointer)
		{
			release_memory(pointer);
		}
	}
};

FTLEND

#endif FOUNDATION_MEMORY_H
