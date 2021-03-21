#include "PipelineManager.h"
#include "API/Context.h"
#include "FrameGraph.h"
#include "Assets/Assets.h"

Array<GraphicsPipeline*>* IRPipelineManager::GetPipelineContainer(uint32 RenderpassId)
{
	Array<GraphicsPipeline*>* pipelineContainer = nullptr;
	for (auto& [pass, container] : PipelineContainer)
	{
		if (pass != RenderpassId) { continue; }
		pipelineContainer = &container;
		break;
	}
	return pipelineContainer;
}

IRPipelineManager::IRPipelineManager(LinearAllocator& InAllocator, 
									 IRAssetManager& InAssetManager,
									 IRFrameGraph& InFrameGraph) :
	Allocator(InAllocator),
	AssetManager(InAssetManager),
	FrameGraph(InFrameGraph),
	PipelineContainer()
{}

IRPipelineManager::~IRPipelineManager()
{
	for (auto& [passHnd, pipelines] : PipelineContainer)
	{
		pipelines.Release();
	}
	PipelineContainer.Release();
}

Handle<GraphicsPipeline> IRPipelineManager::CreateNewGraphicsPipline(const GfxPipelineCreateInfo& CreateInfo)
{
	if (CreateInfo.RenderPassHandle		== INVALID_HANDLE ||
		CreateInfo.VertexShaderHandle	== INVALID_HANDLE ||
		CreateInfo.FragmentShaderHandle == INVALID_HANDLE)
	{
		return INVALID_HANDLE;
	}

	RenderPass& renderPass = FrameGraph.GetRenderPass(CreateInfo.RenderPassHandle);
	Shader* vertexShader = AssetManager.GetShaderWithHandle(CreateInfo.VertexShaderHandle);
	Shader* fragmentShader = AssetManager.GetShaderWithHandle(CreateInfo.FragmentShaderHandle);

	if (!vertexShader || !fragmentShader)
	{
		return INVALID_HANDLE;
	}

	GraphicsPipeline* pipeline = reinterpret_cast<GraphicsPipeline*>(Allocator.Malloc(sizeof(GraphicsPipeline)));
	FMemory::InitializeObject(pipeline);

	pipeline->CullMode = CreateInfo.CullMode;
	pipeline->FrontFace = CreateInfo.FrontFace;
	pipeline->PolygonalMode = CreateInfo.PolygonalMode;
	pipeline->Samples = CreateInfo.Samples;
	pipeline->Topology = CreateInfo.Topology;
	pipeline->VertexInputBindings = Move(const_cast<Array<VertexInputBinding>&>(CreateInfo.VertexInputBindings));
	pipeline->VertexShader = vertexShader;
	pipeline->FragmentShader = fragmentShader;

	pipeline->ColorOutputCount = static_cast<uint32>(renderPass.ColorOutputs.Length());
	pipeline->HasDefaultColorOutput = !renderPass.Flags.Has(RenderPass_Bit_No_Color_Render);

	if (renderPass.Flags.Has(RenderPass_Bit_DepthStencil_Output) ||
		!renderPass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
		pipeline->HasDepthStencil = true;
	}

	size_t idx = PipelineContainer[CreateInfo.RenderPassHandle].Push(pipeline);

	return Handle<GraphicsPipeline>(idx);
}

GraphicsPipeline* IRPipelineManager::GetGraphicsPipelineWithHandle(uint32 RenderpassId, Handle<GraphicsPipeline> Hnd)
{
	auto* pipelineContainer = GetPipelineContainer(RenderpassId);
	if (!pipelineContainer) { return nullptr; }
	auto& container = *pipelineContainer;
	return container[Hnd];
}

bool IRPipelineManager::AddDescriptorSetLayoutToPipeline(uint32 RenderpassId, Handle<GraphicsPipeline> Hnd, DescriptorLayout* SetLayout)
{
	GraphicsPipeline* pipeline = GetGraphicsPipelineWithHandle(RenderpassId, Hnd);
	if (!pipeline) { return false; }
	pipeline->DescriptorLayouts.Push(SetLayout);
	return true;
}

bool IRPipelineManager::BuildGraphicsPipeline(uint32 RenderpassId, Handle<GraphicsPipeline> Hnd)
{
	GraphicsPipeline* pipeline = GetGraphicsPipelineWithHandle(RenderpassId, Hnd);
	if (!pipeline) { return false; }
	gpu::CreateGraphicsPipeline(*pipeline);
	return true;
}

bool IRPipelineManager::AddRenderpassToTable(uint32 PassId)
{
	for (auto& [pass, container] : PipelineContainer)
	{
		if (pass == PassId) { return false; }
	}
	PipelineContainer.Insert(PassId, {});
	return true;
}

void IRPipelineManager::BindPipeline(uint32 PassId, Handle<GraphicsPipeline> PipelineHandle)
{
	auto& container = *GetPipelineContainer(PassId);
	GraphicsPipeline* pipeline = container[PipelineHandle];
	gpu::BindPipeline(*pipeline);
}

void IRPipelineManager::Destroy()
{
	for (auto& [passId, container] : PipelineContainer)
	{
		for (GraphicsPipeline* pipeline : container)
		{
			gpu::DestroyGraphicsPipeline(*pipeline);
		}
	}
}

