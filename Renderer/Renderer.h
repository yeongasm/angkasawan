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
#include "RenderAbstracts/DrawCommand.h"

class IRenderDevice;
struct IDeviceStore;

class RENDERER_API IRenderSystem : public SystemInterface
{
private:

	friend struct IStagingParams;
	friend class IStagingManager;
	friend class IFrameGraph;
	
	struct BindableManager
	{
		struct BindableRange
		{
			astl::ForwardNode<SBindable>* pBegin;
			astl::ForwardNode<SBindable>* pEnd;
		};
		// NOTE(Ygsm): Might swap this out for a static array in the future but we'll go with this for now.
		static astl::LinearAllocator _BindableAllocator;
		static astl::Map<size_t, BindableRange> _Bindables;
		static BindableRange _GlobalBindables;
	};

  struct BufferCopyCommand
  {
    void* pSrc;
    size_t SrcSize;
    astl::Ref<SMemoryBuffer> pDst;
    size_t DstOffset;

    BufferCopyCommand() :
      pSrc{}, SrcSize{}, pDst{}, DstOffset{}
    {}

    BufferCopyCommand(void* pInSrc, size_t InSrcSize, astl::Ref<SMemoryBuffer> pInDst, size_t InDstOffset) :
      pSrc{ pInSrc }, SrcSize{ InSrcSize }, pDst{ pInDst }, DstOffset{ InDstOffset }
    {}

    ~BufferCopyCommand() {}
  };

	using DefaultBuffer = RefHnd<SMemoryBuffer>;

  astl::Array<astl::Array<DrawCommand>> Drawables;
  astl::Array<SBuildCommand> BuildCommandQueue;
  astl::Array<BufferCopyCommand> CopyCommandQueue;

	EngineImpl& Engine;
  DefaultBuffer VertexBuffer;
  DefaultBuffer IndexBuffer;
  DefaultBuffer InstanceBuffer;
  astl::UniquePtr<IRenderDevice> pDevice;
  astl::UniquePtr<IDeviceStore> pStore;
  astl::UniquePtr<IStagingManager> pStaging;
  astl::UniquePtr<IFrameGraph> pFrameGraph;
	Handle<ISystem>	Hnd;

	uint32 CalcStrideForFormat(EShaderAttribFormat Format);
	void PreprocessShader(astl::Ref<SShader> pShader);
	void IterateBindableRange(BindableManager::BindableRange& Range);
	void BindBindable(SBindable& Bindable);
	void BindBindablesForRenderpass(Handle<SRenderPass> Hnd);
	void DynamicStateSetup(astl::Ref<SRenderPass> pRenderPass);
	void BindBuffers();
	void BeginRenderPass(astl::Ref<SRenderPass> pRenderPass);
	void EndRenderPass(astl::Ref<SRenderPass> pRenderPass);
	void RecordDrawCommand(const DrawCommand& Command);
  void BindPushConstant(const DrawCommand& Command);

	uint64 GenHashForImageSampler(const ImageSamplerState& State);

	void Clear();
	void BlitToDefault();

	//void CopyToBuffer(astl::Ref<SMemoryBuffer> pBuffer, void* Data, size_t Size, size_t Offset, bool Update = false);

	uint32 GetImageUsageFlags(astl::Ref<SImage> pImg);

  // Build Functions ... 
	void BuildDescriptorPool(astl::Ref<SDescriptorPool> pPool);
	void BuildDescriptorSetLayout(astl::Ref<SDescriptorSetLayout> pSetLayout);
	void BuildDescriptorSet(astl::Ref<SDescriptorSet> pSet);
	void BuildBuffer(astl::Ref<SMemoryBuffer> pBuffer);
	void BuildImage(astl::Ref<SImage> pImg);
	void BuildImageSampler(astl::Ref<SImageSampler> pImgSampler);
	void BuildShader(astl::Ref<SShader> pShader);
	void BuildGraphicsPipeline(astl::Ref<SPipeline> pPipeline);

  void BufferToGraphicsQueue(VkCommandBuffer CmdBuffer, astl::Ref<SMemoryBuffer> pBuffer, uint32 AccessFlag);
  void ImageToGraphicsQueue(VkCommandBuffer CmdBuffer, astl::Ref<SImage> pImage);
  void GenerateImageMipMaps(VkCommandBuffer CmdBuffer, astl::Ref<SImage> pImage);

  void BuildResourcesInQueue();
  void CopyBuffersInQueue();
  void BeforeBeginFrame();

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
	bool DestroyDescriptorPool(Handle<SDescriptorPool>& Hnd);

	Handle<SDescriptorSetLayout> CreateDescriptorSetLayout();
	bool DescriptorSetLayoutAddBinding(const DescriptorSetLayoutBindingInfo& BindInfo);
	bool DestroyDescriptorSetLayout(Handle<SDescriptorSetLayout>& Hnd);

	Handle<SDescriptorSet> CreateDescriptorSet(const DescriptorSetAllocateInfo& AllocInfo);
	bool DescriptorSetMapToBuffer(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SMemoryBuffer> BufferHnd, size_t Offset0 = 0, size_t Offset1 = 0);
  bool DescriptorSetMapToImage(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SImage>* pImgHnd, uint32 NumImages, Handle<SImageSampler> ImgSamplerHnd, uint32 DstIndex = 0);
	bool DescriptorSetBindToGlobal(Handle<SDescriptorSet> Hnd);
	bool DescriptorSetFlushBindingOffset(Handle<SDescriptorSet> Hnd);
	bool DestroyDescriptorSet(Handle<SDescriptorSet>& Hnd);

	bool DescriptorSetUpdateDataAtBinding(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, void* Data, size_t Size);
	void UpdateDescriptorSetInQueue();

	Handle<SMemoryBuffer> AllocateNewBuffer(const BufferAllocateInfo& AllocInfo);
  size_t GetBufferOffset(Handle<SMemoryBuffer> Hnd);

  bool BufferBindToGlobal(Handle<SMemoryBuffer> Hnd);
  bool BufferBindToPass(Handle<SMemoryBuffer> BufferHnd, Handle<SRenderPass> RenderpassHnd);

  void ResetBufferOffset(Handle<SMemoryBuffer> Hnd);

  // WARNING: Only use this for CPU side buffers. GPU side buffers require data to be staged with the staging manager.
	bool CopyDataToBuffer(Handle<SMemoryBuffer> Hnd, void* Data, size_t Size, size_t Offset, bool UpdateBufferOffset = false);
	bool DestroyBuffer(Handle<SMemoryBuffer>& Hnd);
	
	Handle<SImage> CreateImage(uint32 Width, uint32 Height, uint32 Channels, ETextureType Type, ETextureFormat Format, bool GenMips = false);
	bool DestroyImage(Handle<SImage>& Hnd);

	Handle<SImageSampler> CreateImageSampler(const ImageSamplerCreateInfo& CreateInfo);
	bool DestroyImageSampler(Handle<SImageSampler>& Hnd);

	IStagingManager& GetStagingManager();
	IFrameGraph& GetFrameGraph();

	Handle<SShader> CreateShader(const astl::String& Code, EShaderType Type);
	bool DestroyShader(Handle<SShader>& Hnd);

	Handle<SPipeline> CreatePipeline(const PipelineCreateInfo& CreateInfo);
	bool PipelineAddVertexInputBinding(Handle<SPipeline> Hnd, SPipeline::VertexInputBinding Input);
	bool PipelineAddRenderPass(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderPass);
	bool PipelineAddDescriptorSetLayout(Handle<SPipeline> Hnd, Handle<SDescriptorSetLayout> Layout);
	bool DestroyPipeline(Handle<SPipeline>& Hnd);

	bool BindDescriptorSet(Handle<SDescriptorSet> Hnd, Handle<SRenderPass> RenderpassHnd);
	bool BindPipeline(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderpassHnd);

	void Draw(const DrawSubmissionInfo& Info);
	const uint32 GetCurrentFrameIndex() const;

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
