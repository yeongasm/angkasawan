#pragma once
#ifndef LEARNVK_RENDERER_RENDER_SYSTEM_H
#define LEARNVK_RENDERER_RENDER_SYSTEM_H

#include "RenderPlatform/API.h"
#include "Engine/Interface.h"
#include "API/RendererFlagBits.h"
#include "Library/Containers/Node.h"
#include "Library/Math/Matrix.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderAbstracts/Primitives.h"
#include "RenderAbstracts/FrameGraph.h"
#include "RenderAbstracts/StagingManager.h"

class IRenderDevice;
struct IDeviceStore;

class IStagingManager;
class IFrameGraph;

struct VertexInformation
{
	uint32 VertexOffset;
	uint32 NumVertices;
};

struct IndexInformation
{
	uint32 IndexOffset;
	uint32 NumIndices;
};

struct DrawInfo
{
	math::mat4 Transform;
	Handle<SRenderPass> Renderpass;
	VertexInformation* pVertexInformation;
	IndexInformation* pIndexInformation;
  Handle<SPipeline> PipelineHnd;
  void* pConstants;
  size_t ConstantSize;
	uint32 DrawableCount; // Num of meshes in the model.
  uint32 ConstantsCount;
	uint32 Id;
	bool Instanced = true;
};

class RENDERER_API IRenderSystem : public SystemInterface
{
private:

	friend struct IStagingParams;
	friend class IStagingManager;
	friend class IFrameGraph;
	
	struct DrawManager
	{
		struct DrawCommandRange
		{
			ForwardNode<DrawCommand>* pBegin;
			ForwardNode<DrawCommand>* pEnd;
		};

		struct TransformRange
		{
			ForwardNode<math::mat4>* pBegin;
			ForwardNode<math::mat4>* pEnd;
		};

		struct InstanceEntry
		{
			uint32 Id;
			TransformRange TransformRange;
			DrawCommandRange DrawRange;
		};

		using EntryContainer = StaticArray<InstanceEntry, MAX_INSTANCING_COUNT>;

		static constexpr uint32 MaxDrawablesCount = MAX_DRAWABLE_COUNT;

		// NOTE(Ygsm): Might swap these out for a static array in the future but we'll go with this for now.
		static LinearAllocator _DrawCommandAllocator;
		static LinearAllocator _TransformAllocator;
		static Map<size_t, EntryContainer> _InstancedDraws;
		static Map<size_t, EntryContainer> _NonInstancedDraws;
		static uint32 _NumDrawables;
	};
	
	struct BindableManager
	{
		struct BindableRange
		{
			ForwardNode<SBindable>* pBegin;
			ForwardNode<SBindable>* pEnd;
		};
		// NOTE(Ygsm): Might swap this out for a static array in the future but we'll go with this for now.
		static LinearAllocator _BindableAllocator;
		static Map<size_t, BindableRange> _Bindables;
		static BindableRange _GlobalBindables;
	};

	using DefaultBuffer = Pair<Handle<SMemoryBuffer>, Ref<SMemoryBuffer>>;

	EngineImpl& Engine;
	IRenderDevice* pDevice;
	IDeviceStore* pStore;
	IStagingManager* pStaging;
	IFrameGraph* pFrameGraph;
	Handle<ISystem>	Hnd;

	DefaultBuffer VertexBuffer;
	DefaultBuffer IndexBuffer;
	DefaultBuffer InstanceBuffer;

	Map<size_t, DrawManager::DrawCommandRange> Drawables;

	uint32 CalcStrideForFormat(EShaderAttribFormat Format);
	void PreprocessShader(Ref<SShader> pShader);

	void IterateBindableRange(BindableManager::BindableRange& Range);

	void BindBindable(SBindable& Bindable);
	void BindBindablesForRenderpass(Handle<SRenderPass> Hnd);

	void DynamicStateSetup(Ref<SRenderPass> pRenderPass);

	void BindBuffers();
	void BeginRenderPass(Ref<SRenderPass> pRenderPass);
	void EndRenderPass(Ref<SRenderPass> pRenderPass);

	void RecordDrawCommand(const DrawCommand& Command);
  void BindPushConstant(const DrawCommand& Command);

	void Clear();

	uint64 GenHashForImageSampler(const ImageSamplerState& State);

	void BlitToDefault();

	void CopyToBuffer(Ref<SMemoryBuffer> pBuffer, void* Data, size_t Size, size_t Offset, bool Update = false);

	uint32 GetImageUsageFlags(Ref<SImage> pImg);
	uint32 GetImageFormat(Ref<SImage> pImg);

public:

	IRenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd);
	~IRenderSystem();

	DELETE_COPY_AND_MOVE(IRenderSystem)

	void OnInit			() override;
	void OnUpdate		() override;
	void OnTerminate	() override;

	size_t PadToAlignedSize(size_t Size);

	Handle<SDescriptorPool> CreateDescriptorPool();
	bool DescriptorPoolAddSizeType(Handle<SDescriptorPool> Hnd, SDescriptorPool::Size Type);
	bool BuildDescriptorPool(Handle<SDescriptorPool> Hnd);
	bool DestroyDescriptorPool(Handle<SDescriptorPool>& Hnd);

	Handle<SDescriptorSetLayout> CreateDescriptorSetLayout();
	bool DescriptorSetLayoutAddBinding(const DescriptorSetLayoutBindingInfo& BindInfo);
	bool BuildDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd);
	bool DestroyDescriptorSetLayout(Handle<SDescriptorSetLayout>& Hnd);

	Handle<SDescriptorSet> CreateDescriptorSet(const DescriptorSetAllocateInfo& AllocInfo);
	bool DescriptorSetMapToBuffer(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SMemoryBuffer> BufferHnd, size_t Offset0 = 0, size_t Offset1 = 0);
  bool DescriptorSetMapToImage(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SImage>* pImgHnd, uint32 NumImages, Handle<SImageSampler> ImgSamplerHnd, uint32 DstIndex = 0);
	bool DescriptorSetBindToGlobal(Handle<SDescriptorSet> Hnd);
	bool BuildDescriptorSet(Handle<SDescriptorSet> Hnd);
	bool DescriptorSetFlushBindingOffset(Handle<SDescriptorSet> Hnd);
	bool DestroyDescriptorSet(Handle<SDescriptorSet>& Hnd);

	bool DescriptorSetUpdateDataAtBinding(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, void* Data, size_t Size);
	void UpdateDescriptorSetInQueue();

	Handle<SMemoryBuffer> AllocateNewBuffer(const BufferAllocateInfo& AllocInfo);
	bool CopyDataToBuffer(Handle<SMemoryBuffer> Hnd, void* Data, size_t Size, size_t Offset);
	bool BuildBuffer(Handle<SMemoryBuffer> Hnd);
	bool DestroyBuffer(Handle<SMemoryBuffer>& Hnd);
	
	Handle<SImage> CreateImage(uint32 Width, uint32 Height, uint32 Channels, ETextureType Type, bool GenMips = false);
	bool BuildImage(Handle<SImage> Hnd);
	bool DestroyImage(Handle<SImage>& Hnd);

	Handle<SImageSampler> CreateImageSampler(const ImageSamplerCreateInfo& CreateInfo);
	bool BuildImageSampler(Handle<SImageSampler> Hnd);
	bool DestroyImageSampler(Handle<SImageSampler>& Hnd);

	IStagingManager& GetStagingManager() const;
	IFrameGraph& GetFrameGraph() const;

	Handle<SShader> CreateShader(const String& Code, EShaderType Type);
	bool BuildShader(Handle<SShader> Hnd);
	bool DestroyShader(Handle<SShader>& Hnd);

	Handle<SPipeline> CreatePipeline(const PipelineCreateInfo& CreateInfo);
	bool PipelineAddVertexInputBinding(Handle<SPipeline> Hnd, SPipeline::VertexInputBinding Input);
	bool PipelineAddRenderPass(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderPass);
	bool PipelineAddDescriptorSetLayout(Handle<SPipeline> Hnd, Handle<SDescriptorSetLayout> Layout);
	bool BuildGraphicsPipeline(Handle<SPipeline> Hnd);
	bool DestroyPipeline(Handle<SPipeline>& Hnd);

	bool BindDescriptorSet(Handle<SDescriptorSet> Hnd, Handle<SRenderPass> RenderpassHnd);
	bool BindPipeline(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderpassHnd);

	bool Draw(const DrawInfo& Info);
	const uint32 GetMaxDrawablesCount() const;
	const uint32 GetDrawableCount() const;
	const uint32 GetCurrentFrameIndex() const;

	void PrepareDrawCommands();
	void MakeTransferToGpu();

	void FlushVertexBuffer();
	void FlushIndexBuffer();
	void FlushInstanceBuffer();

	static Handle<ISystem> GetSystemHandle();
};

namespace ao
{
	RENDERER_API IRenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H
