#include "Renderer.h"
#include "Library/Containers/Node.h"
#include "Library/Random/Xoroshiro.h"

using DescriptorBindingUpdates = Map<uint32, StaticArray<SDescriptorSet::UpdateContext, MAX_DESCRIPTOR_BINDING_UPDATES>>;

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

void IRenderSystem::OnInit()
{
	g_RenderSystemHandle = Hnd;
	g_RenderSystemAllocator.Initialize(KILOBYTES(64));
	
	Store = IAllocator::New<IDeviceStore>(g_RenderSystemAllocator, g_RenderSystemAllocator);

	Device = IAllocator::New<IRenderDevice>(g_RenderSystemAllocator);
	Device->Initialize(this->Engine);
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