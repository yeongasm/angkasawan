#include "read_stream.hpp"

namespace core
{
namespace sbf
{
ReadStream::ReadStream(File& file) :
	m_data{ file.data() },
	m_size{ file.mapped_size() }
{}

ReadStream::~ReadStream()
{
	m_data = nullptr;
	m_offset = {};
}

auto ReadStream::flush() -> void
{
	m_offset = 0;
}

auto ReadStream::size() const -> size_t
{
	return m_size;
}

auto ReadStream::read(void* data, size_t sizeBytes) -> bool
{
	if (!m_data	||
		m_offset + sizeBytes > m_size)
	{
		return false;
	}

	std::byte* cursor = static_cast<std::byte*>(m_data);

	std::memcpy(data, cursor + m_offset, sizeBytes);

	m_offset += sizeBytes;

	return true;
}
}
}