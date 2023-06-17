#include "win_fstream.h"
#include "os_platform.h"

FTLBEGIN

WinFileIO::WinFileIO() : 
	m_handle{}, m_size{}, m_eof{}
{}

WinFileIO::~WinFileIO()
{
	close();
}

WinFileIO::WinFileIO(WinFileIO&& rhs) noexcept
{
	*this = std::move(rhs);
}

WinFileIO& WinFileIO::operator=(WinFileIO&& rhs) noexcept
{
	if (this != &rhs)
	{
		close();

		m_handle = rhs.m_handle;
		m_size	 = rhs.m_size;
		m_eof	 = rhs.m_eof;

		new (&rhs) WinFileIO();
	}
	return *this;
}

void WinFileIO::close()
{
	if (m_handle)
	{
		CloseHandle(m_handle);
	}
	m_size = 0;
	m_handle = nullptr;
	m_eof = false;
}

bool WinFileIO::good() const
{
	return (m_handle != INVALID_HANDLE_VALUE) && (m_handle != nullptr);
}

size_t WinFileIO::size() const
{
	return m_size;
}

size_t WinFileIO::tellg() const
{
	if (!m_handle)
	{
		return 0;
	}
	LARGE_INTEGER li{};

	li.LowPart = SetFilePointer(m_handle, 0, &li.HighPart, FILE_CURRENT);

	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		li.QuadPart = -1;
	}
	return static_cast<size_t>(li.QuadPart);
}

bool WinFileIO::seekg(size_t offset, stream::SeekDirectionFlags dir)
{
	bool result = false;
	if (m_handle)
	{
		LARGE_INTEGER li{};
		li.QuadPart = static_cast<LONGLONG>(offset);

		SetFilePointer(m_handle, li.LowPart, &li.HighPart, static_cast<DWORD>(dir));

		if (li.LowPart != INVALID_SET_FILE_POINTER && GetLastError() == NO_ERROR)
		{
			result = true;
		}
	}
	return result;
}

bool WinFileIO::eof() const
{
	return m_eof;
}

bool WinFileIO::platform_open(literal_t filename, stream::StreamModeFlags flags)
{
	OpenFileParams params = configure_params_internal(flags);

	m_handle = CreateFileA(filename, params.access, 0, NULL, params.disposition, params.attribute, NULL);

	if (m_handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	get_file_size_internal();

	if ((flags & stream::StreamMode::Ate) != 0)
	{
		seekg(0, stream::SeekDirection::End);
	}

	return true;
}

bool WinFileIO::platform_open(wide_literal_t filename, stream::StreamModeFlags flags)
{
	OpenFileParams params = configure_params_internal(flags);

	m_handle = CreateFileW(filename, params.access, 0, NULL, params.disposition, params.attribute, NULL);

	if (m_handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	get_file_size_internal();

	if ((flags & stream::StreamMode::Ate) != 0)
	{
		seekg(0, stream::SeekDirection::End);
	}

	return true;
}

bool WinFileIO::get_file_size_internal()
{
	LARGE_INTEGER li{};
	if (GetFileSizeEx(m_handle, &li) != 0)
	{
		m_size = static_cast<size_t>(li.QuadPart);
		return true;
	}
	return false;
}

WinFileIO::OpenFileParams WinFileIO::configure_params_internal(stream::StreamModeFlags flags)
{
	dword disposition = OPEN_ALWAYS;
	dword access	= GENERIC_READ | GENERIC_WRITE;	// Set a default value so it doesn't crash.
	dword attribute = FILE_ATTRIBUTE_NORMAL;

	if (has_bit_internal(flags, stream::StreamMode::In))
	{
		access = GENERIC_READ;
		attribute = FILE_ATTRIBUTE_READONLY;
	}

	if (has_bit_internal(flags, stream::StreamMode::Out))
	{
		access = GENERIC_WRITE;
	}

	if (has_bit_internal(flags, stream::StreamMode::Trunc))
	{
		disposition = CREATE_ALWAYS;
	}

	return { disposition, access, attribute };
}

bool WinFileIO::has_bit_internal(uint32 flag, uint32 bit)
{
	return (flag & bit) != 0;
}


bool WinReadFile::platform_read(void* dst, size_t bytesToRead)
{
	Super::m_eof = false;
	DWORD bytesRead = 0;
	bool result = ReadFile(Super::m_handle, dst, static_cast<DWORD>(bytesToRead), &bytesRead, NULL);

	if (result && bytesRead == 0)
	{
		Super::m_eof = true;
	}

	return result;
}


bool WinWriteFile::platform_write(void* src, size_t bytesToWrite)
{
	DWORD bytesWritten = 0;
	bool result = WriteFile(Super::m_handle, src, static_cast<DWORD>(bytesToWrite), &bytesWritten, NULL);
	Super::m_size += static_cast<size_t>(bytesWritten);

	return result;
}


FTLEND