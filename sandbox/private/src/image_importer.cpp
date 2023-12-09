#include <fstream>
#include "image_importer.h"
#include "ktx.h"

namespace sandbox
{
ImageImporter::ImageImporter(std::filesystem::path const& path) :
	m_path{},
	m_storage{},
	m_img_data{},
	m_mip_infos{},
	m_image_info{}
{
	open(path);
}

ImageImporter::~ImageImporter()
{
	close(true);
}

auto ImageImporter::open(std::filesystem::path const& path) -> bool
{
	if (is_open())
	{
		ASSERTION(false && "Importer currently has a file opened.");
		return false;
	}

	m_path = std::filesystem::absolute(path);

	auto narrowPath = m_path.string();

	ktxTexture2* texture = nullptr;
	KTX_error_code result = ktxTexture2_CreateFromNamedFile(narrowPath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

	if (!texture || result != KTX_SUCCESS)
	{ 
		return false; 
	}

	switch (texture->numDimensions)
	{
	case 1:
		m_image_info.type = rhi::ImageType::Image_1D;
		break;
	case 3:
		m_image_info.type = rhi::ImageType::Image_3D;
		break;
	case 2:
	default:
		m_image_info.type = rhi::ImageType::Image_2D;
		break;
	}

	m_image_info.format = static_cast<rhi::Format>(texture->vkFormat);
	m_image_info.imageUsage = rhi::ImageUsage::Transfer_Dst | rhi::ImageUsage::Sampled;
	m_image_info.dimension.width = texture->baseWidth;
	m_image_info.dimension.height = texture->baseHeight;
	m_image_info.dimension.depth = texture->baseDepth;
	m_image_info.mipLevel = texture->numLevels;

	size_t const numMipLevels = static_cast<size_t>(m_image_info.mipLevel);
	size_t const mipOffsetInfoSize = sizeof(MipInfo) * numMipLevels;
	size_t const totalReservedSize = mipOffsetInfoSize + texture->dataSize;

	m_storage.resize(totalReservedSize);

	MipInfo* pMipInfo = reinterpret_cast<MipInfo*>(m_storage.data());
	uint8* data = m_storage.data() + mipOffsetInfoSize;

	for (uint32 mipLevel = 0; mipLevel < m_image_info.mipLevel; ++mipLevel)
	{
		size_t offset = 0;
		size_t size = 0;
		ktxTexture_GetImageOffset(reinterpret_cast<ktxTexture*>(texture), mipLevel, 0, 0, &pMipInfo->offset);
		pMipInfo->size = ktxTexture_GetImageSize(reinterpret_cast<ktxTexture*>(texture), mipLevel);
		++pMipInfo;
	}

	lib::memcopy(data, texture->pData, texture->dataSize);

	m_mip_infos = std::span{ pMipInfo, numMipLevels };
	m_img_data = std::span{ data, texture->dataSize };

	ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(texture));

	return true;
}

auto ImageImporter::is_open() const -> bool
{
	return std::cmp_not_equal(m_storage.size(), 0);
}

auto ImageImporter::close(bool release) -> void
{
	if (release)
	{
		m_storage.release();
	}
	else
	{
		m_storage.clear();
	}
	m_path.~path();
	lib::memset(&m_image_info, 0, sizeof(rhi::ImageInfo));
}

auto ImageImporter::size_bytes() const -> size_t
{
	return m_img_data.size_bytes();
}

auto ImageImporter::image_info() const -> rhi::ImageInfo
{
	return m_image_info;
}

auto ImageImporter::data(uint32 mipLevel) -> std::span<uint8>
{
	uint8* imgData = m_img_data.data();
	size_t size = m_img_data.size_bytes();

	if (!std::cmp_equal(mipLevel, -1))
	{
		mipLevel = std::clamp(mipLevel, 0u, static_cast<uint32>(m_mip_infos.size()) - 1u);
		MipInfo const& mipInfo = m_mip_infos[mipLevel];
		imgData += mipInfo.offset;
		size = mipInfo.size;
	}
	return std::span{ imgData, size };
}
}