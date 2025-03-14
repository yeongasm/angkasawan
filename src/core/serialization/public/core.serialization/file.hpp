#pragma once
#ifndef CORE_SERIALIZATION_FILE_HPP
#define CORE_SERIALIZATION_FILE_HPP

#include <filesystem>
#include "lib/common.hpp"

namespace core
{
namespace sbf
{
struct FileMemoryMapInfo
{
	size_t offset = 0;
	size_t size = 0;	/*To map the entire file, leave it as 0.*/
};

enum class FileAccess : uint8
{
	Read		= 0x01//,
	//Write		= 0x02,
	//Read_Write	= Read | Write
};

enum class FileShareMode : uint8
{
	None			= 0,
	Shared_Delete	= 0x01,
	Shared_Read		= 0x02,
	Shared_Write	= 0x04,
	Shared_All		= Shared_Delete | Shared_Read | Shared_Write
};

enum class FileCreateDisposition : uint8
{
	Open_Existing,
	Create_Always,
	Create_New,
	Open_Always,
	Truncate_Existing
};

struct FileMMapInfo
{
	std::filesystem::path path;
	FileAccess access;
	FileShareMode shareMode;
	FileCreateDisposition createDisposition;
	bool map;
};

/**
* @brief Memory mapped file.
* 
* Currently only supports reading.
* 
* TODO(afiq):
* Support writing to a memory mapped file. Make this data structure more robust.
*/
class File : public lib::non_copyable_non_movable
{
public:
	using pointer = void*;
	using const_pointer = void const*;

	File() = default;
	File(std::filesystem::path const& path, bool map = true);
	File(FileMMapInfo const& info);

	~File();

	auto open(std::filesystem::path const& path, bool map = true) -> bool;
	auto open(FileMMapInfo const& info) -> bool;
	auto close() -> void;
	auto map(FileMemoryMapInfo const& info = {}) -> bool;
	auto unmap() -> void;
	auto size() const -> size_t;
	auto offset() const -> size_t;
	auto mapped_offset() const -> size_t;
	auto mapped_size() const -> size_t;

	auto is_open() const -> bool;

	explicit operator bool() const;

	template <typename Self>
	auto data(this Self&& self) -> auto&&
	{
		return std::forward<Self>(self).m_data;
	}
private:
	struct
	{
		void* file;
		void* fileMapping;
	} m_handle = {};

	void*	m_data			= {};
	size_t	m_offset		= {};
	size_t	m_mappedSize	= {};
};
}
}

#endif // !CORE_SERIALIZATION_FILE_HPP
