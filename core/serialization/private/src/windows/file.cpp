#include "core.platform/platform_header.hpp"
#include "file.hpp"

namespace core
{
namespace sbf
{
File::File(std::filesystem::path const& path, bool map)
{
	open(path, map);
}

File::~File()
{
	close();
}

auto File::open(std::filesystem::path const& path, bool map) -> bool
{
	m_handle.file = CreateFileW(path.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);

	return (map) ? this->map() : m_handle.file != nullptr;
}

auto File::close() -> void
{
	if (m_handle.file)
	{
		unmap();
		CloseHandle(m_handle.file);
	}
}

auto File::map(FileMemoryMapInfo const& info) -> bool
{
	unmap();

	if (info.offset > size()		||
		std::cmp_equal(size(), 0)	|| 
		m_handle.file == nullptr)
	{
		return false;
	}

	m_handle.fileMapping = CreateFileMappingW(m_handle.file, nullptr, PAGE_READONLY, 0, static_cast<DWORD>(info.size), nullptr);

	if (m_handle.fileMapping == nullptr)
	{
		return false;
	}

	m_offset		= info.offset;
	m_mappedSize	= (info.size && info.offset + info.size < size()) ? info.size : size();

	size_t const mappedOffset = mapped_offset();

	DWORD const HIGH_OFFSET = static_cast<DWORD>(mappedOffset >> 32);
	DWORD const LOW_OFFSET = static_cast<DWORD>(mappedOffset & 0xFFFFFFFFull);

	void* const fileViewPtr = MapViewOfFile(m_handle.fileMapping, FILE_MAP_READ, HIGH_OFFSET, LOW_OFFSET, m_offset - mappedOffset + info.size);

	if (fileViewPtr == nullptr)
	{
		return false;
	}

	m_data = static_cast<std::byte*>(fileViewPtr) + (m_offset - mappedOffset);

	return m_data != nullptr;
}

auto File::unmap() -> void
{
	if (m_data)
	{
		UnmapViewOfFile(m_data);
		CloseHandle(m_handle.fileMapping);
	}
	m_data = nullptr;
	m_handle.fileMapping = nullptr;
	m_offset = 0;
	m_mappedSize = 0;
}

auto File::size() const -> size_t
{
	if (m_handle.file == nullptr)
	{
		return {};
	}

	LARGE_INTEGER size = {};
	
	return GetFileSizeEx(m_handle.file, &size) ? static_cast<size_t>(size.QuadPart) : 0ull;
}

auto File::offset() const -> size_t
{
	return m_offset;
}

auto File::mapped_offset() const -> size_t
{
	// Requested offset needs to be a multiple of the system allocation granularity.
	SYSTEM_INFO sysInfo{};
	GetSystemInfo(&sysInfo);

	size_t const ALLOCATION_GRANULARITY = static_cast<size_t>(sysInfo.dwAllocationGranularity);

	return (m_offset / ALLOCATION_GRANULARITY) * ALLOCATION_GRANULARITY;
}

auto File::mapped_size() const -> size_t
{
	return m_mappedSize;
}

auto File::is_open() const -> bool
{
	return m_handle.file != nullptr;
}

File::operator bool() const
{
	return is_open();
}
}
}