#include "StagingManager.h"
#include "Renderer.h"
#include "API/Device.h"

struct IStagingParams
{
	enum EStagingOp : uint32
	{
		Staging_Op_Upload = 0,
		Staging_Op_Ownership_Transfer = 1,
		Staging_Op_Max = 2
	};

	using StagingBuffer = Pair<Handle<SMemoryBuffer>, SMemoryBuffer*>;

	IRenderDevice::VulkanQueue* pQueue;
	Ref<IRenderSystem> pRenderer;
	VkCommandPool Pool;
	VkCommandBuffer CmdBuf[Staging_Op_Max][MAX_TRANSFER_COMMANDS];
	VkSemaphore Semaphore[MAX_FRAMES_IN_FLIGHT];
	VkFence Fences[MAX_FRAMES_IN_FLIGHT];
	StagingBuffer Buffer[MAX_FRAMES_IN_FLIGHT];
	uint32 NextCmdIndex;
	bool TransferDefaults;


	const IRenderDevice::VulkanQueue* GetQueueForType(EQueueType QueueType) const
	{
		const IRenderDevice::VulkanQueue* queue = nullptr;
		switch (QueueType)
		{
		case EQueueType::Queue_Type_Graphics:
			queue = &pRenderer->pDevice->GetGraphicsQueue();
			break;
		case EQueueType::Queue_Type_Transfer:
		default:
			queue = pQueue;
			break;
		}
		return queue;
	}

} _StagingParams;

bool IStagingManager::DrawDataUploaded() const
{
	return _StagingParams.TransferDefaults;
}

void IStagingManager::ResetTransferDefaultFlag()
{
	_StagingParams.TransferDefaults = false;
}

size_t IStagingManager::PadToAlignedSize(size_t Size)
{
	const size_t minUboAlignment = _StagingParams.pRenderer->pDevice->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
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
	_StagingParams.NextCmdIndex = (_StagingParams.NextCmdIndex + 1) / MAX_TRANSFER_COMMANDS;
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

//const IRenderDevice::VulkanQueue* IStagingManager::GetQueueForType(EQueueType QueueType) const
//{
//	const IRenderDevice::VulkanQueue* queue = nullptr;
//	switch (QueueType)
//	{
//	case EQueueType::Queue_Type_Graphics:
//		queue = &Renderer.pDevice->GetGraphicsQueue();
//		break;
//	case EQueueType::Queue_Type_Transfer:
//	default:
//		queue = pQueue;
//		break;
//	}
//	return queue;
//}

IStagingManager::IStagingManager(IRenderSystem& InRenderer)
{
	_StagingParams.pQueue = nullptr;
	_StagingParams.pRenderer = &InRenderer;
	_StagingParams.Pool = {};
	_StagingParams.TransferDefaults = false;
	_StagingParams.NextCmdIndex = 0;

	IMemory::Memzero(&_StagingParams.CmdBuf, sizeof(VkCommandBuffer) * 2 * 3);
	IMemory::Memzero(&_StagingParams.Semaphore, sizeof(VkSemaphore) * 2);
	IMemory::Memzero(&_StagingParams.Fences, sizeof(VkSemaphore) * 2);
	IMemory::Memzero(&_StagingParams.Buffer, sizeof(VkSemaphore) * 2);
}

IStagingManager::~IStagingManager()
{}

bool IStagingManager::Initialize(size_t StagingBufferSize)
{
	_StagingParams.pQueue = const_cast<IRenderDevice::VulkanQueue*>(&_StagingParams.pRenderer->pDevice->GetTransferQueue());

	_StagingParams.Pool = _StagingParams.pRenderer->pDevice->CreateCommandPool(
		_StagingParams.pQueue->FamilyIndex, 
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	);

	if (_StagingParams.Pool == VK_NULL_HANDLE)
	{ 
		VKT_ASSERT(false && "Unable to create command pool.");
		return false; 
	}

	for (size_t i = 0; i < MAX_TRANSFER_COMMANDS; i++)
	{
		VkCommandBuffer& cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Upload][i];
		cmd = _StagingParams.pRenderer->pDevice->AllocateCommandBuffer(
			_StagingParams.Pool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1
		);
	}

	for (size_t i = 0; i < MAX_TRANSFER_COMMANDS; i++)
	{
		VkCommandBuffer& cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][i];
		cmd = _StagingParams.pRenderer->pDevice->AllocateCommandBuffer(
			_StagingParams.pRenderer->pDevice->GetGraphicsCommandPool(),
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1
		);
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
		Handle<SMemoryBuffer> hnd = _StagingParams.pRenderer->AllocateNewBuffer(allocInfo);
		_StagingParams.pRenderer->BuildBuffer(hnd);
		_StagingParams.Buffer[i] = IStagingParams::StagingBuffer(
			hnd, 
			_StagingParams.pRenderer->pStore->GetBuffer(hnd)
		);
		_StagingParams.Semaphore[i] = _StagingParams.pRenderer->pDevice->CreateVkSemaphore(nullptr);
		_StagingParams.Fences[i] = _StagingParams.pRenderer->pDevice->CreateFence();
	}

	return true;
}

void IStagingManager::Terminate()
{
	_StagingParams.pRenderer->pDevice->DeviceWaitIdle();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		_StagingParams.pRenderer->DestroyBuffer(_StagingParams.Buffer[i].Key);
		_StagingParams.pRenderer->pDevice->DestroyVkSemaphore(_StagingParams.Semaphore[i]);
		_StagingParams.pRenderer->pDevice->DestroyFence(_StagingParams.Fences[i]);
	}

	for (size_t i = 0; i < IStagingParams::EStagingOp::Staging_Op_Max; i++)
	{
		for (size_t j = 0; j < MAX_TRANSFER_COMMANDS; j++)
		{
			_StagingParams.pRenderer->pDevice->ResetCommandBuffer(_StagingParams.CmdBuf[i][j], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		}
	}

	_StagingParams.pRenderer->pDevice->DestroyCommandPool(_StagingParams.Pool);
}

void IStagingManager::ResetStagingBuffer(uint32 Index)
{
	VKT_ASSERT(Index < MAX_FRAMES_IN_FLIGHT && "Index supplied exceeded buffer count.");
	auto [handle, pBuffer] = _StagingParams.Buffer[Index];
	FlushBuffer(*pBuffer);
}

void IStagingManager::BeginStaging()
{
	const uint32 index = _StagingParams.pRenderer->pDevice->GetCurrentFrameIndex();
	VkCommandBuffer cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Upload][_StagingParams.NextCmdIndex];

	_StagingParams.pRenderer->pDevice->WaitFence(_StagingParams.Fences[index]);
	_StagingParams.pRenderer->pDevice->ResetFence(_StagingParams.Fences[index]);
	_StagingParams.pRenderer->pDevice->BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

size_t IStagingManager::StageVertexData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = _StagingParams.pRenderer->VertexBuffer;
	_StagingParams.TransferDefaults = true;
	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
}

size_t IStagingManager::StageIndexData(void* Data, size_t Size)
{
	const auto [hnd, pBuffer] = _StagingParams.pRenderer->IndexBuffer;
	_StagingParams.TransferDefaults = true;
	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
}

//size_t IStagingManager::StageInstanceData(void* Data, size_t Size)
//{
//	const auto [hnd, pBuffer] = _StagingParams.pRenderer->InstanceBuffer;
//	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
//}

size_t IStagingManager::StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd, EQueueType DstQueue)
{
	VkCommandBuffer cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Upload][_StagingParams.NextCmdIndex];
	const IRenderDevice::VulkanQueue* dstQueue = _StagingParams.GetQueueForType(DstQueue);

	auto [hnd, pStaging] = _StagingParams.Buffer[_StagingParams.pRenderer->pDevice->GetCurrentFrameIndex()];
	SMemoryBuffer* pBuffer = _StagingParams.pRenderer->pStore->GetBuffer(DstHnd);
	if (!pBuffer) { return false; }
	if (Size > pStaging->Size) 
	{ 
		VKT_ASSERT(false && "Data size exceeded capacity allowed by staging buffer.");
		return -1; 
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
	IMemory::Memcpy(pData, Data, Size);

	VkBufferCopy copy = {};
	copy.size = Size;
	copy.srcOffset = pStaging->Offset;
	copy.dstOffset = pBuffer->Offset;

	vkCmdCopyBuffer(cmd, pStaging->Hnd, pBuffer->Hnd, 1, &copy);

	_StagingParams.pRenderer->pDevice->BufferBarrier(
		cmd,
		pBuffer->Hnd,
		Size,
		pBuffer->Offset,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		_StagingParams.pQueue->FamilyIndex,
		dstQueue->FamilyIndex
	);

	pStaging->Offset += PadToAlignedSize(Size);
	pBuffer->Offset += PadToAlignedSize(Size);

	return pBuffer->Offset;
}

bool IStagingManager::StageDataForImage(void* Data, size_t Size, Handle<SImage> DstImg, EQueueType DstQueue)
{
	VkCommandBuffer cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Upload][_StagingParams.NextCmdIndex];
	const IRenderDevice::VulkanQueue* dstQueue = _StagingParams.GetQueueForType(DstQueue);

	auto [hnd, pStaging] = _StagingParams.Buffer[_StagingParams.pRenderer->pDevice->GetCurrentFrameIndex()];
	SImage* pImage = _StagingParams.pRenderer->pStore->GetImage(DstImg);
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

	_StagingParams.pRenderer->pDevice->ImageBarrier(
		cmd, 
		pImage->ImgHnd, 
		&range, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		_StagingParams.pQueue->FamilyIndex, 
		_StagingParams.pQueue->FamilyIndex
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

	_StagingParams.pRenderer->pDevice->ImageBarrier(
		cmd, 
		pImage->ImgHnd, 
		&range, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		VK_PIPELINE_STAGE_TRANSFER_BIT, 
		_StagingParams.pQueue->FamilyIndex,
		dstQueue->FamilyIndex
	);
	
	pStaging->Offset += PadToAlignedSize(Size);

	return true;
}

void IStagingManager::EndStaging(bool SignalSemaphore)
{
	const uint32 index = _StagingParams.pRenderer->pDevice->GetCurrentFrameIndex();
	VkCommandBuffer cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Upload][_StagingParams.NextCmdIndex];

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmd;

	if (SignalSemaphore)
	{
		VkSemaphore semaphore = _StagingParams.Semaphore[index];

		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &semaphore;
	}

	_StagingParams.pRenderer->pDevice->EndCommandBuffer(cmd);
	vkQueueSubmit(_StagingParams.pQueue->Hnd, 1, &info, _StagingParams.Fences[index]);

	IncrementCommandIndex();
	ResetStagingBuffer(index);
}

size_t IStagingManager::GetVertexBufferOffset() const
{
	return _StagingParams.pRenderer->VertexBuffer.Value->Offset;
}

size_t IStagingManager::GetIndexBufferOffset() const
{
	return _StagingParams.pRenderer->IndexBuffer.Value->Offset;
}

//size_t IStagingManager::GetInstanceBufferOffset() const
//{
//	return _StagingParams.pRenderer->InstanceBuffer.Value->Offset;
//}

void IStagingManager::BeginTransfer()
{
	_StagingParams.pRenderer->pDevice->ResetCommandBuffer(
		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex],
		VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT
	);
	_StagingParams.pRenderer->pDevice->BeginCommandBuffer(
		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex],
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	);
}

bool IStagingManager::TransferBufferOwnership(Handle<SMemoryBuffer> Hnd, EQueueType Queue)
{
	VKT_ASSERT((Queue != EQueueType::Queue_Type_Present && Queue != EQueueType::Queue_Type_Transfer) &&
		"Ownership transfer is not allowed for the presentation queue and the transfer queue!");

	SMemoryBuffer* pBuffer = _StagingParams.pRenderer->pStore->GetBuffer(Hnd);
	if (!pBuffer) { return false; }

	const IRenderDevice::VulkanQueue* dstQueue = _StagingParams.GetQueueForType(Queue);
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

	_StagingParams.pRenderer->pDevice->BufferBarrier(
		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex],
		pBuffer->Hnd, 
		pBuffer->Size,
		0,
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

	SImage* pImage = _StagingParams.pRenderer->pStore->GetImage(Hnd);
	if (!pImage) { return false; }

	const IRenderDevice::VulkanQueue* dstQueue = _StagingParams.GetQueueForType(Queue);
	if (!dstQueue) { return false; }

	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.baseArrayLayer = 0;
	range.levelCount = 1;
	range.layerCount = 1;

	_StagingParams.pRenderer->pDevice->ImageBarrier(
		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex],
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
	const uint32 index = _StagingParams.pRenderer->pDevice->GetCurrentFrameIndex();
	const IRenderDevice::VulkanQueue* pGfxQueue = &_StagingParams.pRenderer->pDevice->GetGraphicsQueue();
	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex];
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &_StagingParams.Semaphore[index];
	info.pWaitDstStageMask = &waitDstStageMask;

	_StagingParams.pRenderer->pDevice->EndCommandBuffer(
		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex]
	);
	vkQueueSubmit(pGfxQueue->Hnd, 1, &info, VK_NULL_HANDLE);
}
