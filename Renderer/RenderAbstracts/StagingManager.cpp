#include "StagingManager.h"
#include "Renderer.h"

size_t IStagingManager::PadToAlignedSize(size_t Size)
{
	const size_t minUboAlignment = Renderer.Device->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	return (Size + minUboAlignment - 1) & ~(minUboAlignment - 1);
}

void IStagingManager::FlushBuffer(SMemoryBuffer& Buffer)
{
	IMemory::Memzero(Buffer.pData, Buffer.Size);
	Buffer.Offset = 0;
}

bool IStagingManager::CanContentsFit(const SMemoryBuffer& Buffer, size_t Size)
{
	return (Buffer.Offset + PadToAlignedSize(Size) < Buffer.Size);
}

void IStagingManager::IncrementCommandIndex()
{
	NextCmdIndex = (NextCmdIndex + 1) / MAX_TRANSFER_COMMANDS;
}

//void IStagingManager::BufferBarrier(
//	VkCommandBuffer Cmd, 
//	SMemoryBuffer* pBuffer, 
//	size_t Size, 
//	VkAccessFlags SrcAccessMask, 
//	VkAccessFlags DstAccessMask, 
//	VkPipelineStageFlags SrcStageMask, 
//	VkPipelineStageFlags DstStageMask, 
//	uint32 SrcQueue, 
//	uint32 DstQueue
//)
//{
//	VkBufferMemoryBarrier barrier = {};
//	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
//	barrier.buffer = pBuffer->Hnd;
//	barrier.size = Size;
//	barrier.offset = pBuffer->Offset;
//	barrier.srcAccessMask = SrcAccessMask;
//	barrier.dstAccessMask = DstAccessMask;
//	barrier.srcQueueFamilyIndex = SrcQueue;
//	barrier.dstQueueFamilyIndex = DstQueue;
//
//	vkCmdPipelineBarrier(Cmd, SrcStageMask, DstStageMask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
//}

//void IStagingManager::ImageBarrier(
//	VkCommandBuffer Cmd, 
//	SImage* pImage, 
//	VkImageSubresourceRange* pSubRange, 
//	VkImageLayout OldLayout, 
//	VkImageLayout NewLayout, 
//	VkPipelineStageFlags SrcStageMask, 
//	VkPipelineStageFlags DstStageMask, 
//	uint32 SrcQueue, 
//	uint32 DstQueue
//)
//{
//	VkImageMemoryBarrier barrier = {};
//	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//	barrier.oldLayout = OldLayout;
//	barrier.newLayout = NewLayout;
//	barrier.image = pImage->ImgHnd;
//	barrier.subresourceRange = *pSubRange;
//	barrier.srcAccessMask = 0;
//	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	barrier.srcQueueFamilyIndex = SrcQueue;
//	barrier.dstQueueFamilyIndex = DstQueue;
//
//	vkCmdPipelineBarrier(Cmd, SrcStageMask, DstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
//}

const IRenderDevice::VulkanQueue* IStagingManager::GetQueueForType(EQueueType QueueType) const
{
	const IRenderDevice::VulkanQueue* queue = nullptr;
	switch (QueueType)
	{
	case EQueueType::Queue_Type_Graphics:
		queue = &Renderer.Device->GetGraphicsQueue();
		break;
	case EQueueType::Queue_Type_Transfer:
	default:
		queue = pQueue;
		break;
	}
	return queue;
}

IStagingManager::IStagingManager(IRenderSystem& InRenderer) :
	pQueue(nullptr),
	Renderer(InRenderer),
	Pool{},
	CmdBuf{VK_NULL_HANDLE},
	Semaphore{},
	Fences{},
	Buffer{},
	TransferDefaults(false),
	NextCmdIndex(0)
{}

IStagingManager::~IStagingManager()
{}

bool IStagingManager::Initialize(size_t StagingBufferSize)
{
	pQueue = const_cast<IRenderDevice::VulkanQueue*>(&Renderer.Device->GetTransferQueue());
	Pool = Renderer.Device->CreateCommandPool(pQueue->FamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	if (Pool == VK_NULL_HANDLE) 
	{ 
		VKT_ASSERT(false && "Unable to create command pool.");
		return false; 
	}

	for (size_t i = 0; i < Staging_Op_Max; i++)
	{
		for (size_t j = 0; j < MAX_TRANSFER_COMMANDS; j++)
		{
			CmdBuf[i][j] = Renderer.Device->AllocateCommandBuffer(Pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		}
	}

	BufferAllocateInfo allocInfo = {};
	allocInfo.Locality = Buffer_Locality_Cpu;
	allocInfo.Type.Set(Buffer_Type_Vertex);
	allocInfo.Type.Set(Buffer_Type_Index);
	allocInfo.Type.Set(Buffer_Type_Transfer_Src);
	allocInfo.Type.Set(Buffer_Type_Uniform);
	allocInfo.Size = StagingBufferSize;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		Handle<SMemoryBuffer> hnd = Renderer.AllocateNewBuffer(allocInfo);
		Buffer[i] = StagingBuffer(
			hnd, 
			Renderer.Store->GetBuffer(hnd)
		);
		Semaphore[i] = Renderer.Device->CreateVkSemaphore(nullptr);
		Fences[i] = Renderer.Device->CreateFence();
	}
}

void IStagingManager::Terminate()
{
	Renderer.Device->DeviceWaitIdle();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		Renderer.DestroyBuffer(Buffer[i].Key);
		Renderer.Device->DestroyVkSemaphore(Semaphore[i]);
		Renderer.Device->DestroyFence(Fences[i]);
	}

	for (size_t i = 0; i < Staging_Op_Max; i++)
	{
		for (size_t j = 0; j < MAX_TRANSFER_COMMANDS; j++)
		{
			Renderer.Device->ResetCommandBuffer(CmdBuf[i][j], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		}
	}

	Renderer.Device->DestroyCommandPool(Pool);
}

void IStagingManager::ResetStagingBuffer(uint32 Index)
{
	VKT_ASSERT(Index < MAX_FRAMES_IN_FLIGHT && "Index supplied exceeded buffer count.");
	auto [handle, pBuffer] = Buffer[Index];
	FlushBuffer(*pBuffer);
}

void IStagingManager::BeginStaging()
{
	const uint32 index = Renderer.Device->GetCurrentFrameIndex();
	VkCommandBuffer cmd = CmdBuf[Staging_Op_Upload][NextCmdIndex];

	Renderer.Device->WaitFence(Fences[index]);
	Renderer.Device->ResetFence(Fences[index]);
	Renderer.Device->BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

bool IStagingManager::StageVertexData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = Renderer.VertexBuffer;
	if (hnd == INVALID_HANDLE) { return false; }
	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
}

bool IStagingManager::StageIndexData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = Renderer.IndexBuffer;
	if (hnd == INVALID_HANDLE) { return false; }
	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
}

bool IStagingManager::StageInstanceData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = Renderer.InstanceBuffer;
	if (hnd == INVALID_HANDLE) { return false; }
	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
}

bool IStagingManager::StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd, EQueueType DstQueue)
{
	VkCommandBuffer cmd = CmdBuf[Staging_Op_Upload][NextCmdIndex];
	const IRenderDevice::VulkanQueue* dstQueue = GetQueueForType(DstQueue);

	auto [hnd, pStaging] = Buffer[Renderer.Device->GetCurrentFrameIndex()];
	SMemoryBuffer* pBuffer = Renderer.Store->GetBuffer(DstHnd);
	if (!pBuffer) { return false; }
	if (Size > pStaging->Size) 
	{ 
		VKT_ASSERT(false && "Data size exceeded capacity allowed by staging buffer.");
		return false; 
	}

	if (!CanContentsFit(*pStaging, Size))
	{
		// EndStaging() submits to the queue and signals a semaphore to be used for ownership transfer
		// In the rare event that the staging buffer is full, I need to do another submission and
		// I would like the queue to wait for the previous one to complete.
		// Ideally, this operation should not stall the CPU.
		EndStaging(false);
		BeginStaging();
	}

	uint8* pData = pStaging->pData;
	pData += pStaging->Offset;
	IMemory::Memcpy(&pData, &Data, Size);

	VkBufferCopy copy = {};
	copy.size = Size;
	copy.srcOffset = pStaging->Offset;
	copy.dstOffset = pBuffer->Offset;

	vkCmdCopyBuffer(cmd, pStaging->Hnd, pBuffer->Hnd, 1, &copy);

	Renderer.Device->BufferBarrier(
		cmd,
		pBuffer->Hnd,
		Size,
		pBuffer->Offset,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		pQueue->FamilyIndex,
		dstQueue->FamilyIndex
	);

	pStaging->Offset += PadToAlignedSize(Size);
	pBuffer->Offset += PadToAlignedSize(Size);

	return true;
}

bool IStagingManager::StageDataForImage(void* Data, size_t Size, Handle<SImage> DstImg, EQueueType DstQueue)
{
	VkCommandBuffer cmd = CmdBuf[Staging_Op_Upload][NextCmdIndex];
	const IRenderDevice::VulkanQueue* dstQueue = GetQueueForType(DstQueue);

	auto [hnd, pStaging] = Buffer[Renderer.Device->GetCurrentFrameIndex()];
	SImage* pImage = Renderer.Store->GetImage(DstImg);
	if (!pImage) { return false; }
	if (Size > pStaging->Size)
	{
		VKT_ASSERT(false && "Data size exceeded capacity allowed by staging buffer.");
		return false;
	}

	if (!CanContentsFit(*pStaging, Size))
	{
		// EndStaging() submits to the queue and signals a semaphore to be used for ownership transfer
		// In the rare event that the staging buffer is full, I need to do another submission and
		// I would like the queue to wait for the previous one to complete.
		// Ideally, this operation should not stall the CPU.
		EndStaging(false);
		BeginStaging();
	}

	uint8* pData = pStaging->pData;
	pData += pStaging->Offset;
	IMemory::Memcpy(&pData, &Data, Size);

	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.baseArrayLayer = 0;
	range.levelCount = 1;
	range.layerCount = 1;

	Renderer.Device->ImageBarrier(
		cmd, 
		pImage->ImgHnd, 
		&range, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		pQueue->FamilyIndex, 
		pQueue->FamilyIndex
	);

	VkBufferImageCopy copy = {};
	copy.bufferOffset = pStaging->Offset;
	copy.bufferRowLength = 0;
	copy.bufferImageHeight = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.imageSubresource.mipLevel = 0;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.layerCount = 1;
	copy.imageExtent = { pImage->Width, pImage->Height, 1 };

	vkCmdCopyBufferToImage(cmd, pStaging->Hnd, pImage->ImgHnd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

	// TODO(Ygsm):
	// To include mip map generation for images.

	Renderer.Device->ImageBarrier(
		cmd, 
		pImage->ImgHnd, 
		&range, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		pQueue->FamilyIndex, 
		dstQueue->FamilyIndex
	);
	
	pStaging->Offset += PadToAlignedSize(Size);

	return true;
}

void IStagingManager::EndStaging(bool SignalSemaphore)
{
	const uint32 index = Renderer.Device->GetCurrentFrameIndex();
	VkCommandBuffer cmd = CmdBuf[Staging_Op_Upload][NextCmdIndex];

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmd;

	if (SignalSemaphore)
	{
		VkSemaphore semaphore = Semaphore[index];

		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &semaphore;
	}

	Renderer.Device->EndCommandBuffer(cmd);
	vkQueueSubmit(pQueue->Hnd, 1, &info, Fences[index]);

	IncrementCommandIndex();
	ResetStagingBuffer(index);
}

void IStagingManager::BeginTransfer()
{
	Renderer.Device->BeginCommandBuffer(
		CmdBuf[Staging_Op_Ownership_Transfer][NextCmdIndex],
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	);
}

bool IStagingManager::TransferBufferOwnership(Handle<SMemoryBuffer> Hnd, EQueueType Queue)
{
	VKT_ASSERT((Queue != EQueueType::Queue_Type_Present && Queue != EQueueType::Queue_Type_Transfer) &&
		"Ownership transfer is not allowed for the presentation queue and the transfer queue!");

	SMemoryBuffer* pBuffer = Renderer.Store->GetBuffer(Hnd);
	if (!pBuffer) { return false; }

	const IRenderDevice::VulkanQueue* dstQueue = GetQueueForType(Queue);
	if (!dstQueue) { return false; }

	VkAccessFlags dstMask = VK_ACCESS_MEMORY_READ_BIT;

	if (pBuffer->Type.Has(Buffer_Type_Vertex))
	{
		dstMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	}

	if (pBuffer->Type.Has(Buffer_Type_Index))
	{
		dstMask = VK_ACCESS_INDEX_READ_BIT;
	}

	Renderer.Device->BufferBarrier(
		CmdBuf[Staging_Op_Ownership_Transfer][NextCmdIndex], 
		pBuffer->Hnd, 
		pBuffer->Size,
		pBuffer->Offset,
		VK_ACCESS_TRANSFER_WRITE_BIT, 
		dstMask, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 
		dstQueue->FamilyIndex, 
		dstQueue->FamilyIndex
	);

	return true;
}

bool IStagingManager::TransferImageOwnership(Handle<SImage> Hnd, EQueueType Queue)
{
	VKT_ASSERT((Queue != EQueueType::Queue_Type_Present && Queue != EQueueType::Queue_Type_Transfer) &&
		"Ownership transfer is not allowed for the presentation queue and the transfer queue!");

	SImage* pImage = Renderer.Store->GetImage(Hnd);
	if (!pImage) { return false; }

	const IRenderDevice::VulkanQueue* dstQueue = GetQueueForType(Queue);
	if (!dstQueue) { return false; }

	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.baseArrayLayer = 0;
	range.levelCount = 1;
	range.layerCount = 1;

	Renderer.Device->ImageBarrier(
		CmdBuf[Staging_Op_Ownership_Transfer][NextCmdIndex],
		pImage->ImgHnd,
		&range,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		dstQueue->FamilyIndex,
		dstQueue->FamilyIndex
	);

	return true;
}

void IStagingManager::EndTransfer()
{
	const uint32 index = Renderer.Device->GetCurrentFrameIndex();
	const IRenderDevice::VulkanQueue* pGfxQueue = &Renderer.Device->GetGraphicsQueue();
	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &CmdBuf[Staging_Op_Ownership_Transfer][NextCmdIndex];
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &Semaphore[index];
	info.pWaitDstStageMask = &waitDstStageMask;

	Renderer.Device->EndCommandBuffer(CmdBuf[Staging_Op_Ownership_Transfer][NextCmdIndex]);
	vkQueueSubmit(pGfxQueue->Hnd, 1, &info, VK_NULL_HANDLE);
}
