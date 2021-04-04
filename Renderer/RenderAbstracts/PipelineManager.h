#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_PIPELINE_MANAGER_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_PIPELINE_MANAGER_H

#include "Library/Containers/Map.h"
#include "Primitives.h"
#include "RenderPlatform/API.h"
#include "Library/Allocators/LinearAllocator.h"

class IRFrameGraph;
class IRAssetManager;
class IRDescriptorManager;

struct PipelineCreateInfo : public PipelineBase
{
	Handle<RenderPass> RenderPassHandle;
	Handle<Shader> VertexShaderHandle;
	Handle<Shader> FragmentShaderHandle;
	Handle<DescriptorLayout>* pDescriptorLayouts;
	uint32 NumLayouts;
};

class RENDERER_API IRPipelineManager
{
private:

	using PipelineTable = Map<uint32, Array<Handle<SRPipeline>>>;

	LinearAllocator& Allocator;
	IRDescriptorManager& DescriptorManager;
	IRFrameGraph& FrameGraph;
	IRAssetManager& AssetManager;
	Array<SRPipeline*> PipelineContainer;

public:

	Handle<SRPipeline> PreviousHandle = INVALID_HANDLE;

	IRPipelineManager(
		LinearAllocator& InAllocator, 
		IRDescriptorManager& InDescriptorManager,
		IRFrameGraph& InFrameGraph,
		IRAssetManager& InAssetManager
	);
	~IRPipelineManager();

	DELETE_COPY_AND_MOVE(IRPipelineManager)

	Handle<SRPipeline>	CreateNewGraphicsPipline		(const PipelineCreateInfo& CreateInfo);
	Handle<SRPipeline>	GetPipelineHandleWithId			(uint32 Id);
	SRPipeline*			GetGraphicsPipelineWithHandle	(Handle<SRPipeline> Hnd);
	//bool				AddDescriptorSetLayoutToPipeline(Handle<SRPipeline> Hnd, DescriptorLayout* SetLayout);
	bool				BuildGraphicsPipeline			(Handle<SRPipeline> Hnd);
	//bool				DeleteGraphicsPipeline			(Handle<GraphicsPipeline> Hnd);

	void OnWindowResize			();
	void BuildAll				();
	void BindPipeline			(Handle<SRPipeline> PipelineHandle);
	void Destroy				();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_PIPELINE_MANAGER_H