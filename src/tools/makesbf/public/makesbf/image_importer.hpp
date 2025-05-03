#pragma once
#ifndef MAKESMF_IMAGE_IMPORTER_HPP
#define MAKESMF_IMAGE_IMPORTER_HPP

#include <filesystem>
#include "lib/array.hpp"
#include "gpu/common.hpp"

namespace makesbf
{
class ImageImporter
{
public:
	ImageImporter() = default;
	ImageImporter(std::filesystem::path const& uri);

	~ImageImporter();

	auto open(std::filesystem::path const& uri) -> bool;
	auto is_open() const -> bool;
	auto close() -> void;
	auto size_bytes() const -> size_t;
	auto image_info() const -> gpu::ImageInfo;
	auto data(uint32 mipLevel = std::numeric_limits<uint32>::max()) -> std::span<uint8>;
private:
	struct MipInfo
	{
		size_t offset;
		size_t sizeBytes;
	};
	std::filesystem::path m_path;
	void* m_imageHandle;
	lib::array<MipInfo> m_mipInfos;
	gpu::ImageInfo m_imageInfo;
};
}

#endif // !MAKESMF_IMAGE_IMPORTER_HPP
