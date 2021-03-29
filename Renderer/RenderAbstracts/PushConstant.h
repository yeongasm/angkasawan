#pragma once
#ifndef LEARVK_RENDERER_RENDER_ABSTRACT_PUSH_CONSTANT_H
#define LEARVK_RENDERER_RENDER_ABSTRACT_PUSH_CONSTANT_H

#include "Library/Allocators/LinearAllocator.h"
#include "Library/Containers/Array.h"
#include "Primitives.h"
#include "RenderPlatform/API.h"

class IRPipelineManager;

struct PushConstantCreateInfo
{
	Handle<SRPipeline> PipelineHandle;
};

class RENDERER_API IRPushConstantManager
{
private:

	LinearAllocator& Allocator;
	IRPipelineManager& PipelineManager;
	Array<SRPushConstant*> PushConstantContainer;

public:

	Handle<SRPushConstant> PreviousHandle = INVALID_HANDLE;

	IRPushConstantManager(LinearAllocator& InAllocator, IRPipelineManager& InPipelineManager);
	~IRPushConstantManager();

	DELETE_COPY_AND_MOVE(IRPushConstantManager)

	Handle<SRPushConstant> CreateNewPushConstant(const PushConstantCreateInfo& CreateInfo);
	void BindPushConstant(Handle<SRPushConstant> Hnd);
	bool UpdatePushConstantData(Handle<SRPushConstant> Hnd, void* Data, size_t Size);
	//bool UpdatePushConstantData(Handle<SRPushConstant> Hnd, void* Data, size_t Size, size_t Offset);
};

#endif // !LEARVK_RENDERER_RENDER_ABSTRACT_PUSH_CONSTANT_H