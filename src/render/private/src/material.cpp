#include "material.hpp"

namespace render
{
ImageSbfPack::ImageSbfPack(core::sbf::File const& mmapFile)
{
    from_mmap_file(mmapFile);
}

auto ImageSbfPack::from_mmap_file(core::sbf::File const& mmapFile) -> bool
{
    if (std::cmp_equal(mmapFile.size(), 0) ||
        std::cmp_equal(mmapFile.mapped_size(), 0) ||
        m_head != nullptr)
    {
        return false;
    }

    std::byte* ptr = static_cast<std::byte*>(mmapFile.data());

	m_head = ptr;

    auto const& sbfHeader = *reinterpret_cast<core::sbf::SbfFileHeader*>(ptr);

    if (sbfHeader.magic != core::sbf::SBF_HEADER_TAG
    /*|| should probably check the version sometime in the future.*/)
    {
        return false;
    }
    
    ptr += sizeof(core::sbf::SbfFileHeader);

    auto const& imageView = *reinterpret_cast<SbfImageViewHeader*>(ptr);

    if (imageView.descriptor.tag != SBF_IMAGE_VIEW_HEADER_TAG ||
        std::cmp_equal(imageView.descriptor.sizeBytes, 0))
    {
        return false;
    }

    std::memcpy(&m_imageInfo, &imageView.imageInfo, sizeof(SbfImageViewInfo));

    return true;
}

auto ImageSbfPack::mip_count() const -> uint32
{
    return m_imageInfo.mipLevels;
}

auto ImageSbfPack::format() const -> gpu::Format
{
    return m_imageInfo.format;
}

auto ImageSbfPack::type() const -> gpu::ImageType
{
	return m_imageInfo.type;
}

auto ImageSbfPack::dimension() const -> gpu::Extent3D
{
    return m_imageInfo.dimension;
}

auto ImageSbfPack::data(uint32 mipLevel) const -> std::span<std::byte const>
{
    if (m_head == nullptr ||
        mipLevel >= m_imageInfo.mipLevels || 
        std::cmp_equal(m_imageInfo.mipInfos[mipLevel].size, 0))
    {
        return {};
    }

    std::byte const* ptr = static_cast<std::byte*>(m_head);

    auto const& mipInfo = m_imageInfo.mipInfos[mipLevel];

	ptr += mipInfo.offset;

    auto const& mipHeader = *reinterpret_cast<SbfImageMipViewHeader const*>(ptr);
    
    if (mipHeader.descriptor.tag != SBF_IMAGE_MIP_DATA_HEADER_TAG ||
        mipHeader.level != mipLevel)
    {
        ASSERTION(mipHeader.level == mipLevel && "Corrupted SBF file! Requested mip level and mip level in file do not match.");
        
        return {};
    }

    ptr += sizeof(SbfImageMipViewHeader);

    return std::span{ ptr, mipHeader.descriptor.sizeBytes };
}

auto Material::image_for(material::ImageType type) const -> std::optional<lib::ref<Image>>
{
	auto const i = std::to_underlying(type);

	if (std::cmp_greater_equal(i, images.size()))
	{
		return std::nullopt;
	}

	return images[i];
}
}