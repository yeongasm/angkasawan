#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_PIPELINE_MANAGER_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_PIPELINE_MANAGER_H

#include "Library/Containers/Map.h"
#include "Primitives.h"
#include "RenderPlatform/API.h"
#include "Library/Allocators/LinearAllocator.h"

class IRAssetManager;
class IRFrameGraph;
struct RenderPass;

class RENDERER_API IRPipelineManager
{
private:

	LinearAllocator& Allocator;
	IRAssetManager& AssetManager;
	IRFrameGraph& FrameGraph;
	Map<uint32, Array<GraphicsPipeline*>> PipelineContainer;

	Array<GraphicsPipeline*>* GetPipelineContainer(uint32 RenderpassId);

public:

	IRPipelineManager(LinearAllocator& InAllocator, IRAssetManager& InAssetManager, IRFrameGraph& InFrameGraph);
	~IRPipelineManager();

	DELETE_COPY_AND_MOVE(IRPipelineManager)

	Handle<GraphicsPipeline>	CreateNewGraphicsPipline		(const GfxPipelineCreateInfo& CreateInfo);
	GraphicsPipeline*			GetGraphicsPipelineWithHandle	(uint32 RenderpassId, Handle<GraphicsPipeline> Hnd);
	bool						AddDescriptorSetLayoutToPipeline(uint32 RenderpassId, Handle<GraphicsPipeline> Hnd, DescriptorLayout* SetLayout);
	bool						BuildGraphicsPipeline			(uint32 RenderpassId, Handle<GraphicsPipeline> Hnd);
	//bool						DeleteGraphicsPipeline			(Handle<GraphicsPipeline> Hnd);

	bool AddRenderpassToTable	(uint32 PassId);
	void BindPipeline			(uint32 PassId, Handle<GraphicsPipeline> PipelineHandle);
	void Destroy				();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_PIPELINE_MANAGER_H