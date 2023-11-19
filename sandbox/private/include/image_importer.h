#pragma once
#ifndef IMAGE_IMPORTER_H
#define IMAGE_IMPORTER_H

#include <filesystem>
#include "lib/array.h"
#include "rhi/constants.h"

namespace sandbox
{
class ImageImporter
{
public:
	ImageImporter() = default;
	ImageImporter(std::filesystem::path path);

	~ImageImporter();

	auto open(std::filesystem::path const& path) -> bool;
	auto is_open() const -> bool;
	auto close(bool release = true) -> void;
	auto size_bytes() const -> size_t;
	auto width() const -> uint32;
	auto height() const -> uint32;
	auto channels() const -> uint32;
	auto format() const -> rhi::Format;
private:
	std::filesystem::path m_path;
	lib::array<uint8> m_data;
	rhi::Format m_format;
	uint32 m_width;
	uint32 m_height;
	uint32 m_channels;
};
}

#endif // !IMAGE_IMPORTER_H
