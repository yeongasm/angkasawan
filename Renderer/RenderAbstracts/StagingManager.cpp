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

void IStagingManager::BufferBarrier(
	VkCommandBuffer Cmd, 
	SMemoryBuffer* pBuffer, 
	size_t Size, 
	VkAccessFlags SrcAccessMask, 
	VkAccessFlags DstAccessMask, 
	VkPipelineStageFlags SrcStageMask, 
	VkPipelineStageFlags DstStageMask, 
	uint32 SrcQueue, 
	uint32 DstQueue
)
{
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = pBuffer->Hnd;
	barrier.size = Size;
	barrier.offset = pBuffer->Offset;
	barrier.srcAccessMask = SrcAccessMask;
	barrier.dstAccessMask = DstAccessMask;
	barrier.srcQueueFamilyIndex = SrcQueue;
	barrier.dstQueueFamilyIndex = DstQueue;

	vkCmdPipelineBarrier(Cmd, SrcStageMask, DstStageMask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void IStagingManager::ImageBarrier(
	VkCommandBuffer Cmd, 
	SImage* pImage, 
	VkImageSubresourceRange* pSubRange, 
	VkImageLayout OldLayout, 
	VkImageLayout NewLayout, 
	VkPipelineStageFlags SrcStageMask, 
	VkPipelineStageFlags DstStageMask, 
	uint32 SrcQueue, 
	uint32 DstQueue
)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = OldLayout;
	barrier.newLayout = NewLayout;
	barrier.image = pImage->ImgHnd;
	barrier.subresourceRange = *pSubRange;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.srcQueueFamilyIndex = SrcQueue;
	barrier.dstQueueFamilyIndex = DstQueue;

	vkCmdPipelineBarrier(Cmd, SrcStageMask, DstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

IStagingManager::IStagingManager(IRenderSystem& InRenderer) :
	pQueue(nullptr),
	Renderer(InRenderer),
	Pool{},
	TransferCmd{},
	GraphicsCmd{},
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

	for (size_t i = 0; i < MAX_TRANSFER_COMMANDS; i++)
	{
		TransferCmd[i] = Renderer.Device->AllocateCommandBuffer(Pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		GraphicsCmd[i] = Renderer.Device->AllocateCommandBuffer(Pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

		if (TransferCmd == VK_NULL_HANDLE || GraphicsCmd == VK_NULL_HANDLE)
		{
			VKT_ASSERT(false && "Unable to allocate command buffers for transfer and ownership transfer op.");
			return false;
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

	for (size_t i = 0; i < MAX_TRANSFER_COMMANDS; i++)
	{
		Renderer.Device->ResetCommandBuffer(TransferCmd[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		Renderer.Device->ResetCommandBuffer(GraphicsCmd[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
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
	VkCommandBuffer cmd = TransferCmd[NextCmdIndex];

	Renderer.Device->WaitFence(Fences[index]);
	Renderer.Device->ResetFence(Fences[index]);
	Renderer.Device->BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

bool IStagingManager::StageVertexData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = Renderer.VertexBuffer;
	if (hnd == INVALID_HANDLE) { return false; }
	return StageDataForBuffer(Data, Size, hnd);
}

bool IStagingManager::StageIndexData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = Renderer.IndexBuffer;
	if (hnd == INVALID_HANDLE) { return false; }
	return StageDataForBuffer(Data, Size, hnd);
}

bool IStagingManager::StageInstanceData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = Renderer.InstanceBuffer;
	if (hnd == INVALID_HANDLE) { return false; }
	return StageDataForBuffer(Data, Size, hnd);
}

bool IStagingManager::StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd)
{
	VkCommandBuffer cmd = TransferCmd[NextCmdIndex];
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
		EndStaging();
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

	BufferBarrier(
		cmd,
		pBuffer,
		Size,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		pQueue->FamilyIndex,
		Renderer.Device->GetGraphicsQueue().FamilyIndex
	);

	pStaging->Offset += PadToAlignedSize(Size);
	pBuffer->Offset += PadToAlignedSize(Size);

	return true;
}

bool IStagingManager::StageDataForImage(void* Data, size_t Size, Handle<SImage> DstImg)
{
	VkCommandBuffer cmd = TransferCmd[NextCmdIndex];
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
		EndStaging();
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

	ImageBarrier(
		cmd, 
		pImage, 
		&range, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
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

	ImageBarrier(
		cmd, 
		pImage, 
		&range, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		pQueue->FamilyIndex, 
		Renderer.Device->GetGraphicsQueue().FamilyIndex
	);
	
	pStaging->Offset += PadToAlignedSize(Size);

	return true;
}

void IStagingManager::EndStaging()
{
	const uint32 index = Renderer.Device->GetCurrentFrameIndex();
	VkCommandBuffer cmd = TransferCmd[NextCmdIndex];
	VkSemaphore semaphore = Semaphore[index];

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmd;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &semaphore;

	Renderer.Device->EndCommandBuffer(cmd);
	vkQueueSubmit(pQueue->Hnd, 1, &info, Fences[index]);

	IncrementCommandIndex();
	ResetStagingBuffer(index);
}
