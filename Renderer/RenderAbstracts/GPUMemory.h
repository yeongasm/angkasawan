#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_GPU_MEMORY_H
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_GPU_MEMORY_H

#include "Library/Containers/Array.h"
#include "Library/Allocators/LinearAllocator.h"
#include "Assets/GPUHandles.h"
#include "RenderPlatform/API.h"
#include "RenderAbstracts/Primitives.h"

class RenderContext;
class RenderSystem;

using GPUBuffer = SRMemoryBuffer;

struct GPUMemoryAllocateInfo : public SRMemoryBufferBase
{
	bool DeferAllocation;
};

class RENDERER_API IRGPUMemory
{
private:

	friend class RenderContext;
	friend class RenderSystem;

	enum ExistEnum : size_t
	{
		Resource_Not_Exist = -1
	};

	LinearAllocator& Allocator;
	Array<GPUBuffer*> Buffers;

	size_t DoesGPUMemoryExist(size_t Id);

public:

	IRGPUMemory(LinearAllocator& InAllocator);
	~IRGPUMemory();

	DELETE_COPY_AND_MOVE(IRGPUMemory)

	Handle<GPUBuffer>	AllocateNewMemory		(const GPUMemoryAllocateInfo& AllocInfo);
	Handle<GPUBuffer>	GetGPUMemoryHandleWithId(uint32 Id);
	GPUBuffer*			GetGPUMemory			(Handle<GPUBuffer> Hnd);

	bool FlushGPUMemory		(Handle<GPUBuffer> Hnd);
	bool BuildGPUBuffer		(Handle<GPUBuffer> Hnd);
	void BuildAll			();

	void Destroy();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_GPU_MEMORY_H