#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_BUFFER_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_BUFFER_H

#include "RenderPlatform/API.h"
#include "Library/Containers/Array.h"
#include "Library/Allocators/LinearAllocator.h"
#include "Primitives.h"

class RenderContext;
class Model;

using MemoryAllocateInfo = SRMemoryBufferBase;

/**
* Vulkan staging buffer to GPU buffer abstraction layer.
* For every buffer created, the class creates a CPU side buffer and a GPU side buffer.
*/
class RENDERER_API IRenderMemoryManager
{
private:

	friend class RenderSystem;
	enum EExistEnum : size_t { Resource_Not_Exist = -1 };

	LinearAllocator& Allocator;
	Array<SRMemoryBuffer*> Buffers;
	Array<SRMemoryTransferContext> Transfers;

	size_t DoesBufferExist(uint32 Id);

public:

	IRenderMemoryManager(LinearAllocator& InAllocator);
	~IRenderMemoryManager();

	DELETE_COPY_AND_MOVE(IRenderMemoryManager)

	Handle<SRMemoryBuffer> AllocateNewBuffer(const MemoryAllocateInfo& AllocInfo);
	Handle<SRMemoryBuffer> GetBufferHandleWithId(uint32 Id);
	SRMemoryBuffer* GetBuffer(Handle<SRMemoryBuffer> Hnd);

	bool UploadData(SRMemoryBuffer& Buffer, Handle<SRMemoryBuffer> Hnd, bool Immediate = true);

	//bool UploadModel(Model& InModel, Handle<SRMemoryBuffer> Hnd, bool Immediate = true);
	//void UploadTexture();

	void TransferToGPU();
	bool FlushBuffer(Handle<SRMemoryBuffer> Hnd);
	void FlushAllBuffer();
	bool BuildBuffer(Handle<SRMemoryBuffer> Hnd);
	void Destroy();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_BUFFER_H