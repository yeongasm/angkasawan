#pragma once
#ifndef ANGKASA1_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H
#define ANGKASA1_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H

#include "Library/Containers/Array.h"
#include "Library/Containers/Bitset.h"
#include "API/Definitions.h"
#include "API/RendererFlagBits.h"
#include "API/Common.h"

/**
* NOTE(Ygsm):
* Uniform buffers are for known amounts of data sets that will be read in the shader. (static array equivalent).
* Storage buffers are for unknown amounts of data sets that will be read and written to in the shader. (vector equivalent).
*/

struct SMemoryBuffer;
struct SPipeline;

/**
* A single descriptor pool can have multiple types of descriptor pool sizes.
* maxSets would be the total number of descriptor types from each pool sizes.
*/
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
	struct UpdateContext
	{
		using Binding = SDescriptorSetLayout::Binding;

		SDescriptorSet* pSet;
		Binding* pBinding;
		SMemoryBuffer* pBuffer;
		STexture** pTextures;
		uint32 NumTextures;
	};

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

#endif // !ANGKASA1_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H