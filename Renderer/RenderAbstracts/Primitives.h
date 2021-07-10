#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H

#include "Library/Containers/Array.h"
#include "Library/Containers/Bitset.h"
//#include "Library/Containers/Ref.h"
#include "Library/Containers/String.h"
#include "SubSystem/Resource/Handle.h"
#include "Engine/Private/Prototype.h"
#include "API/RendererFlagBits.h"
#include "API/Definitions.h"
#include "API/ShaderAttribute.h"

using VmaAllocation = struct VmaAllocation_T*;
using VkBuffer = struct VkBuffer_T*;
using VkSampler = struct VkSampler_T*;
using VkImage = struct VkImage_T*;
using VkImageView = struct VkImageView_T*;
using VkShaderModule = struct VkShaderModule_T*;
using VkPipeline = struct VkPipeline_T*;
using VkPipelineLayout = struct VkPipelineLayout_T*;
using VkDescriptorPool = struct VkDescriptorPool_T*;
using VkDescriptorSetLayout = struct VkDescriptorSetLayout_T*;
using VkDescriptorSet = struct VkDescriptorSet_T*;

struct SRenderPass;
struct SDescriptorSetLayout;

struct Viewport
{
	float32 x;
	float32 y;
	float32 Width;
	float32 Height;
	float32 MinDepth;
	float32 MaxDepth;
};

struct Offset2D
{
	int32 x;
	int32 y;
};

struct Rect2D
{
	Offset2D Offset;
	WindowInfo::Extent2D Extent;
};

struct SMemoryBuffer
{
	astl::BitSet<EBufferTypeFlagBits>	Type;
	EBufferLocality Locality;
	VmaAllocation Allocation;
	uint8* pData;
	size_t Size;
	size_t Offset;
  uint32 FirstBinding;
	VkBuffer Hnd;
};

struct BufferAllocateInfo
{
	astl::BitSet<EBufferTypeFlagBits> Type;
	EBufferLocality Locality;
	size_t Size;
  uint32 FirstBinding;  // Only set if buffer will be used for vertex attrib binding. Ignore for UBOs or SSBOs.
};

struct ImageSamplerState
{
	ESamplerFilter MinFilter;
	ESamplerFilter MagFilter;
	ESamplerAddressMode AddressModeU;
	ESamplerAddressMode AddressModeV;
	ESamplerAddressMode AddressModeW;
	ECompareOp CompareOp;
	float32 AnisotropyLvl;
  float32 MaxLod;
};

struct SImageSampler : ImageSamplerState
{
	VkSampler Hnd;
	uint64 Hash;
};

using ImageSamplerCreateInfo = ImageSamplerState;

struct SImage
{
	VmaAllocation Allocation;
	astl::BitSet<EImageUsageFlagBits> Usage;
	size_t Size;
	uint32 Width;
	uint32 Height;
	uint32 Channels;
	ETextureType Type;
  ETextureFormat Format;
	VkImage ImgHnd;
	VkImageView ImgViewHnd;
	uint32 MipLevels = 1;
	bool MipMaps;
};

//struct ImageCreateInfo
//{
//	size_t Identifier = ~0ULL; // Ignore for a default generated id.
//	uint32 Width;
//	uint32 Height;
//	uint32 Channels;
//	ETextureType Type;
//};

struct SShader
{
	using AttribsContainer = astl::StaticArray<ShaderAttrib, MAX_SHADER_ATTRIBUTES>;

	astl::Array<uint32> SpirV;
	AttribsContainer Attributes;
	VkShaderModule Hnd;
	EShaderType Type;
};

struct ShaderCreateInfo
{
	astl::String pCode;
	EShaderType Type;
};

struct SPipeline
{
	struct VertexInputBinding
	{
		uint32 Binding;
		uint32 From;
		uint32 To;
		uint32 Stride;
		EVertexInputRateType Type;
	};

	using VertexInBindings = astl::StaticArray<VertexInputBinding, MAX_VERTEX_INPUT_BINDING>;
	using LayoutsContainer = astl::StaticArray<astl::Ref<SDescriptorSetLayout>, MAX_PIPELINE_DESCRIPTOR_LAYOUT>;

	VertexInBindings VertexBindings;
	LayoutsContainer Layouts;
	astl::Ref<SRenderPass> pRenderPass;
	astl::Ref<SShader> pVertexShader;
	astl::Ref<SShader> pFragmentShader;
	astl::Ref<SShader> pGeometryShader;
	astl::Ref<SShader> pComputeShader;
	VkPipeline Hnd;
	VkPipelineLayout LayoutHnd;
	Viewport Viewport;
	Rect2D Scissor;
	ESampleCount Samples;
	ETopologyType Topology;
	EFrontFaceDir FrontFace;
	ECullingMode CullMode;
	EPolygonMode PolygonalMode;
	EPipelineBindPoint BindPoint;
  EBlendFactor SrcColorBlendFactor;
  EBlendFactor DstColorBlendFactor;
  EBlendOp ColorBlendOp;
  EBlendFactor SrcAlphaBlendFactor;
  EBlendFactor DstAlphaBlendFactor;
  EBlendOp AlphaBlendOp;
  uint32 ColorOutputCount;
	bool HasDepthStencil;
};

struct PipelineCreateInfo
{
	Viewport Viewport;
	Rect2D Scissor;
	Handle<SShader> VertexShaderHnd;
	Handle<SShader> FragmentShaderHnd;
	Handle<SShader> GeometryShaderHnd;
	Handle<SShader> ComputeShaderHnd;
	ESampleCount Samples;
	ETopologyType Topology;
	EFrontFaceDir FrontFace;
	ECullingMode CullMode;
	EPolygonMode PolygonalMode;
	EPipelineBindPoint BindPoint;
  EBlendFactor SrcColorBlendFactor;
  EBlendFactor DstColorBlendFactor;
  EBlendOp ColorBlendOp;
  EBlendFactor SrcAlphaBlendFactor;
  EBlendFactor DstAlphaBlendFactor;
  EBlendOp AlphaBlendOp;
	uint32 ColorOutputCount;
	bool HasDepthStencil;
};

struct SPushConstant
{
	uint8 Data[Push_Constant_Size];
	size_t Offset;
	//astl::Ref<SPipeline> pPipeline;
};

struct SDescriptorPool
{
	struct Size
	{
		EDescriptorType Type;	// Descriptor type
		uint32 Count;			// Descriptor count
	};

	astl::StaticArray<Size, MAX_DESCRIPTOR_POOL_TYPE_SIZE> Sizes;
	VkDescriptorPool Hnd[MAX_FRAMES_IN_FLIGHT];
};

/**
* A single descriptor set layout can have multiple bindings.
*/
struct SDescriptorSetLayout
{
	struct Binding
	{
		uint32 BindingSlot;
		uint32 DescriptorCount;
		size_t Stride;							// Stride for data. Ignore for images.
		size_t Offset[MAX_FRAMES_IN_FLIGHT];	// Offset that's in the buffer for each frame. Ignore for images.
		EDescriptorType Type;
		astl::BitSet<uint32> ShaderStages;
		astl::Ref<SMemoryBuffer> pBuffer;

		Binding() :
			BindingSlot(0),
			DescriptorCount(0),
			Stride(0),
			Offset{0},
			Type(Descriptor_Type_Max),
			ShaderStages{},
			pBuffer{}
		{}

		~Binding() {};
	};

	using BindingsContainer = astl::StaticArray<Binding, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;
	VkDescriptorSetLayout Hnd;
	astl::Ref<SPipeline> pPipeline;
	BindingsContainer Bindings;
};

/**
* Generally, each descriptor set can only occupy a slot in the driver (set = ??? in the shader)
* If 2 descriptor sets referencing different layouts (and each layout have different binding values) are bounded to
* the same slot, one will overwrite the other.
*
* A descriptor set can only references one layout.
*/
struct SDescriptorSet
{
	astl::Ref<SDescriptorPool> pPool;
	astl::Ref<SDescriptorSetLayout> pLayout;
	VkDescriptorSet Hnd[MAX_FRAMES_IN_FLIGHT];
	uint32 Slot;
};

struct SDescriptorSetInstance
{
	astl::Ref<SDescriptorSet> pSet;
	size_t Offset;
	uint32 Binding;
};

struct DescriptorSetLayoutBindingInfo
{
	size_t Stride;							// Stride for data. Ignore for images.
	uint32 DescriptorCount;
	uint32 BindingSlot;
	EDescriptorType Type;
	Handle<SDescriptorSetLayout> LayoutHnd;
	astl::BitSet<EShaderTypeFlagBits> ShaderStages;
};

struct DescriptorSetAllocateInfo
{
	Handle<SDescriptorPool> PoolHnd;
	Handle<SDescriptorSetLayout> LayoutHnd;
	uint32 Slot;
};

struct SBindable
{
	union
	{
		astl::Ref<SDescriptorSet> pSet;
		astl::Ref<SPipeline> pPipeline;
    astl::Ref<SMemoryBuffer> pBuffer;
	};
	EBindableType Type;
	bool Bound;

	SBindable() :
		pPipeline{}, Type(EBindableType::Bindable_Type_None), Bound(false)
	{}

	~SBindable() {}
};

struct SBuildCommand
{
  union
  {
    astl::Ref<SMemoryBuffer> pBuffer;
    astl::Ref<SDescriptorPool> pPool;
    astl::Ref<SDescriptorSetLayout> pSetLayout;
    astl::Ref<SDescriptorSet> pSet;
    astl::Ref<SImage> pImg;
    astl::Ref<SImageSampler> pImgSampler;
    astl::Ref<SShader> pShader;
    astl::Ref<SPipeline> pPipeline;
  };
  EResourceBuildType Type;

  SBuildCommand() :
    pBuffer{}, Type{ EResourceBuildType::Resource_Build_Type_None }
  {}

  SBuildCommand(astl::Ref<SMemoryBuffer> pInBuffer) :
    pBuffer{ pInBuffer }, Type{ EResourceBuildType::Resource_Build_Type_Buffer }
  {}

  SBuildCommand(astl::Ref<SDescriptorPool> pInPool) :
    pPool{ pInPool }, Type{ EResourceBuildType::Resource_Build_Type_Descriptor_Pool }
  {}

  SBuildCommand(astl::Ref<SDescriptorSetLayout> pInSetLayout) :
    pSetLayout{ pInSetLayout }, Type{ EResourceBuildType::Resource_Build_Type_Descriptor_Set_Layout }
  {}

  SBuildCommand(astl::Ref<SDescriptorSet> pInSet) :
    pSet{ pInSet }, Type{ EResourceBuildType::Resource_Build_Type_Descriptor_Set }
  {}

  SBuildCommand(astl::Ref<SImage> pInImg) :
    pImg{ pInImg }, Type{ EResourceBuildType::Resource_Build_Type_Image }
  {}

  SBuildCommand(astl::Ref<SImageSampler> pInImgSampler) :
    pImgSampler{ pInImgSampler }, Type{ EResourceBuildType::Resource_Build_Type_Image_Sampler }
  {}

  SBuildCommand(astl::Ref<SShader> pShader) :
    pShader{ pShader }, Type{ EResourceBuildType::Resource_Build_Type_Shader }
  {}

  SBuildCommand(astl::Ref<SPipeline> pInPipeline) :
    pPipeline{ pInPipeline }, Type{ EResourceBuildType::Resource_Build_Type_Pipeline }
  {}

  ~SBuildCommand() {}
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H
