#include "Renderer.h"

Handle<ISystem> g_RenderSystemHandle;
LinearAllocator g_RenderSystemAllocator;
Array<ForwardNode<DescriptorSetInstance>> g_DescriptorInstancePool;
size_t previousVbo = -1;

RenderSystem::RenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd) :
	Engine(InEngine),
	AssetManager(nullptr),
	//Context(),
	FrameGraph(nullptr),
	DescriptorManager(nullptr),
	MemoryManager(nullptr),
	DrawCommands(),
	DescriptorInstances(),
	Hnd(Hnd)
{}

RenderSystem::~RenderSystem() {}

void RenderSystem::OnInit()
{
	g_RenderSystemHandle = Hnd;
	gpu::Initialize();
	//Context.InitializeContext();
	g_RenderSystemAllocator.Initialize(MEGABYTES(16));
	g_DescriptorInstancePool.Reserve(2048);

	MemoryManager = reinterpret_cast<IRenderMemoryManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRenderMemoryManager)));
	FMemory::InitializeObject(MemoryManager, g_RenderSystemAllocator);

	AssetManager = reinterpret_cast<IRAssetManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRAssetManager)));
	FMemory::InitializeObject(AssetManager, *MemoryManager, Engine.Manager);

	FrameGraph = reinterpret_cast<IRFrameGraph*>(g_RenderSystemAllocator.Malloc(sizeof(IRFrameGraph)));
	FMemory::InitializeObject(FrameGraph, g_RenderSystemAllocator);

	DescriptorManager = reinterpret_cast<IRDescriptorManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRDescriptorManager)));
	FMemory::InitializeObject(DescriptorManager, g_RenderSystemAllocator);

}

void RenderSystem::OnUpdate()
{
	if (Engine.Window.WindowSizeChanged)
	{
		gpu::OnWindowResize();
		FrameGraph->OnWindowResize();
	}

	gpu::Clear();
	gpu::BeginFrame();

	//
	// TODO(Ygsm):
	// Figure out how to do global descriptor sets.
	// Update Global Descriptor Set
	//
	RenderPass* renderPass = nullptr;
	uint32 passId = -1;

	for (auto& pair : FrameGraph->RenderPasses)
	{
		passId = pair.Key;
		renderPass = pair.Value;

		auto& drawCommands = DrawCommands[passId];

		gpu::BindRenderpass(*renderPass);
		gpu::BindPipeline(*renderPass);
		//Context.BindRenderPass(*renderPass);
		//BindPerPassDescriptors(passId, *renderPass);

		for (DrawCommand& command : drawCommands)
		{
			BindPerObjectDescriptors(command);
			BindVertexAndIndexBuffer(command);
			gpu::Draw(command);
			//Context.SubmitForRender(command, *renderPass);
		}

		gpu::UnbindRenderpass(*renderPass);
		//Context.UnbindRenderPass(*renderPass);
	}

	gpu::BlitToDefault(*FrameGraph);
	gpu::EndFrame();
	gpu::SwapBuffers();
	//Context.BlitToDefault(*FrameGraph);
	//Context.SwapBuffers();

	FlushRenderer();
}

void RenderSystem::OnTerminate()
{
	DrawCommands.Release();

	FrameGraph->Destroy();
	DescriptorManager->Destroy();
	MemoryManager->Destroy();

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
	DrawCommands.Reserve(numPasses);
	for (auto& pair : FrameGraph->RenderPasses)
	{
		if (pair.Value->IsSubpass()) { continue; }
		auto& commandContainer = DrawCommands.Insert(pair.Key, {});
		commandContainer.Reserve(1024);
	}
}

Handle<DrawCommand> RenderSystem::DrawModel(Model& InModel, uint32 Pass)
{
	DrawCommand command;
	size_t id = -1;
	Mesh* mesh = nullptr;

	if (!InModel.Length()) { return INVALID_HANDLE; }

	for (size_t i = 0; i < InModel.Length(); i++)
	{
		if (!i)
		{
			id = DrawCommands[Pass].Length();
		}

		mesh = InModel[i];

		command.Id = static_cast<uint32>(id);
		command.Vbo = mesh->Vbo;
		command.Ebo = mesh->Ebo;
		command.VertexOffset = mesh->VtxOffset;
		command.NumVertices = mesh->NumOfVertices;
		command.IndexOffset = mesh->IdxOffset;
		command.NumIndices = mesh->NumOfIndices;

		DrawCommands[Pass].Push(Move(command));
	}

	DescriptorInstances.PerObject.Insert(id, {});

	return Handle<DrawCommand>(id);
}

bool RenderSystem::UpdateDescriptorSet(uint32 DescriptorId, Handle<DrawCommand> DrawCmdHandle, uint32 Binding, void* Data, size_t Size)
{
	if (DrawCmdHandle == INVALID_HANDLE)
	{
		return false;
	}

	const uint32 currentFrameIndex = gpu::CurrentFrameIndex();
	BindingDescriptors& bindings = DescriptorInstances.PerObject[DrawCmdHandle];

	DescriptorBinding* binding = nullptr;
	Handle<DescriptorSet> setHandle = DescriptorManager->GetDescriptorSetHandleWithId(DescriptorId);
	DescriptorSet* descriptorSet = DescriptorManager->GetDescriptorSet(setHandle);

	if (!descriptorSet) { return false; }

	for (DescriptorBinding& b : descriptorSet->Layout->Bindings)
	{
		if (b.Binding != Binding)
		{
			continue;
		}
		binding = &b;
		break;
	}

	if (!binding) { return false; }
	if (binding->Allocated >= binding->Size) 
	{
		VKT_ASSERT(false && "Insufficient memory to allocate more descriptor set instances");
		return false; 
	}

	auto& node = g_DescriptorInstancePool.Insert(ForwardNode<DescriptorSetInstance>());

	node.Data.Binding = Binding;
	node.Data.Owner = descriptorSet;
	node.Data.Offset = binding->Allocated;
	binding->Allocated += gpu::PadSizeToAlignedSize(Size);

	gpu::CopyToBuffer(*binding->Buffer, Data, Size, binding->Offset[currentFrameIndex] + node.Data.Offset);

	if (!bindings.Count)
	{
		bindings.Base = &node;
		bindings.Current = &node;
	}
	else
	{
		bindings.Current->Next = &node;
		bindings.Current = &node;
	}

	bindings.Count++;

	return true;
}

bool RenderSystem::BindDescLayoutToPass(uint32 DescriptorLayoutId, Handle<RenderPass> PassHandle)
{
	Handle<DescriptorLayout> layoutHandle = DescriptorManager->GetDescriptorLayoutHandleWithId(DescriptorLayoutId);
	DescriptorLayout* descriptorLayout = DescriptorManager->GetDescriptorLayout(layoutHandle);
	if (!descriptorLayout)
	{
		return false;
	}
	FrameGraph->BindLayoutToRenderPass(*descriptorLayout, PassHandle);
	return true;
}

IRAssetManager& RenderSystem::GetAssetManager()
{
	return *AssetManager;
}

void RenderSystem::FlushRenderer()
{
	previousVbo = -1;
	DescriptorManager->FlushDescriptorSetsOffsets();

	for (auto& pair : DrawCommands)
	{ 
		pair.Value.Empty(); 
	}

	DescriptorInstances.Global.Empty();
	DescriptorInstances.PerPass.Empty();
	DescriptorInstances.PerObject.Empty();
	g_DescriptorInstancePool.Empty();
}

void RenderSystem::BindVertexAndIndexBuffer(DrawCommand& Command)
{
	if (Command.Vbo == previousVbo)
	{
		return;
	}
	gpu::BindVertexBuffer(Command);
	gpu::BindIndexBuffer(Command);
	//Context.BindVertexAndIndexBuffer(Command, Pass);
	previousVbo = Command.Vbo;
}

void RenderSystem::BindPerPassDescriptors(uint32 RenderPassId, RenderPass& Pass)
{
	if (!DescriptorInstances.PerPass.Length())
	{
		return;
	}

	BindingDescriptors& descriptorInstances = DescriptorInstances.PerPass[RenderPassId];
	ForwardNode<DescriptorSetInstance>* node = descriptorInstances.Base;

	while (descriptorInstances.Count)
	{
		gpu::BindDescriptorSetInstance(node->Data);
		//Context.BindDescriptorSetInstance(node->Data, Pass);
		descriptorInstances.Count--;
		node = node->Next;
	}
}

void RenderSystem::BindPerObjectDescriptors(DrawCommand& Command)
{
	if (!DescriptorInstances.PerObject.Length())
	{
		return;
	}

	BindingDescriptors& descriptorInstances = DescriptorInstances.PerObject[Command.Id];
	ForwardNode<DescriptorSetInstance>* node = descriptorInstances.Base;

	while (descriptorInstances.Count)
	{
		gpu::BindDescriptorSetInstance(node->Data);
		//Context.BindDescriptorSetInstance(node->Data, Pass);
		descriptorInstances.Count--;
		node = node->Next;
	}
}

IRDescriptorManager& RenderSystem::GetDescriptorManager()
{
	return *DescriptorManager;
}

IRenderMemoryManager& RenderSystem::GetRenderMemoryManager()
{
	return *MemoryManager;
}

//IRVertexGroupManager& RenderSystem::GetVertexGroupManager()
//{
//	return *VertexGroupManager;
//}

//IRStagingBuffer& RenderSystem::GetStagingBufferManager()
//{
//	return *StagingBufferManager;
//}

Handle<ISystem> RenderSystem::GetSystemHandle()
{
	return g_RenderSystemHandle;
}

//bool RenderSystem::BindDescriptorLayoutToRenderPass(Handle<DescriptorLayout> Layout, Handle<RenderPass> Pass)
//{
//	if (Layout == INVALID_HANDLE || Pass == INVALID_HANDLE)
//	{
//		return false;
//	}
//
//	DescriptorLayout& layout = *Descriptors->GetDescriptorLayout(Layout);
//	Graph->BindLayoutToRenderPass(layout, Pass);
//
//	return true;
//}
//
//void RenderSystem::UpdateDescriptorSetForDrawable(uint32 DrawableId, uint32 SetId, void* Data, size_t Size)
//{
//	Handle<DescriptorSet> handle = Descriptors->GetDescriptorSetHandleWithId(SetId);
//	DescriptorSet* descriptorSet = Descriptors->GetDescriptorSet(handle);
//
//	uint32 instanceOffset = descriptorSet->Offset;
//
//	//auto& perObjectContainer = DescriptorSets.PerObject[DrawableId];
//
//	DescriptorSetInstance descSetInstance;
//	descSetInstance.Offset = instanceOffset;
//	descSetInstance.Owner  = descriptorSet;
//
//	//perObjectContainer.Insert(Move(descSetInstance));
//	Context.BindDataToDescriptorSet(Data, Size, *descriptorSet);
//}

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