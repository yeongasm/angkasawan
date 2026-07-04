#ifndef SERIALIZATION_BUFFER_HPP
#define SERIALIZATION_BUFFER_HPP

#include <span>
#include "lib/memory.hpp"

namespace core
{
namespace sbf
{
struct BufferInfo
{
	size_t blockCapacity = 4_KiB;
};

/**
* 
*/
class Buffer : lib::non_copyable
{
public:
	using value_type		= std::byte;
	using allocator_type	= lib::allocator<value_type>;
	using void_type			= void;
	using size_type			= size_t;
	using difference_type	= ptrdiff_t;
	using pointer			= value_type*;
	using const_pointer		= value_type const*;
	using void_pointer		= void*;
	using const_void_pointer = void const*;

	Buffer()	= default;
	~Buffer();

	Buffer(lib::memory_resource& memoryResource);
	Buffer(BufferInfo const& info, lib::memory_resource& memoryResource);

	template <typename T>
	auto write(T const& data) -> bool requires (std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>)
	{
		return write(&data, sizeof(T));
	}

	template <typename T>
	auto write(std::span<T> r) -> bool requires (std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>)
	{
		return write(r.data(), r.size_bytes());
	}

	auto write(void const* data, size_t sizeBytes) -> bool;

	template <typename T>
	auto fill(T const& val, size_t count) -> bool requires (std::is_standard_layout_v<T>&& std::is_trivially_copyable_v<T>)
	{
		if (m_data == nullptr && !_allocate_memory())
		{
			return false;
		}

		size_t const bytesRequired = sizeof(T) * count;
		size_t const byteOffsetAfterWrite = m_writtenBytes + bytesRequired;

		if (byteOffsetAfterWrite < m_info.blockCapacity)
		{
			T* begin	= reinterpret_cast<T*>(m_data + m_writtenBytes);
			T* end		= reinterpret_cast<T*>(m_data + (m_writtenBytes + bytesRequired));

			std::fill(begin, end, val);

			m_writtenBytes += bytesRequired;
		}

		return byteOffsetAfterWrite < m_info.blockCapacity;
	}

	template <typename T>
	auto reserve_for(size_t count) -> std::span<T> requires (std::is_standard_layout_v<T>&& std::is_trivially_copyable_v<T>)
	{
		if (m_data == nullptr && !_allocate_memory())
		{
			return false;
		}

		size_t const bytesRequired = sizeof(T) * count;
		size_t const byteOffsetAfterWrite = m_writtenBytes + bytesRequired;

		T* ptr = reinterpret_cast<T*>(m_data + m_writtenBytes);

		return (byteOffsetAfterWrite < m_info.blockCapacity) ? std::span{ ptr, count } : std::span<T>{};
	}

	auto data() -> void_pointer;
	auto data() const -> const_void_pointer;
	auto current_byte() -> void_pointer;
	auto current_byte() const -> const_void_pointer;
	auto clear() -> void;
	auto byte_offset() const -> size_type;
	auto capacity() const -> size_type;
	auto remaining_capacity() const -> size_type;
	auto free() -> void;

private:
	lib::allocator<value_type> m_allocator = {};
	pointer m_data			= {};
	size_t m_writtenBytes	= {};
	BufferInfo m_info		= {};

	auto _allocate_memory() -> bool;
};

class MonotonicBuffer : lib::non_copyable
{
public:
	using value_type		= std::byte;
	using allocator_type	= lib::allocator<value_type>;
	using void_type			= void;
	using size_type			= size_t;
	using difference_type	= ptrdiff_t;
	using pointer			= value_type*;
	using const_pointer		= value_type const*;
	using void_pointer		= void*;

	MonotonicBuffer() = default;
	~MonotonicBuffer();

	MonotonicBuffer(lib::memory_resource& memoryResource);
	MonotonicBuffer(BufferInfo const& info, lib::memory_resource& memoryResource);

	template <typename T>
	auto write(T const& data) -> bool requires (std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>)
	{
		return write(&data, sizeof(T));
	}

	template <typename T>
	auto write(std::span<T> r) -> bool requires (std::is_standard_layout_v<T>&& std::is_trivially_copyable_v<T>)
	{
		return write(r.data(), r.size_bytes());
	}

	auto write(void const* data, size_t sizeBytes) -> bool;
	auto clear() -> void;
	auto byte_offset() const -> size_type;
	auto block_count() const -> size_type;
	auto free() -> void;

private:
	struct BufferNode
	{
		Buffer buffer;
		BufferNode* next;
	};

	lib::allocator<value_type> m_allocator = {};
	BufferNode* m_head = {};
	BufferNode* m_tail = {};
	BufferInfo	m_info = {};

	auto _try_allocate_new_block() -> bool;
	auto _allocate_new_block() -> bool;
};
}
}

#endif // !SERIALIZATION_BUFFER_HPP
