#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE

#include "Library/Containers/Map.h"
#include "Assets/GPUHandles.h"
#include "RenderAbstracts/RenderMemory.h"
#include "RenderAbstracts/Primitives.h"
#include "Library/Math/Math.h"
#include "API/Definitions.h"

#define RENDERER_DRAWABLES_SLACK 16

class IRFrameGraph;
class IRenderMemoryManager;
class IRAssetManager;
struct SRPipeline;
struct DescriptorSet;
struct SRPushConstant;
struct SRMaterial;

// Draw command represent a single entity that will be drawn.
// In the case of models, it represents a single mesh.
struct DrawCommand
{
	uint32 NumVertices;
	uint32 NumIndices;
	uint32 VertexOffset;
	uint32 IndexOffset;
	uint32 InstanceOffset;
	uint32 InstanceCount;
	Handle<DescriptorSet> DescriptorSetHandle;
	Handle<SRPushConstant> PushConstantHandle;
	Handle<SRPipeline> PipelineHandle;
};

struct DrawManagerInitInfo
{
	uint32 MaxDrawCount;
	uint32 VertexInputBinding;
	uint32 InstanceInputBinding;
	Handle<SRMemoryBuffer> VtxBufHnd;
	Handle<SRMemoryBuffer> IdxBufHnd;
	MemoryAllocateInfo* pMemAllocInfo;
	IRenderMemoryManager* pMemManager;
};

struct ModelDrawInfo
{
	math::mat4 Transform;
	Model* pModel;
	SRMaterial** pMaterial;
	uint32 NumMaterials;
	uint32 RenderpassId;
	Handle<SRPipeline> PipelineHandle;
	Handle<DescriptorSet> DescriptorSetHandle;
	bool Instanced;
};

// It is decided that this will handle everything related to drawing.
class RENDERER_API IRDrawManager
{
private:

	friend class RenderSystem;
	
	struct SInstanceEntry
	{
		uint32 Id;
		Array<math::mat4> ModelTransform;
		Array<DrawCommand> DrawCommands;
	};
	
	using InstanceEntries = Array<SInstanceEntry>;
	using DrawCommands = Array<DrawCommand, 10>;

	Map<uint32, InstanceEntries> InstanceDraws;
	Map<uint32, InstanceEntries> NonInstanceDraws;
	Map<uint32, DrawCommands> FinalDrawCommands;
	LinearAllocator& Allocator;
	IRFrameGraph& FrameGraph;
	IRAssetManager& AssetManager;
	SRMemoryBuffer* VertexBuffer;
	SRMemoryBuffer* IndexBuffer;
	SRMemoryBuffer* InstanceBuffer;
	uint32 Offsets[MAX_FRAMES_IN_FLIGHT];
	uint32 NumOfDrawables;
	uint32 MaxDrawables;
	uint32 VertexFirstBinding;
	uint32 InstanceFirstBinding;

	void AddDrawCommandList(uint32 PassId);
	void BindBuffers();

public:

	IRDrawManager(LinearAllocator& InAllocator, IRFrameGraph& InFrameGraph, IRAssetManager& InAssetManager);
	~IRDrawManager();

	DELETE_COPY_AND_MOVE(IRDrawManager)

	bool InitializeDrawManager(const DrawManagerInitInfo& AllocInfo);
	bool DrawModel(const ModelDrawInfo& DrawInfo);
	uint32 GetMaxDrawableCount() const;
	void FinalizeInstances();
	void Flush();
	void Destroy();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE