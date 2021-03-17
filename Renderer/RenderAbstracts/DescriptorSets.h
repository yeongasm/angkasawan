#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H

#include "Library/Containers/Bitset.h"
#include "Library/Containers/Pair.h"
#include "Library/Containers/Array.h"
#include "Library/Allocators/LinearAllocator.h"
#include "Assets/GPUHandles.h"
#include "RenderPlatform/API.h"
#include "API/Definitions.h"
#include "API/RendererFlagBits.h"
#include "Primitives.h"

class RenderSystem;

/**
* NOTE(Ygsm):
* Uniform buffers are for known amounts of data sets that will be read in the shader. (static array equivalent).
* Storage buffers are for unknown amounts of data sets that will be read and written to in the shader. (vector equivalent).
*/

struct UniformBufferCreateInfo : public SRMemoryBufferBase
{
	bool DeferAllocation;
};

using UniformBuffer = SRMemoryBuffer;

struct DescriptorPoolBase
{
	uint32 Id;
};

using DescriptorPoolCreateInfo = DescriptorPoolBase;

//struct DescriptorPoolCreateInfo : public DescriptorPoolBase
//{
//	EDescriptorType Type;
//	uint32 NumOfType;
//};

/**
* A single descriptor pool can have multiple types of descriptor pool sizes.
* maxSets would be the total number of descriptor types from each pool sizes.
*/
struct DescriptorPool : public DescriptorPoolBase
{
	Array<Pair<EDescriptorType, uint32>> Sizes;
	Handle<HSetPool> Handle;
};

using EShaderTypeFlagBits = uint32;

struct DescriptorBinding
{
	uint32 Binding;
	size_t Size;
	size_t Allocated;
	size_t Offset[MAX_FRAMES_IN_FLIGHT];
	EDescriptorType Type;
	BitSet<uint32> ShaderStages;
	SRMemoryBuffer* Buffer;
};

struct DescriptorLayoutCreateInfo
{
	uint32 Id;
};

/**
* A single descriptor set layout can have multiple bindings.
*/
struct DescriptorLayout : public DescriptorLayoutCreateInfo
{
	Handle<HPipeline>* Pipeline;
	Handle<HSetLayout> Handle;
	Array<DescriptorBinding> Bindings;
};

struct DescriptorSetBase
{
	uint32 Id;
	uint32 Slot;
};

struct DescriptorSetCreateInfo : public DescriptorSetBase
{
	Handle<DescriptorPool> DescriptorPoolHandle;
	Handle<DescriptorLayout> DescriptorLayoutHandle;
};

/**
* Generally, each descriptor set can only occupy a slot in the driver (set = ??? in the shader)
* If 2 descriptor sets referencing different layouts (and each layout have different binding values) are bounded to 
* the same slot, one will overwrite the other.
* 
* A descriptor set can only references one layout.
*/
struct DescriptorSet : public DescriptorSetBase
{
	//size_t Offset;			// Current memory offset in the descriptor set.
	DescriptorPool* Pool;
	DescriptorLayout* Layout;
	Handle<HSet> Handle;
};

struct DescriptorSetInstance
{
	DescriptorSet* Owner;
	size_t Offset;
	uint32 Binding;
};

/**
* NOTE(Ygsm):
* Buffer is irrelevant until we actually update the descriptor set!!! <-------- IMPORTANT!!!!
*/

/**
* Manages descriptor pools, layouts and sets.
* Also caches them instead of allocating new descriptor sets every frame.
*/
class RENDERER_API IRDescriptorManager
{
private:

	friend class RenderSystem;

	enum EExistEnum : size_t { Resource_Not_Exist = -1 };

	LinearAllocator& Allocator;
	Array<UniformBuffer*> Buffers;
	Array<DescriptorSet*> Sets;
	Array<DescriptorPool*> Pools;
	Array<DescriptorLayout*> Layouts;

	void	BuildUniformBuffers			();
	void	BuildDescriptorPools		();
	void	BuildDescriptorLayouts		();
	void	BuildDescriptorSets			();
	size_t	DoesUniformBufferExist		(size_t Id);
	size_t	DoesDescriptorPoolExist		(size_t Id);
	size_t	DoesDescriptorLayoutExist	(size_t Id);
	size_t	DoesDescriptorSetExist		(size_t Id);

	UniformBuffer*		GetUniformBuffer	(Handle<UniformBuffer> Hnd);
	DescriptorPool*		GetDescriptorPool	(Handle<DescriptorPool> Hnd);
	DescriptorLayout*	GetDescriptorLayout	(Handle<DescriptorLayout> Hnd);
	DescriptorSet*		GetDescriptorSet	(Handle<DescriptorSet> Hnd);

public:

	IRDescriptorManager(LinearAllocator& InAllocator);
	~IRDescriptorManager();

	DELETE_COPY_AND_MOVE(IRDescriptorManager)

	//void Initialize(const DescriptorManagerConfiguration& Config);

	Handle<UniformBuffer>	AllocateUniformBuffer			(const UniformBufferCreateInfo& AllocInfo);
	Handle<UniformBuffer>	GetUniformBufferHandleWithId	(uint32 Id);

	Handle<DescriptorPool>	CreateDescriptorPool			(const DescriptorPoolCreateInfo& CreateInfo);
	Handle<DescriptorPool>	GetDescriptorPoolHandleWithId	(uint32 Id);
	bool					AddSizeTypeToDescriptorPool		(Handle<DescriptorPool> Hnd, EDescriptorType Type, uint32 Size);
	//bool DestroyDescriptorPool(Handle<DescriptorPool> Hnd);

	Handle<DescriptorLayout> CreateDescriptorLayout			(const DescriptorLayoutCreateInfo& CreateInfo);
	Handle<DescriptorLayout> GetDescriptorLayoutHandleWithId(uint32 Id);
	bool					 AddDescriptorSetLayoutBinding	(Handle<DescriptorLayout> Hnd, uint32 Binding, size_t Size, size_t Count, EDescriptorType Type, EShaderTypeFlagBits ShaderStages);
	//bool DestroyDescriptorLayout(Handle<DescriptorLayout> Hnd);

	Handle<DescriptorSet>	CreateDescriptorSet				(const DescriptorSetCreateInfo& CreateInfo);
	Handle<DescriptorSet>	GetDescriptorSetHandleWithId	(uint32 Id);
	bool					UpdateDescriptorSetForBinding	(Handle<DescriptorSet> SetHnd, Handle<UniformBuffer> BufferHnd, uint32 Binding);

	bool					UpdateDescriptorSetData			(Handle<DescriptorSet> SetHnd, uint32 Binding, void* Data, size_t Size);
	void					BindDescriptorSets();

	void BuildAll();
	void Destroy();
	void FlushDescriptorSetsOffsets();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H