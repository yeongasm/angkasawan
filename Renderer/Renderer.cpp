#include "Renderer.h"


LinearAllocator g_FrameGraphAllocator;
LinearAllocator g_RenderGroupAllocator;

RenderSystem::RenderSystem() :
	AssetManager(),
	Context(),
	Graph(nullptr),
	CmdBuffer()
{}

RenderSystem::~RenderSystem() {}

void RenderSystem::OnInit()
{
	Context.InitializeContext();
	AssetManager.Initialize();
	g_FrameGraphAllocator.Initialize(MEGABYTES(1));
	CmdBuffer.RenderCommands.Reserve(1024);
	Graph = reinterpret_cast<FrameGraph*>(g_FrameGraphAllocator.Malloc(sizeof(FrameGraph)));
	FMemory::InitializeObject(Graph, Context, g_FrameGraphAllocator);
}

void RenderSystem::OnUpdate()
{
	Context.Clear();

	for (auto& pair : CmdBuffer.RenderCommands)
	{
		RenderPass* renderPass = Graph->RenderPasses[pair.Key];

		Context.BindRenderPass(*renderPass);
		for (Drawable& drawable : pair.Value)
		{
			Context.SubmitForRender(drawable, *renderPass);
		}
		Context.UnbindRenderPass(*renderPass);
	}

	Context.SwapBuffers();

	// Empty each drawable command that's in the array.
	for (auto& pair : CmdBuffer.RenderCommands)
	{
		pair.Value.Empty();
	}
}

void RenderSystem::OnEvent(const OS::Event& e)
{
	switch (e.EventType)
	{
		case OS::Event::Type::WINDOW_RESIZE:
			Context.OnWindowResize();
			break;
		default:
			break;
	}
}

void RenderSystem::OnTerminate()
{
	CmdBuffer.RenderCommands.Release();
	// Release render graphs and it's memory.
	
	Graph->Destroy();
	g_FrameGraphAllocator.Terminate();

	// Release all resources if it has not been.

	// Terminate context.
	Context.TerminateContext();
	// Terminate asset manager.
	AssetManager.Terminate();
}

//Handle<FrameGraph> RenderSystem::NewFrameGraph(const char* GraphName)
//{
//	FrameGraph* graph = reinterpret_cast<FrameGraph*>(g_FrameGraphAllocator.Malloc(sizeof(FrameGraph)));
//	new (graph) FrameGraph(Context, g_FrameGraphAllocator);
//
//	graph->Name = GraphName;
//
//	Handle<FrameGraph> handle = FrameGraphs.Length();
//	FrameGraphs.Push(graph);
//
//	return handle;
//}
//
//FrameGraph& RenderSystem::GetFrameGraph(Handle<FrameGraph> Handle)
//{
//	VKT_ASSERT("Handle provided is not a valid handle" && Handle != INVALID_HANDLE);
//	FrameGraph* graph = FrameGraphs[Handle];
//	return *graph;
//}
//
//FrameGraph* RenderSystem::GetFrameGraphWithName(const char* GraphName)
//{
//	FrameGraph* renderGraph = nullptr;
//
//	if (!FrameGraphs.Length())
//	{
//		return nullptr;
//	}
//
//	for (FrameGraph* graph : FrameGraphs)
//	{
//		if (graph->Name != GraphName)
//		{
//			continue;
//		}
//		renderGraph = graph;
//		break;
//	}
//	return renderGraph;
//}

FrameGraph* RenderSystem::GetFrameGraph()
{
	return Graph;
}

/**
* Just registers the pass into the the command buffer.
*/
void RenderSystem::FinalizeGraph()
{
	for (auto& pair : Graph->RenderPasses)
	{
		CmdBuffer.RenderCommands.Insert(pair.Key, {});
	}
}

void RenderSystem::PushCommandForRender(DrawCommand& Command)
{
	auto& drawableArray = CmdBuffer.RenderCommands[Command.PassHandle];
	drawableArray.Push(Command);
}

void RenderSystem::RenderModel(Model& InModel, uint32 RenderedPass)
{
	DrawCommand drawCommand;
	drawCommand.PassHandle = RenderedPass;
	
	for (Mesh* mesh : InModel)
	{
		RenderGroup& renderGroup = *RenderGroups[mesh->Group];
		drawCommand.Vbo = renderGroup.Vbo;
		drawCommand.Ebo = renderGroup.Ebo;

		drawCommand.VertexOffset	= mesh->VertexOffset;
		drawCommand.IndexOffset		= mesh->IndexOffset;

		PushCommandForRender(drawCommand);
	}
}

void RenderSystem::NewRenderGroup(uint32 Identifier)
{
	RenderGroup* group = reinterpret_cast<RenderGroup*>(FMemory::Malloc(sizeof(RenderGroup)));
	FMemory::InitializeObject(group, Context);
	RenderGroups.Insert(Identifier, group);
}

void RenderSystem::AddModelToRenderGroup(Model& InModel, uint32 GroupIdentifier)
{
	RenderGroup& renderGroup = *RenderGroups[GroupIdentifier];
	for (Mesh* mesh : InModel)
	{
		renderGroup.AddMeshToGroup(*mesh);
		mesh->Group = GroupIdentifier;
	}
}

void RenderSystem::FinalizeRenderGroup(uint32 Identifier)
{
	RenderGroup* group = RenderGroups[Identifier];
	group->Build();
}

void RenderSystem::TerminateRenderGroup(uint32 Identifier)
{
	RenderGroup* group = RenderGroups[Identifier];
	group->Destroy();
	FMemory::Free(group);
	group = nullptr;
	RenderGroups.Remove(Identifier);
}


RendererAssetManager& RenderSystem::FetchAssetManager()
{
	return AssetManager;
}

namespace ao
{
	RenderSystem& FetchRenderSystem()
	{
		using Identity = RenderSystem::Identity;
		EngineImpl& engine = FetchEngineCtx();
		SystemInterface* system = engine.Systems.GetSystem(Identity::Value, System_Engine_Type);
		return *dynamic_cast<RenderSystem*>(system);
	}
}