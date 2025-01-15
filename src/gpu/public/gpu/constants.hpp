#pragma once
#ifndef GPU_CONSTANTS_H
#define GPU_CONSTANTS_H

#include "lib/common.hpp"

namespace gpu
{
// renderer constants.
inline constexpr uint32 COMMAND_BUFFER_PIPELINE_BARRIER_BATCH_SIZE = 16;
inline constexpr uint32 MAX_PIPELINE_COLOR_ATTACHMENT = 16;
inline constexpr uint32 MAX_COMMAND_BUFFER_ATTACHMENT = 16;
inline constexpr uint32 MAX_COMMAND_BUFFER_PER_POOL = 16;
inline constexpr uint32 MAX_FRAMES_IN_FLIGHT = 4;
// resource limits.
inline constexpr uint32 MAX_BUFFERS = 10'000;
inline constexpr uint32 MAX_IMAGES = 10'000;
inline constexpr uint32 MAX_SAMPLERS = 100;
// shader bindings.
inline constexpr uint32 STORAGE_IMAGE_BINDING = 0;
inline constexpr uint32 COMBINED_IMAGE_SAMPLER_BINDING = 1;
inline constexpr uint32 SAMPLED_IMAGE_BINDING = 2;
inline constexpr uint32 SAMPLER_BINDING = 3;
inline constexpr uint32 BUFFER_DEVICE_ADDRESS_BINDING = 4;
inline constexpr uint32 STORAGE_BUFFER_BINDING = 5;
inline constexpr uint32 UNIFORM_BUFFER_BINDING = 6;

#if _DEBUG
inline constexpr uint32 ENABLE_DEBUG_RESOURCE_NAMES = 1;
#else
inline constexpr uint32 ENABLE_DEBUG_RESOURCE_NAMES = 0;
#endif

// debug utilities.

// Enums and flags.
enum class API
{
	Vulkan
};

enum class ErrorSeverity
{
	Verbose,
	Info,
	Warning,
	Error
};

enum class DeviceType
{
	Other,
	Integrate_Gpu,
	Discrete_Gpu,
	Virtual_Gpu,
	Cpu
};

enum class DeviceQueue
{
	None,
	Main,
	Graphics = Main,
	Transfer,
	Compute
};

enum class PipelineStage : uint32
{
	Top_Of_Pipe	= 1,
	Draw_Indirect = 1 << 1,
	Vertex_Input = 1 << 2,
	Vertex_Shader = 1 << 3,
	Tesselation_Control = 1 << 4,
	Tesselation_Evaluation = 1 << 5,
	Geometry_Shader	= 1 << 6,
	Fragment_Shader	= 1 << 7,
	Pixel_Shader = Fragment_Shader,
	Early_Fragment_Test = 1 << 8,
	Late_Fragment_Test = 1 << 9,
	Color_Attachment_Output = 1 << 10,
	Compute_Shader = 1 << 11,
	Transfer = 1 << 12,
	Bottom_Of_Pipe = 1 << 13,
	Host = 1 << 14,
	All_Graphics = 1 << 15,
	All_Commands = 1 << 16,
	Copy = 1 << 17,
	Resolve = 1 << 18,
	Blit = 1 << 19,
	Clear = 1 << 20,
	Index_Input = 1 << 21,
	Vertex_Attribute_Input = 1 << 22,
	Pre_Rasterization_Shaders = 1 << 23,
	Acceleration_Structure_Build = 1 << 24,
	Ray_Tracing_Shader = 1 << 25,
	Task_Shader = 1 << 26,
	Mesh_Shader = 1 << 27,
	Acceleration_Structure_Copy = 1 << 28,
	None = 0
};

enum class MemoryAccessType : uint32
{
	None = 0,
	Host_Read = 1 << 0,
	Host_Write = 1 << 1,
	Host_Read_Write = Host_Read | Host_Write,
	Memory_Read = 1 << 2,
	Memory_Write = 1 << 3,
	Memory_Read_Write = Memory_Read | Memory_Write
};

enum class TopologyType
{
	Point,
	Line,
	Line_Strip,
	Triangle,
	Triange_Strip,
	Triangle_Fan
};

enum class PolygonMode
{
	Fill,
	Line,
	Point
};

enum class FrontFace
{
	Clockwise,
	Counter_Clockwise
};

enum class CullingMode
{
	None,
	Back,
	Front,
	Front_And_Back
};

enum class BlendFactor
{
	Zero,
	One,
	Src_Color,
	One_Minus_Src_Color,
	Dst_Color,
	One_Minus_Dst_Color,
	Src_Alpha,
	One_Minus_Src_Alpha,
	Dst_Alpha,
	One_Minus_Dst_Alpha,
	Constant_Color,
	One_Minus_Constant_Color,
	Constant_Alpha,
	One_Minus_Constant_Alpha,
	Src_Alpha_Saturate,
	Src1_Color,
	One_Minus_Src1_Color,
	Src1_Alpha,
	One_Minus_Src1_Alpha
};

enum class BlendOp
{
	Add,
	Subtract,
	Reverse_Subtract
};

enum class SampleCount
{
	Sample_Count_1,
	Sample_Count_2,
	Sample_Count_4,
	Sample_Count_8,
	Sample_Count_16,
	Sample_Count_32,
	Sample_Count_64
};

enum class ImageType
{
	Image_1D,
	Image_2D,
	Image_3D
};

enum class Format
{
	Undefined,
	R4G4_Unorm_Pack8,
	R4G4B4_Unorm_Pack16,
	B4G4R4_Unorm_Pack16,
	R5G6B5_Unorm_Pack16,
	B5G6R5_Unorm_Pack16,
	R5G5B5A1_Unorm_Pack16,
	B5G5R5A1_Unorm_Pack16,
	A1R5G5B5_Unorm_Pack16,
	R8_Unorm,
	R8_Norm,
	R8_Uscaled,
	R8_Scaled,
	R8_Uint,
	R8_Int,
	R8_Srgb,
	R8G8_Unorm,
	R8G8_Norm,
	R8G8_Uscaled,
	R8G8_Scaled,
	R8G8_Uint,
	R8G8_Int,
	R8G8_Srgb,
	R8G8B8_Unorm,
	R8G8B8_Norm,
	R8G8B8_Uscaled,
	R8G8B8_Scaled,
	R8G8B8_Uint,
	R8G8B8_Int,
	R8G8B8_Srgb,
	B8G8R8_Unorm,
	B8G8R8_Norm,
	B8G8R8_Uscaled,
	B8G8R8_Scaled,
	B8G8R8_Uint,
	B8G8R8_Int,
	B8G8R8_Srgb,
	R8G8B8A8_Unorm,
	R8G8B8A8_Norm,
	R8G8B8A8_Uscaled,
	R8G8B8A8_Scaled,
	R8G8B8A8_Uint,
	R8G8B8A8_Int,
	R8G8B8A8_Srgb,
	B8G8R8A8_Unorm,
	B8G8R8A8_Norm,
	B8G8R8A8_Uscaled,
	B8G8R8A8_Scaled,
	B8G8R8A8_Uint,
	B8G8R8A8_Int,
	B8G8R8A8_Srgb,
	A8B8G8R8_Unorm_Pack32,
	A8B8G8R8_Norm_Pack32,
	A8B8G8R8_Uscaled_Pack32,
	A8B8G8R8_Scaled_Pack32,
	A8B8G8R8_Uint_Pack32,
	A8B8G8R8_Int_Pack32,
	A8B8G8R8_Srgb_Pack32,
	A2R10G10B10_Unorm_Pack32,
	A2R10G10B10_Norm_Pack32,
	A2R10G10B10_Uscaled_Pack32,
	A2R10G10B10_Scaled_Pack32,
	A2R10G10B10_Uint_Pack32,
	A2R10G10B10_Int_Pack32,
	A2B10G10R10_Unorm_Pack32,
	A2B10G10R10_Norm_Pack32,
	A2B10G10R10_Uscaled_Pack32,
	A2B10G10R10_Scaled_Pack32,
	A2B10G10R10_Uint_Pack32,
	A2B10G10R10_Int_Pack32,
	R16_Unorm,
	R16_Norm,
	R16_Uscaled,
	R16_Scaled,
	R16_Uint,
	R16_Int,
	R16_Float,
	R16G16_Unorm,
	R16G16_Norm,
	R16G16_Uscaled,
	R16G16_Scaled,
	R16G16_Uint,
	R16G16_Int,
	R16G16_Float,
	R16G16B16_Unorm,
	R16G16B16_Norm,
	R16G16B16_Uscaled,
	R16G16B16_Scaled,
	R16G16B16_Uint,
	R16G16B16_Int,
	R16G16B16_Float,
	R16G16B16A16_Unorm,
	R16G16B16A16_Norm,
	R16G16B16A16_Uscaled,
	R16G16B16A16_Scaled,
	R16G16B16A16_Uint,
	R16G16B16A16_Int,
	R16G16B16A16_Float,
	R32_Uint,
	R32_Int,
	R32_Float,
	R32G32_Uint,
	R32G32_Int,
	R32G32_Float,
	R32G32B32_Uint,
	R32G32B32_Int,
	R32G32B32_Float,
	R32G32B32A32_Uint,
	R32G32B32A32_Int,
	R32G32B32A32_Float,
	R64_Uint,
	R64_Int,
	R64_Float,
	R64G64_Uint,
	R64G64_Int,
	R64G64_Float,
	R64G64B64_Uint,
	R64G64B64_Int,
	R64G64B64_Float,
	R64G64B64A64_Uint,
	R64G64B64A64_Int,
	R64G64B64A64_Float,
	B10G11R11_UFloat_Pack32,
	E5B9G9R9_UFloat_Pack32,
	D16_Unorm,
	X8_D24_Unorm_Pack32,
	D32_Float,
	S8_Uint,
	D16_Unorm_S8_Uint,
	D24_Unorm_S8_Uint,
	D32_Float_S8_Uint,
	Bc1_Rgb_Unorm_Block,
	Bc1_Rgb_Srgb_Block,
	Bc1_Rgba_Unorm_Block,
	Bc1_Rgba_Srgb_Block,
	Bc2_Unorm_Block,
	Bc2_Srgb_Block,
	Bc3_Unorm_Block,
	Bc3_Srgb_Block,
	Bc4_Unorm_Block,
	Bc4_Snorm_Block,
	Bc5_Unorm_Block,
	Bc5_Snorm_Block,
	Bc6h_Ufloat_Block,
	Bc6h_Sfloat_Block,
	Bc7_Unorm_Block,
	Bc7_Srgb_Block,
	Etc2_R8G8B8_Unorm_Block,
	Etc2_R8G8B8_Srgb_Block,
	Etc2_R8G8B8A1_Unorm_Block,
	Etc2_R8G8B8A1_Srgb_Block,
	Etc2_R8G8B8A8_Unorm_Block,
	Etc2_R8G8B8A8_Srgb_Block,
	Eac_R11_Unorm_Block,
	Eac_R11_Snorm_Block,
	Eac_R11G11_Unorm_Block,
	Eac_R11G11_Snorm_Block,
	Astc_4x4_Unorm_Block,
	Astc_4x4_Srgb_Block,
	Astc_5x4_Unorm_Block,
	Astc_5x4_Srgb_Block,
	Astc_5x5_Unorm_Block,
	Astc_5x5_Srgb_Block,
	Astc_6x5_Unorm_Block,
	Astc_6x5_Srgb_Block,
	Astc_6x6_Unorm_Block,
	Astc_6x6_Srgb_Block,
	Astc_8x5_Unorm_Block,
	Astc_8x5_Srgb_Block,
	Astc_8x6_Unorm_Block,
	Astc_8x6_Srgb_Block,
	Astc_8x8_Unorm_Block,
	Astc_8x8_Srgb_Block,
	Astc_10x5_Unorm_Block,
	Astc_10x5_Srgb_Block,
	Astc_10x6_Unorm_Block,
	Astc_10x6_Srgb_Block,
	Astc_10x8_Unorm_Block,
	Astc_10x8_Srgb_Block,
	Astc_10x10_Unorm_Block,
	Astc_10x10_Srgb_Block,
	Astc_12x10_Unorm_Block,
	Astc_12x10_Srgb_Block,
	Astc_12x12_Unorm_Block,
	Astc_12x12_Srgb_Block,
	G8B8G8R8_422_Unorm,
	B8G8R8G8_422_Unorm,
	G8_B8_R8_3Plane_420_Unorm,
	G8_B8R8_2Plane_420_Unorm,
	G8_B8_R8_3Plane_422_Unorm,
	G8_B8R8_2Plane_422_Unorm,
	G8_B8_R8_3Plane_444_Unorm,
	R10X6_Unorm_Pack16,
	R10X6G10X6_Unorm_2Pack16,
	R10X6G10X6B10X6A10X6_Unorm_4Pack16,
	G10X6B10X6G10X6R10X6_422_Unorm_4Pack16,
	B10X6G10X6R10X6G10X6_422_Unorm_4Pack16,
	G10X6_B10X6_R10X6_3Plane_420_Unorm_3Pack16,
	G10X6_B10X6R10X6_2Plane_420_Unorm_3Pack16,
	G10X6_B10X6_R10X6_3Plane_422_Unorm_3Pack16,
	G10X6_B10X6R10X6_2Plane_422_Unorm_3Pack16,
	G10X6_B10X6_R10X6_3Plane_444_Unorm_3Pack16,
	R12X4_Unorm_Pack16,
	R12X4G12X4_Unorm_2Pack16,
	R12X4G12X4B12X4A12X4_Unorm_4Pack16,
	G12X4B12X4G12X4R12X4_422_Unorm_4Pack16,
	B12X4G12X4R12X4G12X4_422_Unorm_4Pack16,
	G12X4_B12X4_R12X4_3Plane_420_Unorm_3Pack16,
	G12X4_B12X4R12X4_2Plane_420_Unorm_3Pack16,
	G12X4_B12X4_R12X4_3Plane_422_Unorm_3Pack16,
	G12X4_B12X4R12X4_2Plane_422_Unorm_3Pack16,
	G12X4_B12X4_R12X4_3Plane_444_Unorm_3Pack16,
	G16B16G16R16_422_Unorm,
	B16G16R16G16_422_Unorm,
	G16_B16_R16_3Plane_420_Unorm,
	G16_B16R16_2Plane_420_Unorm,
	G16_B16_R16_3Plane_422_Unorm,
	G16_B16R16_2Plane_422_Unorm,
	G16_B16_R16_3Plane_444_Unorm,
	G8_B8R8_2Plane_444_Unorm,
	G10X6_B10X6R10X6_2Plane_444_Unorm_3Pack16,
	G12X4_B12X4R12X4_2Plane_444_Unorm_3Pack16,
	G16_B16R16_2Plane_444_Unorm,
	A4R4G4B4_Unorm_Pack16,
	A4B4G4R4_Unorm_Pack16,
	Astc_4x4_Sfloat_Block,
	Astc_5x4_Sfloat_Block,
	Astc_5x5_Sfloat_Block,
	Astc_6x5_Sfloat_Block,
	Astc_6x6_Sfloat_Block,
	Astc_8x5_Sfloat_Block,
	Astc_8x6_Sfloat_Block,
	Astc_8x8_Sfloat_Block,
	Astc_10x5_Sfloat_Block,
	Astc_10x6_Sfloat_Block,
	Astc_10x8_Sfloat_Block,
	Astc_10x10_Sfloat_Block,
	Astc_12x10_Sfloat_Block,
	Astc_12x12_Sfloat_Block,
	Pvrtc1_2Bpp_Unorm_Block_Img,
	Pvrtc1_4Bpp_Unorm_Block_Img,
	Pvrtc2_2Bpp_Unorm_Block_Img,
	Pvrtc2_4Bpp_Unorm_Block_Img,
	Pvrtc1_2Bpp_Srgb_Block_Img,
	Pvrtc1_4Bpp_Srgb_Block_Img,
	Pvrtc2_2Bpp_Srgb_Block_Img,
	Pvrtc2_4Bpp_Srgb_Block_Img
};

enum class ColorSpace
{
	Srgb_Non_Linear,
	Display_P3_Non_Linear,
	Extended_Srgb_Linear,
	Display_P3_Linear,
	Dci_P3_Non_Linear,
	Bt709_Linear,
	Bt709_Non_Linear,
	Bt2020_Linear,
	Hdr10_St2084,
	Dolby_Vision,
	Hdr10_Hlg,
	Adobe_Rgb_Linear,
	Adobe_Rgb_Non_Linear,
	Pass_Through,
	Extended_Srgb_Non_Linear,
	Display_Native_Amd
};

/**
* ImageLayouts are meant to be used for pipeline barriers.
*/
enum class ImageLayout
{
	Undefined,
	General,
	Color_Attachment,
	Depth_Stencil_Attachment,
	Depth_Stencil_Read_Only,
	Shader_Read_Only,
	Transfer_Src,
	Transfer_Dst,
	Preinitialized,
	Depth_Read_Only_Stencil_Attachment,
	Depth_Attachment_Stencil_Read_Only,
	Depth_Attachment,
	Depth_Read_Only,
	Stencil_Attachment,
	Stencil_Read_Only,
	Read_Only,
	Attachment,
	Present_Src,
	Shared_Present,
	Fragment_Density_Map,
	Fragment_Shading_Rate_Attachment
};

enum class ImageAspect : uint32
{
	Color = 1 << 0,
	Depth = 1 << 1,
	Stencil = 1 << 2,
	Metadata = 1 << 3,
	Plane_0 = 1 << 4,
	Plane_1 = 1 << 5,
	Plane_2 = 1 << 6
};

enum class ImageUsage : uint32
{
	None = 0,
	Transfer_Src = 1 << 0,
	Transfer_Dst = 1 << 1,
	Sampled = 1 << 2,
	Storage = 1 << 3,
	Color_Attachment = 1 << 4,
	Depth_Stencil_Attachment = 1 << 5,
	Transient_Attachment = 1 << 6,
	Input_Attachment = 1 << 7,
	Fragment_Density_Map = 1 << 8,
	Fragment_Shading_Rate_Attachment = 1 << 9,
};

enum class ImageTiling
{
	Optimal,
	Linear
};

enum class TexelFilter
{
	Nearest,
	Linear,
	Cubic_Image
};

enum class SamplerAddress
{
	Repeat,
	Mirrored_Repeat,
	Clamp_To_Edge,
	Clamp_To_Border,
	Mirror_Clamp_To_Edge
};

enum class MipmapMode
{
	Nearest,
	Linear
};

enum class BorderColor
{
	Float_Transparent_Black,
	Int_Transparent_Black,
	Float_Opaque_Black,
	Int_Opaque_Black,
	Float_Opaque_White,
	Int_Opaque_White,
};

enum class CompareOp
{
	Never,
	Less,
	Equal,
	Less_Or_Equal,
	Greater,
	Not_Equal,
	Greater_Or_Equal,
	Always
};

enum class AttachmentLoadOp
{
	Load,
	Clear,
	Dont_Care,
	None
};

enum class AttachmentStoreOp
{
	Store,
	Dont_Care,
	None
};

enum class SwapchainPresentMode
{
	Immediate,
	Mailbox,
	Fifo,
	Fifo_Relaxed,
	Shared_Demand_Refresh,
	Shared_Continuous_Refresh
};

enum class SwapchainState
{
	Ok,
	Timed_Out,
	Not_Ready,
	Suboptimal,
	Error
};

enum class BufferUsage : uint32
{
	/**
	* \brief Specifies that the buffer can be used as a vertex buffer input for draw commands.
	*/
	Vertex = 1 << 0,
	/**
	* \brief Specifies that the buffer can be used as an index buffer input for indexed draw commands.
	*/
	Index = 1 << 1,
	/**
	* \brief Specifies that the buffer can be used as an uniform buffer descriptor.
	*/
	Uniform = 1 << 2,
	/**
	* \brief Specifies that the buffer can be used as a storage buffer descriptor.
	*/
	Storage = 1 << 3,
	/**
	* \brief Specifies that the buffer can be used as the source of atransfer command.
	*/
	Transfer_Src = 1 << 4,
	/**
	* \brief Specifies that the buffer can be used as the destination of a transfer command.
	*/
	Transfer_Dst = 1 << 5,
	/**
	* \brief Specifies that the buffer can be used as a buffer for indirect draw commands.
	*/
	Indirect_Buffer = 1 << 6
};

enum class IndexType
{
	Uint_8,
	Uint_16,
	Uint_32
};

enum class MemoryUsage : uint32
{
	/**
	* \brief Allocations with this usage flag will have it's own block.
	*/
	Dedicated = 1 << 0,
	/**
	* \brief Allows the block of memory to be aliased by different resources.
	*/
	Can_Alias = 1 << 1,
	/**
	* \brief Allocates host visible memory from the GPU. Memory allocated with this usage is always mapped and can be written to sequentially.
	*/
	Host_Writable = 1 << 2,
	/**
	* \brief Allocates host visible memory from the GPU. Memory allocated with this usage is always mapped, can be written, read and accessed in random access.
	*/
	Host_Accessible = 1 << 3,
	/**
	* \brief When used with Host_Writable or Host_Accessible, a non host visible may be selected if it improves performance (unified memory, BAR, ReBAR).
	*/
	Host_Transferable = 1 << 4,
	/**
	* \brief Chooses the smallest possible free range that minimizes fragmentation in the allocator.
	*/
	Best_Fit = 1 << 5,
	/**
	* \brief Chooses the first suitable free memory range in the allocator. Fastest allocation time.
	*/
	First_Fit = 1 << 6
};

enum class PipelineType
{
	None,
	Rasterization,
	Compute,
	Ray_Tracing
};

enum class SubgroupSize
{
	None,		// Vendor decides the appropriate subgroup size.
	Varying,	// Subgroup size may vary in the shader stage.
	Full		// Subgroup size must be launched with all invocations in the task, mesh or compute stage.
};

enum class ShaderType
{
	Vertex,
	Pixel,
	Fragment = Pixel,
	Geometry,
	Tesselation_Control,
	Tesselation_Evaluation,
	Compute,
	Task,
	Mesh,
	Ray_Generation,
	Any_Hit,
	Closest_Hit,
	Ray_Miss,
	Intersection,
	Callable,
	None
};

enum class ShaderStage
{
	Vertex					= 1 << 0,
	Tesselation_Control		= 1 << 1,
	Tesselation_Evaluation	= 1 << 2,
	Geometry				= 1 << 3,
	Fragment				= 1 << 4,
	Pixel					= Fragment,
	Compute					= 1 << 5,
	Task					= 1 << 6,
	Mesh					= 1 << 7,
	All_Graphics			= 31,
	Ray_Generation			= 1 << 8,
	Any_Hit					= 1 << 9,
	Closest_Hit				= 1 << 10,
	Ray_Miss				= 1 << 11,
	Intersection			= 1 << 12,
	Callable				= 1 << 13,
	All						= ~(1 << 31)
};

enum class SharingMode : uint32
{
	Exclusive,
	Concurrent
};

enum class EventState : uint32
{
	Signaled,
	Unsignaled
};

//enum class CommandBufferState : uint8
//{
//	Initial,
//	Recording,
//	Executable,
//	Pending,
//	Invalid
//};
}

#endif // !GPU_CONSTANTS_H
