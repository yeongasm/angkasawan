#pragma once
#ifndef IMAGE_IMPORTER_H
#define IMAGE_IMPORTER_H

#include <filesystem>
#include "lib/array.h"
#include "gpu/common.h"

namespace sandbox
{
struct ImageImportInfo
{
	lib::string name;
	std::filesystem::path uri;
	uint8 const* data = nullptr;
	size_t size = 0;
};

class ImageImporter
{
public:
	ImageImporter() = default;
	ImageImporter(ImageImportInfo&& info);

	~ImageImporter();

	auto open(ImageImportInfo&& info) -> bool;
	auto is_open() const -> bool;
	auto close(bool release = true) -> void;
	auto size_bytes() const -> size_t;
	auto image_info() const -> gpu::ImageInfo;
	auto data(uint32 mipLevel = std::numeric_limits<uint32>::max()) -> std::span<uint8>;
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
	gpu::ImageInfo m_image_info;
};
}

#endif // !IMAGE_IMPORTER_H
