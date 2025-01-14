#ifndef SERIALIZATION_SBF_WRITER_HPP
#define SERIALIZATION_SBF_WRITER_HPP

#include <filesystem>

#include "buffer.hpp"

namespace core
{
namespace sbf
{
/**
* Wrapper around std::ofstream.
*/
class WriteStream : lib::non_copyable_non_movable
{
public:
	WriteStream(std::filesystem::path const& filename);
	~WriteStream();

	auto open(std::filesystem::path const& filename) -> bool;

	auto write(Buffer const& input) -> bool;
	auto write(void const* data, size_t sizeBytes) -> bool;
	
	template <typename T>
	requires (std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>)
	auto write(T const& obj) -> bool
	{
		return write(&obj, sizeof(T));
	};

	template <typename T>
	requires (std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>)
	auto write(std::span<T> range) -> bool
	{
		return write(range.data(), range.size_bytes());
	};

	auto close() -> void;
private:
	void* m_nativeFileHandle = nullptr;
};
}
}

#endif // !SERIALIZATION_SBF_WRITER_HPP
