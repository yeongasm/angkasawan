#include "RenderMemory.h"
#include "API/Context.h"

size_t IRenderMemoryManager::DoesBufferExist(uint32 Id)
{
	size_t index = Resource_Not_Exist;
	SRMemoryBuffer* buffer = nullptr;
	for (size_t i = 0; i < Buffers.Length(); i++)
	{
		buffer = Buffers[i];
		if (buffer->Id != Id) { continue; }
		index = i;
		break;
	}
	return index;
}

IRenderMemoryManager::IRenderMemoryManager(LinearAllocator& InAllocator) :
	Allocator(InAllocator), Buffers()
{}

IRenderMemoryManager::~IRenderMemoryManager()
{
	Buffers.Release();
}

Handle<SRMemoryBuffer> IRenderMemoryManager::AllocateNewBuffer(const MemoryAllocateInfo& AllocInfo)
{
	if (DoesBufferExist(AllocInfo.Id) != Resource_Not_Exist)
	{
		return INVALID_HANDLE;
	}

	SRMemoryBuffer* buffer = reinterpret_cast<SRMemoryBuffer*>(Allocator.Malloc(sizeof(SRMemoryBuffer)));
	FMemory::InitializeObject(buffer);
	FMemory::Memcpy(buffer, &AllocInfo, sizeof(SRMemoryBufferBase));

	buffer->Offset = 0;
	buffer->Locality = Buffer_Locality_Gpu;
	size_t index = Buffers.Push(buffer);
	BuildBuffer(index);

	return Handle<SRMemoryBuffer>(index);
}

Handle<SRMemoryBuffer> IRenderMemoryManager::GetBufferHandleWithId(uint32 Id)
{
	return Handle<SRMemoryBuffer>(DoesBufferExist(Id));
}

SRMemoryBuffer* IRenderMemoryManager::GetBuffer(Handle<SRMemoryBuffer> Hnd)
{
	if (Hnd == INVALID_HANDLE) { return nullptr; }
	VKT_ASSERT(Hnd != INVALID_HANDLE && "Invalid handle supplied to function");
	return Buffers[Hnd];
}

bool IRenderMemoryManager::UploadData(SRMemoryBuffer& Buffer, Handle<SRMemoryBuffer> Hnd, bool Immediate)
{
	if (Buffer.Handle == INVALID_HANDLE || Hnd == INVALID_HANDLE) 
	{ 
		return false; 
	}

	SRMemoryBuffer* gpuMemory = GetBuffer(Hnd);
	
	if (!gpuMemory) { return false; }

	SRMemoryTransferContext tx = {};
	tx.SrcBuffer = Buffer;
	tx.SrcOffset = 0;
	tx.SrcSize = Buffer.Size;
	tx.DstBuffer = gpuMemory;
	tx.DstOffset = gpuMemory->Offset;

	gpuMemory->Offset += Buffer.Size;
	//gpuMemory->Offset += Context.PadSizeToAlignedSize(Buffer.Size);

	Transfers.Push(Move(tx));

	if (Immediate) { TransferToGPU(); }

	return true;
}

//bool IRenderMemoryManager::UploadModel(Model& InModel, Handle<SRMemoryBuffer> Hnd, bool Immediate)
//{
//	if (Hnd == INVALID_HANDLE) { return false; }
//	SRMemoryBuffer* buffer = GetBuffer(Hnd);
//	if (!buffer) { return false; }
//	if (buffer->Handle == INVALID_HANDLE) { return false; }
//
//	SRMemoryTransferContext tx = {};
//	tx.DstBuffer = buffer->Handle;
//
//	for (Mesh* mesh : InModel)
//	{
//
//		tx.SrcBuffer = mesh->Buffer.Handle;
//		tx.SrcOffset = 0;
//		tx.SrcSize = mesh->Buffer.Size;
//		tx.DstOffset = buffer->Offset;
//		Transfers.Push(Move(tx));
//
//		mesh->Buffer.Offset = buffer->Offset;
//		buffer->Offset += Context.PadSizeToAlignedSize(mesh->Buffer.Size);
//		//Context.DestroyBuffer(mesh->Buffer.Handle);
//
//		mesh->Buffer.Id = buffer->Id;
//		mesh->Buffer.Handle = buffer->Handle;
//		mesh->Buffer.Locality = Buffer_Locality_Gpu;
//	}
//
//	if (Immediate) { TransferToGPU(); }
//
//	return true;
//}

//bool IRenderMemoryManager::TransferBufferToGPU(Handle<SRMemoryBuffer> Hnd)
//{
//	SRMemoryBuffer* bufferObj = GetBufferObj(Hnd);
//	if (!bufferObj) { return false; }
//	if ((bufferObj->Cpu.Handle == INVALID_HANDLE) ||
//		(bufferObj->Gpu.Handle == INVALID_HANDLE))
//	{
//		return false;
//	}
//
//	const size_t copySize = (TransferInfo.SrcSize) ? TransferInfo.SrcSize : bufferObj->Cpu.Size;
//	size_t paddedSize = Context.PadSizeToAlignedSize(copySize);
//
//	SRMemoryTransferContext tx = {};
//	tx.SrcBuffer	= bufferObj->Cpu.Handle;
//	tx.SrcOffset	= TransferInfo.SrcOffset;
//	tx.SrcSize		= paddedSize;
//	tx.DstBuffer	= bufferObj->Gpu.Handle;
//	tx.DstOffset	= (!TransferInfo.DstOffset) ? bufferObj->Gpu.Offset : TransferInfo.DstOffset;
//
//	bufferObj->Gpu.Offset += paddedSize;
//
//	Context.TransferToGPU(&tx, 1);
//
//	return true;
//}

void IRenderMemoryManager::TransferToGPU()
{
	gpu::BeginTransfer();
	//Context.TransferToGPU(Transfers.First(), Transfers.Length());
	for (SRMemoryTransferContext& transfer : Transfers)
	{
		gpu::TransferBuffer(transfer);
	}
	gpu::EndTransfer();
	for (SRMemoryTransferContext& txCtx : Transfers)
	{
		gpu::DestroyBuffer(txCtx.SrcBuffer);
		//Context.DestroyBuffer(txCtx.SrcBuffer);
	}
	Transfers.Empty();
}

bool IRenderMemoryManager::FlushBuffer(Handle<SRMemoryBuffer> Hnd)
{

	SRMemoryBuffer* buffer = GetBuffer(Hnd);
	if (!buffer) { return false; }
	if (buffer->Handle == INVALID_HANDLE) { return false; }
	buffer->Offset = 0;
	return true;
}

void IRenderMemoryManager::FlushAllBuffer()
{
	for (SRMemoryBuffer* buffer : Buffers)
	{
		buffer->Offset = 0;
	}
}

bool IRenderMemoryManager::BuildBuffer(Handle<SRMemoryBuffer> Hnd)
{
	SRMemoryBuffer* buffer = GetBuffer(Hnd);
	if (!buffer) { return false; }
	if (buffer->Handle != INVALID_HANDLE) { return false; }
	gpu::CreateBuffer(*buffer, nullptr, buffer->Size);
	//Context.NewBuffer(*buffer, nullptr, buffer->Size, 1);
	return true;
}

void IRenderMemoryManager::Destroy()
{
	for (SRMemoryBuffer* buffer : Buffers)
	{
		gpu::DestroyBuffer(*buffer);
		//Context.DestroyBuffer(buffer->Handle);
	}
	Buffers.Release();
}

