#include "Renderer.h"

Handle<ISystem> g_RenderSystemHandle;
LinearAllocator g_RenderSystemAllocator;
Array<ForwardNode<DescriptorSetInstance>> g_DescriptorInstancePool;
size_t previousVbo = -1;

RenderSystem::RenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd) :
	Engine(InEngine),
	AssetManager(),
	Context(),
	FrameGraph(nullptr),
	DescriptorManager(nullptr),
	VertexGroupManager(nullptr),
	DrawCommands(),
	DescriptorInstances(),
	Hnd(Hnd)
{}

RenderSystem::~RenderSystem() {}

void RenderSystem::OnInit()
{
	g_RenderSystemHandle = Hnd;
	Context.InitializeContext();
	AssetManager.Initialize();
	g_RenderSystemAllocator.Initialize(MEGABYTES(16));
	g_DescriptorInstancePool.Reserve(2048);

	FrameGraph = reinterpret_cast<IRFrameGraph*>(g_RenderSystemAllocator.Malloc(sizeof(IRFrameGraph)));
	FMemory::InitializeObject(FrameGraph, Context, g_RenderSystemAllocator);

	DescriptorManager = reinterpret_cast<IRDescriptorManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRDescriptorManager)));
	FMemory::InitializeObject(DescriptorManager, Context, g_RenderSystemAllocator);

	VertexGroupManager = reinterpret_cast<IRVertexGroupManager*>(g_RenderSystemAllocator.Malloc(sizeof(IRVertexGroupManager)));
	FMemory::InitializeObject(VertexGroupManager, Context, g_RenderSystemAllocator);
}

void RenderSystem::OnUpdate()
{
	if (Engine.Window.WindowSizeChanged)
	{
		Context.OnWindowResize();
		FrameGraph->OnWindowResize();
	}

	Context.Clear();

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

		Context.BindRenderPass(*renderPass);
		BindPerPassDescriptors(passId, *renderPass);

		for (DrawCommand& command : drawCommands)
		{
			BindPerObjectDescriptors(command.Id, *renderPass);
			BindVertexAndIndexBuffer(command, *renderPass);
			Context.SubmitForRender(command, *renderPass);
		}

		Context.UnbindRenderPass(*renderPass);
	}

	Context.BlitToDefault(*FrameGraph);
	Context.SwapBuffers();

	FlushRenderer();
}

void RenderSystem::OnTerminate()
{
	DrawCommands.Release();
	
	FrameGraph->Destroy();
	DescriptorManager->Destroy();
	VertexGroupManager->Destroy();

	g_RenderSystemAllocator.Terminate();
	Context.TerminateContext();
	AssetManager.Terminate();
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
	size_t id = -1;
	Mesh* mesh = nullptr;
	VertexGroup* group = nullptr;

	if (!InModel.Length())
	{
		return INVALID_HANDLE;
	}

	DrawCommand command;

	for (size_t i = 0; i < InModel.Length(); i++)
	{
		if (!i)
		{
			id = DrawCommands[Pass].Length();
		}

		mesh = InModel[i];

		Handle<VertexGroup> vgHandle = VertexGroupManager->GetVertexGroupHandleWithId(mesh->Group);
		group = VertexGroupManager->GetVertexGroup(vgHandle);

		command.Id = static_cast<uint32>(id);
		command.Vbo = group->Vbo;
		command.Ebo = group->Ebo;
		command.VertexOffset = mesh->VertexOffset;
		command.NumVertices = mesh->NumOfVertices;
		command.IndexOffset = mesh->IndexOffset;
		command.NumIndices = mesh->NumOfIndices;

		DrawCommands[Pass].Push(Move(command));
	}

	DescriptorInstances.PerObject.Insert(id, {});

	return Handle<DrawCommand>(id);
}

bool RenderSystem::UpdateDescriptorSet(uint32 DescriptorId, Handle<DrawCommand> DrawCmdHandle, void* Data, size_t Size)
{
	if (DrawCmdHandle == INVALID_HANDLE)
	{
		return false;
	}

	BindingDescriptors& bindings = DescriptorInstances.PerObject[DrawCmdHandle];

	Handle<DescriptorSet> setHandle = DescriptorManager->GetDescriptorSetHandleWithId(DescriptorId);
	DescriptorSet* descriptorSet = DescriptorManager->GetDescriptorSet(setHandle);

	if (!descriptorSet)
	{
		return false;
	}

	uint32 instanceOffset = descriptorSet->Offset;
	auto& node = g_DescriptorInstancePool.Insert(ForwardNode<DescriptorSetInstance>());

	node.Data.Offset = instanceOffset;
	node.Data.Owner = descriptorSet;

	Context.BindDataToDescriptorSet(Data, Size, *descriptorSet);

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
	return AssetManager;
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

void RenderSystem::BindVertexAndIndexBuffer(DrawCommand& Command, RenderPass& Pass)
{
	if (Command.Vbo == previousVbo)
	{
		return;
	}
	Context.BindVertexAndIndexBuffer(Command, Pass);
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
		Context.BindDescriptorSetInstance(node->Data, Pass);
		descriptorInstances.Count--;
		node = node->Next;
	}
}

void RenderSystem::BindPerObjectDescriptors(uint32 DrawCommandId, RenderPass& Pass)
{
	if (!DescriptorInstances.PerObject.Length())
	{
		return;
	}

	BindingDescriptors& descriptorInstances = DescriptorInstances.PerObject[DrawCommandId];
	ForwardNode<DescriptorSetInstance>* node = descriptorInstances.Base;

	while (descriptorInstances.Count)
	{
		Context.BindDescriptorSetInstance(node->Data, Pass);
		descriptorInstances.Count--;
		node = node->Next;
	}
}

IRDescriptorManager& RenderSystem::GetDescriptorManager()
{
	return *DescriptorManager;
}

IRVertexGroupManager& RenderSystem::GetVertexGroupManager()
{
	return *VertexGroupManager;
}

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