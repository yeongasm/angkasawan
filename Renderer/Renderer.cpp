#include "Renderer.h"


LinearAllocator g_FrameGraphAllocator;

RenderSystem::RenderSystem() {}

RenderSystem::~RenderSystem() {}

void RenderSystem::OnInit()
{
	Context.InitializeContext();
	FrameGraphs.Reserve(10);
	AssetManager.Initialize();
	g_FrameGraphAllocator.Initialize(MEGABYTES(1));
	CmdBuffer.RenderCommands.Reserve(1024);
}

void RenderSystem::OnUpdate()
{
	Context.Clear();

	for (DrawCommand& drawCommand : CmdBuffer.RenderCommands)
	{
		Context.Render(drawCommand);
	}

	Context.SwapBuffers();
	CmdBuffer.RenderCommands.Empty();
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
	for (FrameGraph* graph : FrameGraphs)
	{
		graph->Destroy();
	}
	FrameGraphs.Release();
	g_FrameGraphAllocator.Terminate();

	// Terminate context.
	Context.TerminateContext();

	// Terminate asset manager.
	AssetManager.Terminate();
}

Handle<FrameGraph> RenderSystem::NewFrameGraph(const char* GraphName)
{
	FrameGraph* graph = reinterpret_cast<FrameGraph*>(g_FrameGraphAllocator.Malloc(sizeof(FrameGraph)));
	new (graph) FrameGraph(Context, g_FrameGraphAllocator);

	graph->Name = GraphName;

	Handle<FrameGraph> handle = FrameGraphs.Length();
	FrameGraphs.Push(graph);

	return handle;
}

FrameGraph& RenderSystem::GetFrameGraph(Handle<FrameGraph> Handle)
{
	VKT_ASSERT("Handle provided is not a valid handle" && Handle != INVALID_HANDLE);
	FrameGraph* graph = FrameGraphs[Handle];
	return *graph;
}

FrameGraph* RenderSystem::GetFrameGraphWithName(const char* GraphName)
{
	FrameGraph* renderGraph = nullptr;

	if (!FrameGraphs.Length())
	{
		return nullptr;
	}

	for (FrameGraph* graph : FrameGraphs)
	{
		if (graph->Name != GraphName)
		{
			continue;
		}
		renderGraph = graph;
		break;
	}
	return renderGraph;
}

void RenderSystem::PushCommandForRender(const DrawCommand& Command)
{
	CmdBuffer.RenderCommands.Push(Command);
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