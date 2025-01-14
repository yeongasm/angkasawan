#pragma once
#ifndef CORE_SERIALIZATION_READ_STREAM_HPP
#define CORE_SERIALIZATION_READ_STREAM_HPP

#include <span>
#include "file.hpp"

namespace core
{
namespace sbf
{
class ReadStream : public lib::non_copyable_non_movable
{
public:
	ReadStream() = default;
	ReadStream(File& file);

	~ReadStream();

	auto flush() -> void;
	auto size() const -> size_t;
	auto read(void* data, size_t sizeBytes) -> bool;

	template <typename T>
	requires (std::is_standard_layout_v<T>&& std::is_trivially_copyable_v<T>)
	auto read(T& obj) -> bool
	{
		return read(&obj, sizeof(T));
	}

	template <typename T>
	requires (std::is_standard_layout_v<T>&& std::is_trivially_copyable_v<T>)
	auto read(std::span<T> range) -> bool
	{
		return read(range.data(), range.size_bytes());
	}
private:
	void*	m_data = {};
	size_t	m_size = {};
	size_t	m_offset = {};
};
}
}

#endif // !CORE_SERIALIZATION_READ_STREAM_HPP
