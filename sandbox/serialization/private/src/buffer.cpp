#include "buffer.hpp"

namespace core
{
namespace sbf
{
Buffer::~Buffer()
{
	free();
}

Buffer::Buffer(lib::memory_resource& memoryResource) :
	m_allocator{ &memoryResource },
	m_data{ nullptr },
	m_writtenBytes{},
	m_info{}
{}

Buffer::Buffer(BufferInfo const& info, lib::memory_resource& memoryResource) :
	m_allocator{ &memoryResource },
	m_data{ nullptr },
	m_writtenBytes{},
	m_info{ info }
{}

auto Buffer::write(void const* data, size_t sizeBytes) -> bool
{
	if (m_data == nullptr && !_allocate_memory())
	{
		return false;
	}

	size_t const byteOffsetAfterWrite = m_writtenBytes + sizeBytes;

	if (m_data != nullptr && 
		(byteOffsetAfterWrite <= m_info.blockCapacity))
	{
		std::memcpy(m_data + m_writtenBytes, data, sizeBytes);
		m_writtenBytes += sizeBytes;
	}

	return byteOffsetAfterWrite < m_info.blockCapacity;
}

auto Buffer::data() -> void_pointer
{
	return m_data;
}

auto Buffer::data() const -> const_void_pointer
{
	return m_data;
}

auto Buffer::current_byte() -> void_pointer
{
	return m_data + m_writtenBytes;
}

auto Buffer::current_byte() const -> const_void_pointer
{
	return m_data + m_writtenBytes;
}

auto Buffer::clear() -> void
{
	m_writtenBytes = static_cast<size_t>(0);
}

auto Buffer::byte_offset() const -> size_type
{
	return m_writtenBytes;
}

auto Buffer::capacity() const -> size_type
{
	return m_info.blockCapacity;
}

auto Buffer::remaining_capacity() const -> size_type
{
	return m_info.blockCapacity - m_writtenBytes;
}

auto Buffer::free() -> void
{
	if (m_data != nullptr)
	{
		m_allocator.deallocate_bytes(m_data, m_info.blockCapacity, alignof(value_type));
	}
}

auto Buffer::_allocate_memory() -> bool
{
	m_data = static_cast<pointer>(m_allocator.allocate_bytes(m_info.blockCapacity, alignof(value_type)));
	if (!m_data)
	{
		// TODO(afiq):
		// Call assert system.
	}
	return m_data != nullptr;
}

MonotonicBuffer::MonotonicBuffer(lib::memory_resource& memoryResource) :
	m_allocator{ &memoryResource },
	m_head{},
	m_tail{},
	m_info{}
{}

MonotonicBuffer::MonotonicBuffer(BufferInfo const& info, lib::memory_resource& memoryResource) :
	m_allocator{ &memoryResource },
	m_head{},
	m_tail{},
	m_info{ info }
{}

MonotonicBuffer::~MonotonicBuffer()
{
	free();
}

auto MonotonicBuffer::write(void const* data, size_t sizeBytes) -> bool
{
	std::byte const* p = static_cast<std::byte const*>(data);

	do
	{
		if (!_try_allocate_new_block())
		{
			return false;
		}

		size_t const bytesToWrite = std::min(sizeBytes, m_tail->buffer.capacity() - m_tail->buffer.byte_offset());
		
		m_tail->buffer.write(p, bytesToWrite);

		p += bytesToWrite;

		sizeBytes -= bytesToWrite;
	}
	while (std::cmp_not_equal(sizeBytes, 0));

	return true;
}

auto MonotonicBuffer::clear() -> void
{
	BufferNode* node = m_head;
	while (node)
	{
		node->buffer.clear();
		node = node->next;
	} 
}

auto MonotonicBuffer::byte_offset() const -> size_type
{
	size_type totalByteOffset = 0;
	BufferNode* node = m_head;

	while (node != nullptr)
	{
		totalByteOffset += node->buffer.byte_offset();

		node = node->next;
	}

	return totalByteOffset;
}

auto MonotonicBuffer::block_count() const -> size_type
{
	size_type count = 0;
	BufferNode* node = m_head;
	while (node)
	{
		++count;
		node = node->next;
	}
	return count;
}

auto MonotonicBuffer::free() -> void
{
	BufferNode* node = m_head;
	while (node)
	{
		node->buffer.free();
	}
}

auto MonotonicBuffer::_try_allocate_new_block() -> bool
{
	if ((m_head == nullptr) && (m_tail == nullptr) || 
		(m_tail != nullptr) && std::cmp_equal(m_tail->buffer.byte_offset() - m_tail->buffer.capacity(), 0))
	{
		if (!_allocate_new_block())
		{
			return false;
		}
	}

	return true;
}

auto MonotonicBuffer::_allocate_new_block() -> bool
{
	auto node = m_allocator.allocate_object<BufferNode>();

	if (node == nullptr)
	{
		return false;
	}

	new (node) BufferNode{};

	m_tail->next = node;
	m_tail = node;

	return true;
}

}
}