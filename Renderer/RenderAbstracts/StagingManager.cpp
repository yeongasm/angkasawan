#include "StagingManager.h"
#include "Renderer.h"
#include "API/Device.h"

decltype(auto) IStagingManager::GetQueueForType(EQueueType Type)
{
	VKT_ASSERT(Type != EQueueType::Queue_Type_Present);
	if (Type == EQueueType::Queue_Type_Graphics)
	{
		return &pRenderer->pDevice->GetGraphicsQueue();
	}
	return &pRenderer->pDevice->GetTransferQueue();
}

bool IStagingManager::ShouldMakeTransfer() const
{
	return MakeTransfer;
}

IStagingManager::IStagingManager(IRenderSystem& InRenderer) :
	pRenderer(&InRenderer),
	TxPool{}
{}

IStagingManager::~IStagingManager()
{}

bool IStagingManager::Initialize()
{
	const IRenderDevice::VulkanQueue& txQueue = pRenderer->pDevice->GetTransferQueue();
	TxPool = pRenderer->pDevice->CreateCommandPool(
		txQueue.FamilyIndex, 
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	);

	if (TxPool == VK_NULL_HANDLE)
	{ 
		VKT_ASSERT(false && "Unable to create command pool.");
		return false; 
	}

	return true;
}

void IStagingManager::Terminate()
{
	pRenderer->pDevice->MoveToZombieList(
		TxPool, 
		IRenderDevice::EHandleType::Handle_Type_Command_Pool
	);
}

bool IStagingManager::StageVertexData(void* Data, size_t Size)
{
	MakeTransfer = true;
	const auto [hnd, pBuffer] = pRenderer->VertexBuffer;
	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
}

bool IStagingManager::StageIndexData(void* Data, size_t Size)
{
	MakeTransfer = true;
	const auto [hnd, pBuffer] = pRenderer->IndexBuffer;
	return StageDataForBuffer(Data, Size, hnd, EQueueType::Queue_Type_Graphics);
}

bool IStagingManager::StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd, EQueueType DstQueue)
{
	const IRenderDevice::VulkanQueue* dstQueue = GetQueueForType(DstQueue);
	Ref<SMemoryBuffer> dst = pRenderer->pStore->GetBuffer(DstHnd);
	if (!dst) { return false; }

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = Size;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	
	VmaAllocationCreateInfo alloc = {};
	alloc.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	SMemoryBuffer temp;

	if (vmaCreateBuffer(
		pRenderer->pDevice->GetAllocator(),
		&info,
		&alloc,
		&temp.Hnd,
		&temp.Allocation,
		nullptr
	) != VK_SUCCESS)
	{
		return false;
	}

	vmaMapMemory(pRenderer->pDevice->GetAllocator(), temp.Allocation, reinterpret_cast<void**>(&temp.pData));
	vmaUnmapMemory(pRenderer->pDevice->GetAllocator(), temp.Allocation);

	IMemory::Memcpy(temp.pData, Data, Size);

	UploadContext upload = {};
	upload.DstBuffer = dst;
	upload.DstQueue = DstQueue;
	upload.Size = Size;
	upload.SrcBuffer = Move(temp);
	Uploads.Push(Move(upload));

	return true;
}

//bool IStagingManager::StageDataForImage(void* Data, size_t Size, Handle<SImage> DstImg, EQueueType DstQueue)
//{
//	VkCommandBuffer cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Upload][_StagingParams.NextCmdIndex];
//	const IRenderDevice::VulkanQueue* dstQueue = _StagingParams.GetQueueForType(DstQueue);
//
//	auto [hnd, pStaging] = _StagingParams.Buffer[_StagingParams.pRenderer->pDevice->GetCurrentFrameIndex()];
//	SImage* pImage = _StagingParams.pRenderer->pStore->GetImage(DstImg);
//	if (!pImage) { return false; }
//	if (Size > pStaging->Size)
//	{
//		VKT_ASSERT(false && "Data size exceeded capacity allowed by staging buffer.");
//		return false;
//	}
//
//	if (!CanContentsFit(*pStaging, Size))
//	{
//		// EndStaging() submits to the queue and signals a semaphore to be used for ownership transfer
//		// In the rare event that the staging buffer is full, I need to do another submission and
//		// I would like the queue to wait for the previous one to complete.
//		// Ideally, this operation should not stall the CPU.
//		EndStaging(false);
//		BeginStaging();
//	}
//
//	uint8* pData = pStaging->pData;
//	pData += pStaging->Offset;
//	IMemory::Memcpy(&pData, &Data, Size);
//
//	VkImageSubresourceRange range = {};
//	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	range.baseMipLevel = 0;
//	range.baseArrayLayer = 0;
//	range.levelCount = 1;
//	range.layerCount = 1;
//
//	_StagingParams.pRenderer->pDevice->ImageBarrier(
//		cmd, 
//		pImage->ImgHnd, 
//		&range, 
//		VK_IMAGE_LAYOUT_UNDEFINED, 
//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
//		VK_PIPELINE_STAGE_TRANSFER_BIT, 
//		VK_PIPELINE_STAGE_TRANSFER_BIT, 
//		_StagingParams.pQueue->FamilyIndex, 
//		_StagingParams.pQueue->FamilyIndex
//	);
//
//	VkBufferImageCopy copy = {};
//	copy.bufferOffset = pStaging->Offset;
//	copy.bufferRowLength = 0;
//	copy.bufferImageHeight = 0;
//	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	copy.imageSubresource.mipLevel = 0;
//	copy.imageSubresource.baseArrayLayer = 0;
//	copy.imageSubresource.layerCount = 1;
//	copy.imageExtent = { pImage->Width, pImage->Height, 1 };
//
//	vkCmdCopyBufferToImage(cmd, pStaging->Hnd, pImage->ImgHnd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
//
//	// TODO(Ygsm):
//	// To include mip map generation for images.
//
//	_StagingParams.pRenderer->pDevice->ImageBarrier(
//		cmd, 
//		pImage->ImgHnd, 
//		&range, 
//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
//		VK_PIPELINE_STAGE_TRANSFER_BIT, 
//		VK_PIPELINE_STAGE_TRANSFER_BIT, 
//		_StagingParams.pQueue->FamilyIndex,
//		dstQueue->FamilyIndex
//	);
//	
//	pStaging->Offset += PadToAlignedSize(Size);
//
//	return true;
//}

//void IStagingManager::EndStaging(bool SignalSemaphore)
//{
//	const uint32 index = _StagingParams.pRenderer->pDevice->GetCurrentFrameIndex();
//	VkCommandBuffer cmd = _StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Upload][_StagingParams.NextCmdIndex];
//
//	VkSubmitInfo info = {};
//	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//	info.commandBufferCount = 1;
//	info.pCommandBuffers = &cmd;
//
//	if (SignalSemaphore)
//	{
//		VkSemaphore semaphore = _StagingParams.Semaphore[index];
//
//		info.signalSemaphoreCount = 1;
//		info.pSignalSemaphores = &semaphore;
//	}
//
//	_StagingParams.pRenderer->pDevice->EndCommandBuffer(cmd);
//	vkQueueSubmit(_StagingParams.pQueue->Hnd, 1, &info, _StagingParams.Fences[index]);
//
//	IncrementCommandIndex();
//	ResetStagingBuffer(index);
//}

//size_t IStagingManager::GetInstanceBufferOffset() const
//{
//	return _StagingParams.pRenderer->InstanceBuffer.Value->Offset;
//}

bool IStagingManager::Upload()
{
	VkCommandBuffer cmd = pRenderer->pDevice->AllocateCommandBuffer(TxPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	if (cmd == VK_NULL_HANDLE) { return false; }

	pRenderer->pDevice->BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	for (UploadContext& upload : Uploads)
	{
		auto dstQueue = GetQueueForType(upload.DstQueue);
		VkBufferCopy region = {};
		region.size = upload.Size;
		region.srcOffset = 0;
		region.dstOffset = upload.DstBuffer->Offset;

		vkCmdCopyBuffer(
			cmd,
			upload.SrcBuffer.Hnd,
			upload.DstBuffer->Hnd,
			1,
			&region
		);

		pRenderer->pDevice->BufferBarrier(
			cmd,
			upload.DstBuffer->Hnd,
			upload.Size,
			upload.DstBuffer->Offset,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			pRenderer->pDevice->GetTransferQueue().FamilyIndex,
			dstQueue->FamilyIndex
		);

		upload.DstBuffer->Offset += upload.Size;
	}

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	pRenderer->pDevice->EndCommandBuffer(cmd);
	vkQueueSubmit(
		pRenderer->pDevice->GetTransferQueue().Hnd,
		1,
		&submit,
		VK_NULL_HANDLE
	);
	vkQueueWaitIdle(pRenderer->pDevice->GetTransferQueue().Hnd);
	vkResetCommandPool(pRenderer->pDevice->GetDevice(), TxPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	for (UploadContext& upload : Uploads)
	{
		pRenderer->pDevice->MoveToZombieList(
			upload.SrcBuffer.Hnd,
			IRenderDevice::EHandleType::Handle_Type_Buffer,
			upload.SrcBuffer.Allocation
		);
	}
	pRenderer->pDevice->MoveToZombieList(cmd, TxPool);
	Uploads.Empty();

	return true;
}

//void IStagingManager::BeginTransfer()
//{
//	pRenderer->pDevice->BeginCommandBuffer(
//		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex],
//		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
//	);
//}
//
//bool IStagingManager::TransferBufferOwnership(Handle<SMemoryBuffer> Hnd, EQueueType Queue)
//{
//	SMemoryBuffer* pBuffer = pRenderer->pStore->GetBuffer(Hnd);
//	if (!pBuffer) { return false; }
//
//	const IRenderDevice::VulkanQueue* dstQueue = GetQueueForType(Queue);
//	if (!dstQueue) { return false; }
//
//	VkAccessFlags dstMask = VK_ACCESS_MEMORY_READ_BIT;
//
//	if (pBuffer->Type.Has(Buffer_Type_Vertex))
//	{
//		dstMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
//	}
//
//	if (pBuffer->Type.Has(Buffer_Type_Index))
//	{
//		dstMask = VK_ACCESS_INDEX_READ_BIT;
//	}
//
//	pRenderer->pDevice->BufferBarrier(
//		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex],
//		pBuffer->Hnd, 
//		pBuffer->Size,
//		0,
//		VK_ACCESS_TRANSFER_WRITE_BIT, 
//		dstMask, 
//		VK_PIPELINE_STAGE_TRANSFER_BIT, 
//		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 
//		dstQueue->FamilyIndex, 
//		dstQueue->FamilyIndex
//	);
//
//	return true;
//}
//
//bool IStagingManager::TransferImageOwnership(Handle<SImage> Hnd, EQueueType Queue)
//{
//	VKT_ASSERT((Queue != EQueueType::Queue_Type_Present && Queue != EQueueType::Queue_Type_Transfer) &&
//		"Ownership transfer is not allowed for the presentation queue and the transfer queue!");
//
//	SImage* pImage = _StagingParams.pRenderer->pStore->GetImage(Hnd);
//	if (!pImage) { return false; }
//
//	const IRenderDevice::VulkanQueue* dstQueue = _StagingParams.GetQueueForType(Queue);
//	if (!dstQueue) { return false; }
//
//	VkImageSubresourceRange range = {};
//	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	range.baseMipLevel = 0;
//	range.baseArrayLayer = 0;
//	range.levelCount = 1;
//	range.layerCount = 1;
//
//	_StagingParams.pRenderer->pDevice->ImageBarrier(
//		_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex],
//		pImage->ImgHnd,
//		&range,
//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//		VK_PIPELINE_STAGE_TRANSFER_BIT,
//		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
//		dstQueue->FamilyIndex,
//		dstQueue->FamilyIndex
//	);
//
//	return true;
//}
//
//void IStagingManager::EndTransfer()
//{
//	const uint32 index = pRenderer->pDevice->GetCurrentFrameIndex();
//	const IRenderDevice::VulkanQueue* pGfxQueue = &pRenderer->pDevice->GetGraphicsQueue();
//	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//
//	VkSubmitInfo info = {};
//	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//	info.commandBufferCount = 1;
//	info.pCommandBuffers = &_StagingParams.CmdBuf[IStagingParams::EStagingOp::Staging_Op_Ownership_Transfer][_StagingParams.NextCmdIndex];
//	info.waitSemaphoreCount = 1;
//	info.pWaitSemaphores = &_StagingParams.Semaphore[index];
//	info.pWaitDstStageMask = &waitDstStageMask;
//
//	pRenderer->pDevice->EndCommandBuffer();
//	vkQueueSubmit(pGfxQueue->Hnd, 1, &info, VK_NULL_HANDLE);
//}
