#include "PipelineManager.h"
#include "API/Context.h"
#include "FrameGraph.h"
#include "DescriptorSets.h"
#include "Assets/Assets.h"

IRPipelineManager::IRPipelineManager(
	LinearAllocator& InAllocator, 
	IRDescriptorManager& InDescriptorManager,
	IRFrameGraph& InFrameGraph,
	IRAssetManager& InAssetManager
) :
	Allocator(InAllocator),
	DescriptorManager(InDescriptorManager),
	FrameGraph(InFrameGraph),
	AssetManager(InAssetManager),
	PipelineContainer()
{}

IRPipelineManager::~IRPipelineManager()
{
	PipelineContainer.Release();
}

Handle<SRPipeline> IRPipelineManager::CreateNewGraphicsPipline(const PipelineCreateInfo& CreateInfo)
{
	if (CreateInfo.RenderPassHandle		== INVALID_HANDLE ||
		CreateInfo.VertexShaderHandle	== INVALID_HANDLE ||
		CreateInfo.FragmentShaderHandle == INVALID_HANDLE)
	{
		return INVALID_HANDLE;
	}

	RenderPass& renderPass	= FrameGraph.GetRenderPass(CreateInfo.RenderPassHandle);
	Shader* vertexShader	= AssetManager.GetShaderWithHandle(CreateInfo.VertexShaderHandle);
	Shader* fragmentShader	= AssetManager.GetShaderWithHandle(CreateInfo.FragmentShaderHandle);

	if (!vertexShader || !fragmentShader)
	{
		return INVALID_HANDLE;
	}

	SRPipeline* pipeline = reinterpret_cast<SRPipeline*>(Allocator.Malloc(sizeof(SRPipeline)));
	FMemory::InitializeObject(pipeline);

	pipeline->CullMode = CreateInfo.CullMode;
	pipeline->FrontFace = CreateInfo.FrontFace;
	pipeline->PolygonalMode = CreateInfo.PolygonalMode;
	pipeline->Samples = CreateInfo.Samples;
	pipeline->Topology = CreateInfo.Topology;
	pipeline->VertexInputBindings = Move(const_cast<Array<VertexInputBinding>&>(CreateInfo.VertexInputBindings));
	pipeline->VertexShader = vertexShader;
	pipeline->FragmentShader = fragmentShader;
	pipeline->Renderpass = &renderPass;

	pipeline->ColorOutputCount = static_cast<uint32>(renderPass.ColorOutputs.Length());
	pipeline->HasDefaultColorOutput = !renderPass.Flags.Has(RenderPass_Bit_No_Color_Render);

	Handle<DescriptorLayout>* dlHnd = nullptr;
	for (uint32 i = 0; i < CreateInfo.NumLayouts; i++)
	{
		dlHnd = &CreateInfo.pDescriptorLayouts[i];
		DescriptorLayout* layout = DescriptorManager.GetDescriptorLayout(*dlHnd);
		layout->Pipeline = &pipeline->Handle;
		pipeline->DescriptorLayouts.Push(layout);
	}

	if (renderPass.Flags.Has(RenderPass_Bit_DepthStencil_Output) ||
		!renderPass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
		pipeline->HasDepthStencil = true;
	}

	size_t idx = PipelineContainer.Push(pipeline);

	return Handle<SRPipeline>(idx);
}

Handle<SRPipeline> IRPipelineManager::GetPipelineHandleWithId(uint32 Id)
{
	size_t index = INVALID_HANDLE;
	SRPipeline* pipeline = nullptr;
	for (size_t i = 0; i < PipelineContainer.Length(); i++)
	{
		pipeline = PipelineContainer[i];
		if (pipeline->Id != Id) { continue; }
		index = i;
		break;
	}
	return Handle<SRPipeline>(index);
}

SRPipeline* IRPipelineManager::GetGraphicsPipelineWithHandle(Handle<SRPipeline> Hnd)
{
	if (Hnd == INVALID_HANDLE) { return nullptr; }
	return PipelineContainer[Hnd];
}

//bool IRPipelineManager::AddDescriptorSetLayoutToPipeline(Handle<SRPipeline> Hnd, DescriptorLayout* SetLayout)
//{
//	SRPipeline* pipeline = GetGraphicsPipelineWithHandle(Hnd);
//	if (!pipeline) { return false; }
//	pipeline->DescriptorLayouts.Push(SetLayout);
//	return true;
//}

bool IRPipelineManager::BuildGraphicsPipeline(Handle<SRPipeline> Hnd)
{
	SRPipeline* pipeline = GetGraphicsPipelineWithHandle(Hnd);
	if (!pipeline) { return false; }
	gpu::CreateGraphicsPipeline(*pipeline);
	return true;
}

void IRPipelineManager::BuildAll()
{
	for (SRPipeline* pipeline : PipelineContainer)
	{
		if (pipeline->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateGraphicsPipeline(*pipeline);
	}
}

//bool IRPipelineManager::AddRenderpassToTable(uint32 PassId)
//{
//	for (auto& [pass, container] : Table)
//	{
//		if (pass == PassId) { return false; }
//	}
//	Table.Insert(PassId, {});
//	return true;
//}

void IRPipelineManager::BindPipeline(Handle<SRPipeline> PipelineHandle)
{
	if (PreviousHandle == PipelineHandle) { return; }

	SRPipeline* pipeline = PipelineContainer[PipelineHandle];
	VKT_ASSERT(pipeline && "Pipeline does not exist!");
	gpu::BindPipeline(*pipeline);
	PreviousHandle = PipelineHandle;
}

void IRPipelineManager::Destroy()
{
	for (SRPipeline* pipeline : PipelineContainer)
	{
		gpu::DestroyGraphicsPipeline(*pipeline);
	}
}

void IRPipelineManager::OnWindowResize()
{
	for (SRPipeline* pipeline : PipelineContainer)
	{
		gpu::DestroyGraphicsPipeline(*pipeline);
		gpu::CreateGraphicsPipeline(*pipeline);
	}
}

