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
/**
* Our file format would be similar to GLTF.
* We have:
* a) "description"/"directory" that contains the details of the asset we're storing & byte offset to the data.
* b) Binary data for the stored asset.
* 
* Each individual piece of data (including the "description"/"directory") would have it's own stream header.
* 
* Extension for "description"/"directory" file -> Streamable Description Format .sdf
* Extension for binary file? -> Streamable Binary File .sbf
* 
* A single sbf file uniquely describes a singly type of asset.
* 
*/

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
	uint32 const magic = 'FBS.';
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
