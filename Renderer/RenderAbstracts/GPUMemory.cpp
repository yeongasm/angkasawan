#include "GPUMemory.h"
#include "API/Context.h"

size_t IRGPUMemory::DoesGPUMemoryExist(size_t Id)
{
	GPUBuffer* memory = nullptr;
	size_t index = Resource_Not_Exist;
	for (size_t i = 0; i < Buffers.Length(); i++)
	{
		memory = Buffers[i];
		if (memory->Id != Id) { continue; }
		index = i;
		break;
	}
	return index;
}

IRGPUMemory::IRGPUMemory(LinearAllocator& InAllocator) :
	Allocator(InAllocator),
	Buffers()
{}

IRGPUMemory::~IRGPUMemory()
{
	Buffers.Release();
}

Handle<GPUBuffer> IRGPUMemory::AllocateNewMemory(const GPUMemoryAllocateInfo& AllocInfo)
{
	if (DoesGPUMemoryExist(AllocInfo.Id) != Resource_Not_Exist)
	{
		return INVALID_HANDLE;
	}

	GPUBuffer* memory = reinterpret_cast<GPUBuffer*>(Allocator.Malloc(sizeof(GPUBuffer)));
	FMemory::InitializeObject(memory);
	FMemory::Memcpy(memory, &AllocInfo, sizeof(SRMemoryBufferBase));

	size_t index = Buffers.Push(memory);

	if (!AllocInfo.DeferAllocation)
	{
		BuildGPUBuffer(index);
	}

	return Handle<GPUBuffer>(index);
}

Handle<GPUBuffer> IRGPUMemory::GetGPUMemoryHandleWithId(uint32 Id)
{
	return Handle<GPUBuffer>(DoesGPUMemoryExist(Id));
}

GPUBuffer* IRGPUMemory::GetGPUMemory(Handle<GPUBuffer> Hnd)
{
	if (Hnd == INVALID_HANDLE) { return nullptr; }
	VKT_ASSERT(Hnd != INVALID_HANDLE && "Invalid handle supplied to function");
	return Buffers[Hnd];
}

bool IRGPUMemory::FlushGPUMemory(Handle<GPUBuffer> Hnd)
{
	if (Hnd == INVALID_HANDLE) { return false; }
	GPUBuffer* gpuMem = GetGPUMemory(Hnd);
	if (!gpuMem) { return false; }
	gpuMem->Offset = 0;
	return true;
}

//bool IRGPUMemory::UploadBufferToGPU(Handle<HBuffer> BufferHandle, size_t BufSize, Handle<GPUBuffer> MemoryHandle, bool Immediate)
//{
//	if (!Buffers.Length()) { return false; }
//
//	if (BufferHandle == INVALID_HANDLE || MemoryHandle == INVALID_HANDLE)
//	{
//		return false;
//	}
//
//	GPUBuffer* gpuMem = GetGPUMemory(MemoryHandle);
//
//	if (!gpuMem) { return false; }
//	if (gpuMem->Offset + BufSize > gpuMem->Size) { return false; }
//
//	TransferContext transferContext = {};
//	transferContext.From = BufferHandle;
//	transferContext.FromSize = BufSize;
//	transferContext.To = gpuMem->Handle;
//	transferContext.ToOffset = gpuMem->Offset;
//
//	gpuMem->Offset += static_cast<uint32>(BufSize);
//	Transfers.Push(Move(transferContext));
//
//	if (Immediate) { TransferToGPU(); }
//
//	return true;
//}

bool IRGPUMemory::BuildGPUBuffer(Handle<GPUBuffer> Hnd)
{
	GPUBuffer* buffer = GetGPUMemory(Hnd);
	if (!buffer) { return false; }
	if (buffer->Handle != INVALID_HANDLE) { return false; }
	gpu::CreateBuffer(*buffer, nullptr, buffer->Size);
	//Context.NewBuffer(*buffer, nullptr, buffer->Size, 1);
	return true;
}

void IRGPUMemory::BuildAll()
{
	for (GPUBuffer* buffer : Buffers)
	{
		if (buffer->Handle != INVALID_HANDLE) { continue; }
		gpu::CreateBuffer(*buffer, nullptr, buffer->Size);
		//Context.NewBuffer(*buffer, nullptr, buffer->Size, 1);
	}
}

void IRGPUMemory::Destroy()
{
	if (Buffers.Length())
	{
		for (GPUBuffer* gpuMem : Buffers)
		{
			gpu::DestroyBuffer(*gpuMem);
			//Context.DestroyBuffer(gpuMem->Handle);
		}
	}
}
