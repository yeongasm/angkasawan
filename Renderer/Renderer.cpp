#include "Renderer.h"
#include "Library/Containers/Node.h"
#include "Library/Random/Xoroshiro.h"
#include "RenderAbstracts/StagingManager.h"

Handle<ISystem> g_RenderSystemHandle;
LinearAllocator g_RenderSystemAllocator;
Xoroshiro64 g_Uid(OS::GetPerfCounter());

IRenderSystem::IRenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd) :
	Engine(InEngine),
	Device(nullptr),
	Store(nullptr),
	Hnd(Hnd)
{}

IRenderSystem::~IRenderSystem() {}

/**
* NOTE(Ygsm):
* Objects marked with !!! should have their configurations stored in a config file.
*/

void IRenderSystem::OnInit()
{
	g_RenderSystemHandle = Hnd;

	/* !!! */
	g_RenderSystemAllocator.Initialize(KILOBYTES(64));
	
	/* !!! */
	DescriptorUpdate::_Buffers.Reserve(16);
	/* !!! */
	DescriptorUpdate::_Images.Reserve(16);

	Store = IAllocator::New<IDeviceStore>(g_RenderSystemAllocator, g_RenderSystemAllocator);

	Device = IAllocator::New<IRenderDevice>(g_RenderSystemAllocator);
	Device->Initialize(this->Engine);

	/* !!! */
	// Vertex buffer ...
	BufferAllocateInfo allocInfo = {};
	allocInfo.Locality = Buffer_Locality_Gpu;
	allocInfo.Size = MEGABYTES(256);
	allocInfo.Type.Assign((1 << Buffer_Type_Vertex) || (1 << Buffer_Type_Transfer_Dst));

	Handle<SMemoryBuffer> hnd = AllocateNewBuffer(allocInfo);
	VertexBuffer = DefaultBuffer(hnd, Store->GetBuffer(hnd));
	BuildBuffer(hnd);

	// Index buffer ...
	allocInfo.Type.Assign((1 << Buffer_Type_Index) || (1 << Buffer_Type_Transfer_Dst));
	hnd = AllocateNewBuffer(allocInfo);
	IndexBuffer = DefaultBuffer(hnd, Store->GetBuffer(hnd));
	BuildBuffer(hnd);


	// Instance buffer ...
	allocInfo.Type.Assign((1 << Buffer_Type_Vertex) || (1 << Buffer_Type_Transfer_Dst));
	allocInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
	allocInfo.Size = MEGABYTES(64);

	hnd = AllocateNewBuffer(allocInfo);
	InstanceBuffer = DefaultBuffer(hnd, Store->GetBuffer(hnd));
	BuildBuffer(hnd);
}

void IRenderSystem::OnUpdate()
{
	//RenderPass* renderPass = nullptr;
	//IRDrawManager::DrawCommands* drawCommands = nullptr;

	//if (Engine.Window.WindowSizeChanged)
	//{
	//	gpu::OnWindowResize();
	//	FrameGraph->OnWindowResize();
	//	PipelineManager->OnWindowResize();
	//}

	//gpu::Clear();
	//gpu::BeginFrame();
	//DrawCmdManager->BindBuffers();

	//for (auto& pair : FrameGraph->RenderPasses)
	//{
	//	renderPass = pair.Value;
	//	drawCommands = &DrawCmdManager->FinalDrawCommands[pair.Key];

	//	gpu::BindRenderpass(*renderPass);

	//	for (DrawCommand& command : *drawCommands)
	//	{
	//		PipelineManager->BindPipeline(command.PipelineHandle);
	//		DescriptorManager->BindDescriptorSet(command.DescriptorSetHandle);
	//		PushConstantManager->BindPushConstant(command.PushConstantHandle);

	//		gpu::Draw(command);
	//	}

	//	gpu::UnbindRenderpass(*renderPass);
	//}

	//gpu::BlitToDefault(*FrameGraph);
	//gpu::EndFrame();
	//gpu::SwapBuffers();

	//FlushRenderer();
}

void IRenderSystem::OnTerminate()
{
	// Destroy default buffers ...
	DestroyBuffer(VertexBuffer.Key);
	DestroyBuffer(IndexBuffer.Key);
	DestroyBuffer(InstanceBuffer.Key);

	g_RenderSystemAllocator.Terminate();
	Device->Terminate();
}

Handle<SDescriptorPool> IRenderSystem::CreateDescriptorPool()
{
	size_t id = g_Uid();
	SDescriptorPool* pPool = Store->NewDescriptorPool(id);
	if (!pPool) { return INVALID_HANDLE; }
	return id;
}

bool IRenderSystem::DescriptorPoolAddSizeType(Handle<SDescriptorPool> Hnd, SDescriptorPool::Size Type)
{
	SDescriptorPool* pPool = Store->GetDescriptorPool(Hnd);
	if (pPool) 
	{ 
		VKT_ASSERT(false && "Pool with handle does not exist.");
		return false; 
	}
	VKT_ASSERT((pPool->Sizes.Length() != MAX_DESCRIPTOR_POOL_TYPE_SIZE) && "Maximum pool type size reached.");
	pPool->Sizes.Push(Type);
	return true;
}

bool IRenderSystem::BuildDescriptorPool(Handle<SDescriptorPool> Hnd)
{
	using PoolSizeContainer = StaticArray<VkDescriptorPoolSize, MAX_DESCRIPTOR_POOL_TYPE_SIZE>;

	uint32 maxSets = 0;
	SDescriptorPool* pPool = Store->GetDescriptorPool(Hnd);
	if (!pPool)
	{
		VKT_ASSERT(false && "Pool with handle does not exist.");
		return false;
	}

	PoolSizeContainer poolSizes;

	for (SDescriptorPool::Size& size : pPool->Sizes)
	{
		poolSizes.Push({ 
			Device->GetDescriptorType(size.Type), 
			size.Count 
		});
		maxSets += size.Count;
	}

	VkDescriptorPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.maxSets = maxSets;
	info.pPoolSizes = poolSizes.First();
	info.poolSizeCount = static_cast<uint32>(poolSizes.Length());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkCreateDescriptorPool(Device->GetDevice(), &info, nullptr, &pPool->Hnd[i]);
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorPool(Handle<SDescriptorPool> Hnd)
{
	SDescriptorPool* pPool = Store->GetDescriptorPool(Hnd);
	if (!pPool)
	{
		VKT_ASSERT(false && "Pool with handle does not exist.");
		return false;
	}
	vkDeviceWaitIdle(Device->GetDevice());
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyDescriptorPool(Device->GetDevice(), pPool->Hnd[i], nullptr);
	}
	Store->DeleteDescriptorPool(Hnd);
	return true;
}

Handle<SDescriptorSetLayout> IRenderSystem::CreateDescriptorSetLayout()
{
	size_t id = g_Uid();
	SDescriptorSetLayout* pSetLayout = Store->NewDescriptorSetLayout(id);
	if (!pSetLayout) { return INVALID_HANDLE; }
	return id;
}

bool IRenderSystem::DescriptorSetLayoutAddBinding(const DescriptorSetLayoutBindingInfo& BindInfo)
{
	SDescriptorSetLayout* pSetLayout = Store->GetDescriptorSetLayout(BindInfo.LayoutHnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}

	SDescriptorSetLayout::Binding binding = {};
	binding.Binding = BindInfo.Binding;
	binding.DescriptorCount = BindInfo.DescriptorCount;
	binding.Type = BindInfo.Type;
	binding.ShaderStages = BindInfo.ShaderStages;

	if (BindInfo.BufferHnd != INVALID_HANDLE)
	{
		SMemoryBuffer* pBuffer = Store->GetBuffer(BindInfo.BufferHnd);
		if (!pBuffer)
		{
			VKT_ASSERT(false && "Buffer with handle does not exist.");
			return false;
		}
		binding.Buffer = pBuffer;
	}

	pSetLayout->Bindings.Push(Move(binding));

	return true;
}

bool IRenderSystem::BuildDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd)
{
	using BindingsContainer = StaticArray<VkDescriptorSetLayoutBinding, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;

	SDescriptorSetLayout* pSetLayout = Store->GetDescriptorSetLayout(Hnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}

	BindingsContainer layoutBindings;

	for (const SDescriptorSetLayout::Binding& b : pSetLayout->Bindings)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = b.Binding;
		binding.descriptorCount = b.DescriptorCount;
		binding.descriptorType = Device->GetDescriptorType(b.Type);

		if (b.ShaderStages.Has(Shader_Type_Vertex))
		{
			binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		}

		if (b.ShaderStages.Has(Shader_Type_Fragment))
		{
			binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		layoutBindings.Push(binding);
	}

	VkDescriptorSetLayoutCreateInfo create = {};
	create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create.bindingCount = static_cast<uint32>(layoutBindings.Length());
	create.pBindings = layoutBindings.First();

	if (vkCreateDescriptorSetLayout(Device->GetDevice(), &create, nullptr, &pSetLayout->Hnd) != VK_SUCCESS)
	{
		VKT_ASSERT(false && "Failed to create descriptor set layout on GPU");
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd)
{
	SDescriptorSetLayout* pSetLayout = Store->GetDescriptorSetLayout(Hnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}
	vkDeviceWaitIdle(Device->GetDevice());
	vkDestroyDescriptorSetLayout(Device->GetDevice(), pSetLayout->Hnd, nullptr);
	Store->DeleteDescriptorSetLayout(Hnd);

	return true;
}

Handle<SDescriptorSet> IRenderSystem::CreateDescriptorSet(const DescriptorSetAllocateInfo& AllocInfo)
{
	SDescriptorPool* pPool = Store->GetDescriptorPool(AllocInfo.PoolHnd);
	SDescriptorSetLayout* pLayout = Store->GetDescriptorSetLayout(AllocInfo.LayoutHnd);
	if (!pPool || !pLayout) { return INVALID_HANDLE; }

	size_t id = g_Uid();
	SDescriptorSet* pSet = Store->NewDescriptorSet(id);
	if (!pSet) { return INVALID_HANDLE; }

	pSet->pPool = pPool;
	pSet->pLayout = pLayout;
	pSet->Slot = AllocInfo.Slot;

	return id;
}

bool IRenderSystem::DescriptorSetUpdateBuffer(Handle<SDescriptorSet> Hnd, uint32 Binding, Handle<SMemoryBuffer> BufferHnd)
{
	SDescriptorSet* pSet = Store->GetDescriptorSet(Hnd);
	SMemoryBuffer* pBuffer = Store->GetBuffer(BufferHnd);

	if (!pSet || pBuffer) { return false; }

	SDescriptorSetLayout::Binding* pBinding = nullptr;
	for (SDescriptorSetLayout::Binding& b : pSet->pLayout->Bindings)
	{
		if (b.Binding != Binding) { continue; }
		pBinding = &b;
		break;
	}

	if (!pBinding) { return false; }

	if (pBinding->Type == Descriptor_Type_Sampler ||
		pBinding->Type == Descriptor_Type_Input_Attachment ||
		pBinding->Type == Descriptor_Type_Sampled_Image)
	{
		return false;
	}

	DescriptorUpdate::_Buffers[DescriptorUpdate::Key(pSet, pBinding, DescriptorUpdate::CalcKey(Hnd, Binding))].Push(pBuffer);

	return true;
}

bool IRenderSystem::DescriptorSetUpdateTexture(Handle<SDescriptorSet> Hnd, uint32 Binding, Handle<SImage> ImageHnd)
{
	SDescriptorSet* pSet = Store->GetDescriptorSet(Hnd);
	SImage* pImage = Store->GetImage(ImageHnd);

	if (!pSet || pImage) { return false; }

	SDescriptorSetLayout::Binding* pBinding = nullptr;
	for (SDescriptorSetLayout::Binding& b : pSet->pLayout->Bindings)
	{
		if (b.Binding != Binding) { continue; }
		pBinding = &b;
		break;
	}

	if (!pBinding) { return false; }

	if (pBinding->Type != Descriptor_Type_Sampler ||
		pBinding->Type != Descriptor_Type_Input_Attachment ||
		pBinding->Type != Descriptor_Type_Sampled_Image)
	{
		return false;
	}

	DescriptorUpdate::_Images[DescriptorUpdate::Key(pSet, pBinding, DescriptorUpdate::CalcKey(Hnd, Binding))].Push(pImage);

	return true;
}

bool IRenderSystem::BuildDescriptorSet(Handle<SDescriptorSet> Hnd)
{
	SDescriptorSet* pSet = Store->GetDescriptorSet(Hnd);
	if (!pSet)
	{
		VKT_ASSERT(false && "Set with handle does not exist.");
		return false;
	}

	if (pSet->pPool->Hnd[0] == VK_NULL_HANDLE || 
		pSet->pLayout->Hnd == VK_NULL_HANDLE)
	{
		VKT_ASSERT(false && "Descriptor pool or set layout has not been constructed.");
		return false;
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &pSet->pLayout->Hnd;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		allocInfo.descriptorPool = pSet->pPool->Hnd[i];
		vkAllocateDescriptorSets(Device->GetDevice(), &allocInfo, &pSet->Hnd[i]);
	}

	return true;
}

bool IRenderSystem::DescriptorSetFlushBindingOffset(Handle<SDescriptorSet> Hnd)
{
	SDescriptorSet* pSet = Store->GetDescriptorSet(Hnd);
	if (!pSet)
	{
		VKT_ASSERT(false && "Set with handle does not exist.");
		return false;
	}

	for (SDescriptorSetLayout::Binding& b : pSet->pLayout->Bindings)
	{
		b.Offset[Device->GetCurrentFrameIndex()] = 0;
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorSet(Handle<SDescriptorSet> Hnd)
{
	SDescriptorSet* pSet = Store->GetDescriptorSet(Hnd);
	if (!pSet)
	{
		VKT_ASSERT(false && "Set with handle does not exist.");
		return false;
	}

	vkDeviceWaitIdle(Device->GetDevice());
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkFreeDescriptorSets(Device->GetDevice(), pSet->pPool->Hnd[i], 1, &pSet->Hnd[i]);
	}
	Store->DeleteDescriptorSet(Hnd);

	return true;
}

void IRenderSystem::UpdateDescriptorSetInQueue()
{
	static StaticArray<VkDescriptorBufferInfo, MAX_DESCRIPTOR_BINDING_UPDATES> buffers;
	static StaticArray<VkDescriptorImageInfo, MAX_DESCRIPTOR_BINDING_UPDATES> images;

	// Update buffers ....
	for (auto& [key, container] : DescriptorUpdate::_Buffers)
	{
		SDescriptorSet* pSet = key.pSet;
		SDescriptorSetLayout::Binding* pBinding = key.pBinding;

		for (SMemoryBuffer* buffer : container)
		{
			buffers.Push({
				buffer->Hnd,
				buffer->Offset,
				buffer->Size
			});
		}

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = pBinding->Binding;
		write.descriptorCount = static_cast<uint32>(buffers.Length());
		write.descriptorType = Device->GetDescriptorType(pBinding->Type);
		write.pBufferInfo = buffers.First();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = pSet->Hnd[i];
			vkUpdateDescriptorSets(Device->GetDevice(), 1, &write, 0, nullptr);
		}
	}

	// Update images ....
	for (auto& [key, container] : DescriptorUpdate::_Images)
	{
		SDescriptorSet* pSet = key.pSet;
		SDescriptorSetLayout::Binding* pBinding = key.pBinding;

		for (SImage* image : container)
		{
			images.Push({
				image->pSampler->Hnd,
				image->ImgViewHnd,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});
		}

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = pBinding->Binding;
		write.descriptorCount = static_cast<uint32>(images.Length());
		write.descriptorType = Device->GetDescriptorType(pBinding->Type);
		write.pImageInfo = images.First();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = pSet->Hnd[i];
			vkUpdateDescriptorSets(Device->GetDevice(), 1, &write, 0, nullptr);
		}
	}

	buffers.Empty();
	images.Empty();
}

Handle<SMemoryBuffer> IRenderSystem::AllocateNewBuffer(const BufferAllocateInfo& AllocInfo)
{
	size_t id = g_Uid();
	SMemoryBuffer* pBuffer = Store->NewBuffer(id);

	pBuffer->Locality = AllocInfo.Locality;
	pBuffer->Size = AllocInfo.Size;
	pBuffer->Type = AllocInfo.Type;
	pBuffer->pData = nullptr;

	if (!pBuffer) { return INVALID_HANDLE; }
	return id;
}

bool IRenderSystem::BuildBuffer(Handle<SMemoryBuffer> Hnd)
{
	SMemoryBuffer* pBuffer = Store->GetBuffer(Hnd);
	if (!pBuffer) { return false; }

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = pBuffer->Size;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32 i = 0; i < Buffer_Type_Max; i++)
	{
		if (!pBuffer->Type.Has(i)) { continue; }
		info.usage |= Device->GetBufferUsage(i);
	}

	VmaAllocationCreateInfo alloc = {};
	alloc.usage = Device->GetMemoryUsage(pBuffer->Locality);

	if (vmaCreateBuffer(
		Device->GetAllocator(),
		&info,
		&alloc,
		&pBuffer->Hnd,
		&pBuffer->Allocation,
		nullptr
	))
	{
		return false;
	}

	if (pBuffer->Locality != Buffer_Locality_Gpu)
	{
		vmaMapMemory(Device->GetAllocator(), pBuffer->Allocation, reinterpret_cast<void**>(pBuffer->pData));
		vmaUnmapMemory(Device->GetAllocator(), pBuffer->Allocation);
	}

	return true;
}

bool IRenderSystem::DestroyBuffer(Handle<SMemoryBuffer> Hnd)
{
	SMemoryBuffer* pBuffer = Store->GetBuffer(Hnd);
	if (!pBuffer) { return false; }

	vkDeviceWaitIdle(Device->GetDevice());
	vmaDestroyBuffer(Device->GetAllocator(), pBuffer->Hnd, pBuffer->Allocation);
	Store->DeleteBuffer(Hnd);

	return true;
}

IStagingManager& IRenderSystem::GetStagingManager() const
{
	return *Staging;
}

void IRenderSystem::FlushVertexBuffer()
{
	auto [hnd, pBuffer] = VertexBuffer;
	pBuffer->Offset = 0;
	IMemory::Memzero(pBuffer->pData, pBuffer->Size);
}

void IRenderSystem::FlushIndexBuffer()
{
	auto [hnd, pBuffer] = IndexBuffer;
	pBuffer->Offset = 0;
	IMemory::Memzero(pBuffer->pData, pBuffer->Size);
}

void IRenderSystem::FlushInstanceBuffer()
{
	auto [hnd, pBuffer] = InstanceBuffer;
	pBuffer->Offset = 0;
	IMemory::Memzero(pBuffer->pData, pBuffer->Size);
}

Handle<ISystem> IRenderSystem::GetSystemHandle()
{
	return g_RenderSystemHandle;
}

namespace ao
{
	IRenderSystem& FetchRenderSystem()
	{
		EngineImpl& engine = FetchEngineCtx();
		Handle<ISystem> handle = IRenderSystem::GetSystemHandle();
		SystemInterface* system = engine.GetRegisteredSystem(System_Engine_Type, handle);
		return *(reinterpret_cast<IRenderSystem*>(system));
	}
}

IRenderSystem::DescriptorUpdate::Key::Key(	SDescriptorSet* InSet, 
											SDescriptorSetLayout::Binding* InBinding,
											size_t InKey ) :
	pSet(InSet), pBinding(InBinding), _Key(InKey)
{}

IRenderSystem::DescriptorUpdate::Key::~Key()
{
	pSet = nullptr;
	pBinding = nullptr;
	_Key = -1;
}

size_t IRenderSystem::DescriptorUpdate::Key::First()
{
	return _Key;
}

size_t IRenderSystem::DescriptorUpdate::Key::Length()
{
	return sizeof(size_t);
}

size_t IRenderSystem::DescriptorUpdate::CalcKey(Handle<SDescriptorSet> SetHnd, uint32 Binding)
{
	size_t binding = static_cast<size_t>(Binding);
	return (static_cast<size_t>(SetHnd) << binding) | binding;
}
