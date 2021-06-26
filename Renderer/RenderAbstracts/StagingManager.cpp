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

//bool IStagingManager::ShouldMakeTransfer() const
//{
//	return MakeTransfer;
//}

IStagingManager::OwnershipTransferContext::OwnershipTransferContext() :
  pBuffer{}, Type{ Staging_Upload_Type_None }
{}

IStagingManager::OwnershipTransferContext::OwnershipTransferContext(Ref<SMemoryBuffer> pInBuffer) :
  pBuffer{ pInBuffer }, Type{ Staging_Upload_Type_Buffer }
{}

IStagingManager::OwnershipTransferContext::OwnershipTransferContext(Ref<SImage> pInImage) :
  pImage{ pInImage }, Type{ Staging_Upload_Type_Image }
{}

IStagingManager::OwnershipTransferContext::~OwnershipTransferContext()
{
  if (Type == Staging_Upload_Type_Image)
  {
    pImage = NULLPTR;
  }
  if (Type == Staging_Upload_Type_Buffer)
  {
    pBuffer = NULLPTR;
  }
  Type = Staging_Upload_Type_None;
}

void IStagingManager::UploadToBuffer(VkCommandBuffer CmdBuffer, UploadContext& Ctx)
{
  auto dstQueue = GetQueueForType(Ctx.DstQueue);
  VkBufferCopy region = {};
  region.size = Ctx.Size;
  region.srcOffset = 0;
  region.dstOffset = Ctx.pDstBuf->Offset;

  vkCmdCopyBuffer(
    CmdBuffer,
    Ctx.SrcBuffer.Hnd,
    Ctx.pDstBuf->Hnd,
    1,
    &region
  );

  pRenderer->pDevice->BufferBarrier(
    CmdBuffer,
    Ctx.pDstBuf->Hnd,
    Ctx.Size,
    Ctx.pDstBuf->Offset,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_ACCESS_TRANSFER_READ_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    pRenderer->pDevice->GetTransferQueue().FamilyIndex,
    dstQueue->FamilyIndex
  );

  Ctx.pDstBuf->Offset += Ctx.Size;
}

void IStagingManager::UploadToImage(VkCommandBuffer CmdBuffer, UploadContext& Ctx)
{
  auto dstQueue = GetQueueForType(Ctx.DstQueue);

  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = Ctx.pDstImg->MipLevels;
  subresourceRange.baseArrayLayer = 0;
  subresourceRange.layerCount = 1;

  pRenderer->pDevice->ImageBarrier(
    CmdBuffer,
    Ctx.pDstImg->ImgHnd,
    &subresourceRange,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    pRenderer->pDevice->GetTransferQueue().FamilyIndex,
    pRenderer->pDevice->GetTransferQueue().FamilyIndex
  );

  VkBufferImageCopy copyRegion = {};
  copyRegion.bufferOffset = 0;
  copyRegion.bufferRowLength = 0;
  copyRegion.bufferImageHeight = 0;
  copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copyRegion.imageSubresource.mipLevel = 0;
  copyRegion.imageSubresource.baseArrayLayer = 0;
  copyRegion.imageSubresource.layerCount = 1;
  copyRegion.imageExtent = { Ctx.pDstImg->Width, Ctx.pDstImg->Height, 1 };

  vkCmdCopyBufferToImage(
    CmdBuffer,
    Ctx.SrcBuffer.Hnd,
    Ctx.pDstImg->ImgHnd,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &copyRegion
  );

  pRenderer->pDevice->ImageBarrier(
    CmdBuffer,
    Ctx.pDstImg->ImgHnd,
    &subresourceRange,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    pRenderer->pDevice->GetTransferQueue().FamilyIndex,
    dstQueue->FamilyIndex,
    VK_ACCESS_TRANSFER_WRITE_BIT
  );
}

IStagingManager::IStagingManager(IRenderSystem& InRenderer) :
  Uploads{},
  OwnershipTransfers{},
	pRenderer(&InRenderer),
	TxPool{},
  MakeTransfers{}
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
  MakeTransfers.Set(Ownership_Transfer_Type_Vertex_Buffer);
	return StageDataForBuffer(Data, Size, pRenderer->VertexBuffer, EQueueType::Queue_Type_Graphics);
}

bool IStagingManager::StageIndexData(void* Data, size_t Size)
{
  MakeTransfers.Set(Ownership_Transfer_Type_Index_Buffer);
	return StageDataForBuffer(Data, Size, pRenderer->IndexBuffer, EQueueType::Queue_Type_Graphics);
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
	upload.pDstBuf = dst;
	upload.DstQueue = DstQueue;
	upload.Size = Size;
	upload.SrcBuffer = Move(temp);
  upload.Type = Staging_Upload_Type_Buffer;
	Uploads.Push(Move(upload));

	return true;
}

const Array<IStagingManager::OwnershipTransferContext>& IStagingManager::GetOwnershipTransfers() const
{
  return OwnershipTransfers;
}

bool IStagingManager::StageDataForImage(void* Data, size_t Size, Handle<SImage> DstHnd, EQueueType DstQueue)
{
  const IRenderDevice::VulkanQueue* dstQueue = GetQueueForType(DstQueue);
  Ref<SImage> pImg = pRenderer->pStore->GetImage(DstHnd);
  if (!pImg) { return false; }

  VkBufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  info.size = Size;
  info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

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
  upload.pDstImg = pImg;
  upload.DstQueue = DstQueue;
  upload.Size = Size;
  upload.SrcBuffer = Move(temp);
  upload.Type = Staging_Upload_Type_Image;
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
    if (upload.Type == Staging_Upload_Type_Buffer)
    {
      UploadToBuffer(cmd, upload);
    }
    if (upload.Type == Staging_Upload_Type_Image)
    {
      UploadToImage(cmd, upload);
    }
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

bool IStagingManager::TransferBufferOwnership(Handle<SMemoryBuffer> Hnd)
{
  if (Hnd == INVALID_HANDLE) { return false; }
  Ref<SMemoryBuffer> pBuffer = pRenderer->pStore->GetBuffer(Hnd);
  if (!pBuffer) { return false; }
  OwnershipTransfers.Push(OwnershipTransferContext(pBuffer));
  MakeTransfers.Set(Ownership_Transfer_Type_Buffer_Or_Image);
  return true;
}

bool IStagingManager::TransferImageOwnership(Handle<SImage> Hnd)
{
  if (Hnd == INVALID_HANDLE) { return false; }
  Ref<SImage> pImg = pRenderer->pStore->GetImage(Hnd);
  if (!pImg) { return false; }
  OwnershipTransfers.Push(OwnershipTransferContext(pImg));
  MakeTransfers.Set(Ownership_Transfer_Type_Buffer_Or_Image);
  return true;
}

void IStagingManager::ClearOwnershipTransfers()
{
  OwnershipTransfers.Empty();
}
