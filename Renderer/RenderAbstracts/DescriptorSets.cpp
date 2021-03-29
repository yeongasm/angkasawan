#include "DescriptorSets.h"
#include "API/Context.h"

IRDescriptorManager::IRDescriptorManager(LinearAllocator& InAllocator) :
	Allocator(InAllocator),
	Sets{},
	Pools{},
	Layouts{},
	Buffers{},
	UpdateQueue{}
{}

IRDescriptorManager::~IRDescriptorManager()
{
	Buffers.Release();
	Sets.Release();
	Pools.Release();
	Layouts.Release();
	UpdateQueue.Release();
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

bool IRDescriptorManager::AddDescriptorSetLayoutBinding(const DescriptorLayoutBindingCreateInfo& CreateInfo)
{
	if (CreateInfo.LayoutHandle == INVALID_HANDLE) { return false; }
	DescriptorLayout* layout = GetDescriptorLayout(CreateInfo.LayoutHandle);
	if (!layout) { return false; }

	DescriptorBinding& binding = layout->Bindings.Insert(DescriptorBinding());
	binding.Binding = CreateInfo.Binding;
	binding.Size	= gpu::PadSizeToAlignedSize(CreateInfo.Size) * CreateInfo.Count;
	binding.Type	= CreateInfo.Type;
	binding.ShaderStages.Set(CreateInfo.ShaderStages);
	binding.DescriptorCount = CreateInfo.DescriptorCount;

	if (CreateInfo.BufferHnd != INVALID_HANDLE)
	{
		UniformBuffer* buffer = GetUniformBuffer(CreateInfo.BufferHnd);
		binding.Buffer = buffer;
	}

	return true;
}

bool IRDescriptorManager::QueueBufferForUpdate(Handle<DescriptorSet> SetHnd, uint32 Binding, Handle<UniformBuffer> BufferHnd)
{
	if (SetHnd == INVALID_HANDLE ||
		BufferHnd == INVALID_HANDLE)
	{
		return false;
	}

	DescriptorSet* set = GetDescriptorSet(SetHnd);
	UniformBuffer* buffer = GetUniformBuffer(BufferHnd);

	if (!set || !buffer) { return false; }

	UpdateQueue.Push({ set, Binding, &buffer, 1, nullptr, 0 });

	return true;
}

bool IRDescriptorManager::QueueTexturesForUpdate(Handle<DescriptorSet> SetHnd, uint32 Binding, Texture** Textures, uint32 Count)
{
	if (SetHnd == INVALID_HANDLE ||
		!Textures ||
		!Count)
	{
		return false;
	}

	DescriptorSet* set = GetDescriptorSet(SetHnd);

	if (!set) { return false; }

	UpdateQueue.Push({ set, Binding, nullptr, 0, Textures, Count });

	return true;
}

//DescriptorBinding* IRDescriptorManager::GetDescriptorBindingAt(Handle<DescriptorLayout> Hnd, uint32 Binding)
//{
//	DescriptorBinding* binding = nullptr;
//	DescriptorLayout* layout = GetDescriptorLayout(Hnd);
//	if (!layout) { return nullptr; }
//
//	for (DescriptorBinding& b : layout->Bindings)
//	{
//		if (b.Binding != Binding) { continue; }
//		binding = &b;
//		break;
//	}
//
//	return binding;
//}

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

	DescriptorSet* set = reinterpret_cast<DescriptorSet*>(Allocator.Malloc(sizeof(DescriptorSet)));
	FMemory::InitializeObject(set);
	FMemory::Memcpy(set, &CreateInfo, sizeof(DescriptorSetBase));

	set->Pool = descriptorPool;
	set->Layout = descriptorLayout;

	size_t index = Sets.Push(set);

	return Handle<DescriptorSet>(index);
}

Handle<DescriptorSet> IRDescriptorManager::GetDescriptorSetHandleWithId(uint32 Id)
{
	return Handle<DescriptorSet>(DoesDescriptorSetExist(Id));
}

bool IRDescriptorManager::UpdateDescriptorSetForBinding(DescriptorSet* Set, uint32 Binding, UniformBuffer** Buffer)
{
	if (!Set) { return false; }

	DescriptorBinding* binding = nullptr;
	for (DescriptorBinding& b : Set->Layout->Bindings)
	{
		if (b.Binding != Binding) { continue; }
		binding = &b;
		break;
	}

	if (!binding) { return false; }

	gpu::UpdateDescriptorSet(*Set, *binding, *binding->Buffer);

	return true;
}

bool IRDescriptorManager::UpdateDescriptorSetData(Handle<DescriptorSet> SetHnd, uint32 Binding, void* Data, size_t Size)
{
	if (SetHnd == INVALID_HANDLE) { return false; }
	if (!Data || !Size) { return false; }

	const uint32 currentFrame = gpu::CurrentFrameIndex();
	DescriptorBinding* binding = nullptr;
	DescriptorSet* set = GetDescriptorSet(SetHnd);

	if (!set) { return false; }

	for (DescriptorBinding& b : set->Layout->Bindings)
	{
		if (b.Binding != Binding) { continue; }
		binding = &b;
		break;
	}

	if (!binding) { return false; }
	if (binding->Allocated == binding->Size)
	{
		VKT_ASSERT(false && "Insufficient memory to allocate more descriptor set instances");
		return false;
	}

	size_t typeSize = gpu::PadSizeToAlignedSize(Size);
	gpu::CopyToBuffer(*binding->Buffer, Data, typeSize, binding->Offset[currentFrame] + binding->Allocated);
	binding->Allocated += typeSize;

	return true;
}

void IRDescriptorManager::BindDescriptorSet(Handle<DescriptorSet> Hnd)
{
	if (PreviousHandle == Hnd) { return; }
	DescriptorSet* set = GetDescriptorSet(Hnd);
	if (!set) { return; }
	gpu::BindDescriptorSet(*set);
	PreviousHandle = Hnd;
}

bool IRDescriptorManager::UpdateDescriptorSetImages(DescriptorSet* Set, uint32 Binding, Texture** Textures, size_t Count)
{
	if (!Set) { return false; }
	if (!Textures || !Count) { return false; }

	DescriptorBinding* binding = nullptr;

	for (DescriptorBinding& b : Set->Layout->Bindings)
	{
		if (b.Binding != Binding) { continue; }
		binding = &b;
		break;
	}

	if (!binding) { return false; }

	gpu::UpdateDescriptorSetImage(*Set, *binding, Textures, static_cast<uint32>(Count));

	return true;
}

//void IRDescriptorManager::BindDescriptorSets()
//{
//	for (DescriptorSet* set : Sets)
//	{
//		gpu::BindDescriptorSet(*set);
//	}
//}

DescriptorSet* IRDescriptorManager::GetDescriptorSet(Handle<DescriptorSet> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE);
	if (Hnd == INVALID_HANDLE)
	{
		return nullptr;
	}
	return Sets[Hnd];
}

void IRDescriptorManager::BuildAll()
{
	BuildUniformBuffers();
	BuildDescriptorPools();
	BuildDescriptorLayouts();
	BuildDescriptorSets();
}

void IRDescriptorManager::Update()
{
	for (auto& update : UpdateQueue)
	{
		if (update.pBuffers)
		{
			UpdateDescriptorSetForBinding(update.pSet, update.Binding, update.pBuffers);
		}
		
		if (update.pTextures)
		{
			UpdateDescriptorSetImages(update.pSet, update.Binding, update.pTextures, update.NumOfTextures);
		}
	}
	UpdateQueue.Empty();
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
	}
}

void IRDescriptorManager::BuildDescriptorPools()
{
	for (DescriptorPool* pool : Pools)
	{
		if (pool->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateDescriptorPool(*pool);
	}
}

void IRDescriptorManager::BuildDescriptorLayouts()
{
	for (DescriptorLayout* layout : Layouts)
	{
		if (layout->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateDescriptorSetLayout(*layout);
	}
}

void IRDescriptorManager::BuildDescriptorSets()
{
	for (DescriptorSet* set : Sets)
	{
		if (set->Handle != INVALID_HANDLE) { continue; }
		gpu::AllocateDescriptorSet(*set);
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
