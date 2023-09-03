#pragma once
#ifndef FOUNDATION_STREAM_FLAGS_STREAM_FLAGS_H
#define FOUNDATION_STREAM_FLAGS_STREAM_FLAGS_H

#include "common.h"

FTLBEGIN

namespace stream
{
	enum StreamMode : uint32
	{
		None	= 0,		// Do nothing.
		In		= 1 << 0,	// Open for reading.
		Out		= 1 << 1,	// Open for writing.
		Trunc	= 1 << 2,	// Discard contents of the stream when opening.
		Ate		= 1 << 3	// Seek to end of stream immediately after open.
	};
	using StreamModeFlags = uint32;

	enum SeekDirection : uint32
	{
		Beginning	= 0,	// The beginning of a stream.
		Current		= 1,	// The current position of the stream position indicator.
		End			= 2		// The end of a stream.
	};
	using SeekDirectionFlags = uint32;
}

FTLEND

#endif // !FOUNDATION_STREAM_FLAGS_STREAM_FLAGS_H
