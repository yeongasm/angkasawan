#pragma once
#ifndef LEARNVK_RENDERER_API_CONTEXT_H
#define LEARNVK_RENDERER_API_CONTEXT_H

#include "Vk/VulkanDriver.h"
#include "Library/Allocators/LinearAllocator.h"
#include "Library/Containers/Map.h"
#include "Library/Containers/String.h"

struct SRMemoryBuffer;
struct DescriptorSet;
struct DescriptorPool;
struct DescriptorLayout;
struct RenderPass;
struct ImageSampler;
struct SRPipeline;

template <typename Type>
using XxMap = Map<size_t, Type, XxHash<size_t>>;

/**
* 
*/
struct SRDeviceStore
{
	LinearAllocator&			Allocator;
	XxMap<SRMemoryBuffer*>		Buffers;
	XxMap<DescriptorSet*>		DescriptorSets;
	XxMap<DescriptorPool*>		DescriptorPools;
	XxMap<DescriptorLayout*>	DescriptorSetLayout;
	XxMap<RenderPass*>			RenderPasses;
	XxMap<ImageSampler*>		Samplers;
	XxMap<SRPipeline*>			Pipelines;
	XxMap<SRShader*>			Shaders;
	XxMap<SRTexture*>			Textures;

	bool DoesBufferExist(size_t Id);
	bool DoesDescriptorSetExist(size_t Id);
	bool DoesDescriptorPoolExist(size_t Id);
	bool DoesDescriptorSetLayoutExist(size_t Id);
	bool DoesRenderPassExist(size_t Id);
	bool DoesImageSamplerExist(size_t Id);
	bool DoesPipelineExist(size_t Id);
	bool DoesShaderExist(size_t Id);
	bool DoesTextureExist(size_t Id);

	SRDeviceStore(LinearAllocator& InAllocator);
	~SRDeviceStore();

	DELETE_COPY_AND_MOVE(SRDeviceStore)

	size_t GenerateId(const String128& Name);

	SRMemoryBuffer* NewBuffer(size_t Id);
	SRMemoryBuffer* GetBuffer(size_t Id);
	bool DeleteBuffer(size_t Id);

	DescriptorSet* NewDescriptorSet(size_t Id);
	DescriptorSet* GetDescriptorSet(size_t Id);
	bool DeleteDescriptorSet(size_t Id);

	DescriptorPool* NewDescriptorPool(size_t Id);
	DescriptorPool* GetDescriptorPool(size_t Id);
	bool DeleteDescriptorPool(size_t Id);

	DescriptorLayout* NewDescriptorSetLayout(size_t Id);
	DescriptorLayout* GetDescriptorSetLayout(size_t Id);
	bool DeleteDescriptorSetLayout(size_t Id);

	RenderPass* NewRenderPass(size_t Id);
	RenderPass* GetRenderPass(size_t Id);
	bool DeleteRenderPass(size_t Id);

	ImageSampler* NewImageSampler(size_t Id);
	ImageSampler* GetImageSampler(size_t Id);
	bool DeleteImageSampler(size_t Id);

	SRPipeline* NewPipeline(size_t Id);
	SRPipeline* GetPipeline(size_t Id);
	bool DeletePipeline(size_t Id);

	SRShader* NewShader(size_t Id);
	SRShader* GetShader(size_t Id);
	bool DeleteShader(size_t Id);

	SRTexture* NewTexture(size_t Id);
	SRTexture* GetTexture(size_t Id);
	bool DeleteTexture(size_t Id);
};

#endif // !LEARNVK_RENDERER_API_CONTEXT_H