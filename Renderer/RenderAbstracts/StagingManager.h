#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H

#include "Library/Containers/Array.h"
#include "Library/Containers/Pair.h"
#include "SubSystem/Resource/Handle.h"
#include "API/Device.h"
#include "Primitives.h"

class IRenderSystem;

/**
* NOTE(Ygsm):
* Staging buffer will be a thin abstraction layer for transferring resources.
* 
* Vertex, index and instance buffer will perform an ownership transfer when data is uploaded.
*/
class IStagingManager
{
private:

	using StagingBuffer = Pair<Handle<SMemoryBuffer>, SMemoryBuffer*>;
	
	IRenderDevice::VulkanQueue* pQueue;
	IRenderSystem& Renderer;
	VkCommandPool Pool;
	VkCommandBuffer TransferCmd[MAX_TRANSFER_COMMANDS];
	VkCommandBuffer GraphicsCmd[MAX_TRANSFER_COMMANDS];
	VkSemaphore Semaphore[MAX_FRAMES_IN_FLIGHT];
	VkFence Fences[MAX_FRAMES_IN_FLIGHT];
	StagingBuffer Buffer[MAX_FRAMES_IN_FLIGHT];
	uint32 NextCmdIndex;
	bool TransferDefaults;

	size_t PadToAlignedSize(size_t Size);
	void FlushBuffer(SMemoryBuffer& Buffer);
	bool CanContentsFit(const SMemoryBuffer& Buffer, size_t Size);
	void IncrementCommandIndex();

	void BufferBarrier(VkCommandBuffer Cmd, SMemoryBuffer* pBuffer, size_t Size, VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, uint32 SrcQueue, uint32 DstQueue);
	void ImageBarrier(VkCommandBuffer Cmd, SImage* pImage, VkImageSubresourceRange* pSubRange, VkImageLayout OldLayout, VkImageLayout NewLayout, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, uint32 SrcQueue, uint32 DstQueue);

public:

	IStagingManager(IRenderSystem& InRenderer);
	~IStagingManager();

	DELETE_COPY_AND_MOVE(IStagingManager)

	bool Initialize(size_t StagingBufferSize);
	void Terminate();

	void ResetStagingBuffer(uint32 Index);

	void BeginStaging();
	bool StageVertexData(void* Data, size_t Size);
	bool StageIndexData(void* Data, size_t Size);
	bool StageInstanceData(void* Data, size_t Size);
	bool StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd);
	bool StageDataForImage(void* Data, size_t Size, Handle<SImage> DstImg);
	void EndStaging();

	void BeginTransfer(EQueueType QueueType);
	void TransferBufferOwnership(Handle<SMemoryBuffer> Hnd, EQueueType Queue);
	void TransferImageOwnership(Handle<SImage> Hnd, EQueueType Queue);
	void EndTransfer(EQueueType QueueType);
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H