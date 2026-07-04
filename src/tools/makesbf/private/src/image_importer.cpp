#include "image_importer.hpp"
#include "ktx.h"

namespace makesbf
{
ImageImporter::ImageImporter(std::filesystem::path const& uri) :
	m_path{},
	m_imageHandle{},
	m_mipInfos{},
	m_imageInfo{}
{
	open(uri);
}

ImageImporter::~ImageImporter()
{
	close();
}

auto ImageImporter::open(std::filesystem::path const& uri) -> bool
{
	if (is_open())
	{
		ASSERTION(false && "Importer currently has a file opened.");
		return false;
	}

	ktxTexture2* texture = nullptr;

	if (uri.empty())
	{
		return false;
	}

	m_path = std::filesystem::absolute(uri);

	auto narrowPath = m_path.string();

	KTX_error_code result = ktxTexture2_CreateFromNamedFile(narrowPath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

	if (!texture || result != KTX_SUCCESS)
	{ 
		return false; 
	}

	if (ktxTexture2_NeedsTranscoding(texture))
	{
		ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC7_RGBA, 0);
	}

	switch (texture->numDimensions)
	{
	case 1:
		m_imageInfo.type = gpu::ImageType::Image_1D;
		break;
	case 3:
		m_imageInfo.type = gpu::ImageType::Image_3D;
		break;
	case 2:
	default:
		m_imageInfo.type = gpu::ImageType::Image_2D;
		break;
	}

	m_imageInfo.format = static_cast<gpu::Format>(texture->vkFormat);
	m_imageInfo.imageUsage = gpu::ImageUsage::Transfer_Dst | gpu::ImageUsage::Sampled;
	m_imageInfo.dimension.width = texture->baseWidth;
	m_imageInfo.dimension.height = texture->baseHeight;
	m_imageInfo.dimension.depth = texture->baseDepth;
	m_imageInfo.mipLevel = texture->numLevels;

	m_mipInfos.reserve(static_cast<size_t>(m_imageInfo.mipLevel));

	for (uint32 mipLevel = 0; mipLevel < m_imageInfo.mipLevel; ++mipLevel)
	{
		size_t offset = {};

		ktxTexture_GetImageOffset(reinterpret_cast<ktxTexture*>(texture), mipLevel, 0, 0, &offset);
		size_t const size = ktxTexture_GetImageSize(reinterpret_cast<ktxTexture*>(texture), mipLevel);
		
		ASSERTION(std::cmp_not_equal(size, 0));

		m_mipInfos.emplace_back(offset, size);
	}

	m_imageHandle = texture;

	return true;
}

auto ImageImporter::is_open() const -> bool
{
	return m_imageHandle != nullptr;
}

auto ImageImporter::close() -> void
{
	ktxTexture_Destroy(static_cast<ktxTexture*>(m_imageHandle));
}

auto ImageImporter::size_bytes() const -> size_t
{
	return static_cast<ktxTexture2*>(m_imageHandle)->dataSize;
}

auto ImageImporter::image_info() const -> gpu::ImageInfo
{
	return m_imageInfo;
}

auto ImageImporter::data(uint32 mipLevel) -> std::span<uint8>
{
	ktxTexture2* texture = static_cast<ktxTexture2*>(m_imageHandle);
	MipInfo const& mipInfo = m_mipInfos[mipLevel];

	return std::span{ texture->pData + mipInfo.offset, mipInfo.sizeBytes };
}
}