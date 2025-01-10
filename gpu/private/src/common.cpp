#include "common.hpp"

namespace gpu
{
auto Version::stringify() const -> lib::string
{
	return lib::format("{}.{}.{}", major, minor, patch);
}

auto is_color_format(Format format) -> bool
{
	if (format == Format::D16_Unorm ||
		format == Format::D16_Unorm_S8_Uint ||
		format == Format::D24_Unorm_S8_Uint ||
		format == Format::D32_Float ||
		format == Format::D32_Float_S8_Uint ||
		format == Format::S8_Uint ||
		format == Format::X8_D24_Unorm_Pack32 ||
		format == Format::Undefined)
	{
		return false;
	}
	return true;
}

auto is_depth_format(Format format) -> bool
{
	if (format == Format::D16_Unorm ||
		format == Format::D16_Unorm_S8_Uint ||
		format == Format::D24_Unorm_S8_Uint ||
		format == Format::D32_Float ||
		format == Format::D32_Float_S8_Uint ||
		format == Format::X8_D24_Unorm_Pack32)
	{
		return true;
	}
	return false;
}

auto is_stencil_format(Format format) -> bool
{
	return format == Format::S8_Uint;
}

auto to_string(Format format) -> std::string_view
{
	constexpr const char* imageFormatStr[] = {
		"Undefined",
		"R4G4_Unorm_Pack8",
		"R4G4B4_Unorm_Pack16",
		"B4G4R4_Unorm_Pack16",
		"R5G6B5_Unorm_Pack16",
		"B5G6R5_Unorm_Pack16",
		"R5G5B5A1_Unorm_Pack16",
		"B5G5R5A1_Unorm_Pack16",
		"A1R5G5B5_Unorm_Pack16",
		"R8_Unorm",
		"R8_Norm",
		"R8_Uscaled",
		"R8_Scaled",
		"R8_Uint",
		"R8_Int",
		"R8_Srgb",
		"R8G8_Unorm",
		"R8G8_Norm",
		"R8G8_Uscaled",
		"R8G8_Scaled",
		"R8G8_Uint",
		"R8G8_Int",
		"R8G8_Srgb",
		"R8G8B8_Unorm",
		"R8G8B8_Norm",
		"R8G8B8_Uscaled",
		"R8G8B8_Scaled",
		"R8G8B8_Uint",
		"R8G8B8_Int",
		"R8G8B8_Srgb",
		"B8G8R8_Unorm",
		"B8G8R8_Norm",
		"B8G8R8_Uscaled",
		"B8G8R8_Scaled",
		"B8G8R8_Uint",
		"B8G8R8_Int",
		"B8G8R8_Srgb",
		"R8G8B8A8_Unorm",
		"R8G8B8A8_Norm",
		"R8G8B8A8_Uscaled",
		"R8G8B8A8_Scaled",
		"R8G8B8A8_Uint",
		"R8G8B8A8_Int",
		"R8G8B8A8_Srgb",
		"B8G8R8A8_Unorm",
		"B8G8R8A8_Norm",
		"B8G8R8A8_Uscaled",
		"B8G8R8A8_Scaled",
		"B8G8R8A8_Uint",
		"B8G8R8A8_Int",
		"B8G8R8A8_Srgb",
		"A8B8G8R8_Unorm_Pack32",
		"A8B8G8R8_Norm_Pack32",
		"A8B8G8R8_Uscaled_Pack32",
		"A8B8G8R8_Scaled_Pack32",
		"A8B8G8R8_Uint_Pack32",
		"A8B8G8R8_Int_Pack32",
		"A8B8G8R8_Srgb_Pack32",
		"A2R10G10B10_Unorm_Pack32",
		"A2R10G10B10_Norm_Pack32",
		"A2R10G10B10_Uscaled_Pack32",
		"A2R10G10B10_Scaled_Pack32",
		"A2R10G10B10_Uint_Pack32",
		"A2R10G10B10_Int_Pack32",
		"A2B10G10R10_Unorm_Pack32",
		"A2B10G10R10_Norm_Pack32",
		"A2B10G10R10_Uscaled_Pack32",
		"A2B10G10R10_Scaled_Pack32",
		"A2B10G10R10_Uint_Pack32",
		"A2B10G10R10_Int_Pack32",
		"R16_Unorm",
		"R16_Norm",
		"R16_Uscaled",
		"R16_Scaled",
		"R16_Uint",
		"R16_Int",
		"R16_Float",
		"R16G16_Unorm",
		"R16G16_Norm",
		"R16G16_Uscaled",
		"R16G16_Scaled",
		"R16G16_Uint",
		"R16G16_Int",
		"R16G16_Float",
		"R16G16B16_Unorm",
		"R16G16B16_Norm",
		"R16G16B16_Uscaled",
		"R16G16B16_Scaled",
		"R16G16B16_Uint",
		"R16G16B16_Int",
		"R16G16B16_Float",
		"R16G16B16A16_Unorm",
		"R16G16B16A16_Norm",
		"R16G16B16A16_Uscaled",
		"R16G16B16A16_Scaled",
		"R16G16B16A16_Uint",
		"R16G16B16A16_Int",
		"R16G16B16A16_Float",
		"R32_Uint",
		"R32_Int",
		"R32_Float",
		"R32G32_Uint",
		"R32G32_Int",
		"R32G32_Float",
		"R32G32B32_Uint",
		"R32G32B32_Int",
		"R32G32B32_Float",
		"R32G32B32A32_Uint",
		"R32G32B32A32_Int",
		"R32G32B32A32_Float",
		"R64_Uint",
		"R64_Int",
		"R64_Float",
		"R64G64_Uint",
		"R64G64_Int",
		"R64G64_Float",
		"R64G64B64_Uint",
		"R64G64B64_Int",
		"R64G64B64_Float",
		"R64G64B64A64_Uint",
		"R64G64B64A64_Int",
		"R64G64B64A64_Float",
		"B10G11R11_UFloat_Pack32",
		"E5B9G9R9_UFloat_Pack32",
		"D16_Unorm",
		"X8_D24_Unorm_Pack32",
		"D32_Float",
		"S8_Uint",
		"D16_Unorm_S8_Uint",
		"D24_Unorm_S8_Uint",
		"D32_Float_S8_Uint",
		"Bc1_Rgb_Unorm_Block",
		"Bc1_Rgb_Srgb_Block",
		"Bc1_Rgba_Unorm_Block",
		"Bc1_Rgba_Srgb_Block",
		"Bc2_Unorm_Block",
		"Bc2_Srgb_Block",
		"Bc3_Unorm_Block",
		"Bc3_Srgb_Block",
		"Bc4_Unorm_Block",
		"Bc4_Snorm_Block",
		"Bc5_Unorm_Block",
		"Bc5_Snorm_Block",
		"Bc6h_Ufloat_Block",
		"Bc6h_Sfloat_Block",
		"Bc7_Unorm_Block",
		"Bc7_Srgb_Block",
		"Etc2_R8G8B8_Unorm_Block",
		"Etc2_R8G8B8_Srgb_Block",
		"Etc2_R8G8B8A1_Unorm_Block",
		"Etc2_R8G8B8A1_Srgb_Block",
		"Etc2_R8G8B8A8_Unorm_Block",
		"Etc2_R8G8B8A8_Srgb_Block",
		"Eac_R11_Unorm_Block",
		"Eac_R11_Snorm_Block",
		"Eac_R11G11_Unorm_Block",
		"Eac_R11G11_Snorm_Block",
		"Astc_4x4_Unorm_Block",
		"Astc_4x4_Srgb_Block",
		"Astc_5x4_Unorm_Block",
		"Astc_5x4_Srgb_Block",
		"Astc_5x5_Unorm_Block",
		"Astc_5x5_Srgb_Block",
		"Astc_6x5_Unorm_Block",
		"Astc_6x5_Srgb_Block",
		"Astc_6x6_Unorm_Block",
		"Astc_6x6_Srgb_Block",
		"Astc_8x5_Unorm_Block",
		"Astc_8x5_Srgb_Block",
		"Astc_8x6_Unorm_Block",
		"Astc_8x6_Srgb_Block",
		"Astc_8x8_Unorm_Block",
		"Astc_8x8_Srgb_Block",
		"Astc_10x5_Unorm_Block",
		"Astc_10x5_Srgb_Block",
		"Astc_10x6_Unorm_Block",
		"Astc_10x6_Srgb_Block",
		"Astc_10x8_Unorm_Block",
		"Astc_10x8_Srgb_Block",
		"Astc_10x10_Unorm_Block",
		"Astc_10x10_Srgb_Block",
		"Astc_12x10_Unorm_Block",
		"Astc_12x10_Srgb_Block",
		"Astc_12x12_Unorm_Block",
		"Astc_12x12_Srgb_Block",
		"G8B8G8R8_422_Unorm",
		"B8G8R8G8_422_Unorm",
		"G8_B8_R8_3Plane_420_Unorm",
		"G8_B8R8_2Plane_420_Unorm",
		"G8_B8_R8_3Plane_422_Unorm",
		"G8_B8R8_2Plane_422_Unorm",
		"G8_B8_R8_3Plane_444_Unorm",
		"R10X6_Unorm_Pack16",
		"R10X6G10X6_Unorm_2Pack16",
		"R10X6G10X6B10X6A10X6_Unorm_4Pack16",
		"G10X6B10X6G10X6R10X6_422_Unorm_4Pack16",
		"B10X6G10X6R10X6G10X6_422_Unorm_4Pack16",
		"G10X6_B10X6_R10X6_3Plane_420_Unorm_3Pack16",
		"G10X6_B10X6R10X6_2Plane_420_Unorm_3Pack16",
		"G10X6_B10X6_R10X6_3Plane_422_Unorm_3Pack16",
		"G10X6_B10X6R10X6_2Plane_422_Unorm_3Pack16",
		"G10X6_B10X6_R10X6_3Plane_444_Unorm_3Pack16",
		"R12X4_Unorm_Pack16",
		"R12X4G12X4_Unorm_2Pack16",
		"R12X4G12X4B12X4A12X4_Unorm_4Pack16",
		"G12X4B12X4G12X4R12X4_422_Unorm_4Pack16",
		"B12X4G12X4R12X4G12X4_422_Unorm_4Pack16",
		"G12X4_B12X4_R12X4_3Plane_420_Unorm_3Pack16",
		"G12X4_B12X4R12X4_2Plane_420_Unorm_3Pack16",
		"G12X4_B12X4_R12X4_3Plane_422_Unorm_3Pack16",
		"G12X4_B12X4R12X4_2Plane_422_Unorm_3Pack16",
		"G12X4_B12X4_R12X4_3Plane_444_Unorm_3Pack16",
		"G16B16G16R16_422_Unorm",
		"B16G16R16G16_422_Unorm",
		"G16_B16_R16_3Plane_420_Unorm",
		"G16_B16R16_2Plane_420_Unorm",
		"G16_B16_R16_3Plane_422_Unorm",
		"G16_B16R16_2Plane_422_Unorm",
		"G16_B16_R16_3Plane_444_Unorm",
		"G8_B8R8_2Plane_444_Unorm",
		"G10X6_B10X6R10X6_2Plane_444_Unorm_3Pack16",
		"G12X4_B12X4R12X4_2Plane_444_Unorm_3Pack16",
		"G16_B16R16_2Plane_444_Unorm",
		"A4R4G4B4_Unorm_Pack16",
		"A4B4G4R4_Unorm_Pack16",
		"Astc_4x4_Sfloat_Block",
		"Astc_5x4_Sfloat_Block",
		"Astc_5x5_Sfloat_Block",
		"Astc_6x5_Sfloat_Block",
		"Astc_6x6_Sfloat_Block",
		"Astc_8x5_Sfloat_Block",
		"Astc_8x6_Sfloat_Block",
		"Astc_8x8_Sfloat_Block",
		"Astc_10x5_Sfloat_Block",
		"Astc_10x6_Sfloat_Block",
		"Astc_10x8_Sfloat_Block",
		"Astc_10x10_Sfloat_Block",
		"Astc_12x10_Sfloat_Block",
		"Astc_12x12_Sfloat_Block",
		"Pvrtc1_2Bpp_Unorm_Block_Img",
		"Pvrtc1_4Bpp_Unorm_Block_Img",
		"Pvrtc2_2Bpp_Unorm_Block_Img",
		"Pvrtc2_4Bpp_Unorm_Block_Img",
		"Pvrtc1_2Bpp_Srgb_Block_Img",
		"Pvrtc1_4Bpp_Srgb_Block_Img",
		"Pvrtc2_2Bpp_Srgb_Block_Img",
		"Pvrtc2_4Bpp_Srgb_Block_Img"
	};
	return std::string_view{ imageFormatStr[std::to_underlying(format)] };
}

auto to_string(ColorSpace colorSpace) -> std::string_view
{
	constexpr const char* colorSpaceStr[] = {
		"Srgb_Non_Linear",
		"Display_P3_Non_Linear",
		"Extended_Srgb_Linear",
		"Display_P3_Linear",
		"Dci_P3_Non_Linear",
		"Bt709_Linear",
		"Bt709_Non_Linear",
		"Bt2020_Linear",
		"Hdr10_St2084",
		"Dolby_Vision",
		"Hdr10_Hlg",
		"Adobe_Rgb_Linear",
		"Adobe_Rgb_Non_Linear",
		"Pass_Through",
		"Extended_Srgb_Non_Linear",
		"Display_Native_Amd"
	};
	return std::string_view{ colorSpaceStr[std::to_underlying(colorSpace)] };
}
}