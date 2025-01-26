#pragma once
#ifndef GPU_COMMON_H
#define GPU_COMMON_H

#include <span>
#include "lib/string.hpp"
#include "lib/bit_mask.hpp"
#include "constants.hpp"

namespace gpu
{
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
requires (std::floating_point<T> || std::integral<T>)
struct Color
{
	T r, g, b, a;
};

struct ColorValue
{
	union
	{
		Color<float32> f32;
		Color<int32> i32;
		Color<uint32> u32;
	};
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

struct ClearValue
{
	union
	{
		ColorValue color;
		DepthStencilValue depthStencil;
	};
};

struct DeviceConfig
{
	uint32 maxFramesInFlight;
	uint32 swapchainImageCount;
	uint32 maxBuffers;
	uint32 maxImages;
	uint32 maxSamplers;
	uint32 pushConstantMaxSize; // Default is using whatever is set by the device.
};

struct Version
{
	uint32 major;
	uint32 minor;
	uint32 patch;

	auto stringify() const -> lib::string;
};

struct DeviceInfo
{
	std::string_view name;
	DeviceType type;
	API api;
	std::string_view vendor;
	uint32 vendorID;
	uint32 deviceID;
	std::string_view deviceName;
	Version apiVersion;
	Version driverVersion;
};

struct DeviceInitInfo
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
	/**
	* \brief Validation layer feature toggle.
	*/
	bool validation;
	/**
	* \brief Callback function that is passed into the vulkan validation layer; should the feature be used.
	*/
	std::function<void(ErrorSeverity, literal_t)> callback;
};

struct VertexInputBinding
{
	uint32 binding;
	uint32 from;
	uint32 to;
	uint32 stride;
	bool instanced;
};

struct ColorBlendInfo
{
	bool enable;
	BlendFactor	srcColorBlendFactor;
	BlendFactor	dstColorBlendFactor;
	BlendOp	colorBlendOp;
	BlendFactor	srcAlphaBlendFactor;
	BlendFactor	dstAlphaBlendFactor;
	BlendOp	alphaBlendOp;
};

struct SurfaceInfo
{
	lib::string name;
	lib::array<Format> preferredSurfaceFormats;
	void* instance;
	void* window;
};

struct SwapchainInfo
{
	lib::string name;
	SurfaceInfo surfaceInfo;
	Extent2D dimension;
	uint32 imageCount;
	ImageUsage imageUsage;
	SwapchainPresentMode presentationMode;
};

struct ShaderAttribute
{
	std::string_view name;
	uint32 location;
	Format format;
};

/**
* \brief This struct is not meant to be manually filled.
*/
struct CompiledShaderInfo
{
	std::string_view name;
	std::string_view path;
	ShaderType type;
	std::string_view entryPoint;
	std::span<uint32> binaries;
	std::span<ShaderAttribute> vertexInputAttributes;
};

struct ShaderInfo
{
	lib::string name;
	ShaderType type;
	lib::string entryPoint;
};

struct BufferInfo
{
	lib::string name;
	size_t size;
	BufferUsage	bufferUsage;
	MemoryUsage memoryUsage;
	SharingMode sharingMode;
};

struct ImageInfo
{
	lib::string name;
	ImageType type;
	Format	format;
	SampleCount	samples;
	ImageTiling	tiling;
	ImageUsage imageUsage;
	MemoryUsage memoryUsage;
	Extent3D dimension;
	ClearValue clearValue;
	uint32 mipLevel;
	SharingMode sharingMode;
};

struct SamplerInfo
{
	lib::string name;
	TexelFilter minFilter;
	TexelFilter magFilter;
	MipmapMode mipmapMode;
	SamplerAddress addressModeU;
	SamplerAddress addressModeV;
	SamplerAddress addressModeW;
	float32 mipLodBias;
	float32 maxAnisotropy;
	CompareOp compareOp;
	float32 minLod;
	float32 maxLod;
	BorderColor borderColor;
	bool unnormalizedCoordinates;
};

struct CommandPoolInfo
{
	lib::string name;
	DeviceQueue queue;
};

struct CommandBufferInfo
{
	lib::string name;
};

struct DepthTestInfo
{
	CompareOp depthTestCompareOp;
	float32 minDepthBounds;
	float32	maxDepthBounds;
	bool enableDepthBoundsTest;	// Allows pixels to be discarded if the currently-stored depth value is outside the range specified by minDepthBounds and maxDepthBounds.
	bool enableDepthTest;
	bool enableDepthWrite;
};

struct RasterizationStateInfo
{
	PolygonMode polygonalMode;
	CullingMode cullMode;
	FrontFace frontFace;
	float32 lineWidth;
	bool enableDepthClamp;

};

struct ColorAttachment
{
	Format format;
	ColorBlendInfo blendInfo;
};

struct RasterPipelineInfo
{
	lib::string name;
	lib::array<ColorAttachment> colorAttachments;
	Format depthAttachmentFormat;
	Format stencilAttachmentFormat;
	lib::array<VertexInputBinding> vertexInputBindings;
	RasterizationStateInfo rasterization;
	DepthTestInfo depthTest;
	TopologyType topology;
	uint32 pushConstantSize;
};

struct ComputePipelineInfo
{
	lib::string name;
	uint32 pushConstantSize;
};

struct SemaphoreInfo
{
	lib::string name;
};

struct FenceInfo
{
	lib::string name;
	uint64 initialValue;
};

struct EventInfo
{
	lib::string name;
};

struct MemoryRequirementInfo
{
	size_t size;
	size_t alignment;
	uint32 memoryTypeBits;
	MemoryUsage usage;
};

struct MemoryBlockInfo
{
	lib::string name;
	MemoryUsage usage;
};

struct FormatTexelInfo
{
	size_t size; // In bytes.
	Extent3D blockDimension;
	uint32 texelPerBlock;
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

struct ImageSubresource
{
	ImageAspect	aspectFlags;
	uint32 mipLevel;
	uint32 levelCount;
	uint32 baseArrayLayer;
	uint32 layerCount;
};

struct FrameInfo
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
struct Access
{
	PipelineStage stages;
	MemoryAccessType type;
};

namespace access
{
// read.
inline static constexpr Access TOP_OF_PIPE_READ							= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::Memory_Read };
inline static constexpr Access DRAW_INDIRECT_READ						= { PipelineStage::Draw_Indirect,					MemoryAccessType::Memory_Read };
inline static constexpr Access VERTEX_INPUT_READ						= { PipelineStage::Vertex_Input,					MemoryAccessType::Memory_Read };
inline static constexpr Access VERTEX_SHADER_READ						= { PipelineStage::Vertex_Shader,					MemoryAccessType::Memory_Read };
inline static constexpr Access TESSELATION_CONTROL_READ					= { PipelineStage::Tesselation_Control,				MemoryAccessType::Memory_Read };
inline static constexpr Access TESSELATION_EVALUATION_READ				= { PipelineStage::Tesselation_Evaluation,			MemoryAccessType::Memory_Read };
inline static constexpr Access GEOMETRY_SHADER_READ						= { PipelineStage::Geometry_Shader,					MemoryAccessType::Memory_Read };
inline static constexpr Access FRAGMENT_SHADER_READ						= { PipelineStage::Fragment_Shader,					MemoryAccessType::Memory_Read };
inline static constexpr Access PIXEL_SHADER_READ						= { PipelineStage::Pixel_Shader,					MemoryAccessType::Memory_Read };
inline static constexpr Access EARLY_FRAGMENT_TEST_READ					= { PipelineStage::Early_Fragment_Test,				MemoryAccessType::Memory_Read };
inline static constexpr Access LATE_FRAGMENT_TEST_READ					= { PipelineStage::Late_Fragment_Test,				MemoryAccessType::Memory_Read };
inline static constexpr Access COLOR_ATTACHMENT_OUTPUT_READ				= { PipelineStage::Color_Attachment_Output,			MemoryAccessType::Memory_Read };
inline static constexpr Access COMPUTE_SHADER_READ						= { PipelineStage::Compute_Shader,					MemoryAccessType::Memory_Read };
inline static constexpr Access TRANSFER_READ							= { PipelineStage::Transfer,						MemoryAccessType::Memory_Read };
inline static constexpr Access BOTTOM_OF_PIPE_READ						= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::Memory_Read };
inline static constexpr Access HOST_READ								= { PipelineStage::Host,							MemoryAccessType::Memory_Read };
inline static constexpr Access ALL_GRAPHICS_READ						= { PipelineStage::All_Graphics,					MemoryAccessType::Memory_Read };
inline static constexpr Access ALL_COMMANDS_READ						= { PipelineStage::All_Commands,					MemoryAccessType::Memory_Read };
inline static constexpr Access COPY_READ								= { PipelineStage::Copy,							MemoryAccessType::Memory_Read };
inline static constexpr Access RESOLVE_READ								= { PipelineStage::Resolve,							MemoryAccessType::Memory_Read };
inline static constexpr Access BLIT_READ								= { PipelineStage::Blit,							MemoryAccessType::Memory_Read };
inline static constexpr Access CLEAR_READ								= { PipelineStage::Clear,							MemoryAccessType::Memory_Read };
inline static constexpr Access INDEX_INPUT_READ							= { PipelineStage::Index_Input,						MemoryAccessType::Memory_Read };
inline static constexpr Access VERTEX_ATTRIBUTE_INPUT_READ				= { PipelineStage::Vertex_Attribute_Input,			MemoryAccessType::Memory_Read };
inline static constexpr Access PRE_RASTERIZATION_SHADERS_READ			= { PipelineStage::Pre_Rasterization_Shaders,		MemoryAccessType::Memory_Read };
inline static constexpr Access ACCELERATION_STRUCTURE_BUILD_READ		= { PipelineStage::Acceleration_Structure_Build,	MemoryAccessType::Memory_Read };
inline static constexpr Access RAY_TRACING_SHADER_READ					= { PipelineStage::Ray_Tracing_Shader,				MemoryAccessType::Memory_Read };
inline static constexpr Access TASK_SHADER_READ							= { PipelineStage::Task_Shader,						MemoryAccessType::Memory_Read };
inline static constexpr Access MESH_SHADER_READ							= { PipelineStage::Mesh_Shader,						MemoryAccessType::Memory_Read };
inline static constexpr Access ACCELERATION_STRUCTURE_COPY_READ			= { PipelineStage::Acceleration_Structure_Copy,		MemoryAccessType::Memory_Read };
// write.
inline static constexpr Access TOP_OF_PIPE_WRITE						= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::Memory_Write };
inline static constexpr Access DRAW_INDIRECT_WRITE						= { PipelineStage::Draw_Indirect,					MemoryAccessType::Memory_Write };
inline static constexpr Access VERTEX_INPUT_WRITE						= { PipelineStage::Vertex_Input,					MemoryAccessType::Memory_Write };
inline static constexpr Access VERTEX_SHADER_WRITE						= { PipelineStage::Vertex_Shader,					MemoryAccessType::Memory_Write };
inline static constexpr Access TESSELATION_CONTROL_WRITE				= { PipelineStage::Tesselation_Control,				MemoryAccessType::Memory_Write };
inline static constexpr Access TESSELATION_EVALUATION_WRITE				= { PipelineStage::Tesselation_Evaluation,			MemoryAccessType::Memory_Write };
inline static constexpr Access GEOMETRY_SHADER_WRITE					= { PipelineStage::Geometry_Shader,					MemoryAccessType::Memory_Write };
inline static constexpr Access FRAGMENT_SHADER_WRITE					= { PipelineStage::Fragment_Shader,					MemoryAccessType::Memory_Write };
inline static constexpr Access PIXEL_SHADER_WRITE						= { PipelineStage::Pixel_Shader,					MemoryAccessType::Memory_Write };
inline static constexpr Access EARLY_FRAGMENT_TEST_WRITE				= { PipelineStage::Early_Fragment_Test,				MemoryAccessType::Memory_Write };
inline static constexpr Access LATE_FRAGMENT_TEST_WRITE					= { PipelineStage::Late_Fragment_Test,				MemoryAccessType::Memory_Write };
inline static constexpr Access COLOR_ATTACHMENT_OUTPUT_WRITE			= { PipelineStage::Color_Attachment_Output,			MemoryAccessType::Memory_Write };
inline static constexpr Access COMPUTE_SHADER_WRITE						= { PipelineStage::Compute_Shader,					MemoryAccessType::Memory_Write };
inline static constexpr Access TRANSFER_WRITE							= { PipelineStage::Transfer,						MemoryAccessType::Memory_Write };
inline static constexpr Access BOTTOM_OF_PIPE_WRITE						= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::Memory_Write };
inline static constexpr Access HOST_WRITE								= { PipelineStage::Host,							MemoryAccessType::Memory_Write };
inline static constexpr Access ALL_GRAPHICS_WRITE						= { PipelineStage::All_Graphics,					MemoryAccessType::Memory_Write };
inline static constexpr Access ALL_COMMANDS_WRITE						= { PipelineStage::All_Commands,					MemoryAccessType::Memory_Write };
inline static constexpr Access COPY_WRITE								= { PipelineStage::Copy,							MemoryAccessType::Memory_Write };
inline static constexpr Access RESOLVE_WRITE							= { PipelineStage::Resolve,							MemoryAccessType::Memory_Write };
inline static constexpr Access BLIT_WRITE								= { PipelineStage::Blit,							MemoryAccessType::Memory_Write };
inline static constexpr Access CLEAR_WRITE								= { PipelineStage::Clear,							MemoryAccessType::Memory_Write };
inline static constexpr Access INDEX_INPUT_WRITE						= { PipelineStage::Index_Input,						MemoryAccessType::Memory_Write };
inline static constexpr Access VERTEX_ATTRIBUTE_INPUT_WRITE				= { PipelineStage::Vertex_Attribute_Input,			MemoryAccessType::Memory_Write };
inline static constexpr Access PRE_RASTERIZATION_SHADERS_WRITE			= { PipelineStage::Pre_Rasterization_Shaders,		MemoryAccessType::Memory_Write };
inline static constexpr Access ACCELERATION_STRUCTURE_BUILD_WRITE		= { PipelineStage::Acceleration_Structure_Build,	MemoryAccessType::Memory_Write };
inline static constexpr Access RAY_TRACING_SHADER_WRITE					= { PipelineStage::Ray_Tracing_Shader,				MemoryAccessType::Memory_Write };
inline static constexpr Access TASK_SHADER_WRITE						= { PipelineStage::Task_Shader,						MemoryAccessType::Memory_Write };
inline static constexpr Access MESH_SHADER_WRITE						= { PipelineStage::Mesh_Shader,						MemoryAccessType::Memory_Write };
inline static constexpr Access ACCELERATION_STRUCTURE_COPY_WRITE		= { PipelineStage::Acceleration_Structure_Copy,		MemoryAccessType::Memory_Write };
// read write.
inline static constexpr Access TOP_OF_PIPE_READ_WRITE					= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::Memory_Read_Write };
inline static constexpr Access DRAW_INDIRECT_READ_WRITE					= { PipelineStage::Draw_Indirect,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access VERTEX_INPUT_READ_WRITE					= { PipelineStage::Vertex_Input,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access VERTEX_SHADER_READ_WRITE					= { PipelineStage::Vertex_Shader,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access TESSELATION_CONTROL_READ_WRITE			= { PipelineStage::Tesselation_Control,				MemoryAccessType::Memory_Read_Write };
inline static constexpr Access TESSELATION_EVALUATION_READ_WRITE		= { PipelineStage::Tesselation_Evaluation,			MemoryAccessType::Memory_Read_Write };
inline static constexpr Access GEOMETRY_SHADER_READ_WRITE				= { PipelineStage::Geometry_Shader,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access FRAGMENT_SHADER_READ_WRITE				= { PipelineStage::Fragment_Shader,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access PIXEL_SHADER_READ_WRITE					= { PipelineStage::Pixel_Shader,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access EARLY_FRAGMENT_TEST_READ_WRITE			= { PipelineStage::Early_Fragment_Test,				MemoryAccessType::Memory_Read_Write };
inline static constexpr Access LATE_FRAGMENT_TEST_READ_WRITE			= { PipelineStage::Late_Fragment_Test,				MemoryAccessType::Memory_Read_Write };
inline static constexpr Access COLOR_ATTACHMENT_OUTPUT_READ_WRITE		= { PipelineStage::Color_Attachment_Output,			MemoryAccessType::Memory_Read_Write };
inline static constexpr Access COMPUTE_SHADER_READ_WRITE				= { PipelineStage::Compute_Shader,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access TRANSFER_READ_WRITE						= { PipelineStage::Transfer,						MemoryAccessType::Memory_Read_Write };
inline static constexpr Access BOTTOM_OF_PIPE_READ_WRITE				= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access HOST_READ_WRITE							= { PipelineStage::Host,							MemoryAccessType::Memory_Read_Write };
inline static constexpr Access ALL_GRAPHICS_READ_WRITE					= { PipelineStage::All_Graphics,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access ALL_COMMANDS_READ_WRITE					= { PipelineStage::All_Commands,					MemoryAccessType::Memory_Read_Write };
inline static constexpr Access COPY_READ_WRITE							= { PipelineStage::Copy,							MemoryAccessType::Memory_Read_Write };
inline static constexpr Access RESOLVE_READ_WRITE						= { PipelineStage::Resolve,							MemoryAccessType::Memory_Read_Write };
inline static constexpr Access BLIT_READ_WRITE							= { PipelineStage::Blit,							MemoryAccessType::Memory_Read_Write };
inline static constexpr Access CLEAR_READ_WRITE							= { PipelineStage::Clear,							MemoryAccessType::Memory_Read_Write };
inline static constexpr Access INDEX_INPUT_READ_WRITE					= { PipelineStage::Index_Input,						MemoryAccessType::Memory_Read_Write };
inline static constexpr Access VERTEX_ATTRIBUTE_INPUT_READ_WRITE		= { PipelineStage::Vertex_Attribute_Input,			MemoryAccessType::Memory_Read_Write };
inline static constexpr Access PRE_RASTERIZATION_SHADERS_READ_WRITE		= { PipelineStage::Pre_Rasterization_Shaders,		MemoryAccessType::Memory_Read_Write };
inline static constexpr Access ACCELERATION_STRUCTURE_BUILD_READ_WRITE	= { PipelineStage::Acceleration_Structure_Build,	MemoryAccessType::Memory_Read_Write };
inline static constexpr Access RAY_TRACING_SHADER_READ_WRITE			= { PipelineStage::Ray_Tracing_Shader,				MemoryAccessType::Memory_Read_Write };
inline static constexpr Access TASK_SHADER_READ_WRITE					= { PipelineStage::Task_Shader,						MemoryAccessType::Memory_Read_Write };
inline static constexpr Access MESH_SHADER_READ_WRITE					= { PipelineStage::Mesh_Shader,						MemoryAccessType::Memory_Read_Write };
inline static constexpr Access ACCELERATION_STRUCTURE_COPY_READ_WRITE	= { PipelineStage::Acceleration_Structure_Copy,		MemoryAccessType::Memory_Read_Write };
// ownership transfers.
inline static constexpr Access TOP_OF_PIPE_NONE							= { PipelineStage::Top_Of_Pipe,						MemoryAccessType::None };
inline static constexpr Access BOTTOM_OF_PIPE_NONE						= { PipelineStage::Bottom_Of_Pipe,					MemoryAccessType::None };
}

auto is_color_format(Format format) -> bool;
auto is_depth_format(Format format) -> bool;
auto is_stencil_format(Format format) -> bool;
auto to_string(Format format) -> std::string_view;
auto to_string(ColorSpace colorSpace) -> std::string_view;
auto format_texel_info(Format format) -> FormatTexelInfo;
}

#endif // !GPU_COMMON_H
