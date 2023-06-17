#pragma once
#ifndef FOUNDATION_STREAM_FSTREAM_H
#define FOUNDATION_STREAM_FSTREAM_H

#ifdef _WIN32
#include "win/win_fstream.h"
#else
#endif

FTLBEGIN

#ifdef _WIN32
using PlatformReadFile = WinReadFile;
using PlatformWriteFile = WinWriteFile;
#else
#endif

/**
* Synchronous input file stream.
*/
class Ifstream final : public PlatformReadFile
{
private:
	using Super = PlatformReadFile;

	Ifstream(const Ifstream&) = delete;
	Ifstream& operator=(const Ifstream&) = delete;
public:
	Ifstream() : Super{} {}
	~Ifstream() {}

	Ifstream(Ifstream&& rhs) noexcept { *this = std::move(rhs); }
	Ifstream& operator=(Ifstream&& rhs) noexcept
	{
		Super::operator=(std::move(rhs));
		return *this;
	}

	Ifstream(literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None) : 
		Super{}
	{
		Super::platform_open(filename, flags | stream::StreamMode::In);
	}

	Ifstream(wide_literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None) :
		Super{}
	{
		Super::platform_open(filename, flags | stream::StreamMode::In);
	}

	bool open(literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None)
	{
		return Super::platform_open(filename, flags | stream::StreamMode::In);
	}

	bool open(wide_literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None)
	{
		return Super::platform_open(filename, flags | stream::StreamMode::In);
	}

	/**
	* Reads contents of the file into dst buffer.
	* Updates the file internal file pointer to previousOffset += bytesToRead.
	*/
	void read(void* dst, size_t bytesToRead)
	{
		Super::platform_read(dst, bytesToRead);
	}
};

/**
* Synchronous output file stream.
*/
class Ofstream final : public PlatformWriteFile
{
private:
	using Super = PlatformWriteFile;

	Ofstream(const Ofstream&) = delete;
	Ofstream& operator=(const Ofstream&) = delete;
public:
	Ofstream() : Super{} {}
	~Ofstream() {}

	Ofstream(Ofstream&& rhs) noexcept { *this = std::move(rhs); }
	Ofstream& operator=(Ofstream&& rhs) noexcept
	{
		Super::operator=(std::move(rhs));
		return *this;
	}

	Ofstream(literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None) :
		Super{}
	{
		Super::platform_open(filename, flags | stream::StreamMode::Out);
	}

	Ofstream(wide_literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None) :
		Super{}
	{
		Super::platform_open(filename, flags | stream::StreamMode::Out);
	}

	bool open(literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None)
	{
		return Super::platform_open(filename, flags | stream::StreamMode::Out);
	}

	bool open(wide_literal_t filename, stream::StreamModeFlags flags = stream::StreamMode::None)
	{
		return Super::platform_open(filename, flags | stream::StreamMode::Out);
	}

	/**
	* Writes contents of the src buffer into the file.
	* Updates the file internal file pointer to previousOffset += bytesToRead.
	*/
	void write(void* src, size_t bytesToWrite)
	{
		Super::platform_write(src, bytesToWrite);
	}
};

FTLEND

#endif // !FOUNDATION_STREAM_FSTREAM_H
