#ifndef RENDER_MATERIAL_HPP
#define RENDER_MATERIAL_HPP

#include "core.serialization/sbf_header.hpp"
#include "core.serialization/file.hpp"
#include "gpu/gpu.hpp"

namespace render
{
inline static constexpr uint32 SBF_IMAGE_VIEW_HEADER_TAG		= 'VGMI';	// IMGV - Image View
inline static constexpr uint32 SBF_IMAGE_MIP_DATA_HEADER_TAG	= 'DPIM';	// MIPD - Mip Data

struct ImageMipInfo
{
	size_t offset;
	size_t size;
};

struct SbfImageViewInfo
{
	gpu::Extent3D dimension;
	gpu::Format format;
	gpu::ImageType type;
	uint32 mipLevels;
	ImageMipInfo mipInfos[4];
};

struct SbfImageViewHeader
{
	core::sbf::SbfDataDescriptor descriptor = { .tag = SBF_IMAGE_VIEW_HEADER_TAG };
	SbfImageViewInfo imageInfo;
};

struct SbfImageMipViewHeader
{
	core::sbf::SbfDataDescriptor descriptor = { .tag = SBF_IMAGE_MIP_DATA_HEADER_TAG };
	uint32 level;
};

class ImageSbfPack : public lib::non_copyable_non_movable
{
public:
	ImageSbfPack() = default;
	ImageSbfPack(core::sbf::File const& mmapFile);

	auto from_mmap_file(core::sbf::File const& mmapFile) -> bool;

	auto mip_count() const -> uint32;
	auto format() const -> gpu::Format;
	auto type() const -> gpu::ImageType;
	auto dimension() const -> gpu::Extent3D;
	auto data(uint32 mipLevel) const -> std::span<std::byte const>;
public:
	void* m_head = nullptr;
	SbfImageViewInfo m_imageInfo = {};
};

namespace material
{
enum class ImageType
{
	Base_Color = 0,
	Metallic_Roughness,
	Normal,
	Occlusion,
	Emissive
};

enum class AlphaMode
{
	Opaque,
	Mask,
	Blend
};

namespace util
{
struct ImageToImageTypePair
{
	std::string uri;	// NOTE(afiq) - Change to GUID when we finally have a virtual filesystem and an asset db.
	ImageType type;
};

struct MaterialMultiplier
{
	std::array<float32, 4> baseColor;
	float32 metallic;
	float32 roughness;
};

struct EmissiveInfo
{
	std::array<float32, 3> factor;
	float32 strength;
};

struct MaterialRepresentation
{
	MaterialMultiplier multipliers;
	EmissiveInfo emissive;
	AlphaMode alphaMode;
	float32 alphaCutoff;
	bool unlit;
	std::vector<ImageToImageTypePair> images;
};

struct MaterialJSON
{
	std::string mesh;
	std::vector<render::material::util::MaterialRepresentation> materials;
	uint32 numImages;
};
}
}

struct Image
{
	gpu::image image;
	gpu::sampler sampler;
	material::ImageType materialType;
	uint32 mipLevel;
	uint32 binding;
};

struct Material
{
	std::span<Image> images;
	gpu::device_address modifier;

	auto image_for(material::ImageType type) const -> std::optional<lib::ref<Image>>;
};
}

#endif // !RENDER_MATERIAL_HPP
