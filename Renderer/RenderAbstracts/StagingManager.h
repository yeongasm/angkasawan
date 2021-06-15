#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H

#include "Library/Containers/Array.h"
#include "Library/Containers/Pair.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "Primitives.h"

class IRenderSystem;
class IRenderDevice;

using VkCommandPool = struct VkCommandPool_T*;
using VkBuffer = struct VkBuffer_T*;

class RENDERER_API IStagingManager
{
private:

	friend class IRenderSystem;

	struct UploadContext
	{
		SMemoryBuffer SrcBuffer;
		Ref<SMemoryBuffer> DstBuffer;
		size_t Size;
		EQueueType DstQueue;
	};

	Array<UploadContext> Uploads;
	Ref<IRenderSystem> pRenderer;
	VkCommandPool TxPool;
	bool MakeTransfer;

	decltype(auto) GetQueueForType(EQueueType Type);

	bool ShouldMakeTransfer() const;

public:

	IStagingManager(IRenderSystem& InRenderer);
	~IStagingManager();

	DELETE_COPY_AND_MOVE(IStagingManager)

	bool Initialize();
	void Terminate();
	bool StageVertexData(void* Data, size_t Size);
	bool StageIndexData(void* Data, size_t Size);
	bool StageDataForBuffer(void* Data, size_t Size, Handle<SMemoryBuffer> DstHnd, EQueueType DstQueue);
	bool Upload();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_STAGING_MANAGER_H