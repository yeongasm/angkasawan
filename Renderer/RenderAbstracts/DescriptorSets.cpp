#include "DescriptorSets.h"
#include "API/Context.h"

IRDescriptorManager::IRDescriptorManager(LinearAllocator& InAllocator, SRDeviceStore& InDeviceStore) :
	Allocator(InAllocator),
	Device(InDeviceStore),
	//Sets{},
	//Pools{},
	//Layouts{},
	//Buffers{},
	UpdateQueue{}
{}

IRDescriptorManager::~IRDescriptorManager()
{
	//Buffers.Release();
	//Sets.Release();
	//Pools.Release();
	//Layouts.Release();
	UpdateQueue.Release();
}

//Handle<UniformBuffer> IRDescriptorManager::AllocateUniformBuffer(const MemoryAllocateInfo& AllocInfo)
//{
//	if (DoesUniformBufferExist(AllocInfo.Name) != INVALID_HANDLE)
//	{
//		return INVALID_HANDLE;
//	}
//
//	size_t id = Device.GenerateId(AllocInfo.Name);
//	SRMemoryBuffer* buffer = Device.NewBuffer(id);
//
//	VKT_ASSERT(buffer);
//	if (!buffer) { return INVALID_HANDLE; }
//
//	*buffer = {};
//	FMemory::Memcpy(buffer, &AllocInfo, sizeof(SRMemoryBufferBase));
//
//	/**
//	* CHANGELOG:
//	* We no longer defer allocations and resources are created right after it gets called.
//	*/
//
//	buffer->Locality = Buffer_Locality_Cpu_To_Gpu;
//	gpu::CreateBuffer(*buffer, nullptr, buffer->Size);
//
//	return Handle<UniformBuffer>(id);
//}

//Handle<UniformBuffer> IRDescriptorManager::GetUniformBufferHandleWithName(const String128& Name)
//{
//	return Handle<UniformBuffer>(DoesUniformBufferExist(Name));
//}

//UniformBuffer* IRDescriptorManager::GetUniformBuffer(Handle<UniformBuffer> Hnd)
//{
//	VKT_ASSERT(Hnd != INVALID_HANDLE && "Invalid handle provided to function");
//	if (Hnd == INVALID_HANDLE)
//	{
//		return nullptr;
//	}
//	return Device.GetBuffer(Hnd);
//}

Handle<DescriptorPool> IRDescriptorManager::CreateDescriptorPool(const DescriptorPoolCreateInfo& CreateInfo)
{
	if (DoesDescriptorPoolExist(CreateInfo.Name) != INVALID_HANDLE)
	{
		return INVALID_HANDLE;
	}

	size_t id = Device.GenerateId(CreateInfo.Name);
	DescriptorPool* pool = Device.NewDescriptorPool(id);

	VKT_ASSERT(pool);
	if (!pool) { return INVALID_HANDLE; }
	*pool = {};

	return Handle<DescriptorPool>(id);
}

Handle<DescriptorPool> IRDescriptorManager::GetDescriptorPoolHandleWithName(const String128& Name)
{
	return Handle<DescriptorPool>(DoesDescriptorPoolExist(Name));
}

bool IRDescriptorManager::AddSizeTypeToDescriptorPool(Handle<DescriptorPool> Hnd, EDescriptorType Type, uint32 DescriptorCount)
{
	DescriptorPool* descriptorPool = GetDescriptorPool(Hnd);

	VKT_ASSERT(descriptorPool);
	if (!descriptorPool) { return false; }
	descriptorPool->Sizes.Push({ Type, DescriptorCount });

	return true;
}

DescriptorPool* IRDescriptorManager::GetDescriptorPool(Handle<DescriptorPool> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE);
	if (Hnd == INVALID_HANDLE)
	{
		return nullptr;
	}
	return Device.GetDescriptorPool(Hnd);
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
	if (DoesDescriptorLayoutExist(CreateInfo.Name) != INVALID_HANDLE)
	{
		return INVALID_HANDLE;
	}

	size_t id = Device.GenerateId(CreateInfo.Name);
	DescriptorLayout* layout = Device.NewDescriptorSetLayout(id);
	*layout = {};

	return Handle<DescriptorLayout>(id);
}

Handle<DescriptorLayout> IRDescriptorManager::GetDescriptorLayoutHandleWithName(const String128& Name)
{
	return Handle<DescriptorLayout>(DoesDescriptorLayoutExist(Name));
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
		UniformBuffer* buffer = Device.GetBuffer(CreateInfo.BufferHnd);
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
	UniformBuffer* buffer = Device.GetBuffer(BufferHnd);

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
	return Device.GetDescriptorSetLayout(Hnd);
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
		(DoesDescriptorSetExist(CreateInfo.Name) != INVALID_HANDLE))
	{
		return INVALID_HANDLE;
	}

	size_t id = Device.GenerateId(CreateInfo.Name);
	DescriptorSet* set = Device.NewDescriptorSet(id);

	*set = {};
	FMemory::Memcpy(set, &CreateInfo, sizeof(DescriptorSetBase));

	set->Pool = descriptorPool;
	set->Layout = descriptorLayout;

	return Handle<DescriptorSet>(id);
}

Handle<DescriptorSet> IRDescriptorManager::GetDescriptorSetHandleWithName(const String128& Name)
{
	return Handle<DescriptorSet>(DoesDescriptorSetExist(Name));
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
	return Device.GetDescriptorSet(Hnd);
}

void IRDescriptorManager::BuildAll()
{
	//BuildUniformBuffers();
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
	for (auto& [id, layout] : Device.DescriptorSetLayout)
	{
		gpu::DestroyDescriptorSetLayout(*layout);
	}

	for (auto& [id, pool] : Device.DescriptorPools)
	{
		gpu::DestroyDescriptorPool(*pool);
	}

	Device.DescriptorSets.Release();
	Device.DescriptorSetLayout.Release();
	Device.DescriptorPools.Release();
}

void IRDescriptorManager::FlushDescriptorSetsOffsets()
{
	for (auto& [id, set] : Device.DescriptorSets)
	{
		for (DescriptorBinding& binding : set->Layout->Bindings)
		{
			binding.Allocated = 0;
		}
	}
}

//void IRDescriptorManager::BuildUniformBuffers()
//{
//	for (UniformBuffer* buffer : Buffers)
//	{
//		if (buffer->Handle != INVALID_HANDLE) { continue; }
//		gpu::CreateBuffer(*buffer, nullptr, buffer->Size);
//	}
//}

void IRDescriptorManager::BuildDescriptorPools()
{
	for (auto& [id, pool] : Device.DescriptorPools)
	{
		if (pool->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateDescriptorPool(*pool);
	}
}

void IRDescriptorManager::BuildDescriptorLayouts()
{
	for (auto& [id, layout] : Device.DescriptorSetLayout)
	{
		if (layout->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateDescriptorSetLayout(*layout);
	}
}

void IRDescriptorManager::BuildDescriptorSets()
{
	for (auto& [id, set] : Device.DescriptorSets)
	{
		if (set->Handle != INVALID_HANDLE) { continue; }
		gpu::AllocateDescriptorSet(*set);
	}
}

//size_t IRDescriptorManager::DoesUniformBufferExist(const String128& Name)
//{
//	size_t id = Device.GenerateId(Name);
//	return Device.GetBuffer(id) ? id : INVALID_HANDLE;
//}

size_t IRDescriptorManager::DoesDescriptorPoolExist(const String128& Name)
{
	size_t id = Device.GenerateId(Name);
	return Device.GetDescriptorPool(id) ? id : INVALID_HANDLE;
}

size_t IRDescriptorManager::DoesDescriptorLayoutExist(const String128& Name)
{
	size_t id = Device.GenerateId(Name);
	return Device.GetDescriptorSetLayout(id) ? id : INVALID_HANDLE;
}

size_t IRDescriptorManager::DoesDescriptorSetExist(const String128& Name)
{
	size_t id = Device.GenerateId(Name);
	return Device.GetDescriptorSet(id) ? id : INVALID_HANDLE;
}
