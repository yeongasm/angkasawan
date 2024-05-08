module;

#include <span>
#include "lib/string.h"
#include "lib/bit_mask.h"
#include "rhi_api.h"

export module forge.common;

namespace frg
{
// renderer constants.
export RHI_API extern uint32 const COMMAND_BUFFER_PIPELINE_BARRIER_BATCH_SIZE = 16;
export RHI_API extern uint32 const MAX_PIPELINE_COLOR_ATTACHMENT = 16;
export RHI_API extern uint32 const MAX_COMMAND_BUFFER_ATTACHMENT = 16;
export RHI_API extern uint32 const MAX_COMMAND_BUFFER_PER_POOL = 16;
export RHI_API extern uint32 const MAX_FRAMES_IN_FLIGHT = 4;
// resource limits.
export RHI_API extern uint32 const MAX_BUFFERS = 10'000;
export RHI_API extern uint32 const MAX_IMAGES = 10'000;
export RHI_API extern uint32 const MAX_SAMPLERS = 100;
// shader bindings.
export RHI_API extern uint32 const STORAGE_IMAGE_BINDING = 0;
export RHI_API extern uint32 const COMBINED_IMAGE_SAMPLER_BINDING = 1;
export RHI_API extern uint32 const SAMPLED_IMAGE_BINDING = 2;
export RHI_API extern uint32 const SAMPLER_BINDING = 3;
export RHI_API extern uint32 const BUFFER_DEVICE_ADDRESS_BINDING = 4;
export RHI_API extern uint32 const STORAGE_BUFFER_BINDING = 5;
export RHI_API extern uint32 const UNIFORM_BUFFER_BINDING = 6;
// debug utilities.
export RHI_API extern uint32 const ENABLE_DEBUG_RESOURCE_NAMES = 1;

// Enums and flags.
export enum class API
{
	Vulkan
};

export enum class ShaderLang
{
	GLSL
};

export enum class ErrorSeverity
{
	Verbose,
	Info,
	Warning,
	Error
};

export enum class DeviceType
{
	Other,
	Integrate_Gpu,
	Discrete_Gpu,
	Virtual_Gpu,
	Cpu
};

export enum class DeviceQueue
{
	None,
	Main,
	Graphics = Main,
	Transfer,
	Compute
};

export enum class PipelineStage : uint32
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

export enum class MemoryAccessType : uint32
{
	None = 0,
	Host_Read = 1 << 0,
	Host_Write = 1 << 1,
	Host_Read_Write = Host_Read | Host_Write,
	Memory_Read = 1 << 2,
	Memory_Write = 1 << 3,
	Memory_Read_Write = Memory_Read | Memory_Write
};

export enum class TopologyType
{
	Point,
	Line,
	Line_Strip,
	Triangle,
	Triange_Strip,
	Triangle_Fan
};

export enum class PolygonMode
{
	Fill,
	Line,
	Point
};

export enum class FrontFace
{
	Clockwise,
	Counter_Clockwise
};

export enum class CullingMode
{
	None,
	Back,
	Front,
	Front_And_Back
};

export enum class BlendFactor
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

export enum class BlendOp
{
	Add,
	Subtract,
	Reverse_Subtract
};

export enum class SampleCount
{
	Sample_Count_1,
	Sample_Count_2,
	Sample_Count_4,
	Sample_Count_8,
	Sample_Count_16,
	Sample_Count_32,
	Sample_Count_64
};

export enum class ImageType
{
	Image_1D,
	Image_2D,
	Image_3D
};

export enum class Format
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

export enum class ColorSpace
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
export enum class ImageLayout
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

export enum class ImageAspect : uint32
{
	Color = 1 << 0,
	Depth = 1 << 1,
	Stencil = 1 << 2,
	Metadata = 1 << 3,
	Plane_0 = 1 << 4,
	Plane_1 = 1 << 5,
	Plane_2 = 1 << 6
};

export enum class ImageUsage : uint32
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

export enum class ImageTiling
{
	Optimal,
	Linear
};

export enum class TexelFilter
{
	Nearest,
	Linear,
	Cubic_Image
};

export enum class SamplerAddress
{
	Repeat,
	Mirrored_Repeat,
	Clamp_To_Edge,
	Clamp_To_Border,
	Mirror_Clamp_To_Edge
};

export enum class MipmapMode
{
	Nearest,
	Linear
};

export enum class BorderColor
{
	Float_Transparent_Black,
	Int_Transparent_Black,
	Float_Opaque_Black,
	Int_Opaque_Black,
	Float_Opaque_White,
	Int_Opaque_White,
};

export enum class CompareOp
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

export enum class AttachmentLoadOp
{
	Load,
	Clear,
	Dont_Care,
	None
};

export enum class AttachmentStoreOp
{
	Store,
	Dont_Care,
	None
};

export enum class SwapchainPresentMode
{
	Immediate,
	Mailbox,
	Fifo,
	Fifo_Relaxed,
	Shared_Demand_Refresh,
	Shared_Continuous_Refresh
};

export enum class SwapchainState
{
	Ok,
	Timed_Out,
	Not_Ready,
	Suboptimal,
	Error
};

export enum class BufferUsage : uint32
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

export enum class IndexType
{
	Uint_8,
	Uint_16,
	Uint_32
};

export enum class MemoryUsage : uint32
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

export enum class PipelineType
{
	None,
	Rasterization,
	Compute,
	Ray_Tracing
};

export enum class ShaderType
{
	Vertex,
	Pixel,
	Fragment = Pixel,
	Geometry,
	Tesselation_Control,
	Tesselation_Evaluation,
	Compute,
	None
};

export enum class ShaderStage
{
	Vertex					= 1 << 0,
	Tesselation_Control		= 1 << 1,
	Tesselation_Evaluation	= 1 << 2,
	Geometry				= 1 << 3,
	Fragment				= 1 << 4,
	Pixel					= Fragment,
	Compute					= 1 << 5,
	All_Graphics			= 1 << 6,
	All						= 1 << 7
};

export enum class CommandBufferState : uint8
{
	Initial,
	Recording,
	Executable,
	Pending,
	Invalid
};

export struct Viewport
{
	float32 x = 0.f;
	float32 y = 0.f;
	float32 width;
	float32 height;
	float32 minDepth = 0.f;
	float32 maxDepth = 1.f;
};

export struct Depth
{
	float32 min;
	float32 max;
};

export struct Offset2D
{
	int32 x;
	int32 y;
};

export struct Offset3D
{
	int32 x;
	int32 y;
	int32 z;
};

export template <typename T>
requires std::floating_point<T> || std::integral<T>
struct Color
{
	T r, g, b, a;
};

export union ColorValue
{
    Color<float32>  f32;
    Color<int32>    i32;
    Color<uint32>   u32;
};

export struct Extent2D
{
	uint32 width;
	uint32 height;
};

export struct Extent3D
{
	uint32 width;
	uint32 height;
	uint32 depth;
};

export struct Rect2D
{
	Offset2D offset;
	Extent2D extent;
};

export struct Cube3D
{
	Offset3D offset;
	Extent3D extent;
};

export struct DepthStencilValue
{
	float32 depth;
	uint32	stencil;
};

export struct ClearValue
{
	union
	{
		ColorValue color;
		DepthStencilValue depthStencil;
	};
};

export struct DeviceConfig
{
	uint32 maxFramesInFlight = 2;
	uint32 swapchainImageCount = 3;
	uint32 maxBuffers = MAX_BUFFERS;
	uint32 maxImages = MAX_IMAGES;
	uint32 maxSamplers = MAX_SAMPLERS;
	uint32 pushConstantMaxSize = UINT32_MAX; // Default is using whatever is set by the device.
};

export struct Version
{
	uint32 major;
	uint32 minor;
	uint32 patch;

	auto stringify() const -> lib::string;
};

export struct DeviceInfo
{
	std::string_view name;
	DeviceType type;
	API api;
	ShaderLang shaderLang;
	std::string_view vendor;
	uint32 vendorID;
	uint32 deviceID;
	std::string_view deviceName;
	Version apiVersion;
	Version driverVersion;
};

export struct DeviceInitInfo
{
	struct Version
	{
		uint32 variant;
		uint32 major; 
		uint32 minor;
		uint32 patch;
	};
	std::string_view name;
	std::string_view appName;
	Version	appVersion;
	std::string_view engineName;
	Version	engineVersion;
	DeviceType preferredDevice;
	DeviceConfig config;
	ShaderLang shadingLanguage;
	/**
	* \brief Validation layer feature toggle.
	*/
	bool validation;
	/**
	* \brief Callback function that is passed into the vulkan validation layer; should the feature be used.
	*/
	std::function<void(ErrorSeverity, literal_t)> callback;
};

export struct VertexInputBinding
{
	uint32 binding;
	uint32 from;
	uint32 to;
	uint32 stride;
	bool instanced;
};

export struct ColorBlendInfo
{
	bool enable = true;
	BlendFactor	srcColorBlendFactor = BlendFactor::One;
	BlendFactor	dstColorBlendFactor = BlendFactor::Zero;
	BlendOp	colorBlendOp = BlendOp::Add;
	BlendFactor	srcAlphaBlendFactor = BlendFactor::One;
	BlendFactor	dstAlphaBlendFactor = BlendFactor::Zero;
	BlendOp	alphaBlendOp = BlendOp::Add;
};

export struct SurfaceInfo
{
	lib::string name;
	lib::array<Format> preferredSurfaceFormats;
	void* instance;
	void* window;
};

export struct SwapchainInfo
{
	lib::string name;
	SurfaceInfo surfaceInfo;
	Extent2D dimension;
	uint32 imageCount;
	ImageUsage imageUsage;
	SwapchainPresentMode presentationMode;
};

export struct ShaderAttribute
{
	std::string_view name;
	uint32 location;
	Format format;
};

/**
* \brief This struct is not meant to be manually filled.
*/
export struct CompiledShaderInfo
{
	std::string_view name;
	std::string_view path;
	ShaderType type;
	std::string_view entryPoint;
	std::span<uint32> binaries;
	std::span<ShaderAttribute> vertexInputAttributes;
};

export struct ShaderInfo
{
	lib::string name;
	ShaderType type;
	lib::string entryPoint;
};

// export struct PipelineShaderInfo
// {
// 	Shader* vertexShader;
// 	Shader* pixelShader;
// 	std::span<ShaderAttribute> vertexInputAttributes = {};
// };

export struct BufferInfo
{
	lib::string name;
	size_t size;
	BufferUsage	bufferUsage;
	MemoryUsage memoryUsage/* = MemoryUsage::Can_Alias | MemoryUsage::Dedicated*/;
};

export struct ImageInfo
{
	lib::string name;
	ImageType type;
	Format	format;
	SampleCount	samples;
	ImageTiling	tiling = ImageTiling::Optimal;
	ImageUsage imageUsage;
	MemoryUsage memoryUsage = MemoryUsage::Dedicated;
	Extent3D dimension;
	ClearValue clearValue;
	uint32 mipLevel;
};

export struct SamplerInfo
{
	lib::string name;
	TexelFilter minFilter = TexelFilter::Linear;
	TexelFilter magFilter = TexelFilter::Linear;
	MipmapMode mipmapMode = MipmapMode::Linear;
	SamplerAddress addressModeU = SamplerAddress::Clamp_To_Edge;
	SamplerAddress addressModeV = SamplerAddress::Clamp_To_Edge;
	SamplerAddress addressModeW = SamplerAddress::Clamp_To_Edge;
	float32 mipLodBias = 0.f;
	float32 maxAnisotropy = 0.f;
	CompareOp compareOp = CompareOp::Less;
	float32 minLod = 0.f;
	float32 maxLod = 1.f;
	BorderColor borderColor = BorderColor::Float_Opaque_Black;
	bool unnormalizedCoordinates = false;
};

export struct CommandPoolInfo
{
	lib::string name;
	DeviceQueue queue;
};

export struct CommandBufferInfo
{
	lib::string name;
};

export struct DepthTestInfo
{
	CompareOp depthTestCompareOp = CompareOp::Less;
	float32 minDepthBounds = 0.f;
	float32	maxDepthBounds = 1.f;
	bool enableDepthBoundsTest = false;	// Allows pixels to be discarded if the currently-stored depth value is outside the range specified by minDepthBounds and maxDepthBounds.
	bool enableDepthTest	= false;
	bool enableDepthWrite = false;
};

export struct RasterizationStateInfo
{
	PolygonMode polygonalMode = PolygonMode::Fill;
	CullingMode cullMode = CullingMode::Back;
	FrontFace frontFace = FrontFace::Counter_Clockwise;
	float32 lineWidth = 1.f;
	bool enableDepthClamp = false;

};

export struct ColorAttachment
{
	Format format;
	ColorBlendInfo blendInfo;
};

export struct RasterPipelineInfo
{
	lib::string name;
	lib::array<ColorAttachment> colorAttachments;
	Format depthAttachmentFormat = Format::Undefined;
	Format stencilAttachmentFormat = Format::Undefined;
	lib::array<VertexInputBinding> vertexInputBindings;
	RasterizationStateInfo rasterization = {};
	DepthTestInfo depthTest = {};
	TopologyType topology = TopologyType::Triangle;
	uint32 pushConstantSize = 128;
};

export struct SemaphoreInfo
{
	lib::string name;
};

export struct FenceInfo
{
	lib::string name;
	uint64 initialValue = 0;
};

/**
* https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#VkPipeline
* The Vulkan specification states that,
* There are 3 total pipelines supported on the GPU.
* 1. Graphics Pipeline.
* 2. Compute Pipeline.
* 3. Ray Tracing Pipeline.
* 
* Graphics Pipeline can be operated in 2 modes:
* a) Primitive shading.
* b) Mesh shading.
*/

export struct ImageSubresource
{
	ImageAspect	aspectFlags = ImageAspect::Color;
	uint32 mipLevel = 0u;
	uint32 levelCount = 1u;
	uint32 baseArrayLayer = 0u;
	uint32 layerCount = 1u;
};

export struct FrameInfo
{
	/**
	* \brief Total elapsed frame. Increments monotonically.
	*/
	uint64 count;
	/**
	* \brief Current index of the frame. Values is always between 0 <= index <= DeviceConfig.maxFramesInFlight.
	*/
	uint32 index;
};

/**
* Pipeline barrier access information.
*/
export struct Access
{
	PipelineStage stages = PipelineStage::None;
	MemoryAccessType type = MemoryAccessType::None;
};

namespace access
{
// read.
export RHI_API extern const Access TOP_OF_PIPE_READ							= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::Memory_Read };
export RHI_API extern const Access DRAW_INDIRECT_READ						= { PipelineStage::Draw_Indirect,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access VERTEX_INPUT_READ						= { PipelineStage::Vertex_Input,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access VERTEX_SHADER_READ						= { PipelineStage::Vertex_Shader,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access TESSELATION_CONTROL_READ					= { PipelineStage::Tesselation_Control,				MemoryAccessType::Memory_Read };
export RHI_API extern const Access TESSELATION_EVALUATION_READ				= { PipelineStage::Tesselation_Evaluation,			MemoryAccessType::Memory_Read };
export RHI_API extern const Access GEOMETRY_SHADER_READ						= { PipelineStage::Geometry_Shader,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access FRAGMENT_SHADER_READ						= { PipelineStage::Fragment_Shader,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access PIXEL_SHADER_READ						= { PipelineStage::Pixel_Shader,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access EARLY_FRAGMENT_TEST_READ					= { PipelineStage::Early_Fragment_Test,				MemoryAccessType::Memory_Read };
export RHI_API extern const Access LATE_FRAGMENT_TEST_READ					= { PipelineStage::Late_Fragment_Test,				MemoryAccessType::Memory_Read };
export RHI_API extern const Access COLOR_ATTACHMENT_OUTPUT_READ				= { PipelineStage::Color_Attachment_Output,			MemoryAccessType::Memory_Read };
export RHI_API extern const Access COMPUTE_SHADER_READ						= { PipelineStage::Compute_Shader,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access TRANSFER_READ							= { PipelineStage::Transfer,						MemoryAccessType::Memory_Read };
export RHI_API extern const Access BOTTOM_OF_PIPE_READ						= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access HOST_READ								= { PipelineStage::Host,							MemoryAccessType::Memory_Read };
export RHI_API extern const Access ALL_GRAPHICS_READ						= { PipelineStage::All_Graphics,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access ALL_COMMANDS_READ						= { PipelineStage::All_Commands,					MemoryAccessType::Memory_Read };
export RHI_API extern const Access COPY_READ								= { PipelineStage::Copy,							MemoryAccessType::Memory_Read };
export RHI_API extern const Access RESOLVE_READ								= { PipelineStage::Resolve,							MemoryAccessType::Memory_Read };
export RHI_API extern const Access BLIT_READ								= { PipelineStage::Blit,							MemoryAccessType::Memory_Read };
export RHI_API extern const Access CLEAR_READ								= { PipelineStage::Clear,							MemoryAccessType::Memory_Read };
export RHI_API extern const Access INDEX_INPUT_READ							= { PipelineStage::Index_Input,						MemoryAccessType::Memory_Read };
export RHI_API extern const Access VERTEX_ATTRIBUTE_INPUT_READ				= { PipelineStage::Vertex_Attribute_Input,			MemoryAccessType::Memory_Read };
export RHI_API extern const Access PRE_RASTERIZATION_SHADERS_READ			= { PipelineStage::Pre_Rasterization_Shaders,		MemoryAccessType::Memory_Read };
export RHI_API extern const Access ACCELERATION_STRUCTURE_BUILD_READ		= { PipelineStage::Acceleration_Structure_Build,	MemoryAccessType::Memory_Read };
export RHI_API extern const Access RAY_TRACING_SHADER_READ					= { PipelineStage::Ray_Tracing_Shader,				MemoryAccessType::Memory_Read };
export RHI_API extern const Access TASK_SHADER_READ							= { PipelineStage::Task_Shader,						MemoryAccessType::Memory_Read };
export RHI_API extern const Access MESH_SHADER_READ							= { PipelineStage::Mesh_Shader,						MemoryAccessType::Memory_Read };
export RHI_API extern const Access ACCELERATION_STRUCTURE_COPY_READ			= { PipelineStage::Acceleration_Structure_Copy,		MemoryAccessType::Memory_Read };
// write.
export RHI_API extern const Access TOP_OF_PIPE_WRITE						= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::Memory_Write };
export RHI_API extern const Access DRAW_INDIRECT_WRITE						= { PipelineStage::Draw_Indirect,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access VERTEX_INPUT_WRITE						= { PipelineStage::Vertex_Input,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access VERTEX_SHADER_WRITE						= { PipelineStage::Vertex_Shader,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access TESSELATION_CONTROL_WRITE				= { PipelineStage::Tesselation_Control,				MemoryAccessType::Memory_Write };
export RHI_API extern const Access TESSELATION_EVALUATION_WRITE				= { PipelineStage::Tesselation_Evaluation,			MemoryAccessType::Memory_Write };
export RHI_API extern const Access GEOMETRY_SHADER_WRITE					= { PipelineStage::Geometry_Shader,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access FRAGMENT_SHADER_WRITE					= { PipelineStage::Fragment_Shader,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access PIXEL_SHADER_WRITE						= { PipelineStage::Pixel_Shader,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access EARLY_FRAGMENT_TEST_WRITE				= { PipelineStage::Early_Fragment_Test,				MemoryAccessType::Memory_Write };
export RHI_API extern const Access LATE_FRAGMENT_TEST_WRITE					= { PipelineStage::Late_Fragment_Test,				MemoryAccessType::Memory_Write };
export RHI_API extern const Access COLOR_ATTACHMENT_OUTPUT_WRITE			= { PipelineStage::Color_Attachment_Output,			MemoryAccessType::Memory_Write };
export RHI_API extern const Access COMPUTE_SHADER_WRITE						= { PipelineStage::Compute_Shader,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access TRANSFER_WRITE							= { PipelineStage::Transfer,						MemoryAccessType::Memory_Write };
export RHI_API extern const Access BOTTOM_OF_PIPE_WRITE						= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access HOST_WRITE								= { PipelineStage::Host,							MemoryAccessType::Memory_Write };
export RHI_API extern const Access ALL_GRAPHICS_WRITE						= { PipelineStage::All_Graphics,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access ALL_COMMANDS_WRITE						= { PipelineStage::All_Commands,					MemoryAccessType::Memory_Write };
export RHI_API extern const Access COPY_WRITE								= { PipelineStage::Copy,							MemoryAccessType::Memory_Write };
export RHI_API extern const Access RESOLVE_WRITE							= { PipelineStage::Resolve,							MemoryAccessType::Memory_Write };
export RHI_API extern const Access BLIT_WRITE								= { PipelineStage::Blit,							MemoryAccessType::Memory_Write };
export RHI_API extern const Access CLEAR_WRITE								= { PipelineStage::Clear,							MemoryAccessType::Memory_Write };
export RHI_API extern const Access INDEX_INPUT_WRITE						= { PipelineStage::Index_Input,						MemoryAccessType::Memory_Write };
export RHI_API extern const Access VERTEX_ATTRIBUTE_INPUT_WRITE				= { PipelineStage::Vertex_Attribute_Input,			MemoryAccessType::Memory_Write };
export RHI_API extern const Access PRE_RASTERIZATION_SHADERS_WRITE			= { PipelineStage::Pre_Rasterization_Shaders,		MemoryAccessType::Memory_Write };
export RHI_API extern const Access ACCELERATION_STRUCTURE_BUILD_WRITE		= { PipelineStage::Acceleration_Structure_Build,	MemoryAccessType::Memory_Write };
export RHI_API extern const Access RAY_TRACING_SHADER_WRITE					= { PipelineStage::Ray_Tracing_Shader,				MemoryAccessType::Memory_Write };
export RHI_API extern const Access TASK_SHADER_WRITE						= { PipelineStage::Task_Shader,						MemoryAccessType::Memory_Write };
export RHI_API extern const Access MESH_SHADER_WRITE						= { PipelineStage::Mesh_Shader,						MemoryAccessType::Memory_Write };
export RHI_API extern const Access ACCELERATION_STRUCTURE_COPY_WRITE		= { PipelineStage::Acceleration_Structure_Copy,		MemoryAccessType::Memory_Write };
// read write.
export RHI_API extern const Access TOP_OF_PIPE_READ_WRITE					= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access DRAW_INDIRECT_READ_WRITE					= { PipelineStage::Draw_Indirect,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access VERTEX_INPUT_READ_WRITE					= { PipelineStage::Vertex_Input,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access VERTEX_SHADER_READ_WRITE					= { PipelineStage::Vertex_Shader,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access TESSELATION_CONTROL_READ_WRITE			= { PipelineStage::Tesselation_Control,				MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access TESSELATION_EVALUATION_READ_WRITE		= { PipelineStage::Tesselation_Evaluation,			MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access GEOMETRY_SHADER_READ_WRITE				= { PipelineStage::Geometry_Shader,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access FRAGMENT_SHADER_READ_WRITE				= { PipelineStage::Fragment_Shader,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access PIXEL_SHADER_READ_WRITE					= { PipelineStage::Pixel_Shader,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access EARLY_FRAGMENT_TEST_READ_WRITE			= { PipelineStage::Early_Fragment_Test,				MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access LATE_FRAGMENT_TEST_READ_WRITE			= { PipelineStage::Late_Fragment_Test,				MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access COLOR_ATTACHMENT_OUTPUT_READ_WRITE		= { PipelineStage::Color_Attachment_Output,			MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access COMPUTE_SHADER_READ_WRITE				= { PipelineStage::Compute_Shader,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access TRANSFER_READ_WRITE						= { PipelineStage::Transfer,						MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access BOTTOM_OF_PIPE_READ_WRITE				= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access HOST_READ_WRITE							= { PipelineStage::Host,							MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access ALL_GRAPHICS_READ_WRITE					= { PipelineStage::All_Graphics,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access ALL_COMMANDS_READ_WRITE					= { PipelineStage::All_Commands,					MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access COPY_READ_WRITE							= { PipelineStage::Copy,							MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access RESOLVE_READ_WRITE						= { PipelineStage::Resolve,							MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access BLIT_READ_WRITE							= { PipelineStage::Blit,							MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access CLEAR_READ_WRITE							= { PipelineStage::Clear,							MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access INDEX_INPUT_READ_WRITE					= { PipelineStage::Index_Input,						MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access VERTEX_ATTRIBUTE_INPUT_READ_WRITE		= { PipelineStage::Vertex_Attribute_Input,			MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access PRE_RASTERIZATION_SHADERS_READ_WRITE		= { PipelineStage::Pre_Rasterization_Shaders,		MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access ACCELERATION_STRUCTURE_BUILD_READ_WRITE	= { PipelineStage::Acceleration_Structure_Build,	MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access RAY_TRACING_SHADER_READ_WRITE			= { PipelineStage::Ray_Tracing_Shader,				MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access TASK_SHADER_READ_WRITE					= { PipelineStage::Task_Shader,						MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access MESH_SHADER_READ_WRITE					= { PipelineStage::Mesh_Shader,						MemoryAccessType::Memory_Read_Write };
export RHI_API extern const Access ACCELERATION_STRUCTURE_COPY_READ_WRITE	= { PipelineStage::Acceleration_Structure_Copy,		MemoryAccessType::Memory_Read_Write };
// ownership transfers.
export RHI_API extern const Access TOP_OF_PIPE_NONE							= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::None };
export RHI_API extern const Access BOTTOM_OF_PIPE_NONE						= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::None };
}

export RHI_API auto is_color_format(Format format) -> bool;
export RHI_API auto is_depth_format(Format format) -> bool;
export RHI_API auto is_stencil_format(Format format) -> bool;
export RHI_API auto get_image_format_string(Format format) -> std::string_view;
export RHI_API auto get_color_space_string(ColorSpace colorSpace) -> std::string_view;
}