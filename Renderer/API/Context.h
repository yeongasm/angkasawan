#pragma once
#ifndef LEARNVK_RENDERER_API_CONTEXT_H
#define LEARNVK_RENDERER_API_CONTEXT_H

#include "Vk/VulkanDriver.h"
#include "Library/Allocators/LinearAllocator.h"
#include "Library/Containers/Map.h"

template <typename Type>
using XxMap = Map<uint32, Type, XxHash<uint32, 1380144216U>>;

class SRDeviceStore
{
private:

	LinearAllocator&			Allocator;
	XxMap<SRMemoryBuffer*>		Buffers;
	XxMap<DescriptorSet*>		DescriptorSets;
	XxMap<DescriptorPool*>		DescriptorPools;
	XxMap<DescriptorLayout*>	DescriptorSetLayout;
	XxMap<RenderPass*>			RenderPass;
	XxMap<ImageSampler*>		Samplers;
	XxMap<SRPipeline>			Pipelines;

public:

	SRDeviceStore(LinearAllocator& InAllocator) :
		Allocator(InAllocator)
	{}

	~SRDeviceStore()
	{}

	DELETE_COPY_AND_MOVE(SRDeviceStore)


};

#endif // !LEARNVK_RENDERER_API_CONTEXT_H