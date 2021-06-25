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
	BitSet<EBufferTypeFlagBits>	Type;
	EBufferLocality Locality;
	VmaAllocation Allocation;
	uint8* pData;
	size_t Size;
	size_t Offset;
	VkBuffer Hnd;
};

struct BufferAllocateInfo
{
	BitSet<EBufferTypeFlagBits> Type;
	EBufferLocality Locality;
	size_t Size;
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
	BitSet<EImageUsageFlagBits> Usage;
	size_t Size;
	uint32 Width;
	uint32 Height;
	uint32 Channels;
	ETextureType Type;
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
	using AttribsContainer = StaticArray<ShaderAttrib, MAX_SHADER_ATTRIBUTES>;

	Array<uint32> SpirV;
	AttribsContainer Attributes;
	VkShaderModule Hnd;
	EShaderType Type;
};

struct ShaderCreateInfo
{
	String pCode;
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

	using VertexInBindings = StaticArray<VertexInputBinding, MAX_VERTEX_INPUT_BINDING>;
	using LayoutsContainer = StaticArray<Ref<SDescriptorSetLayout>, MAX_PIPELINE_DESCRIPTOR_LAYOUT>;

	VertexInBindings VertexBindings;
	LayoutsContainer Layouts;
	Ref<SRenderPass> pRenderPass;
	Ref<SShader> pVertexShader;
	Ref<SShader> pFragmentShader;
	Ref<SShader> pGeometryShader;
	Ref<SShader> pComputeShader;
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
	uint32 ColorOutputCount;
	bool HasDepthStencil;
};

struct SPushConstant
{
	uint8 Data[Push_Constant_Size];
	size_t Offset;
	//Ref<SPipeline> pPipeline;
};

struct SDescriptorPool
{
	struct Size
	{
		EDescriptorType Type;	// Descriptor type
		uint32 Count;			// Descriptor count
	};

	StaticArray<Size, MAX_DESCRIPTOR_POOL_TYPE_SIZE> Sizes;
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
		BitSet<uint32> ShaderStages;
		Ref<SMemoryBuffer> pBuffer;

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

	using BindingsContainer = StaticArray<Binding, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;
	VkDescriptorSetLayout Hnd;
	Ref<SPipeline> pPipeline;
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
	Ref<SDescriptorPool> pPool;
	Ref<SDescriptorSetLayout> pLayout;
	VkDescriptorSet Hnd[MAX_FRAMES_IN_FLIGHT];
	uint32 Slot;
};

struct SDescriptorSetInstance
{
	Ref<SDescriptorSet> pSet;
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
	BitSet<EShaderTypeFlagBits> ShaderStages;
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
		Ref<SDescriptorSet> pSet;
		Ref<SPipeline> pPipeline;
	};
	EBindableType Type;
	bool Bound;

	SBindable() :
		pPipeline{}, Type(EBindableType::Bindable_Type_None), Bound(false)
	{}

	~SBindable() {}
};

struct DrawCommand
{
  Ref<SPipeline> pPipeline;
  uint8  Constants[128];
	uint32 NumVertices;
	uint32 NumIndices;
	uint32 VertexOffset;
	uint32 IndexOffset;
	uint32 InstanceOffset;
	uint32 InstanceCount;
  bool   HasPushConstants = false;
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H
