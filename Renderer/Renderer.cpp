#include "Renderer.h"


LinearAllocator g_FrameGraphAllocator;

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
		drawCommand.Vbo = mesh->Vbo;
		drawCommand.Ebo = mesh->Ebo;
		drawCommand.VertexCount = static_cast<uint32>(mesh->Vertices.Length());
		drawCommand.IndexCount	= static_cast<uint32>(mesh->Indices.Length());

		PushCommandForRender(drawCommand);
	}
}

RendererAssetManager& RenderSystem::FetchAssetManager()
{
	return AssetManager;
}

bool RenderSystem::FinalizeModel(Model& InModel)
{
	for (Mesh* mesh : InModel)
	{
		if (!Context.NewVertexBuffer(*mesh))
		{
			return false;
		}
	}
	return true;
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