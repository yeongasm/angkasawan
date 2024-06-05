#include <fstream>
#include "image_importer.h"
#include "ktx.h"

namespace sandbox
{
ImageImporter::ImageImporter(ImageImportInfo&& info) :
	m_path{},
	m_storage{},
	m_img_data{},
	m_mip_infos{},
	m_image_info{}
{
	open(std::move(info));
}

ImageImporter::~ImageImporter()
{
	close(true);
}

auto ImageImporter::open(ImageImportInfo&& info) -> bool
{
	if (is_open())
	{
		ASSERTION(false && "Importer currently has a file opened.");
		return false;
	}

	ktxTexture2* texture = nullptr;

	if (!info.uri.empty())
	{
		m_path = std::filesystem::absolute(info.uri);

		auto narrowPath = m_path.string();

		KTX_error_code result = ktxTexture2_CreateFromNamedFile(narrowPath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

		if (!texture || result != KTX_SUCCESS)
		{ 
			return false; 
		}
	}
	else if (info.data != nullptr && info.size)
	{
		KTX_error_code result = ktxTexture2_CreateFromMemory(info.data, info.size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

		if (!texture || result != KTX_SUCCESS)
		{
			return false;
		}
	}

	if (ktxTexture2_NeedsTranscoding(texture))
	{
		ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC7_RGBA, 0);
	}

	switch (texture->numDimensions)
	{
	case 1:
		m_image_info.type = gpu::ImageType::Image_1D;
		break;
	case 3:
		m_image_info.type = gpu::ImageType::Image_3D;
		break;
	case 2:
	default:
		m_image_info.type = gpu::ImageType::Image_2D;
		break;
	}

	m_image_info.name = std::move(info.name);
	m_image_info.format = static_cast<gpu::Format>(texture->vkFormat);
	m_image_info.imageUsage = gpu::ImageUsage::Transfer_Dst | gpu::ImageUsage::Sampled;
	m_image_info.dimension.width = texture->baseWidth;
	m_image_info.dimension.height = texture->baseHeight;
	m_image_info.dimension.depth = texture->baseDepth;
	m_image_info.mipLevel = texture->numLevels;

	size_t const numMipLevels = static_cast<size_t>(m_image_info.mipLevel);
	size_t const mipOffsetInfoSize = sizeof(MipInfo) * numMipLevels;
	size_t const totalReservedSize = mipOffsetInfoSize + texture->dataSize;

	m_storage.resize(totalReservedSize);

	size_t totalMipSize = 0;
	m_mip_infos = std::span{ reinterpret_cast<MipInfo*>(m_storage.data()), numMipLevels };
	uint8* data = m_storage.data() + mipOffsetInfoSize;

	for (uint32 mipLevel = 0; mipLevel < m_image_info.mipLevel; ++mipLevel)
	{
		ktxTexture_GetImageOffset(reinterpret_cast<ktxTexture*>(texture), mipLevel, 0, 0, &m_mip_infos[mipLevel].offset);
		m_mip_infos[mipLevel].size = ktxTexture_GetImageSize(reinterpret_cast<ktxTexture*>(texture), mipLevel);
		totalMipSize += m_mip_infos[mipLevel].size;
	}

	ASSERTION(totalMipSize == texture->dataSize && "You done messed up A.ARON");

	lib::memcopy(data, texture->pData, texture->dataSize);

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
	lib::memset(&m_image_info, 0, sizeof(gpu::ImageInfo));
}

auto ImageImporter::size_bytes() const -> size_t
{
	return m_img_data.size_bytes();
}

auto ImageImporter::image_info() const -> gpu::ImageInfo
{
	return m_image_info;
}

auto ImageImporter::data(uint32 mipLevel) -> std::span<uint8>
{
	size_t offset = 0;
	size_t size = m_img_data.size_bytes();

	if (!std::cmp_equal(mipLevel, -1))
	{
		mipLevel = std::clamp(mipLevel, 0u, static_cast<uint32>(m_mip_infos.size()) - 1u);
		MipInfo const& mipInfo = m_mip_infos[mipLevel];
		offset = mipInfo.offset;
		size = mipInfo.size;
	}

	return std::span{ &m_img_data[offset], size};
}
}