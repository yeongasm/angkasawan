#include "rhi.h"

namespace rhi
{

bool is_color_format(ImageFormat format)
{
	if (format == ImageFormat::D16_Unorm ||
		format == ImageFormat::D16_Unorm_S8_Uint ||
		format == ImageFormat::D24_Unorm_S8_Uint ||
		format == ImageFormat::D32_Float ||
		format == ImageFormat::D32_Float_S8_Uint ||
		format == ImageFormat::S8_Uint ||
		format == ImageFormat::X8_D24_Unorm_Pack32 ||
		format == ImageFormat::Undefined)
	{
		return false;
	}

	return true;
}

bool is_depth_format(ImageFormat format)
{
	if (format == ImageFormat::D16_Unorm ||
		format == ImageFormat::D16_Unorm_S8_Uint ||
		format == ImageFormat::D24_Unorm_S8_Uint ||
		format == ImageFormat::D32_Float ||
		format == ImageFormat::D32_Float_S8_Uint ||
		format == ImageFormat::X8_D24_Unorm_Pack32)
	{
		return true;
	}

	return false;
}

bool is_stencil_format(ImageFormat format)
{
	return format == ImageFormat::S8_Uint;
}

std::string_view get_image_format_string(ImageFormat format)
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
		"D32_Float_S8_Uint"
	};
	return std::string_view{ imageFormatStr[static_cast<std::underlying_type_t<ImageFormat>>(format)] };
}

}

rhi::ImageUsage operator| (rhi::ImageUsage a, rhi::ImageUsage b)
{ 
	return static_cast<rhi::ImageUsage>(static_cast<rhi::RhiFlag>(a) | static_cast<rhi::RhiFlag>(b)); 
}

rhi::ImageUsage operator& (rhi::ImageUsage a, rhi::ImageUsage b)
{ 
	return static_cast<rhi::ImageUsage>(static_cast<rhi::RhiFlag>(a) & static_cast<rhi::RhiFlag>(b)); 
}

rhi::BufferUsage operator| (rhi::BufferUsage a, rhi::BufferUsage b)
{ 
	return static_cast<rhi::BufferUsage>(static_cast<rhi::RhiFlag>(a) | static_cast<rhi::RhiFlag>(b)); 
}

rhi::BufferUsage operator& (rhi::BufferUsage a, rhi::BufferUsage b)
{ 
	return static_cast<rhi::BufferUsage>(static_cast<rhi::RhiFlag>(a) & static_cast<rhi::RhiFlag>(b)); 
}

rhi::ImageAspect operator| (rhi::ImageAspect a, rhi::ImageAspect b)
{ 
	return static_cast<rhi::ImageAspect>(static_cast<rhi::RhiFlag>(a) | static_cast<rhi::RhiFlag>(b)); 
}

rhi::ImageAspect operator& (rhi::ImageAspect a, rhi::ImageAspect b)
{ 
	return static_cast<rhi::ImageAspect>(static_cast<rhi::RhiFlag>(a) & static_cast<rhi::RhiFlag>(b)); 
}

rhi::PipelineStage operator| (rhi::PipelineStage a, rhi::PipelineStage b)
{	
	return static_cast<rhi::PipelineStage>(static_cast<rhi::RhiFlag>(a) | static_cast<rhi::RhiFlag>(b)); 
}

rhi::PipelineStage operator& (rhi::PipelineStage a, rhi::PipelineStage b)
{ 
	return static_cast<rhi::PipelineStage>(static_cast<rhi::RhiFlag>(a) & static_cast<rhi::RhiFlag>(b)); 
}

rhi::MemoryAccessType operator| (rhi::MemoryAccessType a, rhi::MemoryAccessType b) 
{ 
	return static_cast<rhi::MemoryAccessType>(static_cast<rhi::RhiFlag>(a) | static_cast<rhi::RhiFlag>(b)); 
}

rhi::MemoryAccessType operator& (rhi::MemoryAccessType a, rhi::MemoryAccessType b) 
{ 
	return static_cast<rhi::MemoryAccessType>(static_cast<rhi::RhiFlag>(a) & static_cast<rhi::RhiFlag>(b)); 
}