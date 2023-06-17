#pragma once
#ifndef FOUNDATION_STREAM_FSTREAM_WIN_WIN_FSTREAM_H
#define FOUNDATION_STREAM_FSTREAM_WIN_WIN_FSTREAM_H

#include "os_shims.h"
#include "stream/flags/stream_flags.h"
#include "foundation_api.h"

FTLBEGIN

class FOUNDATION_API WinFileIO
{
public:
	void	close	();
	bool	good	() const;
	size_t	size	() const;
	size_t	tellg	() const;
	bool	seekg	(size_t offset, stream::SeekDirectionFlags dir);
	bool	eof		() const;
protected:
	os::Handle	m_handle;
	size_t		m_size;
	bool		m_eof;

	WinFileIO();
	~WinFileIO();

	WinFileIO(WinFileIO&&) noexcept;
	WinFileIO& operator=(WinFileIO&&) noexcept;

	bool	platform_open	(literal_t filename, stream::StreamModeFlags flags);
	bool	platform_open	(wide_literal_t filename, stream::StreamModeFlags flags);
private:
	struct OpenFileParams
	{
		dword	disposition;
		dword	access;
		dword	attribute;
	};

	bool get_file_size_internal();
	OpenFileParams configure_params_internal(stream::StreamModeFlags flags);

	bool has_bit_internal(stream::StreamModeFlags flag, uint32 bit);

	WinFileIO(const WinFileIO&)				= delete;
	WinFileIO& operator=(const WinFileIO&)	= delete;
};

class FOUNDATION_API WinReadFile : public virtual WinFileIO
{
protected:
	using Super = WinFileIO;

	bool platform_read(void* dst, size_t bytesToRead);
};

class FOUNDATION_API WinWriteFile : public virtual WinFileIO
{
protected:
	using Super = WinFileIO;

	bool platform_write(void* src, size_t bytesToWrite);
};

FTLEND

#endif // !FOUNDATION_STREAM_FSTREAM_WIN_WIN_FSTREAM_H
