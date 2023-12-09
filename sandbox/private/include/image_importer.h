#pragma once
#ifndef IMAGE_IMPORTER_H
#define IMAGE_IMPORTER_H

#include <filesystem>
#include "lib/array.h"
#include "rhi/common.h"

namespace sandbox
{
class ImageImporter
{
public:
	ImageImporter() = default;
	ImageImporter(std::filesystem::path const& path);

	~ImageImporter();

	auto open(std::filesystem::path const& path) -> bool;
	auto is_open() const -> bool;
	auto close(bool release = true) -> void;
	auto size_bytes() const -> size_t;
	auto image_info() const -> rhi::ImageInfo;
	auto data(uint32 mipLevel = -1u) -> std::span<uint8>;
private:
	struct MipInfo
	{
		size_t offset;
		size_t size;
	};
	std::filesystem::path m_path;
	lib::array<uint8> m_storage;
	std::span<uint8> m_img_data;
	std::span<MipInfo> m_mip_infos;
	rhi::ImageInfo m_image_info;
};
}

#endif // !IMAGE_IMPORTER_H
