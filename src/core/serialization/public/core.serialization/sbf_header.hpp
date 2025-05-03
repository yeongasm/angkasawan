#pragma once
#ifndef SERIALIZATION_SERIALIZE_HPP
#define SERIALIZATION_SERIALIZE_HPP

#include "lib/common.hpp"

#ifndef SBF_VERSION_MAJOR
#define SBF_VERSION_MAJOR 1
#endif // !SBF_VERSION_MAJOR

#ifndef SBF_VERSION_MINOR
#define SBF_VERSION_MINOR 0
#endif // !SBF_VERSION_MINOR


namespace core
{
namespace sbf
{
inline static constexpr uint32 SBF_HEADER_TAG = 'FBS.';

struct SbfVersion
{
	int8 major;
	int8 minor;
};

/**
* @brief Used to describe the data it represents after the header.
*/
struct SbfFileHeader
{
	uint32 const magic = SBF_HEADER_TAG;
	SbfVersion const version = { .major = SBF_VERSION_MAJOR, .minor = SBF_VERSION_MINOR };		// Version of the file.
};

struct SbfDataDescriptor
{
	uint32 const tag;
	uint32 sizeBytes;	// Size of data excluding the current header.
};
}
}

#endif // !SERIALIZATION_SERIALIZE_HPP
