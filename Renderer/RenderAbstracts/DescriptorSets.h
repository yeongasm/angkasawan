#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H

#include "Library/Containers/Map.h"
#include "Library/Containers/Pair.h"
#include "Library/Allocators/LinearAllocator.h"
#include "RenderPlatform/API.h"
#include "API/Definitions.h"
#include "Primitives.h"

class RenderSystem;
struct SRDeviceStore;

/**
* NOTE(Ygsm):
* Uniform buffers are for known amounts of data sets that will be read in the shader. (static array equivalent).
* Storage buffers are for unknown amounts of data sets that will be read and written to in the shader. (vector equivalent).
*/

//struct UniformBufferCreateInfo : public SRMemoryBufferBase
//{
//	bool DeferAllocation;
//};

using UniformBuffer = SRMemoryBuffer;

//struct DescriptorPoolBase
//{
//	uint32 Id;
//};

struct DescriptorPoolCreateInfo
{
	String128 Name;
};

//struct DescriptorPoolCreateInfo : public DescriptorPoolBase
//{
//	EDescriptorType Type;
//	uint32 NumOfType;
//};

/**
* A single descriptor pool can have multiple types of descriptor pool sizes.
* maxSets would be the total number of descriptor types from each pool sizes.
*/
struct DescriptorPool /*: public DescriptorPoolBase*/
{
	struct Size
	{
		EDescriptorType Type;	// Descriptor type
		uint32 Count;			// Descriptor count
	};

	StaticArray<Size, MAX_DESCRIPTOR_POOL_TYPE_SIZE> Sizes;
	//Array<Pair<EDescriptorType, uint32>> Sizes;
	Handle<HSetPool> Handle;
};

struct DescriptorBinding
{
	uint32 Binding;
	uint32 DescriptorCount;
	size_t Size;
	size_t Allocated;
	size_t Offset[MAX_FRAMES_IN_FLIGHT];
	EDescriptorType Type;
	BitSet<uint32> ShaderStages;
	SRMemoryBuffer* Buffer;
};

struct DescriptorLayoutCreateInfo
{
	String128 Name;
};

/**
* A single descriptor set layout can have multiple bindings.
*/
struct DescriptorLayout /*: public DescriptorLayoutCreateInfo*/
{
	using BindingsContainer = StaticArray<DescriptorBinding, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;
	Handle<HPipeline>* Pipeline;
	Handle<HSetLayout> Handle;
	BindingsContainer Bindings;
	//Array<DescriptorBinding> Bindings;
};

struct DescriptorSetBase
{
	//uint32 Id;
	uint32 Slot;
};

struct DescriptorSetCreateInfo : public DescriptorSetBase
{
	String128 Name;
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

struct DescriptorLayoutBindingCreateInfo
{
	size_t Size;
	size_t Count;
	uint32 DescriptorCount;
	uint32 Binding;
	Handle<DescriptorLayout> LayoutHandle;
	EDescriptorType Type;
	uint32 ShaderStages;
	Handle<UniformBuffer> BufferHnd;
};

/**
* Manages descriptor pools, layouts and sets.
* Also caches them instead of allocating new descriptor sets every frame.
*/
class RENDERER_API IRDescriptorManager
{
private:

	friend class RenderSystem;
	friend class IRPipelineManager;

	//enum EExistEnum : size_t { Resource_Not_Exist = -1 };

	struct DescriptorUpdateParam
	{
		DescriptorSet* pSet;
		uint32 Binding;
		UniformBuffer** pBuffers;
		uint32 NumOfBuffers;
		Texture** pTextures;
		uint32 NumOfTextures;
	};

	LinearAllocator& Allocator;
	SRDeviceStore& Device;
	//Array<UniformBuffer*> Buffers;
	//Array<DescriptorSet*>				Sets;
	//Array<DescriptorPool*>				Pools;
	//Array<DescriptorLayout*>			Layouts;
	Array<DescriptorUpdateParam> UpdateQueue;

	//void	BuildUniformBuffers			();
	void	BuildDescriptorPools		();
	void	BuildDescriptorLayouts		();
	void	BuildDescriptorSets			();
	//size_t	DoesUniformBufferExist		(const String128& Name);
	size_t	DoesDescriptorPoolExist		(const String128& Name);
	size_t	DoesDescriptorLayoutExist	(const String128& Name);
	size_t	DoesDescriptorSetExist		(const String128& Name);

	//UniformBuffer*		GetUniformBuffer	(Handle<UniformBuffer> Hnd);
	DescriptorPool*		GetDescriptorPool	(Handle<DescriptorPool> Hnd);
	DescriptorLayout*	GetDescriptorLayout	(Handle<DescriptorLayout> Hnd);
	DescriptorSet*		GetDescriptorSet	(Handle<DescriptorSet> Hnd);

	bool UpdateDescriptorSetForBinding(DescriptorSet* Set, uint32 Binding, UniformBuffer** Buffer);
	bool UpdateDescriptorSetImages(DescriptorSet* Set, uint32 Binding, Texture** Textures, size_t Count);

public:

	Handle<DescriptorSet> PreviousHandle = INVALID_HANDLE;

	IRDescriptorManager(LinearAllocator& InAllocator, SRDeviceStore& InDeviceStore);
	~IRDescriptorManager();

	DELETE_COPY_AND_MOVE(IRDescriptorManager)

	//Handle<UniformBuffer>	AllocateUniformBuffer			(const MemoryAllocateInfo& AllocInfo);
	//Handle<UniformBuffer>	GetUniformBufferHandleWithName	(const String128& Name);

	Handle<DescriptorPool>	CreateDescriptorPool			(const DescriptorPoolCreateInfo& CreateInfo);
	Handle<DescriptorPool>	GetDescriptorPoolHandleWithName	(const String128& Name);
	bool					AddSizeTypeToDescriptorPool		(Handle<DescriptorPool> Hnd, EDescriptorType Type, uint32 DescriptorCount);
	//bool DestroyDescriptorPool(Handle<DescriptorPool> Hnd);

	Handle<DescriptorLayout> CreateDescriptorLayout			(const DescriptorLayoutCreateInfo& CreateInfo);
	Handle<DescriptorLayout> GetDescriptorLayoutHandleWithName(const String128& Name);
	bool					 AddDescriptorSetLayoutBinding	(const DescriptorLayoutBindingCreateInfo& CreateInfo);
	
	bool QueueBufferForUpdate	(Handle<DescriptorSet> SetHnd, uint32 Binding, Handle<UniformBuffer> BufferHnd);
	bool QueueTexturesForUpdate	(Handle<DescriptorSet> SetHnd, uint32 Binding, Texture** Textures, uint32 Count);

	Handle<DescriptorSet>	CreateDescriptorSet				(const DescriptorSetCreateInfo& CreateInfo);
	Handle<DescriptorSet>	GetDescriptorSetHandleWithName	(const String128& Name);
	bool					UpdateDescriptorSetData			(Handle<DescriptorSet> SetHnd, uint32 Binding, void* Data, size_t Size);

	void BindDescriptorSet(Handle<DescriptorSet> Hnd);

	void BuildAll();
	void Update();
	void Destroy();
	void FlushDescriptorSetsOffsets();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_DESCRIPTOR_SETS_H