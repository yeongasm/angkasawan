#include "device.h"
#include "vulkan/vk_device.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_messenger_callback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
	[[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData)
{
	auto translate_message_severity = [](VkDebugUtilsMessageSeverityFlagBitsEXT flag) -> rhi::ErrorSeverity
	{
		switch (flag)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return rhi::ErrorSeverity::Verbose;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return rhi::ErrorSeverity::Info;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return rhi::ErrorSeverity::Warning;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		default:
			return rhi::ErrorSeverity::Error;
		}
	};

	rhi::DeviceInitInfo* pInfo = static_cast<rhi::DeviceInitInfo*>(pUserData);
	rhi::ErrorSeverity sv = translate_message_severity(severity);
	pInfo->callback(sv, pCallbackData->pMessage);

	return VK_TRUE;
}


VkDebugUtilsMessengerCreateInfoEXT populate_debug_messenger(void* data)
{
	return VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

		.pfnUserCallback = debug_util_messenger_callback,
		.pUserData = data
	};
}

namespace rhi
{

auto translate_image_usage_flags(ImageUsage flags) -> VkImageUsageFlags
{
	uint32 const mask = static_cast<uint32>(flags);
	if (mask == 0)
	{
		return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
	}

	constexpr VkImageUsageFlagBits flagBits[] = {
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT,
		VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR
	};
	VkImageUsageFlags result{};
	for (uint32 i = 0; i < static_cast<uint32>(std::size(flagBits)); ++i)
	{
		uint32 const exist = (mask & (1u << i)) != 0u;
		auto const bit = flagBits[i];
		result |= exist * bit;
	}
	return result;
}

auto translate_swapchain_presentation_mode(SwapchainPresentMode mode) -> VkPresentModeKHR
{
	switch (mode)
	{
	case SwapchainPresentMode::Immediate:
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	case SwapchainPresentMode::Mailbox:
		return VK_PRESENT_MODE_MAILBOX_KHR;
	case SwapchainPresentMode::Fifo:
		return VK_PRESENT_MODE_FIFO_KHR;
	case SwapchainPresentMode::Fifo_Relaxed:
		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	case SwapchainPresentMode::Shared_Demand_Refresh:
		return VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR;
	case SwapchainPresentMode::Shared_Continuous_Refresh:
		return VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR;
	default:
		return VK_PRESENT_MODE_MAX_ENUM_KHR;
	}
}

auto translate_shader_stage(ShaderType type) -> VkShaderStageFlagBits
{
	switch (type)
	{
	case ShaderType::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderType::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderType::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderType::Tesselation_Control:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case ShaderType::Tesselation_Evaluation:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case ShaderType::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	case ShaderType::None:
	default:
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

auto translate_buffer_usage_flags(BufferUsage flags) -> VkBufferUsageFlags
{
	uint32 const mask = static_cast<uint32>(flags);
	constexpr VkBufferUsageFlagBits flagBits[] = {
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
	};
	VkBufferUsageFlags result{};
	for (uint32 i = 0; i < static_cast<uint32>(std::size(flagBits)); ++i)
	{
		uint32 const exist = (mask & (1u << i)) != 0u;
		auto const bit = flagBits[i];
		result |= exist * bit;
	}
	return result;
}

auto translate_image_type(ImageType type) -> VkImageType
{
	switch (type)
	{
	case ImageType::Image_1D:
		return VK_IMAGE_TYPE_1D;
	case ImageType::Image_2D:
		return VK_IMAGE_TYPE_2D;
	case ImageType::Image_3D:
		return VK_IMAGE_TYPE_3D;
	default:
		return VK_IMAGE_TYPE_MAX_ENUM;
	}
}

auto translate_image_view_type(ImageType type) -> VkImageViewType
{
	switch (type)
	{
	case ImageType::Image_1D:
		return VK_IMAGE_VIEW_TYPE_1D;
	case ImageType::Image_2D:
		return VK_IMAGE_VIEW_TYPE_2D;
	case ImageType::Image_3D:
		return VK_IMAGE_VIEW_TYPE_3D;
	default:
		return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	}
}

auto translate_image_tiling(ImageTiling tiling) -> VkImageTiling
{
	switch (tiling)
	{
	case ImageTiling::Optimal:
		return VK_IMAGE_TILING_OPTIMAL;
	case ImageTiling::Linear:
		return VK_IMAGE_TILING_LINEAR;
	default:
		return VK_IMAGE_TILING_MAX_ENUM;
	}
}

auto translate_format(Format format) -> VkFormat
{
	constexpr VkFormat formats[] = {
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_R4G4_UNORM_PACK8,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_B5G6R5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,
		VK_FORMAT_A1R5G5B5_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_USCALED,
		VK_FORMAT_R8_SSCALED,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8_SRGB,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_USCALED,
		VK_FORMAT_R8G8_SSCALED,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8_SRGB,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_SNORM,
		VK_FORMAT_R8G8B8_USCALED,
		VK_FORMAT_R8G8B8_SSCALED,
		VK_FORMAT_R8G8B8_UINT,
		VK_FORMAT_R8G8B8_SINT,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_B8G8R8_UNORM,
		VK_FORMAT_B8G8R8_SNORM,
		VK_FORMAT_B8G8R8_USCALED,
		VK_FORMAT_B8G8R8_SSCALED,
		VK_FORMAT_B8G8R8_UINT,
		VK_FORMAT_B8G8R8_SINT,
		VK_FORMAT_B8G8R8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_USCALED,
		VK_FORMAT_R8G8B8A8_SSCALED,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SNORM,
		VK_FORMAT_B8G8R8A8_USCALED,
		VK_FORMAT_B8G8R8A8_SSCALED,
		VK_FORMAT_B8G8R8A8_UINT,
		VK_FORMAT_B8G8R8A8_SINT,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_A8B8G8R8_UNORM_PACK32,
		VK_FORMAT_A8B8G8R8_SNORM_PACK32,
		VK_FORMAT_A8B8G8R8_USCALED_PACK32,
		VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
		VK_FORMAT_A8B8G8R8_UINT_PACK32,
		VK_FORMAT_A8B8G8R8_SINT_PACK32,
		VK_FORMAT_A8B8G8R8_SRGB_PACK32,
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_A2R10G10B10_SNORM_PACK32,
		VK_FORMAT_A2R10G10B10_USCALED_PACK32,
		VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
		VK_FORMAT_A2R10G10B10_UINT_PACK32,
		VK_FORMAT_A2R10G10B10_SINT_PACK32,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_SNORM_PACK32,
		VK_FORMAT_A2B10G10R10_USCALED_PACK32,
		VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
		VK_FORMAT_A2B10G10R10_UINT_PACK32,
		VK_FORMAT_A2B10G10R10_SINT_PACK32,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_USCALED,
		VK_FORMAT_R16_SSCALED,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_USCALED,
		VK_FORMAT_R16G16_SSCALED,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SNORM,
		VK_FORMAT_R16G16B16_USCALED,
		VK_FORMAT_R16G16B16_SSCALED,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_USCALED,
		VK_FORMAT_R16G16B16A16_SSCALED,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R64_UINT,
		VK_FORMAT_R64_SINT,
		VK_FORMAT_R64_SFLOAT,
		VK_FORMAT_R64G64_UINT,
		VK_FORMAT_R64G64_SINT,
		VK_FORMAT_R64G64_SFLOAT,
		VK_FORMAT_R64G64B64_UINT,
		VK_FORMAT_R64G64B64_SINT,
		VK_FORMAT_R64G64B64_SFLOAT,
		VK_FORMAT_R64G64B64A64_UINT,
		VK_FORMAT_R64G64B64A64_SINT,
		VK_FORMAT_R64G64B64A64_SFLOAT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT
	};
	return formats[static_cast<std::underlying_type_t<Format>>(format)];
}

auto translate_texel_filter(TexelFilter filter) -> VkFilter
{
	switch (filter)
	{
	case TexelFilter::Nearest:
		return VK_FILTER_NEAREST;
	case TexelFilter::Cubic_Image:
		return VK_FILTER_CUBIC_IMG;
	default:
	case TexelFilter::Linear:
		return VK_FILTER_LINEAR;
	}
}

auto translate_mipmap_mode(MipmapMode mode) -> VkSamplerMipmapMode
{
	switch (mode)
	{
	case MipmapMode::Nearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case MipmapMode::Linear:
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

auto translate_sampler_address_mode(SamplerAddress address) -> VkSamplerAddressMode
{
	switch (address)
	{
	case SamplerAddress::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case SamplerAddress::Mirrored_Repeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case SamplerAddress::Clamp_To_Border:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case SamplerAddress::Mirror_Clamp_To_Edge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case SamplerAddress::Clamp_To_Edge:
	default:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
}

auto translate_compare_op(CompareOp op) -> VkCompareOp
{
	switch (op)
	{
	case CompareOp::Less:
		return VK_COMPARE_OP_LESS;
	case CompareOp::Equal:
		return VK_COMPARE_OP_EQUAL;
	case CompareOp::Less_Or_Equal:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case CompareOp::Greater:
		return VK_COMPARE_OP_GREATER;
	case CompareOp::Not_Equal:
		return VK_COMPARE_OP_NOT_EQUAL;
	case CompareOp::Greater_Or_Equal:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case CompareOp::Always:
		return VK_COMPARE_OP_ALWAYS;
	case CompareOp::Never:
	default:
		return VK_COMPARE_OP_NEVER;
	}
}

auto translate_border_color(BorderColor color) -> VkBorderColor
{
	switch (color)
	{
	case BorderColor::Int_Transparent_Black:
		return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	case BorderColor::Float_Opaque_Black:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	case BorderColor::Int_Opaque_Black:
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	case BorderColor::Float_Opaque_White:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	case BorderColor::Int_Opaque_White:
		return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	case BorderColor::Float_Transparent_Black:
	default:
		return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	}
}

auto translate_attachment_load_op(AttachmentLoadOp loadOp) -> VkAttachmentLoadOp
{
	switch (loadOp)
	{
	case AttachmentLoadOp::Clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case AttachmentLoadOp::Dont_Care:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case AttachmentLoadOp::None:
		return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
	case AttachmentLoadOp::Load:
	default:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
}

auto translate_attachment_store_op(AttachmentStoreOp storeOp) -> VkAttachmentStoreOp
{
	switch (storeOp)
	{
	case AttachmentStoreOp::Dont_Care:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case AttachmentStoreOp::None:
		return VK_ATTACHMENT_STORE_OP_NONE;
	case AttachmentStoreOp::Store:
	default:
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
}


auto translate_sample_count(SampleCount samples) -> VkSampleCountFlagBits
{
	switch (samples)
	{
	case SampleCount::Sample_Count_1:
		return VK_SAMPLE_COUNT_1_BIT;
	case SampleCount::Sample_Count_2:
		return VK_SAMPLE_COUNT_2_BIT;
	case SampleCount::Sample_Count_4:
		return VK_SAMPLE_COUNT_4_BIT;
	case SampleCount::Sample_Count_8:
		return VK_SAMPLE_COUNT_8_BIT;
	case SampleCount::Sample_Count_16:
		return VK_SAMPLE_COUNT_16_BIT;
	case SampleCount::Sample_Count_32:
		return VK_SAMPLE_COUNT_32_BIT;
	case SampleCount::Sample_Count_64:
		return VK_SAMPLE_COUNT_64_BIT;
	default:
		return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	}
}

auto stride_for_shader_attrib_format(Format format) -> uint32
{
	switch (format)
	{
	case Format::R32_Int:
	case Format::R32_Uint:
	case Format::R32_Float:
		return 4;
	case Format::R32G32_Float:
		return 8;
	case Format::R32G32B32_Float:
		return 12;
	case Format::R32G32B32A32_Float:
		return 16;
	case Format::Undefined:
	default:
		return 0;
	}
}

auto translate_shader_attrib_format(Format format) -> VkFormat
{
	switch (format)
	{
	case Format::R32_Int:
		return VK_FORMAT_R32_SINT;
	case Format::R32_Uint:
		return VK_FORMAT_R32_UINT;
	case Format::R32_Float:
		return VK_FORMAT_R32_SFLOAT;
	case Format::R32G32_Float:
		return VK_FORMAT_R32G32_SFLOAT;
	case Format::R32G32B32_Float:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case Format::R32G32B32A32_Float:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case Format::Undefined:
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

auto translate_topology(TopologyType topology) -> VkPrimitiveTopology
{
	switch (topology)
	{
	case TopologyType::Point:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case TopologyType::Line:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case TopologyType::Line_Strip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case TopologyType::Triange_Strip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case TopologyType::Triangle_Fan:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	case TopologyType::Triangle:
	default:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

auto translate_polygon_mode(PolygonMode mode) -> VkPolygonMode
{
	switch (mode)
	{
	case PolygonMode::Line:
		return VK_POLYGON_MODE_LINE;
	case PolygonMode::Point:
		return VK_POLYGON_MODE_POINT;
	case PolygonMode::Fill:
	default:
		return VK_POLYGON_MODE_FILL;
	}
}

auto translate_cull_mode(CullingMode mode) -> VkCullModeFlags
{
	switch (mode)
	{
	case CullingMode::None:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_NONE);
	case CullingMode::Front:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_FRONT_BIT);
	case CullingMode::Front_And_Back:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_FRONT_AND_BACK);
	case CullingMode::Back:
	default:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_BACK_BIT);
	}
}

auto translate_front_face_dir(FrontFace face) -> VkFrontFace
{
	switch (face)
	{
	case FrontFace::Clockwise:
		return VK_FRONT_FACE_CLOCKWISE;
	case FrontFace::Counter_Clockwise:
	default:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

auto translate_blend_factor(BlendFactor factor) -> VkBlendFactor
{
	switch (factor)
	{
	case BlendFactor::One:
		return VK_BLEND_FACTOR_ONE;
	case BlendFactor::Src_Color:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case BlendFactor::One_Minus_Src_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case BlendFactor::Dst_Color:
		return VK_BLEND_FACTOR_DST_COLOR;
	case BlendFactor::One_Minus_Dst_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case BlendFactor::Src_Alpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case BlendFactor::One_Minus_Src_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BlendFactor::Dst_Alpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case BlendFactor::One_Minus_Dst_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case BlendFactor::Constant_Color:
		return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case BlendFactor::One_Minus_Constant_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case BlendFactor::Constant_Alpha:
		return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case BlendFactor::One_Minus_Constant_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	case BlendFactor::Src_Alpha_Saturate:
		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	case BlendFactor::Src1_Color:
		return VK_BLEND_FACTOR_SRC1_COLOR;
	case BlendFactor::One_Minus_Src1_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
	case BlendFactor::Src1_Alpha:
		return VK_BLEND_FACTOR_SRC1_ALPHA;
	case BlendFactor::One_Minus_Src1_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	case BlendFactor::Zero:
	default:
		return VK_BLEND_FACTOR_ZERO;
	}
}

auto translate_blend_op(BlendOp op) -> VkBlendOp
{
	switch (op)
	{
	case BlendOp::Subtract:
		return VK_BLEND_OP_SUBTRACT;
	case BlendOp::Reverse_Subtract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case BlendOp::Add:
	default:
		return VK_BLEND_OP_ADD;
	}
}

auto get_sampler_info_packed_uint64(SamplerInfo const& info) -> uint64
{
	uint64 packed = 0;
	uint64 const minf = static_cast<uint64>(info.minFilter);
	uint64 const magf = static_cast<uint64>(info.magFilter);
	uint64 const mipm = static_cast<uint64>(info.mipmapMode);
	uint64 const admu = static_cast<uint64>(info.addressModeU);
	uint64 const admv = static_cast<uint64>(info.addressModeV);
	uint64 const admw = static_cast<uint64>(info.addressModeW);
	uint64 const cmpe = (info.compareOp != CompareOp::Never) ? 1ull : 0ull;
	uint64 const anie = (info.maxAnisotropy > 0.f) ? 1ull : 0ull;
	uint64 const cmpo = static_cast<uint64>(info.compareOp);
	uint64 const bcol = static_cast<uint64>(info.borderColor);
	uint64 const cord = (info.unnormalizedCoordinates) ? 1ull : 0ull;

	uint64 const mplb = static_cast<uint64>(info.mipLodBias);
	uint64 const maxa = static_cast<uint64>(info.maxAnisotropy);
	uint64 const minl = static_cast<uint64>(info.minLod);
	uint64 const maxl = static_cast<uint64>(info.maxLod);

	packed |= 0x000000000000000FULL & minf;
	packed |= 0x00000000000000F0ULL & magf;
	packed |= 0x0000000000000F00ULL & mipm;
	packed |= 0x000000000000F000ULL & bcol;
	packed |= 0x00000000000F0000ULL & admu;
	packed |= 0x0000000000F00000ULL & admv;
	packed |= 0x000000000F000000ULL & admw;
	packed |= 0x00000000F0000000ULL & cmpo;
	packed |= 0x0000000F00000000ULL & mplb;
	packed |= 0x000000F000000000ULL & maxa;
	packed |= 0x00000F0000000000ULL & minl;
	packed |= 0x0000F00000000000ULL & maxl;
	packed |= 0x2000000000000000ULL & anie;
	packed |= 0x4000000000000000ULL & cmpe;
	packed |= 0x8000000000000000ULL & cord;

	return packed;
}

auto translate_memory_usage(MemoryUsage usage) -> VmaAllocationCreateFlags
{
	uint32 const inputFlags = static_cast<uint32>(usage);
	VmaAllocationCreateFlagBits bits[] = {
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
		VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT,
		VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT
	};
	VmaAllocationCreateFlags result = 0;
	for (uint32 i = 0; i < static_cast<uint32>(std::size(bits)); ++i)
	{
		uint32 const exist = (inputFlags & (1u << i)) != 0u;
		result |= exist * bits[i];
	}
	return result;
}

auto ResourcePool::location_of(vulkan::Swapchain& swapchain) -> uint32
{
	return swapchains.index_of(swapchain).id;
}

auto ResourcePool::location_of(vulkan::Shader& shader) -> uint32
{
	return shaders.index_of(shader).id;
}

auto ResourcePool::location_of(vulkan::Buffer& buffer) -> uint32
{
	return buffers.index_of(buffer).id;
}

auto ResourcePool::location_of(vulkan::Image& image) -> uint32
{
	return images.index_of(image).id;
}

auto ResourcePool::location_of(vulkan::Sampler& sampler) -> uint32
{
	return samplers.index_of(sampler).id;
}

auto ResourcePool::location_of(vulkan::Pipeline& pipeline) -> uint32
{
	return pipelines.index_of(pipeline).id;
}

auto ResourcePool::location_of(vulkan::CommandPool& commandPool) -> uint32
{
	return commandPools.index_of(commandPool).id;
}

auto ResourcePool::location_of(vulkan::Semaphore& semaphore) -> uint32
{
	return semaphores.index_of(semaphore).id;
}

auto APIContext::create_vulkan_instance(DeviceInitInfo const& info) -> bool
{
	const auto& appVer = info.appVersion;
	const auto& engineVer = info.engineVersion;

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = info.appName.data(),
		.applicationVersion = VK_MAKE_API_VERSION(appVer.variant, appVer.major, appVer.minor, appVer.patch),
		.pEngineName = info.engineName.data(),
		.engineVersion = VK_MAKE_API_VERSION(engineVer.variant, engineVer.major, engineVer.minor, engineVer.patch),
		.apiVersion = VK_API_VERSION_1_3
	};

	lib::array<literal_t> extensions;
	extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if WIN32
	extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	if (validation)
	{
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	VkInstanceCreateInfo instanceInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = static_cast<uint32>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};

	if (validation)
	{
		literal_t layers[] = { "VK_LAYER_LUNARG_monitor", "VK_LAYER_KHRONOS_validation" };
		instanceInfo.enabledLayerCount = 2;
		instanceInfo.ppEnabledLayerNames = layers;
		auto debugUtil = populate_debug_messenger(const_cast<DeviceInitInfo*>(&info));
		instanceInfo.pNext = &debugUtil;
	}

	return vkCreateInstance(&instanceInfo, nullptr, &instance) == VK_SUCCESS;
}

auto APIContext::create_debug_messenger(DeviceInitInfo const& info) -> bool
{
	if (validation)
	{
		auto debugUtilInfo = populate_debug_messenger(const_cast<DeviceInitInfo*>(&info));
		return vkCreateDebugUtilsMessengerEXT(instance, &debugUtilInfo, nullptr, &debugger) == VK_SUCCESS;
	}
	return true;
}

auto APIContext::choose_physical_device() -> bool
{
	constexpr uint32 MAX_PHYSICAL_DEVICE = 8u;
	struct PhysicalDevices
	{
		std::array<VkPhysicalDevice, MAX_PHYSICAL_DEVICE> devices = {};
		size_t count = 0;
	};
	std::array<PhysicalDevices, 5> deviceTable = {};
	std::array<VkPhysicalDeviceType, 5> priority = {
		VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
		VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
		VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
		VK_PHYSICAL_DEVICE_TYPE_CPU,
		VK_PHYSICAL_DEVICE_TYPE_OTHER
	};

	uint32 numDevices = 0;
	vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);

	if (!numDevices) { return false; }

	if (numDevices > MAX_PHYSICAL_DEVICE)
	{
		numDevices = MAX_PHYSICAL_DEVICE;
	}

	VkPhysicalDevice devices[MAX_PHYSICAL_DEVICE] = { VK_NULL_HANDLE };
	vkEnumeratePhysicalDevices(instance, &numDevices, devices);

	for (uint32 i = 0; i < numDevices; ++i)
	{
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(devices[i], &prop);

		PhysicalDevices& physDevice = deviceTable[prop.deviceType];
		if (physDevice.count < MAX_PHYSICAL_DEVICE)
		{
			physDevice.devices[physDevice.count++] = devices[i];
		}
	}

	auto score_device = [&](VkPhysicalDevice& device) -> uint32
	{
		uint32 score = 0;
		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(device, &properties);

		score += static_cast<uint32>(properties.limits.maxMemoryAllocationCount / 1000u);
		score += static_cast<uint32>(properties.limits.maxBoundDescriptorSets / 1000u);
		score += static_cast<uint32>(properties.limits.maxDrawIndirectCount / 1000u);
		score += static_cast<uint32>(properties.limits.maxDrawIndexedIndexValue / 1000u);

		return score;
	};

	for (PhysicalDevices& entry : deviceTable)
	{
		std::sort(
			std::begin(entry.devices), 
			std::begin(entry.devices) + entry.count, 
			[score_device, this](VkPhysicalDevice& a, VkPhysicalDevice& b) -> bool
			{
				if (a == VK_NULL_HANDLE || b == VK_NULL_HANDLE)
				{
					return false;
				}
				uint32 aScore = score_device(a);
				uint32 bScore = score_device(b);
				return aScore > bScore;
			}
		);
	}

	auto preferredDeviceIndex = static_cast<std::underlying_type_t<DeviceType>>(init.preferredDevice);
	if (deviceTable[preferredDeviceIndex].count)
	{
		gpu = deviceTable[preferredDeviceIndex].devices[0];
	}
	else
	{
		for (VkPhysicalDeviceType type : priority)
		{
			if (deviceTable[type].count)
			{
				gpu = deviceTable[type].devices[0];
				break;
			}
		}
	}

	vkGetPhysicalDeviceProperties(gpu, &properties);
	vkGetPhysicalDeviceFeatures(gpu, &features);

	return true;
}

auto APIContext::get_device_queue_family_indices() -> void
{
	constexpr uint32 MAX_QUEUE_FAMILY_SEARCH = 8u;
	/**
	* The Vulkan spec states that implementation MUST support at least ONE queue family.
	* That means the posibility of the GPU not having a queue family is non-existent.
	*/
	constexpr uint32 uint32Max = std::numeric_limits<uint32>::max();

	uint32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);

	if (queueCount > MAX_QUEUE_FAMILY_SEARCH)
	{
		queueCount = MAX_QUEUE_FAMILY_SEARCH;
	}

	/*VkBool32 presentSupport[MAX_QUEUE_FAMILY_SEARCH];*/
	VkQueueFamilyProperties queueProperties[MAX_QUEUE_FAMILY_SEARCH];
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, queueProperties);

	for (uint32 i = 0; i < queueCount; ++i)
	{
		if (mainQueue.familyIndex		!= uint32Max &&
			transferQueue.familyIndex	!= uint32Max &&
			computeQueue.familyIndex	!= uint32Max)
		{
			break;
		}

		VkQueueFamilyProperties const& property = queueProperties[i];
		/*vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, i, m_swapchain.surface, &presentSupport[i]);*/
		if (property.queueCount)
		{
			// We look for a queue that's able to do graphics, transfer, compute and presentation.
			if (mainQueue.familyIndex == uint32Max &&
				/*presentSupport[i] &&*/
				(property.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0	&&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0	&&
				(property.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
			{
				mainQueue.familyIndex	= i;
				mainQueue.properties	= property;
			}
			// We look for a queue that can only do transfer operations.
			if (transferQueue.familyIndex == uint32Max &&
				(property.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0	&&
				(property.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0	&&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
			{
				transferQueue.familyIndex	= i;
				transferQueue.properties	= property;
			}
			// We look for a queue that is capable of doing compute but is not in the same family index as the main graphics queue.
			// This is for async compute.
			if (computeQueue.familyIndex == uint32Max &&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0	&&
				(mainQueue.familyIndex		!= i)					&&
				(transferQueue.familyIndex	!= i))
			{
				computeQueue.familyIndex	= i;
				computeQueue.properties		= property;
			}
		}
	}
	// If the transfer queue family index is still not set by this point, we fall back to the graphics queue.
	if (transferQueue.familyIndex == uint32Max)
	{
		transferQueue.familyIndex	= mainQueue.familyIndex;
		transferQueue.properties	= mainQueue.properties;
	}
	// If the compute queue family index is still not set by this point, we fall back to the graphics queue.
	if (computeQueue.familyIndex == uint32Max)
	{
		computeQueue.familyIndex	= mainQueue.familyIndex;
		computeQueue.properties		= mainQueue.properties;
	}
}

auto APIContext::create_logical_device() -> bool
{
	constexpr float32 priority = 1.f;
	size_t reserveCount = 3;

	if (mainQueue.familyIndex == transferQueue.familyIndex) 
	{ 
		--reserveCount; 
	}

	if (mainQueue.familyIndex == computeQueue.familyIndex) 
	{ 
		--reserveCount; 
	}

	lib::array<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(reserveCount);

	// Always create a graphics queue.
	queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = mainQueue.familyIndex,
		.queueCount = 1,
		.pQueuePriorities = &priority
	});

	// If the transfer queue and the graphics queue are different, we create one.
	if (transferQueue.familyIndex != mainQueue.familyIndex)
	{
		queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = transferQueue.familyIndex,
			.queueCount = 1,
			.pQueuePriorities = &priority
		});
	}
	// If the compute queue and the graphics queue are different, we create one.
	if (computeQueue.familyIndex != mainQueue.familyIndex)
	{
		queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = computeQueue.familyIndex,
			.queueCount = 1,
			.pQueuePriorities = &priority
		});
	}

	literal_t extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceVulkan13Features deviceFeatures13{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE
	};

	//VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeature{
	//	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
	//	.pNext = &deviceFeatures13,
	//	.bufferDeviceAddress = VK_TRUE
	//};

	VkPhysicalDeviceVulkan12Features deviceFeatures12{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &deviceFeatures13,
		.drawIndirectCount = VK_TRUE,
		.shaderInt8 = VK_TRUE,
		.descriptorIndexing = VK_TRUE,
		.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
		.shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
		.descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.scalarBlockLayout = VK_TRUE,
		.timelineSemaphore = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
		.vulkanMemoryModel = VK_TRUE
	};

	VkPhysicalDeviceFeatures2 deviceFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &deviceFeatures12,
		.features = {
			.fullDrawIndexUint32 = VK_TRUE,
			.multiDrawIndirect = VK_TRUE,
			.multiViewport = VK_TRUE,
			.samplerAnisotropy = VK_TRUE,
			.shaderUniformBufferArrayDynamicIndexing = VK_TRUE,
			.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
			.shaderStorageBufferArrayDynamicIndexing = VK_TRUE,
			.shaderStorageImageArrayDynamicIndexing = VK_TRUE,
			.shaderFloat64 = VK_TRUE,
			.shaderInt64 = VK_TRUE,
			.shaderInt16 = VK_TRUE
		}
	};

	//VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeature{
	//	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
	//	.pNext = &deviceFeatures,
	//	.bufferDeviceAddress = VK_TRUE
	//};

	VkDeviceCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &deviceFeatures,
		.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = extensions
	};

	return vkCreateDevice(gpu, &info, nullptr, &device) == VK_SUCCESS;
}

auto APIContext::get_queue_handles() -> void
{
	vkGetDeviceQueue(device, mainQueue.familyIndex, 0, &mainQueue.queue);
	vkGetDeviceQueue(device, transferQueue.familyIndex, 0, &transferQueue.queue);
	vkGetDeviceQueue(device, computeQueue.familyIndex, 0, &computeQueue.queue);
}

auto APIContext::create_device_allocator() -> bool
{
	VmaVulkanFunctions vmaVulkanFunctions{
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr
	};
	VmaAllocatorCreateInfo info{
		.flags							= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice					= gpu,
		.device							= device,
		.preferredLargeHeapBlockSize	= 0, // Sets it to lib internal default (256MiB).
		.pAllocationCallbacks			= nullptr,
		.pDeviceMemoryCallbacks			= nullptr,
		.pHeapSizeLimit					= nullptr,
		.pVulkanFunctions				= &vmaVulkanFunctions,
		.instance						= instance,
		.vulkanApiVersion				= VK_API_VERSION_1_3,
	};
	return vmaCreateAllocator(&info, &allocator) == VK_SUCCESS;
}

auto APIContext::clear_zombies() -> void
{
	// The device has to stop using the resources before we can clean them up.
	/**
	* TODO(afiq):
	* Figure out a way to not wait for the device to idle before releasing the used resource.
	*/
	vkDeviceWaitIdle(device);

	for (vulkan::Zombie const& zombie : gpuResourcePool.zombies)
	{
		if (zombie.type == vulkan::Resource::Swapchain)
		{
			destroy_swapchain_zombie(zombie.location);
		}
		else if (zombie.type == vulkan::Resource::Surface)
		{
			destroy_surface_zombie(zombie.address);
		}
		else if (zombie.type == vulkan::Resource::Shader)
		{
			destroy_shader_zombie(zombie.location);
		}
		else if (zombie.type ==  vulkan::Resource::Buffer)
		{
			destroy_buffer_zombie(zombie.location);
		}
		else if (zombie.type ==  vulkan::Resource::Image)
		{
			destroy_image_zombie(zombie.location);
		}
		else if (zombie.type == vulkan::Resource::Sampler)
		{
			destroy_sampler_zombie(zombie.location);
		}
		else if (zombie.type == vulkan::Resource::CommandPool)
		{
			destroy_command_pool_zombie(zombie.location);
		}
		else if (zombie.type == vulkan::Resource::CommandBuffer)
		{
			destroy_command_buffer_zombie(zombie.address, static_cast<size_t>(zombie.location));
		}
		else if (zombie.type == vulkan::Resource::Semaphore)
		{
			destroy_semaphore_zombie(zombie.location);
		}
	}
}

auto APIContext::initialize_resource_pool() -> void
{
	// Readjust configuration values.
	config.maxImages = std::min(std::min(properties.limits.maxDescriptorSetSampledImages, properties.limits.maxDescriptorSetStorageImages), config.maxImages);
	config.maxBuffers = std::min(properties.limits.maxDescriptorSetStorageBuffers, config.maxBuffers);
	config.maxSamplers = std::min(properties.limits.maxDescriptorSetSamplers, config.maxSamplers);
	config.pushConstantMaxSize = std::min(properties.limits.maxPushConstantsSize, config.pushConstantMaxSize);
}

auto APIContext::cleanup_resource_pool() -> void
{
	// Don't need to call vkDeviceWaitIdle here since it's already invoked in the function below.
	clear_zombies();
	clear_descriptor_cache();
}

auto APIContext::destroy_surface_zombie(std::uintptr_t address) -> void
{
	vulkan::Surface& surface = gpuResourcePool.surfaces[address];
	vkDestroySurfaceKHR(instance, surface.handle, nullptr);
	gpuResourcePool.surfaces.erase(address);
}

auto APIContext::destroy_swapchain_zombie(uint32 location) -> void
{
	vulkan::Swapchain& swapchain = gpuResourcePool.swapchains[location];
	for (auto& imageView : swapchain.imageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}
	vkDestroySwapchainKHR(device, swapchain.handle, nullptr);
	gpuResourcePool.swapchains.erase(location);
}

auto APIContext::destroy_shader_zombie(uint32 location) -> void
{
	vulkan::Shader& shader = gpuResourcePool.shaders[location];
	vkDestroyShaderModule(device, shader.handle, nullptr);
	gpuResourcePool.shaders.erase(location);
}

auto APIContext::destroy_buffer_zombie(uint32 location) -> void
{
	vulkan::Buffer& buffer = gpuResourcePool.buffers[location];
	vmaDestroyBuffer(allocator, buffer.handle, buffer.allocation);
	gpuResourcePool.buffers.erase(location);
}

auto APIContext::destroy_image_zombie(uint32 location) -> void
{
	vulkan::Image& image = gpuResourcePool.images[location];
	vmaDestroyImage(allocator, image.handle, image.allocation);
	gpuResourcePool.images.erase(location);
}

auto APIContext::destroy_sampler_zombie(uint32 location) -> void
{
	vulkan::Sampler& sampler = gpuResourcePool.samplers[location];
	vkDestroySampler(device, sampler.handle, nullptr);
	gpuResourcePool.samplers.erase(location);
}

auto APIContext::destroy_command_pool_zombie(uint32 location) -> void
{
	vulkan::CommandPool& cmdPool = gpuResourcePool.commandPools[location];
	std::uintptr_t address = reinterpret_cast<std::uintptr_t>(&cmdPool);
	vkDestroyCommandPool(device, cmdPool.handle, nullptr);
	gpuResourcePool.commandPools.erase(location);
	gpuResourcePool.commandBufferPools.erase(address);
}

auto APIContext::destroy_command_buffer_zombie(std::uintptr_t address, size_t location) -> void
{
	vulkan::CommandBufferPool& cmdBufferPool = gpuResourcePool.commandBufferPools[address];
	
	// The standard says uintptr_t is an unsigned integer capable of holding a pointer.
	// But I'm not too sure if the line of code below would work.
	// Worth experimenting.
	VkCommandPool commandPool = reinterpret_cast<VkCommandPool>(address);
	vkFreeCommandBuffers(device, commandPool, 1, &cmdBufferPool.commandBuffers[location].handle);
	lib::memzero(&cmdBufferPool.commandBuffers[location], sizeof(vulkan::CommandBuffer));

	// Increase free slot ring buffer counter and assign free index.
	cmdBufferPool.freeSlots[cmdBufferPool.currentFreeSlot] = location;
	++cmdBufferPool.freeSlotCount;
	cmdBufferPool.currentFreeSlot = (cmdBufferPool.currentFreeSlot + 1) % MAX_COMMAND_BUFFER_PER_POOL;
}

auto APIContext::destroy_semaphore_zombie(uint32 location) -> void
{
	vulkan::Semaphore& semaphore = gpuResourcePool.semaphores[location];
	vkDestroySemaphore(device, semaphore.handle, nullptr);
	gpuResourcePool.semaphores.erase(location);
}

auto APIContext::create_surface(SurfaceInfo const& info) -> vulkan::Surface*
{
	std::uintptr_t key = reinterpret_cast<std::uintptr_t>(info.instance) | reinterpret_cast<std::uintptr_t>(info.window);
	VkSurfaceKHR handle;

#ifdef PLATFORM_OS_WINDOWS
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{
		.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance	= reinterpret_cast<HINSTANCE>(info.instance),
		.hwnd		= reinterpret_cast<HWND>(info.window)
	};

	VkResult result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &handle);
	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		return nullptr;
	}
#elif PLATFORM_OS_LINUX
#endif

	vulkan::Surface& surfaceResource = gpuResourcePool.surfaces[key];
	surfaceResource.handle = handle;

	uint32 count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, handle, &count, nullptr);

	surfaceResource.availableColorFormats.reserve(static_cast<size_t>(count));
	surfaceResource.availableColorFormats.resize(static_cast<size_t>(count));

	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, handle, &count, surfaceResource.availableColorFormats.data());
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, handle, &surfaceResource.capabilities);

	return &surfaceResource;
}

auto APIContext::create_descriptor_pool() -> bool
{
	VkDescriptorPoolSize bufferPoolSize{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = config.maxBuffers + 1
	};

	VkDescriptorPoolSize storageImagePoolSize{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = config.maxImages
	};

	VkDescriptorPoolSize sampledImagePoolSize{
		.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = config.maxImages
	};

	VkDescriptorPoolSize samplerPoolSize{
		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = config.maxSamplers
	};

	VkDescriptorPoolSize poolSizes[] = {
		bufferPoolSize,
		storageImagePoolSize,
		sampledImagePoolSize,
		samplerPoolSize
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets = 1,
		.poolSizeCount = static_cast<uint32>(std::size(poolSizes)),
		.pPoolSizes = poolSizes,
	};

	return vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorCache.descriptorPool) == VK_SUCCESS;
}

auto APIContext::create_descriptor_set_layout() -> bool
{
	VkDescriptorSetLayoutBinding storageImageLayout{
		.binding = STORAGE_IMAGE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding combinedImageSamplerLayout{
		.binding = COMBINED_IMAGE_SAMPLER_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding sampledImageLayout{
		.binding = SAMPLED_IMAGE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding samplerLayout{
		.binding = SAMPLER_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = config.maxSamplers,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding bufferDeviceAddressLayout{
		.binding = BUFFER_DEVICE_ADDRESS_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
		storageImageLayout,
		combinedImageSamplerLayout,
		sampledImageLayout,
		samplerLayout,
		bufferDeviceAddressLayout
	};

	VkDescriptorBindingFlags bindingFlags[] = {
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = static_cast<uint32>(std::size(bindingFlags)),
		.pBindingFlags = bindingFlags,
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &layoutBindingFlagsCreateInfo,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = static_cast<uint32>(std::size(descriptorSetLayoutBindings)),
		.pBindings = descriptorSetLayoutBindings,
	};
	
	return vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorCache.descriptorSetLayout) == VK_SUCCESS;
}

auto APIContext::allocate_descriptor_set() -> bool
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorCache.descriptorPool,
		.descriptorSetCount = 1u,
		.pSetLayouts = &descriptorCache.descriptorSetLayout,
	};
	return vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorCache.descriptorSet) == VK_SUCCESS;
}

auto APIContext::create_pipeline_layouts() -> bool
{
	// The vulkan spec states that the size of a push constant must be a multiple of 4.
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPushConstantRange.html
	uint32 constexpr multiple = 4u;
	uint32 const count = static_cast<uint32>(config.pushConstantMaxSize / multiple) + 1u;

	descriptorCache.pipelineLayouts.reserve(count);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1u,
		.pSetLayouts = &descriptorCache.descriptorSetLayout,
	};

	VkPipelineLayout layout0;
	if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout0) != VK_SUCCESS)
	{
		return false;
	}
	descriptorCache.pipelineLayouts.emplace(0u, std::move(layout0));

	for (uint32 i = 1u; i < count; ++i)
	{
		VkPushConstantRange range{
			.stageFlags = VK_SHADER_STAGE_ALL,
			.offset = 0u,
			.size = static_cast<uint32>(i * multiple)
		};

		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &range;

		VkPipelineLayout hlayout;
		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &hlayout) == VK_SUCCESS)
		{
			descriptorCache.pipelineLayouts.emplace(range.size, hlayout);
		}
	}
	return true;
}

auto APIContext::get_appropriate_pipeline_layout(uint32 pushConstantSize, const uint32 max) -> VkPipelineLayout
{
	uint32 num = std::min(pushConstantSize, max);
	if (pushConstantSize < max)
	{
		--num;
		for (uint32 i = 1; i < sizeof(uint32) * CHAR_BIT; i *= 2)
		{
			num |= num >> i;
		}
		++num;
	}
	return descriptorCache.pipelineLayouts[num];
}

auto APIContext::initialize_descriptor_cache() -> bool
{
	if (!create_descriptor_pool())
	{
		return false;
	}
	if (!create_descriptor_set_layout())
	{
		return false;
	}
	if (!allocate_descriptor_set())
	{
		return false;
	}
	if (!create_pipeline_layouts())
	{
		return false;
	}

	// Set up buffer device address.
	{	
		// Set up buffer device address.
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = config.maxBuffers * sizeof(uint64),
			.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		VmaAllocationCreateInfo allocInfo{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		};

		if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &descriptorCache.bdaBuffer, &descriptorCache.bdaAllocation, nullptr) == VK_SUCCESS)
		{
			vmaMapMemory(allocator, descriptorCache.bdaAllocation, reinterpret_cast<void**>(&descriptorCache.bdaHostAddress));

			VkDescriptorBufferInfo descriptorBufferInfo{
				.buffer = descriptorCache.bdaBuffer,
				.offset = 0,
				.range = VK_WHOLE_SIZE,
			};

			VkWriteDescriptorSet bdaWriteInfo{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorCache.descriptorSet,
				.dstBinding = BUFFER_DEVICE_ADDRESS_BINDING,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &descriptorBufferInfo
			};

			vkUpdateDescriptorSets(device, 1, &bdaWriteInfo, 0, nullptr);
		}
	}


	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		VkDebugUtilsObjectNameInfoEXT debugObjectNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(descriptorCache.descriptorPool),
			.pObjectName = "<descriptor_pool>:application",
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

		debugObjectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
		debugObjectNameInfo.objectHandle = reinterpret_cast<uint64_t>(descriptorCache.descriptorSetLayout);
		debugObjectNameInfo.pObjectName = "<descriptor_set_layout>:application";
		vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

		debugObjectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
		debugObjectNameInfo.objectHandle = reinterpret_cast<uint64_t>(descriptorCache.descriptorSet);
		debugObjectNameInfo.pObjectName = "<descriptor_set>:application";
		vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

		lib::string layoutName{ 256 };
		for (auto&& [size, layout] : descriptorCache.pipelineLayouts)
		{
			layoutName.format("<pipeline_layout>:push_constant_size = {} bytes", size);

			debugObjectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
			debugObjectNameInfo.objectHandle = reinterpret_cast<uint64_t>(layout);
			debugObjectNameInfo.pObjectName = layoutName.c_str();
			vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

			layoutName.clear();
		}
	}

	return true;
}

auto APIContext::clear_descriptor_cache() -> void
{
	vmaUnmapMemory(allocator, descriptorCache.bdaAllocation);
	vmaDestroyBuffer(allocator, descriptorCache.bdaBuffer, descriptorCache.bdaAllocation);
	// Remove pre-allocated pipeline layouts.
	for (auto const& [_, layout] : descriptorCache.pipelineLayouts)
	{
		vkDestroyPipelineLayout(device, layout, nullptr);
	}
	descriptorCache.pipelineLayouts.clear();

	// Remove all descriptor related items.
	vkFreeDescriptorSets(device, descriptorCache.descriptorPool, 1, &descriptorCache.descriptorSet);
	vkDestroyDescriptorSetLayout(device, descriptorCache.descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorCache.descriptorPool, nullptr);

	descriptorCache.descriptorSet = VK_NULL_HANDLE;
	descriptorCache.descriptorSetLayout = VK_NULL_HANDLE;
	descriptorCache.descriptorPool = VK_NULL_HANDLE;
}

bool APIContext::initialize(DeviceInitInfo const& info)
{
	init = info;
	config = info.config;
	validation = info.validation;

	volkInitialize();

	if (!create_vulkan_instance(init))
	{ 
		return false; 
	}

	volkLoadInstance(instance);

	if (!create_debug_messenger(init)) 
	{ 
		return false; 
	}

	if (!choose_physical_device()) 
	{ 
		return false; 
	}

	get_device_queue_family_indices();

	if (!create_logical_device()) 
	{ 
		return false; 
	}

	// Set up debug markers for the VkQueues.
	//if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	//{
	//	VkDebugUtilsObjectNameInfoEXT queueDebugObjectName{
	//		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
	//		.objectType = VK_OBJECT_TYPE_QUEUE,
	//		.objectHandle = reinterpret_cast<uint64>(mainQueue.queue),
	//		.pObjectName = "<queue>:main",
	//	};
	//	vkSetDebugUtilsObjectNameEXT(device, &queueDebugObjectName);

	//	queueDebugObjectName.pObjectName = "<queue>:compute";
	//	queueDebugObjectName.objectHandle = reinterpret_cast<uint64>(computeQueue.queue);
	//	vkSetDebugUtilsObjectNameEXT(device, &queueDebugObjectName);

	//	queueDebugObjectName.pObjectName = "<queue>:transfer";
	//	queueDebugObjectName.objectHandle = reinterpret_cast<uint64>(transferQueue.queue);
	//	vkSetDebugUtilsObjectNameEXT(device, &queueDebugObjectName);
	//}

	/**
	* To avoid a dispatch overhead (~7%), we tell volk to directly load device-related Vulkan entrypoints.
	*/
	volkLoadDevice(device);
	get_queue_handles();

	if (!create_device_allocator()) 
	{ 
		return false; 
	}

	initialize_resource_pool();

	if (!initialize_descriptor_cache())
	{
		return false;
	}

	return true;
}

auto APIContext::terminate() -> void
{
	flush_submit_info_buffers();
	cleanup_resource_pool();

	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, nullptr);

	if (validation)
	{
		vkDestroyDebugUtilsMessengerEXT(instance, debugger, nullptr);
	}
	gpu = VK_NULL_HANDLE;
	vkDestroyInstance(instance, nullptr);
}

auto APIContext::create_swapchain(SwapchainInfo&& info, Swapchain* oldSwapchain) -> Swapchain
{
	if (!info.surfaceInfo.instance || 
		!info.surfaceInfo.window)
	{
		return Swapchain{};
	}

	std::uintptr_t surfaceAddress = reinterpret_cast<std::uintptr_t>(info.surfaceInfo.instance) | reinterpret_cast<std::uintptr_t>(info.surfaceInfo.window);
	auto cachedSurface = gpuResourcePool.surfaces.at(surfaceAddress);
	vulkan::Surface* pSurface = nullptr;

	vulkan::Swapchain* pOldSwapchain = nullptr;
	if (oldSwapchain &&
		oldSwapchain->valid())
	{
		pOldSwapchain = &oldSwapchain->as<vulkan::Swapchain>();
	}

	if (cachedSurface.is_null())
	{
		pSurface = create_surface(info.surfaceInfo);
	}
	else
	{
		pSurface = &cachedSurface->second;
	}

	VkExtent2D extent{
		std::min(info.dimension.width, pSurface->capabilities.currentExtent.width),
		std::min(info.dimension.height, pSurface->capabilities.currentExtent.height)
	};
	uint32 imageCount{ std::min(std::max(info.imageCount, pSurface->capabilities.minImageCount), pSurface->capabilities.maxImageCount) };

	VkImageUsageFlags imageUsage = translate_image_usage_flags(info.imageUsage);
	VkPresentModeKHR presentationMode = translate_swapchain_presentation_mode(info.presentationMode);

	bool foundPreferred = false;
	VkSurfaceFormatKHR surfaceFormat = pSurface->availableColorFormats[0];

	for (Format format : info.surfaceInfo.preferredSurfaceFormats)
	{
		VkFormat preferredFormat = translate_format(format);

		if (foundPreferred)
		{
			break;
		}

		for (VkSurfaceFormatKHR colorFormat : pSurface->availableColorFormats)
		{
			if (preferredFormat == colorFormat.format)
			{
				surfaceFormat = colorFormat;
				foundPreferred = true;
				break;
			}
		}
	}

	VkSwapchainKHR hnd;
	VkSwapchainCreateInfoKHR swapchainCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface				= pSurface->handle,
		.minImageCount			= imageCount,
		.imageFormat			= surfaceFormat.format,
		.imageColorSpace		= surfaceFormat.colorSpace,
		.imageExtent			= extent,
		.imageArrayLayers		= 1,
		.imageUsage				= imageUsage,
		.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount	= 1,
		.pQueueFamilyIndices	= &mainQueue.familyIndex,
		.preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode			= presentationMode,
		.clipped				= VK_TRUE,
		.oldSwapchain			= (pOldSwapchain != nullptr) ? pOldSwapchain->handle : nullptr
	};

	VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &hnd);
	ASSERTION(result == VK_SUCCESS && "Error! Failed to create swapchain.");
	if (result != VK_SUCCESS)
	{
		return Swapchain{};
	}

	auto [index, swapchainResource] = gpuResourcePool.swapchains.emplace(pSurface, hnd, surfaceFormat);

	swapchainResource.images.reserve(imageCount);
	swapchainResource.images.resize(imageCount);
	swapchainResource.imageViews.reserve(imageCount);
	swapchainResource.imageViews.resize(imageCount);

	vkGetSwapchainImagesKHR(device, swapchainResource.handle, &imageCount, swapchainResource.images.data());

	VkImageViewCreateInfo imageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = surfaceFormat.format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};
	for (uint32 i = 0; i < imageCount; ++i)
	{
		VkImage img = swapchainResource.images[i];
		imageViewInfo.image = img;
		vkCreateImageView(device, &imageViewInfo, nullptr, &swapchainResource.imageViews[i]);
	}
	// Update info to contain the updated values.
	info.name.format("<swapchain>:{}", info.name.c_str());
	info.surfaceInfo.name.format("<surface>:{}", info.surfaceInfo.name.c_str());
	info.imageCount = imageCount;
	info.dimension.width = extent.width;
	info.dimension.height = extent.height;

	return Swapchain{ std::move(info), this, &swapchainResource, resource_type_id_v<vulkan::Swapchain> };
}

auto APIContext::destroy_swapchain(Swapchain& swapchain, bool destroySurface) -> void
{
	if (swapchain.valid())
	{
		Resource& base = swapchain;
		SwapchainInfo const& info = swapchain.info();
		std::uintptr_t surfaceAddress = reinterpret_cast<std::uintptr_t>(info.surfaceInfo.instance) | 
										reinterpret_cast<std::uintptr_t>(info.surfaceInfo.window);
		vulkan::Swapchain& swapchainResource = base.as<vulkan::Swapchain>();
		uint32 location = gpuResourcePool.location_of(swapchainResource);
		gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Swapchain);
		if (destroySurface)
		{
			gpuResourcePool.zombies.emplace_back(surfaceAddress, 0u, vulkan::Resource::Surface);
		}
	}
	swapchain.~Swapchain();
}

auto APIContext::create_shader(CompiledShaderInfo&& info) -> Shader
{
	if (!info.binaries.size())
	{
		return Shader{};
	}

	static constexpr std::string_view shader_types[] = {
		"vertex",
		"pixel",
		"geometry",
		"tesselation_control",
		"tesselation_evaluation",
		"compute"
	};

	auto [index, shader] = gpuResourcePool.shaders.emplace();

	VkShaderModuleCreateInfo shaderInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = info.binaries.size_bytes(),
		.pCode = info.binaries.data()
	};

	VkResult result = vkCreateShaderModule(device, &shaderInfo, nullptr, &shader.handle);
	if (result != VK_SUCCESS)
	{
		gpuResourcePool.shaders.erase(index);
		return Shader{};
	}
	shader.stage = translate_shader_stage(info.type);

	using index_type = std::underlying_type_t<ShaderType>;
	lib::string name{ "<shader:{}>:{}", shader_types[(index_type)info.type].data(), info.path }; 

	return Shader{ ShaderInfo{ .name = std::move(name), .type = info.type, .entryPoint = info.entryPoint }, this, &shader, resource_type_id_v<vulkan::Shader>};
}

auto APIContext::destroy_shader(Shader& shader) -> void
{
	if (shader.valid())
	{
		Resource& base = shader;
		vulkan::Shader& shaderResource = base.as<vulkan::Shader>();
		uint32 location = gpuResourcePool.location_of(shaderResource);
		gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Shader);
	}
	shader.~Shader();
}

auto APIContext::allocate_buffer(BufferInfo&& info) -> Buffer
{
	auto [index, buffer] = gpuResourcePool.buffers.emplace();

	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = info.size,
		.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | translate_buffer_usage_flags(info.bufferUsage),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	auto allocationFlags = translate_memory_usage(info.memoryUsage);

	if ((allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) != 0 ||
		(allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT) != 0)
	{
		allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	VmaAllocationCreateInfo allocInfo{
		.flags = allocationFlags,
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	};

	VmaAllocationInfo allocationInfo = {};
	VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.handle, &buffer.allocation, &allocationInfo);

	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		gpuResourcePool.buffers.erase(index);
		return Buffer{};
	}

	VkBufferDeviceAddressInfo addressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_EXT,
		.buffer = buffer.handle
	};
	buffer.address = vkGetBufferDeviceAddress(device, &addressInfo);

	info.name.format("<buffer>:{}", info.name.c_str());

	return Buffer{ std::move(info), allocationInfo.pMappedData, this, &buffer, resource_type_id_v<vulkan::Buffer> };
}

auto APIContext::release_buffer(Buffer& buffer) -> void
{
	if (buffer.valid())
	{
		Resource& base = buffer;
		vulkan::Buffer& bufferResource = base.as<vulkan::Buffer>();
		uint32 location = gpuResourcePool.location_of(bufferResource);
		gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Buffer);
	}
	buffer.~Buffer();
}

auto APIContext::create_image(ImageInfo&& info) -> Image
{
	auto [index, image] = gpuResourcePool.images.emplace();
	constexpr uint32 MAX_IMAGE_MIP_LEVEL = 4u;

	VkFormat format = translate_format(info.format);
	VkImageUsageFlags usage = translate_image_usage_flags(info.imageUsage);

	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
	{
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	uint32 depth{ 1 };

	if (info.dimension.depth > 1)
	{
		depth = info.dimension.depth;
	}

	uint32 mipLevels{ 1 };

	if (info.mipLevel > 1)
	{
		const float32 width = static_cast<const float32>(info.dimension.width);
		const float32 height = static_cast<const float32>(info.dimension.height);

		mipLevels = static_cast<uint32>(std::floorf(std::log2f(std::max(width, height)))) + 1u;
		mipLevels = std::min(mipLevels, MAX_IMAGE_MIP_LEVEL);
	}

	VkImageCreateInfo imgInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = translate_image_type(info.type),
		.format	= format,
		.extent	= VkExtent3D{ info.dimension.width, info.dimension.width, depth },
		.mipLevels	= mipLevels,
		.arrayLayers = 1,
		.samples = translate_sample_count(info.samples),
		.tiling	= translate_image_tiling(info.tiling),
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	auto allocationFlags = translate_memory_usage(info.memoryUsage);
	
	if ((allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) != 0 ||
		(allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT) != 0)
	{
		allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	VmaAllocationCreateInfo allocInfo{
		.flags = allocationFlags,
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE 
	};
	VmaAllocationInfo allocationInfo = {};

	VkResult result = vmaCreateImage(allocator, &imgInfo, &allocInfo, &image.handle, &image.allocation, &allocationInfo);
	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		gpuResourcePool.images.erase(index);
		return Image{};
	}

	VkImageViewCreateInfo imgViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image.handle,
		.viewType = translate_image_view_type(info.type),
		.format	= format,
		.subresourceRange = {
			.aspectMask	= aspectFlags,
			.baseMipLevel = 0,
			.levelCount	= mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	vkCreateImageView(device, &imgViewInfo, nullptr, &image.imageView);

	return Image{ std::move(info), this, &image, resource_type_id_v<vulkan::Image> };
}

auto APIContext::destroy_image(Image& image) -> void
{
	if (image.valid())
	{
		Resource& base = image;
		vulkan::Image& imageResource = base.as<vulkan::Image>();
		uint32 location = gpuResourcePool.location_of(imageResource);
		gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Image);
	}
	image.~Image();
}

auto APIContext::create_sampler(SamplerInfo&& info) -> Sampler
{
	uint64 packed = get_sampler_info_packed_uint64(info);

	if (gpuResourcePool.samplerCache.contains(packed))
	{
		uint32 location = gpuResourcePool.samplerCache[packed];
		vulkan::Sampler& existingSampler = gpuResourcePool.samplers[location];
		++existingSampler.references;

		return Sampler{ std::move(info), packed, this, &existingSampler, resource_type_id_v<vulkan::Sampler> };
	}

	auto [index, sampler] = gpuResourcePool.samplers.emplace();

	++sampler.references;

	VkSamplerCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = translate_texel_filter(info.magFilter),
		.minFilter = translate_texel_filter(info.minFilter),
		.mipmapMode = translate_mipmap_mode(info.mipmapMode),
		.addressModeU = translate_sampler_address_mode(info.addressModeU),
		.addressModeV = translate_sampler_address_mode(info.addressModeV),
		.addressModeW = translate_sampler_address_mode(info.addressModeW),
		.mipLodBias = info.mipLodBias,
		.anisotropyEnable = (info.maxAnisotropy > 0.f) ? VK_TRUE : VK_FALSE,
		.maxAnisotropy = info.maxAnisotropy,
		.compareEnable = (info.compareOp != CompareOp::Never) ? VK_TRUE : VK_FALSE,
		.compareOp = translate_compare_op(info.compareOp),
		.minLod = info.minLod,
		.maxLod = info.maxLod,
		.borderColor = translate_border_color(info.borderColor),
		.unnormalizedCoordinates = info.unnormalizedCoordinates ? VK_TRUE : VK_FALSE
	};

	VkResult result = vkCreateSampler(device, &createInfo, nullptr, &sampler.handle);
	if (result != VK_SUCCESS)
	{
		gpuResourcePool.samplers.erase(index);
		return Sampler{};
	}
	// Store the sampler info's hash.
	gpuResourcePool.samplerCache[packed] = index.id;

	return Sampler{ std::move(info), packed, this, &sampler, resource_type_id_v<vulkan::Sampler> };
}

auto APIContext::destroy_sampler(Sampler& sampler) -> void
{
	if (sampler.valid())
	{
		Resource& base = sampler;
		vulkan::Sampler& samplerResource = base.as<vulkan::Sampler>();

		if (samplerResource.references == 1)
		{
			gpuResourcePool.samplerCache.erase(sampler.m_packed_info);
			uint32 location = gpuResourcePool.location_of(samplerResource);
			gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Sampler);
		}
		else
		{
			--samplerResource.references;
		}
	}
	sampler.~Sampler();
}

auto APIContext::create_pipeline(RasterPipelineInfo&& info, PipelineShaderInfo const& pipelineShaders) -> RasterPipeline
{
	/**
	* TODO(afiq):
	* Add support for compute, tesselation control, tesselation evaluation and geometry shaders.
 	**/
	// It is necessary for raster pipelines to have a vertex shader and fragment shader.
	if (!pipelineShaders.vertexShader || 
		!pipelineShaders.pixelShader)
	{
		return RasterPipeline{};
	}

	auto [index, rasterPipeline] = gpuResourcePool.pipelines.emplace();

	vulkan::Shader& vertexShader = pipelineShaders.vertexShader->as<vulkan::Shader>();
	vulkan::Shader& pixelShader = pipelineShaders.pixelShader->as<vulkan::Shader>();

	// Vertex shader.
	VkPipelineShaderStageCreateInfo vertexStage{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = vertexShader.stage,
		.module = vertexShader.handle,
		.pName = pipelineShaders.vertexShader->info().entryPoint.c_str()
	};

	// Pixel shader.
	VkPipelineShaderStageCreateInfo pixelShadingStage{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = pixelShader.stage,
		.module = pixelShader.handle,
		.pName = pipelineShaders.pixelShader->info().entryPoint.c_str()
	};

	std::array shaderStages = { vertexStage, pixelShadingStage };

	uint32 numBindings = 0;
	uint32 numAttributes = 0;
	std::array<VkVertexInputBindingDescription, 32> inputBindings{};
	std::array<VkVertexInputAttributeDescription, 64> attributeDescriptions{};

	for (VertexInputBinding const& input : info.vertexInputBindings)
	{
		inputBindings[numBindings] = {
			.binding = numBindings,
			.stride = input.stride,
			.inputRate = (input.instanced) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX
		};
		if (pipelineShaders.vertexInputAttributes.size())
		{
			uint32 stride = 0;
			for (uint32 i = input.from; i <= input.to; ++i)
			{
				auto&& attribute = pipelineShaders.vertexInputAttributes[i];
				attributeDescriptions[numAttributes] = {
					.location = attribute.location,
					.binding = numBindings,
					.format = translate_shader_attrib_format(attribute.format),
					.offset = stride,
				};
				stride += stride_for_shader_attrib_format(attribute.format);
				++numAttributes;
			}
		}
		++numBindings;
	}

	VkPipelineVertexInputStateCreateInfo pipelineVertexState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = numBindings,
		.pVertexBindingDescriptions	= inputBindings.data(),
		.vertexAttributeDescriptionCount = numAttributes,
		.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssembly{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = translate_topology(info.topology),
		.primitiveRestartEnable = VK_FALSE
	};

	// NOTE(Afiq):
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkGraphicsPipelineCreateInfo.html
	// The vulkan spec states that a VkPipelineViewportStateCreateInfo is unnecessary for pipelines with dynamic states "VK_DYNAMIC_STATE_VIEWPORT" and "VK_DYNAMIC_STATE_SCISSOR".
	/*VkViewport viewport{ 0.f, 0.f, info.viewport.width, info.viewport.height, 0.f, 1.0f };

	VkRect2D scissor{
		.offset = {
			.x = info.scissor.offset.x,
			.y = info.scissor.offset.y
		},
		.extent = {
			.width = info.scissor.extent.width,
			.height = info.scissor.extent.height
		}
	};*/

	VkPipelineViewportStateCreateInfo pipelineViewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo pipelineDynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = states
	};

	VkPipelineRasterizationStateCreateInfo pipelineRasterState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = info.rasterization.enableDepthClamp,
		.polygonMode = translate_polygon_mode(info.rasterization.polygonalMode),
		.cullMode = translate_cull_mode(info.rasterization.cullMode),
		.frontFace = translate_front_face_dir(info.rasterization.frontFace),
		.lineWidth = 1.f
	};

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleSate{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.f
	};

	size_t attachmentCount = info.colorAttachments.size();
	ASSERTION(
		((attachmentCount <= properties.limits.maxColorAttachments) && (attachmentCount < 32)) &&
		"attachmentCount must not exceed the device's maxColorAttachments limit. If the device can afford more, we limit to 32."
	);
	std::array<VkPipelineColorBlendAttachmentState, 32> colorAttachmentBlendStates{};
	std::array<VkFormat, 32> colorAttachmentFormats{};

	for (size_t i = 0; i < attachmentCount; ++i)
	{
		auto const& attachment = info.colorAttachments[i];
		colorAttachmentBlendStates[i] = {
			.blendEnable = attachment.blendInfo.enable ? VK_TRUE : VK_FALSE,
			.srcColorBlendFactor = translate_blend_factor(attachment.blendInfo.srcColorBlendFactor),
			.dstColorBlendFactor = translate_blend_factor(attachment.blendInfo.dstColorBlendFactor),
			.colorBlendOp = translate_blend_op(attachment.blendInfo.colorBlendOp),
			.srcAlphaBlendFactor = translate_blend_factor(attachment.blendInfo.srcAlphaBlendFactor),
			.dstAlphaBlendFactor = translate_blend_factor(attachment.blendInfo.dstAlphaBlendFactor),
			.alphaBlendOp = translate_blend_op(attachment.blendInfo.alphaBlendOp),
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		colorAttachmentFormats[i] = translate_format(attachment.format);
	}

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_CLEAR,
		.attachmentCount = static_cast<uint32>(attachmentCount),
		.pAttachments = colorAttachmentBlendStates.data(),
		.blendConstants	= { 1.f, 1.f, 1.f, 1.f }
	};

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = info.depthTest.enableDepthTest,
		.depthWriteEnable = info.depthTest.enableDepthWrite,
		.depthCompareOp	= translate_compare_op(info.depthTest.depthTestCompareOp),
		.depthBoundsTestEnable = info.depthTest.enableDepthBoundsTest,
		.minDepthBounds = info.depthTest.minDepthBounds,
		.maxDepthBounds	= info.depthTest.maxDepthBounds
	};

	VkPipelineRenderingCreateInfo pipelineRendering{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount = static_cast<uint32>(attachmentCount),
		.pColorAttachmentFormats = colorAttachmentFormats.data(),
		.depthAttachmentFormat = VK_FORMAT_UNDEFINED,
		.stencilAttachmentFormat = VK_FORMAT_UNDEFINED
	};

	VkPipelineLayout layoutHandle = get_appropriate_pipeline_layout(info.pushConstantSize, properties.limits.maxPushConstantsSize);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &pipelineRendering,
		.stageCount	= (uint32)shaderStages.size(),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pipelineVertexState,
		.pInputAssemblyState = &pipelineInputAssembly,
		.pTessellationState = nullptr,
		.pViewportState = &pipelineViewportState,
		.pRasterizationState = &pipelineRasterState,
		.pMultisampleState = &pipelineMultisampleSate,
		.pDepthStencilState = &pipelineDepthStencilState,
		.pColorBlendState = &pipelineColorBlendState,
		.pDynamicState = &pipelineDynamicState,
		.layout	= layoutHandle,
		.renderPass	= nullptr,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0
	};

	// TODO(Afiq):
	// The only way this can fail is when we run out of host / device memory OR shader linkage has failed.
	VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &rasterPipeline.handle);
	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		gpuResourcePool.pipelines.erase(index);
		return RasterPipeline{};
	}

	rasterPipeline.layout = layoutHandle;

	info.name.format("<pipeline>:{}", info.name.c_str());

	return RasterPipeline{ std::move(info), this, &rasterPipeline, resource_type_id_v<vulkan::Pipeline> };
}

auto APIContext::destroy_pipeline(RasterPipeline& pipeline) -> void
{
	if (pipeline.valid())
	{
		destroy_pipeline_internal(pipeline);
	}
	pipeline.~RasterPipeline();
}

auto APIContext::destroy_pipeline_internal(Pipeline& pipeline) -> void
{
	vulkan::Pipeline& pipelineResource = pipeline.as<vulkan::Pipeline>();
	uint32 location = gpuResourcePool.location_of(pipelineResource);
	gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Pipeline);
}

auto APIContext::flush_submit_info_buffers() -> void
{
	wait_semaphores.clear();
	signal_semaphores.clear();
	wait_dst_stage_masks.clear();
	wait_timeline_semaphore_values.clear();
	signal_timeline_semaphore_values.clear();
	submit_command_buffers.clear();
	present_swapchains.clear();
	present_image_indices.clear();
}

auto APIContext::clear_destroyed_resources() -> void
{
	clear_zombies();
}

auto APIContext::wait_idle() -> void
{
	vkDeviceWaitIdle(device);
}

auto APIContext::create_command_pool(CommandPoolInfo&& info) -> CommandPool
{
	vulkan::Queue* pQueue = nullptr;
	switch (info.queue)
	{
	case DeviceQueueType::Transfer:
		pQueue = &transferQueue;
		break;
	case DeviceQueueType::Compute:
		pQueue = &computeQueue;
		break;
	case DeviceQueueType::Main:
	default:
		pQueue = &mainQueue;
		break;
	}
	VkCommandPoolCreateInfo commandPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = pQueue->familyIndex
	};
	VkCommandPool hcommandPool = VK_NULL_HANDLE;
	VkResult result = vkCreateCommandPool(device, &commandPoolInfo, nullptr, &hcommandPool);
	if (result != VK_SUCCESS)
	{
		return CommandPool{};
	}
	auto [index, commandPoolResource] = gpuResourcePool.commandPools.emplace(hcommandPool);

	info.name.format("<command_pool>:{}", info.name.c_str());

	return CommandPool{ std::move(info), this, &commandPoolResource, resource_type_id_v<vulkan::CommandPool> };
}

auto APIContext::destroy_command_pool(CommandPool& commandPool) -> void
{
	if (commandPool.valid())
	{
		vulkan::CommandPool& cmdPool = commandPool.as<vulkan::CommandPool>();
		uint32 location = gpuResourcePool.location_of(cmdPool);
		gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::CommandPool);
	}
	commandPool.~CommandPool();
}

auto APIContext::create_binary_semaphore(SemaphoreInfo&& info) -> Semaphore
{
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	VkSemaphore semaphoreHandle = VK_NULL_HANDLE;
	VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphoreHandle);
	if (result != VK_SUCCESS)
	{
		return Semaphore{};
	}
	auto [index, binarySemaphoreResource] = gpuResourcePool.semaphores.emplace(semaphoreHandle, VK_SEMAPHORE_TYPE_BINARY);

	info.name.format("<semaphore>:{}", info.name.c_str());

	return Semaphore{ std::move(info), this, &binarySemaphoreResource, resource_type_id_v<vulkan::Semaphore> };
}

auto APIContext::destroy_binary_semaphore(Semaphore& semaphore) -> void
{
	if (semaphore.valid())
	{
		vulkan::Semaphore& binarySemaphore = semaphore.as<vulkan::Semaphore>();
		uint32 location = gpuResourcePool.location_of(binarySemaphore);
		gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Semaphore);
	}
	semaphore.~Semaphore();
}

auto APIContext::create_timeline_semaphore(FenceInfo&& info) -> Fence
{
	VkSemaphoreTypeCreateInfo timelineSemaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = info.initialValue
	};
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &timelineSemaphoreInfo
	};
	VkSemaphore semaphoreHandle = VK_NULL_HANDLE;
	VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphoreHandle);
	if (result != VK_SUCCESS)
	{
		return Fence{};
	}
	auto [index, semaphoreResource] = gpuResourcePool.semaphores.emplace(semaphoreHandle, VK_SEMAPHORE_TYPE_TIMELINE);

	info.name.format("<timeline_semaphore>:{}", info.name.c_str());

	return Fence{ std::move(info), this, &semaphoreResource, resource_type_id_v<vulkan::Semaphore> };
}

auto APIContext::destroy_timeline_semaphore(Fence& fence) -> void
{
	if (fence.valid())
	{
		vulkan::Semaphore& timelineSemaphore = fence.as<vulkan::Semaphore>();
		uint32 location = gpuResourcePool.location_of(timelineSemaphore);
		gpuResourcePool.zombies.emplace_back(0ull, location, vulkan::Resource::Semaphore);
	}
	fence.~Fence();
}

auto APIContext::submit(SubmitInfo const& info) -> bool
{
	flush_submit_info_buffers();

	VkQueue queue = mainQueue.queue;

	if (info.queue == rhi::DeviceQueueType::Transfer)
	{
		queue = transferQueue.queue;
	}
	else if (info.queue == rhi::DeviceQueueType::Compute)
	{
		queue = computeQueue.queue;
	}

	for (CommandBuffer& submittedCmdBuffer : info.commandBuffers)
	{
		if (submittedCmdBuffer.m_state == CommandBuffer::State::Executable)
		{
			vulkan::Semaphore& semaphore = submittedCmdBuffer.m_completion_timeline.as<vulkan::Semaphore>();
			vulkan::CommandBuffer& cmdBuffer = submittedCmdBuffer.as<vulkan::CommandBuffer>();
			submit_command_buffers.push_back(cmdBuffer.handle);
			
			signal_semaphores.push_back(semaphore.handle);
			signal_timeline_semaphore_values.push_back(submittedCmdBuffer.m_recording_timeline);

			submittedCmdBuffer.m_state = CommandBuffer::State::Pending;
		}
	}

	for (auto&& [fence, waitValue] : info.waitFences)
	{
		vulkan::Semaphore& timelineSemaphore = (*fence).as<vulkan::Semaphore>();
		wait_semaphores.push_back(timelineSemaphore.handle);
		wait_timeline_semaphore_values.push_back(waitValue);
		wait_dst_stage_masks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	for (Semaphore const& waitSemaphore : info.waitSemaphores)
	{
		vulkan::Semaphore& semaphore = waitSemaphore.as<vulkan::Semaphore>();
		wait_semaphores.push_back(semaphore.handle);
		wait_timeline_semaphore_values.push_back(0);
		wait_dst_stage_masks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	for (auto&& [fence, signalValue] : info.signalFences)
	{
		vulkan::Semaphore& timelineSemaphore = (*fence).as<vulkan::Semaphore>();
		signal_semaphores.push_back(timelineSemaphore.handle);
		signal_timeline_semaphore_values.push_back(signalValue);
	}

	for (Semaphore const& signalSemaphore : info.signalSemaphores)
	{
		vulkan::Semaphore& semaphore = signalSemaphore.as<vulkan::Semaphore>();
		signal_semaphores.push_back(semaphore.handle);
		signal_timeline_semaphore_values.push_back(0);
	}

	VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.waitSemaphoreValueCount = static_cast<uint32>(wait_timeline_semaphore_values.size()),
		.pWaitSemaphoreValues = wait_timeline_semaphore_values.data(),
		.signalSemaphoreValueCount = static_cast<uint32>(signal_timeline_semaphore_values.size()),
		.pSignalSemaphoreValues = signal_timeline_semaphore_values.data()
	};

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = &timelineSemaphoreSubmitInfo,
		.waitSemaphoreCount = static_cast<uint32>(wait_semaphores.size()),
		.pWaitSemaphores = wait_semaphores.data(),
		.pWaitDstStageMask = wait_dst_stage_masks.data(),
		.commandBufferCount = static_cast<uint32>(submit_command_buffers.size()),
		.pCommandBuffers = submit_command_buffers.data(),
		.signalSemaphoreCount = static_cast<uint32>(signal_semaphores.size()),
		.pSignalSemaphores = signal_semaphores.data()
	};

	return vkQueueSubmit(queue, 1, &submitInfo, nullptr) == VK_SUCCESS;
}

auto APIContext::present(PresentInfo const& info) -> bool
{
	flush_submit_info_buffers();

	for (Swapchain const& swapchain : info.swapchains)
	{
		vulkan::Swapchain& vulkanSwapchain = swapchain.as<vulkan::Swapchain>();
		vulkan::Semaphore& presentSemaphore = swapchain.current_present_semaphore().as<vulkan::Semaphore>();

		present_swapchains.push_back(vulkanSwapchain.handle);
		present_image_indices.push_back(swapchain.m_next_image_index);
		wait_semaphores.push_back(presentSemaphore.handle);
	}

	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = static_cast<uint32>(wait_semaphores.size()),
		.pWaitSemaphores = wait_semaphores.data(),
		.swapchainCount = static_cast<uint32>(present_swapchains.size()),
		.pSwapchains = present_swapchains.data(),
		.pImageIndices = present_image_indices.data(),
	};

	return vkQueuePresentKHR(mainQueue.queue, &presentInfo) == VK_SUCCESS;
}

auto APIContext::setup_debug_name(Swapchain const& swapchain) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = swapchain.info().name;
		if (name.size())
		{
			vulkan::Swapchain& swp = swapchain.as<vulkan::Swapchain>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext = nullptr,
				.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
				.objectHandle = reinterpret_cast<uint64_t>(swp.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(Shader const& shader) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = shader.info().name;
		if (name.size())
		{
			vulkan::Shader& shdr = shader.as<vulkan::Shader>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_SHADER_MODULE,
				.objectHandle = reinterpret_cast<uint64_t>(shdr.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(Buffer const& buffer) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = buffer.info().name;
		if (name.size())
		{
			vulkan::Buffer& bff = buffer.as<vulkan::Buffer>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_BUFFER,
				.objectHandle = reinterpret_cast<uint64_t>(bff.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(Image const& image) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = image.info().name;
		if (name.size())
		{
			vulkan::Image& img = image.as<vulkan::Image>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = reinterpret_cast<uint64_t>(img.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(Sampler const& sampler) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = sampler.info().name;
		if (name.size())
		{
			vulkan::Sampler& smp = sampler.as<vulkan::Sampler>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_SAMPLER,
				.objectHandle = reinterpret_cast<uint64_t>(smp.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(RasterPipeline const& pipeline) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = pipeline.info().name;
		if (name.size())
		{
			vulkan::Pipeline& pln = pipeline.as<vulkan::Pipeline>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_PIPELINE,
				.objectHandle = reinterpret_cast<uint64_t>(pln.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(CommandPool const& commandPool) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = commandPool.info().name;
		if (name.size())
		{
			vulkan::CommandPool& cmdpl = commandPool.as<vulkan::CommandPool>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
				.objectHandle = reinterpret_cast<uint64_t>(cmdpl.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(CommandBuffer const& commandBuffer) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = commandBuffer.info().name;
		if (name.size())
		{
			vulkan::CommandBuffer& cmd = commandBuffer.as<vulkan::CommandBuffer>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
				.objectHandle = reinterpret_cast<uint64_t>(cmd.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(Semaphore const& semaphore) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = semaphore.info().name;
		if (name.size())
		{
			vulkan::Semaphore& sem = semaphore.as<vulkan::Semaphore>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_SEMAPHORE,
				.objectHandle = reinterpret_cast<uint64_t>(sem.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

auto APIContext::setup_debug_name(Fence const& fence) -> void
{
	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		auto&& name = fence.info().name;
		if (name.size())
		{
			vulkan::Semaphore& sem = fence.as<vulkan::Semaphore>();
			VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_SEMAPHORE,
				.objectHandle = reinterpret_cast<uint64_t>(sem.handle),
				.pObjectName = name.c_str(),
			};
			vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
		}
	}
}

//auto APIContext::update_descriptor_set_buffer(VkBuffer buffer, size_t offset, size_t size, uint32 index) -> void
//{
//	VkDescriptorBufferInfo bufferInfo{
//		.buffer = buffer,
//		.offset = offset,
//		.range = size,
//	};
//	VkWriteDescriptorSet writeDescriptorSetBuffer{
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = descriptorCache.descriptorSet,
//		.dstBinding = BUFFER_BINDING,
//		.dstArrayElement = index,
//		.descriptorCount = 1,
//		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//		.pBufferInfo = &bufferInfo
//	};
//	vkUpdateDescriptorSets(device, 1, &writeDescriptorSetBuffer, 0, nullptr);
//}

//auto APIContext::update_descriptor_set_image(VkImageView imageView, ImageUsage usage, uint32 index) -> void
//{
//	uint32 count = 0;
//	std::array<VkWriteDescriptorSet, 2> descriptorSetWrites{};
//
//	VkDescriptorImageInfo imageInfo{
//		.sampler = VK_NULL_HANDLE,
//		.imageView = imageView,
//		.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
//	};
//
//	VkDescriptorImageInfo storageImageInfo{
//		.sampler = VK_NULL_HANDLE,
//		.imageView = imageView,
//		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
//	};
//
//	if ((usage & ImageUsage::Sampled) != ImageUsage::None)
//	{
//		descriptorSetWrites[count++] = {
//			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//			.dstSet = descriptorCache.descriptorSet,
//			.dstBinding = SAMPLED_IMAGE_BINDING,
//			.dstArrayElement = index,
//			.descriptorCount = 1,
//			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
//			.pImageInfo = &imageInfo
//		};
//	}
//
//	if ((usage & ImageUsage::Storage) != ImageUsage::None)
//	{
//		descriptorSetWrites[count++] = {
//			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//			.dstSet = descriptorCache.descriptorSet,
//			.dstBinding = STORAGE_IMAGE_BINDING,
//			.dstArrayElement = index,
//			.descriptorCount = 1,
//			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
//			.pImageInfo = &storageImageInfo
//		};
//	}
//
//	vkUpdateDescriptorSets(device, count, descriptorSetWrites.data(), 0, nullptr);
//}

//auto APIContext::update_descriptor_set_sampler(VkSampler sampler, uint32 index) -> void
//{
//	VkDescriptorImageInfo samplerInfo{
//		.sampler = sampler,
//		.imageView = VK_NULL_HANDLE,
//		.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
//	};
//
//	VkWriteDescriptorSet writeDescriptorSetImage{
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = descriptorCache.descriptorSet,
//		.dstBinding = SAMPLER_BINDING,
//		.dstArrayElement = index,
//		.descriptorCount = 1,
//		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
//		.pImageInfo = &samplerInfo
//	};
//	vkUpdateDescriptorSets(device, 1, &writeDescriptorSetImage, 0, nullptr);
//}

auto Device::device_info() const -> DeviceInfo const&
{
	return m_info;
}

auto Device::device_config() const -> DeviceConfig const&
{
	return m_context->config;
}

auto Device::create_swapchain(SwapchainInfo&& info, Swapchain* oldSwapchain) -> Swapchain
{
	return m_context->create_swapchain(std::move(info), oldSwapchain);
}

auto Device::destroy_swapchain(Swapchain& swapchain, bool destroySurface) -> void
{
	m_context->destroy_swapchain(swapchain, destroySurface);
}

auto Device::create_shader(CompiledShaderInfo&& info) -> Shader
{
	return m_context->create_shader(std::move(info));
}

auto Device::destroy_shader(Shader& shader) -> void
{
	m_context->destroy_shader(shader);
}

auto Device::allocate_buffer(BufferInfo&& info) -> Buffer
{
	return m_context->allocate_buffer(std::move(info));
}

auto Device::release_buffer(Buffer& buffer) -> void
{
	m_context->release_buffer(buffer);
}

auto Device::create_image(ImageInfo&& info) -> Image
{
	return m_context->create_image(std::move(info));
}

auto Device::destroy_image(Image& image) -> void
{
	m_context->destroy_image(image);
}

auto Device::create_sampler(SamplerInfo&& info) -> Sampler
{
	return m_context->create_sampler(std::move(info));
}

auto Device::destroy_sampler(Sampler& sampler) -> void
{
	m_context->destroy_sampler(sampler);
}

auto Device::create_pipeline(RasterPipelineInfo&& info, PipelineShaderInfo const& pipelineShaders) -> RasterPipeline
{
	return m_context->create_pipeline(std::move(info), pipelineShaders);
}

auto Device::destroy_pipeline(RasterPipeline& pipeline) -> void
{
	m_context->destroy_pipeline(pipeline);
}

auto Device::clear_destroyed_resources() -> void
{
	m_context->clear_destroyed_resources();
}

auto Device::wait_idle() -> void
{
	m_context->wait_idle();
}

auto Device::create_command_pool(CommandPoolInfo&& info) -> CommandPool
{
	return m_context->create_command_pool(std::move(info));
}

auto Device::destroy_command_pool(CommandPool& commandPool) -> void
{
	m_context->destroy_command_pool(commandPool);
}

auto Device::create_semaphore(SemaphoreInfo&& info) -> Semaphore
{
	return m_context->create_binary_semaphore(std::move(info));
}

auto Device::destroy_semaphore(Semaphore& semaphore) -> void
{
	m_context->destroy_binary_semaphore(semaphore);
}

auto Device::create_fence(FenceInfo&& info) -> Fence
{
	return m_context->create_timeline_semaphore(std::move(info));
}

auto Device::destroy_fence(Fence& fence) -> void
{
	m_context->destroy_timeline_semaphore(fence);
}

auto Device::submit(SubmitInfo const& info) -> bool
{
	return m_context->submit(info);
}

auto Device::present(PresentInfo const& info) -> bool
{
	return m_context->present(info);
}

auto Device::initialize(DeviceInitInfo const& info) -> bool
{
	static constexpr std::pair<uint32, literal_t> vendorIdToName[] = {
		{ 0x1002u, "AMD" },
		{ 0x1010u, "ImgTec" },
		{ 0x10DEu, "NVIDIA" },
		{ 0x13B5u, "ARM" },
		{ 0x5143u, "Qualcomm" },
		{ 0x8086u, "INTEL" }
	};

	static constexpr DeviceType deviceTypeMap[] = {
		DeviceType::Other,
		DeviceType::Integrate_Gpu,
		DeviceType::Discrete_Gpu,
		DeviceType::Virtual_Gpu,
		DeviceType::Cpu
	};

	m_context = static_cast<APIContext*>(lib::allocate_memory({ .size = sizeof(APIContext) }));
	if (!m_context)
	{
		return false;
	}
	new (m_context) APIContext{};

	bool initialized = m_context->initialize(info);

	if (initialized)
	{
		m_info = {
			.name = m_context->init.name,
			.type = deviceTypeMap[m_context->properties.deviceType],
			.api = API::Vulkan,
			.shaderLang = m_context->init.shadingLanguage,
			.vendorID = m_context->properties.vendorID,
			.deviceID = m_context->properties.deviceID,
			.deviceName = m_context->properties.deviceName,
			.apiVersion = {
				.major = VK_API_VERSION_MAJOR(m_context->properties.apiVersion),
				.minor = VK_API_VERSION_MINOR(m_context->properties.apiVersion),
				.patch = VK_API_VERSION_PATCH(m_context->properties.apiVersion)
			},
			.driverVersion = {
				.major = VK_API_VERSION_MAJOR(m_context->properties.driverVersion),
				.minor = VK_API_VERSION_MINOR(m_context->properties.driverVersion),
				.patch = VK_API_VERSION_PATCH(m_context->properties.driverVersion)
			}
		};

		std::for_each(
			std::begin(vendorIdToName),
			std::end(vendorIdToName),
			[&](const std::pair<uint32, literal_t> vendorInfo) -> void
			{
				if (m_context->properties.vendorID == vendorInfo.first)
				{
					m_info.vendor = vendorInfo.second;
				}
			}
		);
	}

	return initialized;
}

auto Device::terminate() -> void
{
	m_context->terminate();
	lib::release_memory(m_context);
}

}