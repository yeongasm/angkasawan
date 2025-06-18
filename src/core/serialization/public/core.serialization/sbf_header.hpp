#pragma once
#include <utility>
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
	uint32 magic = SBF_HEADER_TAG;
	SbfVersion version = { .major = SBF_VERSION_MAJOR, .minor = SBF_VERSION_MINOR };		// Version of the file.
};

struct SbfDataDescriptor
{
	uint32 tag;
	uint32 sizeBytes;	// Size of data excluding the current header.
};

/*
* Checks if an SBF header is valid. Returns a pointer to the next segment if it is a valid header and nullptr if not.
*/
inline auto check_header(void const* data, [[maybe_unused]] SbfVersion const& version = {}) -> void const*
{
	auto const& sbfHeader = *reinterpret_cast<core::sbf::SbfFileHeader const*>(data);

	if (sbfHeader.magic != core::sbf::SBF_HEADER_TAG
	/*|| should probably check the version sometime in the future.*/)
	{
		return nullptr;
	}
	
	return static_cast<std::byte const*>(data) + sizeof(core::sbf::SbfFileHeader);
};

template <typename SectionHeaderType>
auto check_section_header(void const* data) -> void const*
{
	auto const& header = *static_cast<SectionHeaderType const*>(data);

	if (header.descriptor.tag != SectionHeaderType::TAG ||
		std::cmp_equal(header.descriptor.sizeBytes, 0))
	{
		return nullptr;
	}

	return static_cast<std::byte const*>(data) + sizeof(SectionHeaderType);
};
}
}

#endif // !SERIALIZATION_SERIALIZE_HPP
