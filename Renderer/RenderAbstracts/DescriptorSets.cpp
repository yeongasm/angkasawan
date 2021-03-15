#include "DescriptorSets.h"
#include "API/Context.h"

IRDescriptorManager::IRDescriptorManager(LinearAllocator& InAllocator) :
	Allocator(InAllocator),
	Sets{},
	Pools{},
	Layouts{},
	Buffers{}
{}

IRDescriptorManager::~IRDescriptorManager()
{
	Sets.Release();
	Pools.Release();
	Layouts.Release();
}

Handle<UniformBuffer> IRDescriptorManager::AllocateUniformBuffer(const UniformBufferCreateInfo& AllocInfo)
{
	if (DoesUniformBufferExist(AllocInfo.Id) != Resource_Not_Exist)
	{
		return INVALID_HANDLE;
	}

	UniformBuffer* buffer = reinterpret_cast<UniformBuffer*>(Allocator.Malloc(sizeof(UniformBuffer)));
	FMemory::InitializeObject(buffer);
	FMemory::Memcpy(buffer, &AllocInfo, sizeof(SRMemoryBufferBase));

	buffer->Locality = Buffer_Locality_Cpu_To_Gpu;
	size_t index = Buffers.Push(buffer);

	if (!AllocInfo.DeferAllocation)
	{
		BuildUniformBuffers();
	}

	return Handle<UniformBuffer>(index);
}

Handle<UniformBuffer> IRDescriptorManager::GetUniformBufferHandleWithId(uint32 Id)
{
	return Handle<UniformBuffer>(DoesUniformBufferExist(Id));
}

UniformBuffer* IRDescriptorManager::GetUniformBuffer(Handle<UniformBuffer> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE && "Invalid handle provided to function");
	if (Hnd == INVALID_HANDLE)
	{
		return nullptr;
	}
	return Buffers[Hnd];
}

Handle<DescriptorPool> IRDescriptorManager::CreateDescriptorPool(const DescriptorPoolCreateInfo& CreateInfo)
{
	if (DoesDescriptorPoolExist(CreateInfo.Id) != Resource_Not_Exist)
	{
		return INVALID_HANDLE;
	}

	DescriptorPool* pool = reinterpret_cast<DescriptorPool*>(Allocator.Malloc(sizeof(DescriptorPool)));
	FMemory::InitializeObject(pool);
	FMemory::Memcpy(pool, &CreateInfo, sizeof(DescriptorPoolBase));

	//pool->Slots = pool->Capacity;
	//pool->DescriptorTypes.Assign(CreateInfo.DescriptorTypes);

	size_t index = Pools.Push(pool);

	return Handle<DescriptorPool>(index);
}

Handle<DescriptorPool> IRDescriptorManager::GetDescriptorPoolHandleWithId(uint32 Id)
{
	return Handle<DescriptorPool>(DoesDescriptorPoolExist(Id));
}

bool IRDescriptorManager::AddSizeTypeToDescriptorPool(Handle<DescriptorPool> Hnd, EDescriptorType Type, uint32 Size)
{
	DescriptorPool* descriptorPool = GetDescriptorPool(Hnd);

	VKT_ASSERT(descriptorPool);
	if (!descriptorPool) { return false; }

	descriptorPool->Sizes.Push({ Type, Size });
	return true;
}

DescriptorPool* IRDescriptorManager::GetDescriptorPool(Handle<DescriptorPool> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE);
	if (Hnd == INVALID_HANDLE)
	{
		return nullptr;
	}
	return Pools[Hnd];
}

//bool IRDescriptorManager::DestroyDescriptorPool(Handle<DescriptorPool> Hnd)
//{
//	DescriptorPool* pool = GetDescriptorPool(Hnd);
//	VKT_ASSERT(pool, "Invalid Handle provided to function");
//	if (!pool)
//	{
//		return false;
//	}
//	// NOTE:
//	// We don't really need to free the descriptors sets before releasing the descriptor pool since 
//	// descriptor sets and destroyed if their pool is released.
//	// But because our descriptor set implementation hold buffers, we need to force all descriptor sets
//	// allocated from the pool to be released.
//	
//	// NOTE:
//	// Perhaps we can solve this by decoupling buffers from descriptor sets.
//	// Which should be done anyways because Vulkan benefits from using only a single buffer.
//	VKT_ASSERT(pool->Slots == pool->Capacity, "Descriptor sets needs to be returned to the pools in our implementation.");
//	if (pool->Slots != pool->Capacity)
//	{
//		return false;
//	}
//	Context.DestroyDescriptorPool(pool->Handle);
//	// NOTE:
//	// Not too sure if we should remove it from the container ...
//	// Are there any instances where you'll recreate pools in runtime???
//	Pools.PopAt(Hnd, false);
//}

Handle<DescriptorLayout> IRDescriptorManager::CreateDescriptorLayout(const DescriptorLayoutCreateInfo& CreateInfo)
{
	if (DoesDescriptorLayoutExist(CreateInfo.Id) != Resource_Not_Exist)
	{
		return INVALID_HANDLE;
	}

	DescriptorLayout* layout = reinterpret_cast<DescriptorLayout*>(Allocator.Malloc(sizeof(DescriptorLayout)));
	FMemory::InitializeObject(layout);
	FMemory::Memcpy(layout, &CreateInfo, sizeof(DescriptorLayoutCreateInfo));

	size_t index = Layouts.Push(layout);

	return Handle<DescriptorLayout>(index);
}

Handle<DescriptorLayout> IRDescriptorManager::GetDescriptorLayoutHandleWithId(uint32 Id)
{
	return Handle<DescriptorLayout>(DoesDescriptorLayoutExist(Id));
}

bool IRDescriptorManager::AddDescriptorSetLayoutBinding(Handle<DescriptorLayout> Hnd, uint32 Binding, size_t Size, size_t Count, EDescriptorType Type, EShaderTypeFlagBits ShaderStages)
{
	if (Hnd == INVALID_HANDLE) { return false; }
	DescriptorLayout* layout = GetDescriptorLayout(Hnd);
	if (!layout) { return false; }

	DescriptorBinding& binding = layout->Bindings.Insert(DescriptorBinding());
	binding.Binding = Binding;
	binding.Size	= gpu::PadSizeToAlignedSize(Size) * Count;
	binding.Type	= Type;
	binding.ShaderStages.Set(ShaderStages);

	return true;
}

DescriptorLayout* IRDescriptorManager::GetDescriptorLayout(Handle<DescriptorLayout> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE);
	if (Hnd == INVALID_HANDLE)
	{
		return nullptr;
	}
	return Layouts[Hnd];
}

//bool IRDescriptorManager::DestroyDescriptorLayout(Handle<DescriptorLayout> Hnd)
//{
//	DescriptorLayout* layout = GetDescriptorLayout(Hnd);
//	VKT_ASSERT(layout, "Invalid handle provided to function");
//	if (!layout)
//	{
//		return false;
//	}
//	Context.DestroyDescriptorSetLayout(layout->Handle);
//	return true;
//}

Handle<DescriptorSet> IRDescriptorManager::CreateDescriptorSet(const DescriptorSetCreateInfo& CreateInfo)
{
	DescriptorPool* descriptorPool = GetDescriptorPool(CreateInfo.DescriptorPoolHandle);
	DescriptorLayout* descriptorLayout = GetDescriptorLayout(CreateInfo.DescriptorLayoutHandle);

	if (!descriptorPool			||
		!descriptorLayout		||
		(DoesDescriptorSetExist(CreateInfo.Id) != Resource_Not_Exist))
	{
		return INVALID_HANDLE;
	}

	// Problem here ... think about it later
	//if (!descriptorPool->DescriptorTypes.Has(CreateInfo.Type))
	//{
	//	return INVALID_HANDLE;
	//}

	//size_t remainingSize = uniformBuffer->Size - uniformBuffer->Offset;

	//if (remainingSize < (CreateInfo.NumOfData * CreateInfo.TypeSize))
	//{
	//	return INVALID_HANDLE;
	//}

	DescriptorSet* set = reinterpret_cast<DescriptorSet*>(Allocator.Malloc(sizeof(DescriptorSet)));
	FMemory::InitializeObject(set);
	FMemory::Memcpy(set, &CreateInfo, sizeof(DescriptorSetBase));

	set->Pool = descriptorPool;
	set->Layout = descriptorLayout;
	//set->Buffer = uniformBuffer;
	//set->Base = uniformBuffer->Offset;

	// Padded alongside the maximum number of frames in flights.
	//size_t paddedSize = gpu::PadSizeToAlignedSize(set->NumOfData * set->TypeSize * MAX_FRAMES_IN_FLIGHT);
	//uniformBuffer->Offset += paddedSize;

	size_t index = Sets.Push(set);

	return Handle<DescriptorSet>(index);
}

Handle<DescriptorSet> IRDescriptorManager::GetDescriptorSetHandleWithId(uint32 Id)
{
	return Handle<DescriptorSet>(DoesDescriptorSetExist(Id));
}

bool IRDescriptorManager::UpdateDescriptorSetForBinding(Handle<DescriptorSet> SetHnd, Handle<UniformBuffer> BufferHnd, uint32 Binding)
{
	if (SetHnd == INVALID_HANDLE || BufferHnd == INVALID_HANDLE)
	{
		return false;
	}

	DescriptorSet* set = GetDescriptorSet(SetHnd);
	UniformBuffer* buffer = GetUniformBuffer(BufferHnd);

	if (!set || !buffer)
	{
		return false;
	}

	DescriptorBinding* binding = nullptr;
	for (DescriptorBinding& b : set->Layout->Bindings)
	{
		if (b.Binding != Binding) { continue; }
		binding = &b;
		break;
	}

	if (!binding) { return false; }

	gpu::UpdateDescriptorSet(*set, *binding, *buffer);
	binding->Buffer = buffer;

	return true;
}

DescriptorSet* IRDescriptorManager::GetDescriptorSet(Handle<DescriptorSet> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE);
	if (Hnd == INVALID_HANDLE)
	{
		return nullptr;
	}
	return Sets[Hnd];
}

//bool IRDescriptorManager::DestroyDescriptorSet(Handle<DescriptorSet> Hnd)
//{
//	DescriptorSet* set = GetDescriptorSet(Hnd);
//	VKT_ASSERT(set, "Invalid handle provided to function");
//	if (!set)
//	{
//		return false;
//	}
//	Sets.PopAt(Hnd, false);
//	return true;
//}

//bool IRDescriptorManager::MapDescriptorSetToBuffer(Handle<DescriptorSet> DescriptorSetHandle, Handle<UniformBuffer> BufferHandle)
//{
//	DescriptorSet* descriptorSet = GetDescriptorSet(DescriptorSetHandle);
//	UniformBuffer* uniformBuffer = GetUniformBuffer(BufferHandle);
//
//	if (!descriptorSet || !uniformBuffer)
//	{
//		return false;
//	}
//
//	uint32 remainingSize = uniformBuffer->Capacity - uniformBuffer->Offset;
//
//	if (remainingSize < (descriptorSet->NumOfData * descriptorSet->TypeSize))
//	{
//		return false;
//	}
//
//	descriptorSet->Base = uniformBuffer->Offset;
//	uint32 paddedSize = Context.MapDescriptorSetToBuffer(*descriptorSet, *uniformBuffer);
//	uniformBuffer->Offset += paddedSize;
//
//	return true;
//}

void IRDescriptorManager::BuildAll()
{
	BuildUniformBuffers();
	BuildDescriptorPools();
	BuildDescriptorLayouts();
	BuildDescriptorSets();

	//for (DescriptorSet* descriptorSet : Sets)
	//{
	//	Context.MapDescriptorSetToBuffer(*descriptorSet);
	//}
}

void IRDescriptorManager::Destroy()
{
	for (DescriptorLayout* layout : Layouts)
	{
		gpu::DestroyDescriptorSetLayout(*layout);
	}

	for (DescriptorPool* pool : Pools)
	{
		gpu::DestroyDescriptorPool(*pool);
	}

	for (UniformBuffer* buffer : Buffers)
	{
		gpu::DestroyBuffer(*buffer);
	}

	Sets.Release();
	Pools.Release();
	Layouts.Release();
	Buffers.Release();
}

void IRDescriptorManager::FlushDescriptorSetsOffsets()
{
	for (DescriptorSet* set : Sets)
	{
		for (DescriptorBinding& binding : set->Layout->Bindings)
		{
			binding.Allocated = 0;
		}
	}
}

void IRDescriptorManager::BuildUniformBuffers()
{
	for (UniformBuffer* buffer : Buffers)
	{
		if (buffer->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateBuffer(*buffer, nullptr, buffer->Size);
		//Context.NewBuffer(*buffer, nullptr, buffer->Size, 1);
	}
}

void IRDescriptorManager::BuildDescriptorPools()
{
	for (DescriptorPool* pool : Pools)
	{
		if (pool->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateDescriptorPool(*pool);
		//Context.NewDescriptorPool(*pool);
	}
}

void IRDescriptorManager::BuildDescriptorLayouts()
{
	for (DescriptorLayout* layout : Layouts)
	{
		if (layout->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateDescriptorSetLayout(*layout);
		//Context.NewDestriptorSetLayout(*layout);
	}
}

void IRDescriptorManager::BuildDescriptorSets()
{
	for (DescriptorSet* set : Sets)
	{
		if (set->Handle != INVALID_HANDLE) { continue; }
		gpu::AllocateDescriptorSet(*set);
		//Context.NewDescriptorSet(*set);
	}
}

size_t IRDescriptorManager::DoesUniformBufferExist(size_t Id)
{
	UniformBuffer* buffer = nullptr;
	for (size_t i = 0; i < Buffers.Length(); i++)
	{
		buffer = Buffers[i];
		if (buffer->Id == Id) { return i; }
	}
	return Resource_Not_Exist;
}

size_t IRDescriptorManager::DoesDescriptorPoolExist(size_t Id)
{
	DescriptorPool* pool = nullptr;
	for (size_t i = 0; i < Pools.Length(); i++)
	{
		pool = Pools[i];
		if (pool->Id == Id) { return i; }
	}
	return Resource_Not_Exist;
}

size_t IRDescriptorManager::DoesDescriptorLayoutExist(size_t Id)
{
	DescriptorLayout* layout = nullptr;
	for (size_t i = 0; i < Layouts.Length(); i++)
	{
		layout = Layouts[i];
		if (layout->Id == Id) { return i; }
	}
	return Resource_Not_Exist;
}

size_t IRDescriptorManager::DoesDescriptorSetExist(size_t Id)
{
	DescriptorSet* set = nullptr;
	for (size_t i = 0; i < Sets.Length(); i++)
	{
		set = Sets[i];
		if (set->Id == Id) { return i; }
	}
	return Resource_Not_Exist;
}
