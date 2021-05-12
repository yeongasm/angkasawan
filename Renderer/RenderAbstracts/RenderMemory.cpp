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
	IMemory::InitializeObject(buffer);
	IMemory::Memcpy(buffer, &AllocInfo, sizeof(SRMemoryBufferBase));

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

	Transfers.Push(Move(tx));

	if (Immediate) { TransferToGPU(); }

	return true;
}

void IRenderMemoryManager::TransferToGPU()
{
	gpu::BeginTransfer();
	for (SRMemoryTransferContext& transfer : Transfers)
	{
		gpu::TransferBuffer(transfer);
		// We need to include another phase here to transfer ownership to the graphics queue.
	}
	gpu::EndTransfer();
	gpu::BeginOwnershipTransfer();
	for (SRMemoryBuffer* buffer : Buffers)
	{
		gpu::TransferBufferOwnership(*buffer);
	}
	gpu::EndOwnershipTransfer();

	for (SRMemoryTransferContext& txCtx : Transfers)
	{
		gpu::DestroyBuffer(txCtx.SrcBuffer);
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
	return true;
}

void IRenderMemoryManager::Destroy()
{
	for (SRMemoryBuffer* buffer : Buffers)
	{
		gpu::DestroyBuffer(*buffer);
	}
	Buffers.Release();
}

