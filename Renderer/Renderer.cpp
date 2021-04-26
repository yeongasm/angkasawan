#include "Renderer.h"

Handle<ISystem> g_RenderSystemHandle;
LinearAllocator g_RenderSystemAllocator;
Array<ForwardNode<DescriptorSetInstance>> g_DescriptorInstancePool;

RenderSystem::RenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd) :
	Engine(InEngine),
	DeviceStore(nullptr),
	AssetManager(nullptr),
	FrameGraph(nullptr),
	DescriptorManager(nullptr),
	MemoryManager(nullptr),
	TextureMemoryManager(nullptr),
	DrawCmdManager(nullptr),
	PipelineManager(nullptr),
	Hnd(Hnd)
{}

RenderSystem::~RenderSystem() {}

void RenderSystem::OnInit()
{
	g_RenderSystemHandle = Hnd;
	gpu::Initialize();
	g_RenderSystemAllocator.Initialize(KILOBYTES(64));
	g_DescriptorInstancePool.Reserve(2048);

	DeviceStore = reinterpret_cast<SRDeviceStore*>(g_RenderSystemAllocator.Malloc(sizeof(SRDeviceStore)));
	FMemory::InitializeObject(DeviceStore, g_RenderSystemAllocator);

	MemoryManager = reinterpret_cast<IRenderMemoryManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRenderMemoryManager)));
	FMemory::InitializeObject(MemoryManager, g_RenderSystemAllocator);

	// TODO:
	// To remove and replace with staging manager.
	//TextureMemoryManager = reinterpret_cast<IRTextureMemoryManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRTextureMemoryManager)));
	//FMemory::InitializeObject(TextureMemoryManager);

	//AssetManager = reinterpret_cast<IRAssetManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRAssetManager)));
	//FMemory::InitializeObject(AssetManager, Engine.Manager, *DeviceStore);

	FrameGraph = reinterpret_cast<IRFrameGraph*>(g_RenderSystemAllocator.Malloc(sizeof(IRFrameGraph)));
	FMemory::InitializeObject(FrameGraph, g_RenderSystemAllocator, *DeviceStore);

	DescriptorManager = reinterpret_cast<IRDescriptorManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRDescriptorManager)));
	FMemory::InitializeObject(DescriptorManager, g_RenderSystemAllocator, *DeviceStore);

	DrawCmdManager = reinterpret_cast<IRDrawManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRDrawManager)));
	FMemory::InitializeObject(DrawCmdManager, g_RenderSystemAllocator, *FrameGraph, *AssetManager);

	PipelineManager = reinterpret_cast<IRPipelineManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRPipelineManager)));
	FMemory::InitializeObject(PipelineManager, g_RenderSystemAllocator, *DescriptorManager, *FrameGraph, *AssetManager);

	PushConstantManager = reinterpret_cast<IRPushConstantManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRPushConstantManager)));
	FMemory::InitializeObject(PushConstantManager, g_RenderSystemAllocator, *PipelineManager);

	MaterialManager = reinterpret_cast<IRMaterialManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRMaterialManager)));
	FMemory::InitializeObject(MaterialManager, g_RenderSystemAllocator, *PipelineManager, *DescriptorManager, *AssetManager, *PushConstantManager);
}

void RenderSystem::OnUpdate()
{
	RenderPass* renderPass = nullptr;
	IRDrawManager::DrawCommands* drawCommands = nullptr;

	if (Engine.Window.WindowSizeChanged)
	{
		gpu::OnWindowResize();
		FrameGraph->OnWindowResize();
		PipelineManager->OnWindowResize();
	}

	gpu::Clear();
	gpu::BeginFrame();
	DrawCmdManager->BindBuffers();

	for (auto& pair : FrameGraph->RenderPasses)
	{
		renderPass = pair.Value;
		drawCommands = &DrawCmdManager->FinalDrawCommands[pair.Key];

		gpu::BindRenderpass(*renderPass);

		for (DrawCommand& command : *drawCommands)
		{
			PipelineManager->BindPipeline(command.PipelineHandle);
			DescriptorManager->BindDescriptorSet(command.DescriptorSetHandle);
			PushConstantManager->BindPushConstant(command.PushConstantHandle);

			gpu::Draw(command);
		}

		gpu::UnbindRenderpass(*renderPass);
	}

	gpu::BlitToDefault(*FrameGraph);
	gpu::EndFrame();
	gpu::SwapBuffers();

	FlushRenderer();
}

void RenderSystem::OnTerminate()
{
	DrawCmdManager->Destroy();
	AssetManager->Destroy();
	FrameGraph->Destroy();
	DescriptorManager->Destroy();
	MemoryManager->Destroy();
	PipelineManager->Destroy();
	MaterialManager->Destroy();
	g_RenderSystemAllocator.Terminate();
	gpu::Terminate();
}

IRFrameGraph& RenderSystem::GetFrameGraph()
{
	return *FrameGraph;
}

/**
* Just registers the pass into the the command buffer.
*/
void RenderSystem::FinalizeGraph()
{
	size_t numPasses = FrameGraph->RenderPasses.Length();
	DrawCmdManager->InstanceDraws.Reserve(numPasses);
	DrawCmdManager->NonInstanceDraws.Reserve(numPasses);
	for (auto& pair : FrameGraph->RenderPasses)
	{
		//if (pair.Value->IsSubpass()) { continue; }
		DrawCmdManager->AddDrawCommandList(pair.Key);
	}
}

//bool RenderSystem::UpdateDescriptorSet(uint32 DescriptorId, Handle<DrawCommand> DrawCmdHandle, uint32 Binding, void* Data, size_t Size)
//{
//	if (DrawCmdHandle == INVALID_HANDLE)
//	{
//		return false;
//	}
//
//	const uint32 currentFrameIndex = gpu::CurrentFrameIndex();
//	BindingDescriptors& bindings = DescriptorInstances.PerObject[DrawCmdHandle];
//
//	DescriptorBinding* binding = nullptr;
//	Handle<DescriptorSet> setHandle = DescriptorManager->GetDescriptorSetHandleWithId(DescriptorId);
//	DescriptorSet* descriptorSet = DescriptorManager->GetDescriptorSet(setHandle);
//
//	if (!descriptorSet) { return false; }
//
//	for (DescriptorBinding& b : descriptorSet->Layout->Bindings)
//	{
//		if (b.Binding != Binding)
//		{
//			continue;
//		}
//		binding = &b;
//		break;
//	}
//
//	if (!binding) { return false; }
//	if (binding->Allocated >= binding->Size) 
//	{
//		VKT_ASSERT(false && "Insufficient memory to allocate more descriptor set instances");
//		return false; 
//	}
//
//	auto& node = g_DescriptorInstancePool.Insert(ForwardNode<DescriptorSetInstance>());
//
//	node.Data.Binding = Binding;
//	node.Data.Owner = descriptorSet;
//	node.Data.Offset = binding->Allocated;
//	binding->Allocated += gpu::PadSizeToAlignedSize(Size);
//
//	gpu::CopyToBuffer(*binding->Buffer, Data, Size, binding->Offset[currentFrameIndex] + node.Data.Offset);
//
//	if (!bindings.Count)
//	{
//		bindings.Base = &node;
//		bindings.Current = &node;
//	}
//	else
//	{
//		bindings.Current->Next = &node;
//		bindings.Current = &node;
//	}
//
//	bindings.Count++;
//
//	return true;
//}

//bool RenderSystem::BindDescLayoutToPass(uint32 DescriptorLayoutId, Handle<RenderPass> PassHandle)
//{
//	Handle<DescriptorLayout> layoutHandle = DescriptorManager->GetDescriptorLayoutHandleWithId(DescriptorLayoutId);
//	DescriptorLayout* descriptorLayout = DescriptorManager->GetDescriptorLayout(layoutHandle);
//	if (!descriptorLayout)
//	{
//		return false;
//	}
//	FrameGraph->BindLayoutToRenderPass(*descriptorLayout, PassHandle);
//	return true;
//}

IRAssetManager& RenderSystem::GetAssetManager()
{
	return *AssetManager;
}

void RenderSystem::FlushRenderer()
{
	PushConstantManager->PreviousHandle = INVALID_HANDLE;
	PipelineManager->PreviousHandle = INVALID_HANDLE;
	DescriptorManager->PreviousHandle = INVALID_HANDLE;
	DescriptorManager->FlushDescriptorSetsOffsets();
	DrawCmdManager->Flush();
}

//void RenderSystem::BindPerPassDescriptors(uint32 RenderPassId, RenderPass& Pass)
//{
//	if (!DescriptorInstances.PerPass.Length())
//	{
//		return;
//	}
//
//	BindingDescriptors& descriptorInstances = DescriptorInstances.PerPass[RenderPassId];
//	ForwardNode<DescriptorSetInstance>* node = descriptorInstances.Base;
//
//	while (descriptorInstances.Count)
//	{
//		gpu::BindDescriptorSetInstance(node->Data);
//		//Context.BindDescriptorSetInstance(node->Data, Pass);
//		descriptorInstances.Count--;
//		node = node->Next;
//	}
//}

//void RenderSystem::BindPerObjectDescriptors(DrawCommand& Command)
//{
//	if (!DescriptorInstances.PerObject.Length())
//	{
//		return;
//	}
//
//	BindingDescriptors& descriptorInstances = DescriptorInstances.PerObject[Command.Id];
//	ForwardNode<DescriptorSetInstance>* node = descriptorInstances.Base;
//
//	while (descriptorInstances.Count)
//	{
//		gpu::BindDescriptorSetInstance(node->Data);
//		//Context.BindDescriptorSetInstance(node->Data, Pass);
//		descriptorInstances.Count--;
//		node = node->Next;
//	}
//}

IRDescriptorManager& RenderSystem::GetDescriptorManager()
{
	return *DescriptorManager;
}

IRenderMemoryManager& RenderSystem::GetRenderMemoryManager()
{
	return *MemoryManager;
}

IRDrawManager& RenderSystem::GetDrawManager()
{
	return *DrawCmdManager;
}

IRPipelineManager& RenderSystem::GetPipelineManager()
{
	return *PipelineManager;
}

IRMaterialManager& RenderSystem::GetMaterialManager()
{
	return *MaterialManager;
}

IRTextureMemoryManager& RenderSystem::GetTextureMemoryManager()
{
	return *TextureMemoryManager;
}

Handle<ISystem> RenderSystem::GetSystemHandle()
{
	return g_RenderSystemHandle;
}

namespace ao
{
	RenderSystem& FetchRenderSystem()
	{
		EngineImpl& engine = FetchEngineCtx();
		Handle<ISystem> handle = RenderSystem::GetSystemHandle();
		SystemInterface* system = engine.GetRegisteredSystem(System_Engine_Type, handle);
		return *(reinterpret_cast<RenderSystem*>(system));
	}
}