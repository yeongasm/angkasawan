#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H

#include "Library/Containers/Array.h"
#include "Library/Containers/Pair.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
//#include "API/Device.h"
#include "Primitives.h"

class IRenderSystem;
class IRenderDevice;

/**
* NOTE(Ygsm):
* Staging buffer will be a thin abstraction layer for transferring resources.
* 
* Vertex, index and instance buffer will perform an ownership transfer when data is uploaded.
*/
class RENDERER_API IStagingManager
{
private:

	friend class IRenderSystem;

	//enum EStagingOp : uint32
	//{
	//	Staging_Op_Upload = 0,
	//	Staging_Op_Ownership_Transfer = 1,
	//	Staging_Op_Max = 2
	//};

	//using StagingBuffer = Pair<Handle<SMemoryBuffer>, SMemoryBuffer*>;
	//using VulkanQueue = typename IRenderDevice::VulkanQueue;

	//VulkanQueue* pQueue;
	//IRenderSystem& Renderer;
	//VkCommandPool Pool;
	//VkCommandBuffer CmdBuf[Staging_Op_Max][MAX_TRANSFER_COMMANDS];
	//VkSemaphore Semaphore[MAX_FRAMES_IN_FLIGHT];
	//VkFence Fences[MAX_FRAMES_IN_FLIGHT];
	//StagingBuffer Buffer[MAX_FRAMES_IN_FLIGHT];
	//uint32 NextCmdIndex;
	//bool TransferDefaults;

	bool DrawDataUploaded() const;
	void ResetTransferDefaultFlag();

	size_t PadToAlignedSize(size_t Size);
	void FlushBuffer(SMemoryBuffer& Buffer);
	bool CanContentsFit(const SMemoryBuffer& Buffer, size_t Size);
	void IncrementCommandIndex();

	//void BufferBarrier(VkCommandBuffer Cmd, SMemoryBuffer* pBuffer, size_t Size, VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, uint32 SrcQueue, uint32 DstQueue);
	//void ImageBarrier(VkCommandBuffer Cmd, SImage* pImage, VkImageSubresourceRange* pSubRange, VkImageLayout OldLayout, VkImageLayout NewLayout, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, uint32 SrcQueue, uint32 DstQueue);

	//const VulkanQueue* GetQueueForType(EQueueType QueueType) const;

public:

	IStagingManager(IRenderSystem& InRenderer);
	~IStagingManager();

	DELETE_COPY_AND_MOVE(IStagingManager)

	bool Initialize(size_t StagingBufferSize);
	void Terminate();

	void ResetStagingBuffer(uint32 Index);

	void BeginStaging();
	size_t StageVertexData(void* Data, size_t Size);
	size_t StageIndexData(void* Data, size_t Size);
	//size_t StageInstanceData(void* Data, size_t Size);
	size_t StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd, EQueueType DstQueue);
	bool StageDataForImage(void* Data, size_t Size, Handle<SImage> DstImg, EQueueType DstQueue);
	void EndStaging(bool SignalSemaphore = true);

	size_t GetVertexBufferOffset() const;
	size_t GetIndexBufferOffset() const;
	//size_t GetInstanceBufferOffset() const;

	void BeginTransfer();
	bool TransferBufferOwnership(Handle<SMemoryBuffer> Hnd, EQueueType Queue);
	bool TransferImageOwnership(Handle<SImage> Hnd, EQueueType Queue);
	void EndTransfer();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H