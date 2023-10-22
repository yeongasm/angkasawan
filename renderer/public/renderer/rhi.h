#pragma once
#ifndef RENDERER_RHI_FLAGS_H
#define RENDERER_RHI_FLAGS_H

#include <vector>
#include <string_view>
#include <span>
#include <variant>
#include <array>
#include <string>
#include "containers/map.h"
#include "types/strong_type.h"
#include "memory/memory.h"
#include "common.h"
#include "renderer_api.h"

namespace rhi
{

// renderer constants.
inline constexpr uint32 MAX_COMMAND_BUFFER_PIPELINE_BARRIER_BATCH_SIZE = 16;
inline constexpr uint32 MAX_PIPELINE_COLOR_ATTACHMENT_COUNT = 8;
inline constexpr uint32 MAX_COMMAND_BUFFER_PER_POOL = 16;
inline constexpr uint32 MAX_COMMAND_BUFFER_SUBMISSION_RING_SIZE = 4;
inline constexpr uint32 MAX_FRAMES_IN_FLIGHT_ALLOWED = 4;
//inline constexpr uint32 MAX_TASK_WORKLOAD_PER_THREAD = 64;

// This object is not meant to be called outside of the renderer. Modules would fix this.
using RHIObjTypeID = ftl::NamedType<const uint32, struct _RHI_TYPE_ID>;
// This object is not meant to be called outside of the renderer. Modules would fix this.
RHIObjTypeID get_next_rhi_object_id();
// This object is not meant to be called outside of the renderer. Modules would fix this.
template <typename T>
inline static const RHIObjTypeID rhi_obj_type_id_v{ get_next_rhi_object_id() };

/**
* This has to be set by the device.
*/
class RHIObjPtr final
{
public:
	RHIObjPtr() :
		m_slot{ 0 },m_data { nullptr }, m_type{ 0 }
	{}

	RHIObjPtr(void* data, RHIObjTypeID type) :
		m_slot{ 0 }, m_data { data }, m_type{ type }
	{}

	RHIObjPtr(uint32 slot, void* data, RHIObjTypeID type) :
		m_slot{ slot }, m_data{ data }, m_type{ type }
	{}

	uint32			slot	() const { return m_slot; }
	bool			valid	() const { return m_data != nullptr && m_type != 0; }
	RHIObjTypeID	type_id	() const { return m_type; }

	template <typename T>
	T& as() const
	{
		ASSERTION(valid() && "Object state is not valid. This will cause a null reference and will result in an error!");
		ASSERTION(m_type == rhi_obj_type_id_v<T> && "Object is being casted to a different type. This is an error!");
		return *static_cast<T*>(m_data);
	}
private:
	uint32			m_slot;
	void*			m_data;
	RHIObjTypeID	m_type;
};

inline static constexpr uint32 DEVICE_INVALID_HANDLE = std::numeric_limits<uint32>::max();

template <typename T>
using GpuResourceHandle = ftl::NamedType<const uint32, T>;

using SurfaceHnd		= GpuResourceHandle<struct DeviceSurface>;
using SwapchainHnd		= GpuResourceHandle<struct DeviceSwapchain>;
using BufferHnd			= GpuResourceHandle<struct DeviceBuffer>;
using ImageHnd			= GpuResourceHandle<struct DeviceImage>;
using ShaderHnd			= GpuResourceHandle<struct DeviceShader>;
using PipelineHnd		= GpuResourceHandle<struct DevicePipeline>;
using SamplerHnd		= GpuResourceHandle<struct DeviceSampler>;
using SemaphoreHnd		= GpuResourceHandle<struct DeviceSemaphore>;
using CommandPoolHnd	= GpuResourceHandle<struct DeviceCommandPool>;
using CommandBufferHnd	= GpuResourceHandle<struct DeviceCommandBuffer>;

using RhiFlag = uint32;

#define DECL_HANDLE(type, name) type name{ DEVICE_INVALID_HANDLE }

template <typename T>
static constexpr T device_invalid_handle_v{ DEVICE_INVALID_HANDLE };

template <typename T, std::integral U>
constexpr void make_handle(T& handle, U value)
{
	new (&handle) T{ value };
}

template <typename T, std::same_as<T> U>
constexpr void make_handle(T& handle, U& value)
{
	new (&handle) T{ value };
}

enum class API
{
	Vulkan
};

enum class ShaderLang
{
	GLSL
};

enum class ErrorSeverity
{
	Verbose,
	Info,
	Warning,
	Error
};

enum class DeviceBuildStatus
{
	Success,
	Queued,
	Building,
	Failed
};

enum class DeviceType
{
	Discrete_Gpu,
	Virtual_Gpu,
	Integrate_Gpu,
	Cpu,
	Other
};

enum class DeviceQueueType
{
	None,
	Main,
	Graphics = Main,
	Transfer,
	Compute
};

enum class PipelineStage : uint64
{
	Top_Of_Pipe						= 1ull << 0ull,
	Draw_Indirect					= 1ull << 1ull,
	Vertex_Input					= 1ull << 2ull,
	Vertex_Shader					= 1ull << 3ull,
	Tesselation_Control				= 1ull << 4ull,
	Tesselation_Evaluation			= 1ull << 5ull,
	Geometry_Shader					= 1ull << 6ull,
	Fragment_Shader					= 1ull << 7ull,
	Pixel_Shader					= Fragment_Shader,
	Early_Fragment_Test				= 1ull << 8ull,
	Late_Fragment_Test				= 1ull << 9ull,
	Color_Attachment_Output			= 1ull << 10ull,
	Compute_Shader					= 1ull << 11ull,
	Transfer						= 1ull << 12ull,
	Bottom_Of_Pipe					= 1ull << 13ull,
	Host							= 1ull << 14ull,
	All_Graphics					= 1ull << 15ull,
	All_Commands					= 1ull << 16ull,
	Copy							= 1ull << 32ull,
	Resolve							= 1ull << 33ull,
	Blit							= 1ull << 19ull,
	Clear							= 1ull << 35ull,
	Index_Input						= 1ull << 36ull,
	Vertex_Attribute_Input			= 1ull << 36ull,
	Pre_Rasterization_Shaders		= 1ull << 37ull,
	Acceleration_Structure_Build	= 1ull << 25ull,
	Ray_Tracing_Shader				= 1ull << 21ull,
	Task_Shader						= 1ull << 19ull,
	Mesh_Shader						= 1ull << 20ull,
	Acceleration_Structure_Copy		= 1ull << 28ull,
	None							= 0ull
};

enum class MemoryAccessType : uint64
{
	Indirect_Command_Read			= 1ull << 0ull,
	Index_Read						= 1ull << 1ull,
	Vertex_Attribute_Read			= 1ull << 2ull,
	Uniform_Read					= 1ull << 3ull,
	Input_Attachment_Read			= 1ull << 4ull,
	Shader_Read						= 1ull << 5ull,
	Shader_Write					= 1ull << 6ull,
	Color_Attachment_Read			= 1ull << 7ull,
	Color_Attachment_Write			= 1ull << 8ull,
	Depth_Stencil_Attachment_Read	= 1ull << 9ull,
	Depth_Stencil_Attachment_Write	= 1ull << 10ull,
	Transfer_Read					= 1ull << 11ull,
	Transfer_Write					= 1ull << 12ull,
	Host_Read						= 1ull << 13ull,
	Host_Write						= 1ull << 14ull,
	Memory_Read						= 1ull << 15ull,
	Memory_Write					= 1ull << 16ull,
	Shader_Sampled_Read				= 1ull << 32ull,
	Shader_Storage_Read				= 1ull << 33ull,
	Shader_Storage_Write			= 1ull << 33ull,
	None							= 0ull
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

enum class ImageFormat
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
	D32_Float_S8_Uint
};

enum class DescriptorType
{
	Sampler,
	Combined_Image_Sampler,
	Sampled_Image,
	Storage_Image,
	Uniform_Texel_Buffer,
	Storage_Texel_Buffer,
	Uniform_Buffer,
	Storage_Buffer,
	Uniform_Buffer_Dynamic,
	Storage_Buffer_Dynamic,
	Input_Attachment,
	Inline_Uniform_Block,
	Acceleration_Structure
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

enum class ImageAspect : RhiFlag
{
	Color		= 1 << 0,
	Depth		= 1 << 1,
	Stencil		= 1 << 2,
	Metadata	= 1 << 3,
	Plane_0		= 1 << 4,
	Plane_1		= 1 << 5,
	Plane_2		= 1 << 6
};

enum class ImageUsage : RhiFlag
{
	Transfer_Src						= 1 << 0,
	Transfer_Dst						= 1 << 1,
	Sampled								= 1 << 2,
	Storage								= 1 << 3,
	Color_Attachment					= 1 << 4,
	Depth_Stencil_Attachment			= 1 << 5,
	Transient_Attachment				= 1 << 6,
	Input_Attachment					= 1 << 7,
	Fragment_Density_Map				= 1 << 8,
	Fragment_Shading_Rate_Attachment	= 1 << 9,
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

enum class SamplerMipMap
{
	Nearest,
	Linear
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

enum class SemaphoreType
{
	Binary,
	Timeline
};

enum class BufferUsage : RhiFlag
{
	Vertex			= 1 << 0,
	Index			= 1 << 1,
	Uniform			= 1 << 2,
	Storage			= 1 << 3,
	Transfer_Src	= 1 << 4,
	Transfer_Dst	= 1 << 5
};

struct Viewport
{
	float32 x;
	float32 y;
	float32 width;
	float32 height;
	float32 minDepth;
	float32 maxDepth;
};

struct Depth
{
	float32 min;
	float32 max;
};

struct Offset2D
{
	int32 x;
	int32 y;
};

struct Offset3D
{
	int32 x;
	int32 y;
	int32 z;
};

template <typename T>
requires std::floating_point<T> || std::integral<T>
struct Color
{
	T r;
	T g;
	T b;
	T a;
};

struct Extent2D
{
	uint32 width;
	uint32 height;
};

struct Extent3D
{
	uint32 width;
	uint32 height;
	uint32 depth;
};

struct Rect2D
{
	Offset2D offset;
	Extent2D extent;
};

struct Cube3D
{
	Offset3D offset;
	Extent3D extent;
};

struct DepthStencilValue
{
	float32 depth;
	uint32	stencil;
};

struct ColorValue
{
	Color<float32>	f32;
	Color<int32>	i32;
	Color<uint32>	u32;
};

struct ClearValue
{
	union
	{
		ColorValue			color;
		DepthStencilValue	depthStencil;
	};
};

struct DeviceInfo
{
	std::string_view name;
	DeviceType type;

	std::string_view vendor;
	uint32 vendorID;
	uint32 deviceID;

	struct
	{
		uint32 major;
		uint32 minor;
		uint32 patch;
	} apiVersion;

	struct
	{
		uint32 major;
		uint32 minor;
		uint32 patch;
	} driverVersion;
};

struct DeviceConfig
{
	uint32 framesInFlight;
	uint32 swapchainImageCount;
	uint32 bufferDescriptorsCount;
	uint32 storageImageDescriptorsCount;
	uint32 sampledImageDescriptorsCount;
	uint32 samplerDescriptorCount;
	uint32 pushConstantMaxSize;
};

struct DeviceInitInfo
{
	struct Version
	{
		uint32 variant, major, minor, patch;
	};
	std::string_view	applicationName;
	Version				applicationVersion;
	std::string_view	engineName;
	Version				engineVersion;
	DeviceConfig		config;
	ShaderLang			shadingLanguage;
	bool				validation;			// Validation layer feature toggle.
	void*				data;				// Data that is passed into the validation layer's callback function.
	std::function<void(ErrorSeverity, literal_t, void*)> callback; // Callback function that is passed into the vulkan validation layer; should the feature be used.
};

enum class ShaderType
{
	None,
	Vertex,
	Pixel,
	Fragment = Pixel,
	Geometry,
	Tesselation_Control,
	Tesselation_Evaluation,
	Compute
};

enum class ShaderVarType
{
	Char,
	Int,
	Float,
	Vec2,
	Vec3,
	Vec4,
	Mat2,
	Mat3,
	Mat4,
	Sampler2D,
	Sampler3D
};

struct ShaderAttribute
{
	enum class Format
	{
		Undefined,
		Int,
		Uint,
		Float,
		Vec2,
		Vec3,
		Vec4
	};
	std::string name;
	uint32		location;
	Format		format;
};

/*struct DeviceQueue
{
	uint32		familyIndex;
	RHIObjPtr	data;
};*/

struct PipelineInputBinding
{
	uint32	binding;
	uint32	from;
	uint32	to;
	uint32	stride;
	bool	instanced;
};

struct ColorBlendInfo
{
	BlendFactor		srcColorBlendFactor = BlendFactor::Src_Color;
	BlendFactor		dstColorBlendFactor = BlendFactor::Dst_Color;
	BlendOp			colorBlendOp		= BlendOp::Add;
	BlendFactor		srcAlphaBlendFactor = BlendFactor::Src_Alpha;
	BlendFactor		dstAlphaBlendFactor = BlendFactor::Dst_Alpha;
	BlendOp			alphaBlendOp		= BlendOp::Add;
};

struct SemaphoreInfo
{
	SemaphoreType	type;
	uint32			initialValue; // Only used for timeline semaphores.
};

// In Linux, this is equivalent to Display. VkXlibSurfaceCreateInfoKHR has this parameter as dpy.
/*os::Handle				hinstance;*/
// In Linux, this is equivalent to Window. VkXlibSurfaceCreateInfoKHR has this parameter as window.
/*os::WndHandle*			hwnd;*/
struct SurfaceInfo
{
	void* instance;
	void* window;
};

struct SwapchainInfo
{
	Extent2D				dimension;
	uint32					imageCount;
	ImageUsage				imageUsage;
	SwapchainPresentMode	presentationMode;
};

/**
* \brief
* Shader's information after compilation and reflection.
*/
struct ShaderCompileInfo
{
	using PreprocessorMap = ftl::Map<std::string_view, std::string_view>;

	ShaderType		type;
	std::string		filename;
	std::string		entryPoint;
	std::string		sourceCode;
	std::string		defines;		// Use add_preprocessor() instead of manually appending.
	PreprocessorMap	preprocessors;

	void add_preprocessor(std::string_view key, size_t definesReserveSize = 1_KiB)
	{
		if (!defines.capacity())
		{
			defines.reserve(definesReserveSize);
		}
		auto start = defines.end();
		defines.append(key);
		preprocessors.emplace(std::string_view{ start, defines.end() }, std::string_view{});
	}

	void add_preprocessor(std::string_view key, std::string_view value, size_t definesReserveSize = 1_KiB)
	{
		if (!defines.capacity())
		{
			defines.reserve(definesReserveSize);
		}
		auto kstart = defines.end();
		defines.append(key);
		std::string_view k{ kstart, defines.end() };

		auto vstart = defines.end();
		defines.append(value);
		std::string_view v{ vstart, defines.end() };

		preprocessors.emplace(std::move(k), std::move(v));
	}

	void clear_preprocessors()
	{
		preprocessors.clear();
		defines.clear();
	}

	void remove_preprocessor(std::string_view key)
	{
		preprocessors.erase(key);
	}
};

struct ShaderInfo
{
	ShaderType						type;
	std::string						filename;
	std::string						entryPoint;
	std::vector<uint32>				binaries;
	std::vector<ShaderAttribute>	attributes;
};

struct BufferInfo
{
	BufferUsage	usage;
	MemoryUsage memoryUsage;
	size_t size;
};

struct ImageInfo
{
	ImageType type;
	ImageFormat	format;
	SampleCount	samples;
	ImageTiling	tiling;
	ImageUsage imageUsage;
	MemoryUsage	memoryUsage;
	Extent3D dimension;
	ClearValue clearValue;
	uint32 mipLevel;
};

struct DepthTestInfo
{
	CompareOp	depthTestCompareOp		= CompareOp::Less;
	float32		minDepthBounds			= 0.f;
	float32		maxDepthBounds			= 1.f;
	bool		enableDepthBoundsTest	= false;	// Allows pixels to be discarded if the currently-stored depth value is outside the range specified by minDepthBounds and maxDepthBounds.
	bool		enableDepthTest			= false;
	bool		enableDepthWrite		= false;
};

struct RasterizationStateInfo
{
	PolygonMode polygonalMode		= PolygonMode::Fill;
	CullingMode cullMode			= CullingMode::Back;
	FrontFace	frontFace			= FrontFace::Counter_Clockwise;
	float32		lineWidth			= 1.f;
	bool		enableDepthClamp	= false;

};

struct RasterPipelineInfo
{
	std::vector<PipelineInputBinding>	inputBindings		= {};
	RasterizationStateInfo				rasterization		= {};
	DepthTestInfo						depthTest			= {};
	TopologyType						topology			= TopologyType::Triangle;
	uint32								pushConstantSize	= 0;
};

struct BindInfo
{
	uint32	slot;
	uint32	location;
	bool	bound;
};

/**
* NOTE(afiq):
* Now that we have RHIObjPtr, should we still store the handles in the resources?
* Need to give it some thought.
*/

struct Surface
{
	SurfaceInfo info;
	SurfaceHnd	hnd;
	RHIObjPtr	data;
};

struct Swapchain
{
	Surface			surface;
	SwapchainInfo	info;
	SwapchainState	state;
	SwapchainHnd	hnd;
	RHIObjPtr		data;
};

struct Shader
{
	ShaderInfo	info;
	ShaderHnd	hnd;
	RHIObjPtr	data;
};

struct Buffer
{
	BufferInfo		info;
	BufferHnd		hnd;
	void*			address;
	size_t			offset;
	RHIObjPtr		data;
	DeviceQueueType owner;
};

struct Image
{
	ImageInfo		info;
	ImageHnd		hnd;
	RHIObjPtr		data;
	DeviceQueueType owner;
};

/*struct AttachmentDescriptionInfo
{
	ImageFormat			format;
	SampleCount			samples;
	AttachmentLoadOp	loadOp;
	AttachmentStoreOp	storeOp;
	AttachmentLoadOp	stencilLoadOp;
	AttachmentStoreOp	stencilStoreOp;
	ImageLayout			initialLayout;
	ImageLayout			finalLayout;
};*/

struct ColorAttachment
{
	ImageFormat		format;
	ColorBlendInfo	blendInfo;	// Color blending is only valid for color attachments. Depth stencil attachments don't require color blend information.
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
struct RasterPipeline
{
	RasterPipelineInfo				info;
	PipelineHnd						hnd;
	RHIObjPtr						data;
	std::vector<Shader*>			shaders;
	std::vector<ColorAttachment>	colorAttachments;
};

struct Semaphore
{
	SemaphoreInfo	info;
	SemaphoreHnd	hnd;
	RHIObjPtr		data;
};

struct ImageSubresource
{
	ImageAspect	aspectFlags	= ImageAspect::Color;
	uint32 mipLevel	= 0u;
	uint32 levelCount = 1u;
	uint32 baseArrayLayer = 0u;
	uint32 layerCount = 1u;
};

struct FrameInfo
{
	size_t	count; // Total elapsed frame. Always incrementing.
	uint32	index; // Current index of the frame. Values should be between 0 <= value <= rhi::DeviceConfig.framesInFlight
};

bool				is_color_format			(ImageFormat format);
bool				is_depth_format			(ImageFormat format);
bool				is_stencil_format		(ImageFormat format);
std::string_view	get_image_format_string	(ImageFormat format);

}

RENDERER_API rhi::ImageUsage		operator| (rhi::ImageUsage a, rhi::ImageUsage b);
RENDERER_API rhi::ImageUsage		operator& (rhi::ImageUsage a, rhi::ImageUsage b);
RENDERER_API rhi::BufferUsage		operator| (rhi::BufferUsage a, rhi::BufferUsage b);
RENDERER_API rhi::BufferUsage		operator& (rhi::BufferUsage a, rhi::BufferUsage b);
RENDERER_API rhi::ImageAspect		operator| (rhi::ImageAspect a, rhi::ImageAspect b);
RENDERER_API rhi::ImageAspect		operator& (rhi::ImageAspect a, rhi::ImageAspect b);
RENDERER_API rhi::PipelineStage		operator| (rhi::PipelineStage a, rhi::PipelineStage b);
RENDERER_API rhi::PipelineStage		operator& (rhi::PipelineStage a, rhi::PipelineStage b);
RENDERER_API rhi::MemoryAccessType	operator| (rhi::MemoryAccessType a, rhi::MemoryAccessType b);
RENDERER_API rhi::MemoryAccessType	operator& (rhi::MemoryAccessType a, rhi::MemoryAccessType b);

#endif // !RENDERER_RHI_FLAGS_H
