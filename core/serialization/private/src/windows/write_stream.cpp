#include "core.platform/platform_header.hpp"
#include <strsafe.h>
#include "write_stream.hpp"

#include "lib/string.hpp"

namespace core
{
namespace sbf
{
WriteStream::WriteStream(std::filesystem::path const& filename) :
	m_nativeFileHandle{}
{
	open(filename);
}

WriteStream::~WriteStream()
{  
	close();
}

auto WriteStream::open(std::filesystem::path const& filename) -> bool
{
	m_nativeFileHandle = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);

	return m_nativeFileHandle != INVALID_HANDLE_VALUE;
}

auto WriteStream::close() -> void
{
	if (m_nativeFileHandle)
	{
		CloseHandle(m_nativeFileHandle);
		m_nativeFileHandle = nullptr;
	}
}

auto WriteStream::write(Buffer const& input) -> bool
{
	return write(input.data(), input.byte_offset());
}

auto WriteStream::write(void const* data, size_t sizeBytes) -> bool
{
	if (!m_nativeFileHandle)
	{
		return false;
	}

	DWORD bytesWritten = {};

	auto const errFlag = WriteFile(static_cast<HANDLE>(m_nativeFileHandle), data, static_cast<DWORD>(sizeBytes), &bytesWritten, nullptr);

	//if (!bytesWritten)
	//{
	//	LPVOID lpMsgBuf = nullptr;
	//	LPVOID lpDisplayBuf = nullptr;
	//	LPCTSTR lpszFunction = TEXT("WriteStream::write");
	//	auto dw = GetLastError();
	//	FormatMessage(
	//		FORMAT_MESSAGE_ALLOCATE_BUFFER |
	//		FORMAT_MESSAGE_FROM_SYSTEM |
	//		FORMAT_MESSAGE_IGNORE_INSERTS,
	//		NULL,
	//		dw,
	//		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	//		(LPTSTR)&lpMsgBuf,
	//		0,
	//		NULL
	//	);
	//	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	//	StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);
	//	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	//	LocalFree(lpMsgBuf);
	//	LocalFree(lpDisplayBuf);
	//	LocalFree(lpMsgBuf);
	//}

	return bytesWritten == sizeBytes && errFlag == TRUE;
}
}
}