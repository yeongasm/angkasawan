#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H

#include "Library/Containers/Bitset.h"
#include "Library/Containers/Array.h"
#include "Library/Allocators/LinearAllocator.h"
#include "Assets/GPUHandles.h"
#include "RenderPlatform/API.h"
#include "API/RendererFlagBits.h"

class RenderContext;
class RenderSystem;

struct UniformBufferCreateInfo
{
	uint32 Id;
	uint32 Capacity;
};

struct UniformBuffer : public UniformBufferCreateInfo
{
	uint32 Offset;
	Handle<HBuffer> Handle;
};

struct DescriptorPoolBase
{
	uint32 Id;
	uint32 Capacity;
};

struct DescriptorPoolCreateInfo : public DescriptorPoolBase
{
	uint32 DescriptorTypes;
};

struct DescriptorPool : public DescriptorPoolBase
{
	uint32 Slots;
	BitSet<uint32> DescriptorTypes;
	Handle<HDescPool> Handle;
};

struct DescriptorLayoutBase
{
	uint32 Id;
	uint32 Binding;
	EDescriptorType Type;
};

struct DescriptorLayoutCreateInfo : DescriptorLayoutBase
{
	uint32 ShaderStages;
};

struct DescriptorLayout : public DescriptorLayoutBase
{
	BitSet<uint32> ShaderStages;
	Handle<HDescLayout> Handle;
};

struct DescriptorSetBase
{
	uint32 Id;
	uint32 TypeSize;
	uint32 NumOfData;
	EDescriptorType Type;
};

struct DescriptorSetCreateInfo : public DescriptorSetBase
{
	Handle<DescriptorPool> DescriptorPoolHandle;
	Handle<DescriptorLayout> DescriptorLayoutHandle;
	Handle<UniformBuffer> UniformBufferHandle;
};

struct DescriptorSet : public DescriptorSetBase
{
	uint32 Base;			// Base offset to the descriptor set's memory region.
	uint32 Offset;			// Current memory offset in the descriptor set.
	DescriptorPool* Pool;
	DescriptorLayout* Layout;
	UniformBuffer* Buffer;
	Handle<HDescSet> Handle;
};

struct DescriptorSetInstance
{
	DescriptorSet* Owner;
	uint32 Offset;
};

struct DescriptorManagerConfiguration
{
	size_t PoolReserveCount;
	size_t LayoutReserveCount;
	size_t SetReserveCount;
};

/**
* Manages descriptor pools, layouts and sets.
* Also caches them instead of allocating new descriptor sets every frame.
*/
class RENDERER_API IRDescriptorManager
{
private:

	friend class RenderSystem;

	enum ExistEnum : size_t
	{
		Resource_Not_Exist = -1
	};

	RenderContext&			Context;
	LinearAllocator&		Allocator;
	Array<DescriptorSet*>	Sets;
	Array<DescriptorPool*>	Pools;
	Array<DescriptorLayout*> Layouts;
	Array<UniformBuffer*>	Buffers;

	void	BuildUniformBuffers			();
	void	BuildDescriptorPools		();
	void	BuildDescriptorLayouts		();
	void	BuildDescriptorSets			();
	size_t	DoesUniformBufferExist		(uint32 Id);
	size_t	DoesDescriptorPoolExist		(uint32 Id);
	size_t	DoesDescriptorLayoutExist	(uint32 Id);
	size_t	DoesDescriptorSetExist		(uint32 Id);

public:

	IRDescriptorManager(RenderContext& Context, LinearAllocator& InAllocator);
	~IRDescriptorManager();

	DELETE_COPY_AND_MOVE(IRDescriptorManager)

	void Initialize(const DescriptorManagerConfiguration& Config);

	Handle<UniformBuffer>	CreateUniformBuffer				(const UniformBufferCreateInfo& CreateInfo);
	Handle<UniformBuffer>	GetUniformBufferHandleWithId	(uint32 Id);
	UniformBuffer*			GetUniformBuffer				(Handle<UniformBuffer> Hnd);

	Handle<DescriptorPool>	CreateDescriptorPool			(const DescriptorPoolCreateInfo& CreateInfo);
	Handle<DescriptorPool>	GetDescriptorPoolHandleWithId	(uint32 Id);
	DescriptorPool*			GetDescriptorPool				(Handle<DescriptorPool> Hnd);
	//bool DestroyDescriptorPool(Handle<DescriptorPool> Hnd);

	Handle<DescriptorLayout> CreateDescriptorLayout			(const DescriptorLayoutCreateInfo& CreateInfo);
	Handle<DescriptorLayout> GetDescriptorLayoutHandleWithId(uint32 Id);
	DescriptorLayout*		 GetDescriptorLayout			(Handle<DescriptorLayout> Hnd);
	//bool DestroyDescriptorLayout(Handle<DescriptorLayout> Hnd);

	Handle<DescriptorSet>	CreateDescriptorSet				(const DescriptorSetCreateInfo& CreateInfo);
	Handle<DescriptorSet>	GetDescriptorSetHandleWithId	(uint32 Id);
	DescriptorSet*			GetDescriptorSet				(Handle<DescriptorSet> Hnd);
	//bool DestroyDescriptorSet(Handle<DescriptorSet> Hnd);

	//bool MapDescriptorSetToBuffer(Handle<DescriptorSet> DescriptorSetHandle, Handle<UniformBuffer> BufferHandle);

	void Build();
	void Destroy();
	void FlushDescriptorSetsOffsets();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H