#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H

#include "Library/Containers/Array.h"
#include "Library/Containers/Pair.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "Primitives.h"

class IRenderSystem;
class IRenderDevice;

using VkCommandPool = struct VkCommandPool_T*;
using VkCommandBuffer = struct VkCommandBuffer_T*;
using VkBuffer = struct VkBuffer_T*;

class RENDERER_API IStagingManager
{
private:

	friend class IRenderSystem;

	struct UploadContext
	{
		SMemoryBuffer SrcBuffer;
		astl::Ref<SMemoryBuffer> pDstBuf;
    astl::Ref<SImage> pDstImg;
		size_t Size;
    size_t DstOffset;
    EStagingUploadType Type;
		EQueueType DstQueue;
	};

  struct OwnershipTransferContext
  {
    astl::Ref<SMemoryBuffer> pBuffer;
    astl::Ref<SImage> pImage;
    EStagingUploadType Type;

    OwnershipTransferContext();
    OwnershipTransferContext(astl::Ref<SMemoryBuffer> pInBuffer);
    OwnershipTransferContext(astl::Ref<SImage> pInImage);
    ~OwnershipTransferContext();
  };

	astl::Array<UploadContext> Uploads;
  astl::Array<OwnershipTransferContext> OwnershipTransfers;
	astl::Ref<IRenderSystem> pRenderer;
	VkCommandPool TxPool;
  astl::BitSet<EOwnershipTransferTypeFlagBits> MakeTransfers;
  bool HasUpload;

	decltype(auto) GetQueueForType(EQueueType Type);

	//bool ShouldMakeTransfer() const;
  void UploadToBuffer(VkCommandBuffer CmdBuffer, UploadContext& Ctx);
  void UploadToImage(VkCommandBuffer CmdBuffer, UploadContext& Ctx);

  const astl::Array<OwnershipTransferContext>& GetOwnershipTransfers() const;
  void ClearOwnershipTransfers();

public:

	IStagingManager(IRenderSystem& InRenderer);
	~IStagingManager();

	DELETE_COPY_AND_MOVE(IStagingManager)

	bool Initialize();
	void Terminate();
	bool StageVertexData(void* Data, size_t Size);
	bool StageIndexData(void* Data, size_t Size);
	bool StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd, EQueueType DstQueue);
  bool StageDataForImage(void* Data, size_t Size, Handle<SImage> DstHnd, EQueueType DstQueue);
	bool Upload();

  size_t GetVertexBufferOffset();
  size_t GetIndexBufferOffset();

  // NOTE(Ygsm):
  // For now, we will only transfer to the graphics queue.
  // Will be enhanced in the future.
  bool TransferBufferOwnership(Handle<SMemoryBuffer> Hnd);
  bool TransferImageOwnership(Handle<SImage> Hnd);
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H
