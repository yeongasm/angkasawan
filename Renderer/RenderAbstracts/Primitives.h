#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H

#include "Library/Containers/Array.h"
#include "Library/Containers/Bitset.h"
#include "API/Common.h"
#include "API/RendererFlagBits.h"
#include "API/Definitions.h"
#include "API/ShaderAttribute.h"

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

struct SImageSampler
{
	ESamplerFilter MinFilter;
	ESamplerFilter MagFilter;
	ESamplerAddressMode AddressModeU;
	ESamplerAddressMode AddressModeV;
	ESamplerAddressMode AddressModeW;
	ECompareOp CompareOp;
	float32 AnisotropyLvl;
	VkSampler Hnd;
};

struct SImage
{
	VmaAllocation Allocation;
	SImageSampler* pSampler;
	BitSet<EImageUsageFlagBits> Usage;
	size_t Size;
	uint32 Width;
	uint32 Height;
	uint32 Channels;
	ETextureType Type;
	VkImage ImgHnd;
	VkImageView ImgViewHnd;
};

struct ImageCreateInfo
{
	size_t Identifier = -1; // Ignore for a default generated id.
	uint32 Width;
	uint32 Height;
	uint32 Channels;
	ETextureType Type;
};

struct SShader
{
	using AttribsContainer = StaticArray<ShaderAttrib, MAX_SHADER_ATTRIBUTES>;

	AttribsContainer Attributes;
	VkShaderModule Hnd;
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
	using LayoutsContainer = StaticArray<SDescriptorSetLayout*, MAX_PIPELINE_DESCRIPTOR_LAYOUT>;

	VertexInBindings VertexBindings;
	LayoutsContainer Layouts;
	VkPipeline Hnd;
	ESampleCount Samples;
	ETopologyType Topology;
	EFrontFaceDir FrontFace;
	ECullingMode CullMode;
	EPolygonMode PolygonalMode;
	uint32 ColorOutputCount;
	bool HasDefaultColorOutput;
	bool HasDepthStencil;
};

struct SPushConstant
{
	uint8 Data[Push_Constant_Size];
	size_t Offset;
	SPipeline* pPipeline;
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
		uint32 Binding;
		uint32 DescriptorCount;
		size_t Size;
		size_t Allocated;
		size_t Offset[MAX_FRAMES_IN_FLIGHT];
		EDescriptorType Type;
		BitSet<uint32> ShaderStages;
		SMemoryBuffer* Buffer;
	};

	using BindingsContainer = StaticArray<Binding, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;
	VkDescriptorSetLayout Hnd;
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
	SDescriptorPool* pPool;
	SDescriptorSetLayout* pLayout;
	VkDescriptorSet Hnd[MAX_FRAMES_IN_FLIGHT];
	uint32 Slot;
};

struct SDescriptorSetInstance
{
	SDescriptorSet* pSet;
	size_t Offset;
	uint32 Binding;
};

struct DescriptorSetLayoutBindingInfo
{
	size_t Size;
	uint32 DescriptorCount;
	uint32 Binding;
	EDescriptorType Type;
	Handle<SDescriptorSetLayout> LayoutHnd;
	Handle<SMemoryBuffer> BufferHnd;
	BitSet<EShaderTypeFlagBits> ShaderStages;
};

struct DescriptorSetAllocateInfo
{
	Handle<SDescriptorPool> PoolHnd;
	Handle<SDescriptorSetLayout> LayoutHnd;
	uint32 Slot;
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H