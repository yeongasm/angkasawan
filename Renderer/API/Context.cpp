#include "Context.h"

SRDeviceStore::SRDeviceStore(LinearAllocator& InAllocator) :
	Allocator{ InAllocator },
	Buffers{},
	DescriptorSets{},
	DescriptorPools{},
	DescriptorSetLayout{},
	RenderPasses{},
	Samplers{},
	Pipelines{},
	Shaders{},
	Textures{}
{}

SRDeviceStore::~SRDeviceStore()
{
	Buffers.Release();
	DescriptorSets.Release();
	DescriptorPools.Release();
	DescriptorSetLayout.Release();
	RenderPasses.Release();
	Samplers.Release();
	Pipelines.Release();
	Shaders.Release();
	Textures.Release();
}

bool SRDeviceStore::DoesBufferExist(size_t Id)
{
	for (auto& [key, value] : Buffers)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesDescriptorSetExist(size_t Id)
{
	for (auto& [key, value] : DescriptorSets)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesDescriptorPoolExist(size_t Id)
{
	for (auto& [key, value] : DescriptorPools)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesDescriptorSetLayoutExist(size_t Id)
{
	for (auto& [key, value] : DescriptorSetLayout)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesRenderPassExist(size_t Id)
{
	for (auto& [key, value] : RenderPasses)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesImageSamplerExist(size_t Id)
{
	for (auto& [key, value] : Samplers)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesPipelineExist(size_t Id)
{
	for (auto& [key, value] : Pipelines)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesShaderExist(size_t Id)
{
	for (auto& [key, value] : Shaders)
	{
		if (key == Id) { return true; }
	}
	return false;
}

bool SRDeviceStore::DoesTextureExist(size_t Id)
{
	for (auto& [key, value] : Textures)
	{
		if (key == Id) { return true; }
	}
	return false;
}

size_t SRDeviceStore::GenerateId(const String128& Name)
{
	static size_t seed{ static_cast<size_t>(OS::GetPerfCounter()) };
	size_t id = 0;
	XXHash64(Name.C_Str(), Name.Length(), &id, seed);
	return id;
}

SRMemoryBuffer* SRDeviceStore::NewBuffer(size_t Id)
{
	if (DoesBufferExist(Id)) 
	{ 
		return nullptr; 
	}
	SRMemoryBuffer* resource = reinterpret_cast<SRMemoryBuffer*>(Allocator.Malloc(sizeof(SRMemoryBuffer)));
	Buffers.Insert(Id, resource);
	return resource;
}

SRMemoryBuffer* SRDeviceStore::GetBuffer(size_t Id)
{
	if (!DoesBufferExist(Id))
	{
		return nullptr;
	}
	return Buffers[Id];
}

bool SRDeviceStore::DeleteBuffer(size_t Id)
{
	if (!DoesBufferExist(Id))
	{
		return false;
	}
	Buffers.Remove(Id);
	return true;
}

DescriptorSet* SRDeviceStore::NewDescriptorSet(size_t Id)
{
	if (DoesDescriptorSetExist(Id))
	{
		return nullptr;
	}
	DescriptorSet* resource = reinterpret_cast<DescriptorSet*>(Allocator.Malloc(sizeof(DescriptorSet)));
	DescriptorSets.Insert(Id, resource);
	return resource;
}

DescriptorSet* SRDeviceStore::GetDescriptorSet(size_t Id)
{
	if (!DoesDescriptorSetExist(Id))
	{
		return nullptr;
	}
	return DescriptorSets[Id];
}

bool SRDeviceStore::DeleteDescriptorSet(size_t Id)
{
	if (!DoesDescriptorSetExist(Id))
	{
		return false;
	}
	DescriptorSets.Remove(Id);
	return true;
}

DescriptorPool* SRDeviceStore::NewDescriptorPool(size_t Id)
{
	if (DoesDescriptorPoolExist(Id))
	{
		return nullptr;
	}
	DescriptorPool* resource = reinterpret_cast<DescriptorPool*>(Allocator.Malloc(sizeof(DescriptorPool)));
	DescriptorPools.Insert(Id, resource);
	return resource;
}

DescriptorPool* SRDeviceStore::GetDescriptorPool(size_t Id)
{
	if (!DoesDescriptorPoolExist(Id))
	{
		return nullptr;
	}
	return DescriptorPools[Id];
}

bool SRDeviceStore::DeleteDescriptorPool(size_t Id)
{
	if (!DoesDescriptorPoolExist(Id))
	{
		return false;
	}
	DescriptorPools.Remove(Id);
	return true;
}

DescriptorLayout* SRDeviceStore::NewDescriptorSetLayout(size_t Id)
{
	if (DoesDescriptorSetLayoutExist(Id))
	{
		return nullptr;
	}
	DescriptorLayout* resource = reinterpret_cast<DescriptorLayout*>(Allocator.Malloc(sizeof(DescriptorLayout)));
	DescriptorSetLayout.Insert(Id, resource);
	return resource;
}

DescriptorLayout* SRDeviceStore::GetDescriptorSetLayout(size_t Id)
{
	if (!DoesDescriptorSetLayoutExist(Id))
	{
		return nullptr;
	}
	return DescriptorSetLayout[Id];
}

bool SRDeviceStore::DeleteDescriptorSetLayout(size_t Id)
{
	if (!DoesDescriptorSetLayoutExist(Id))
	{
		return false;
	}
	DescriptorSetLayout.Remove(Id);
	return true;
}

RenderPass* SRDeviceStore::NewRenderPass(size_t Id)
{
	if (DoesRenderPassExist(Id))
	{
		return nullptr;
	}
	RenderPass* resource = reinterpret_cast<RenderPass*>(Allocator.Malloc(sizeof(RenderPass)));
	RenderPasses.Insert(Id, resource);
	return resource;
}

RenderPass* SRDeviceStore::GetRenderPass(size_t Id)
{
	if (!DoesRenderPassExist(Id))
	{
		return nullptr;
	}
	return RenderPasses[Id];
}

bool SRDeviceStore::DeleteRenderPass(size_t Id)
{
	if (!DoesRenderPassExist(Id))
	{
		return false;
	}
	RenderPasses.Remove(Id);
	return true;
}

ImageSampler* SRDeviceStore::NewImageSampler(size_t Id)
{
	if (DoesImageSamplerExist(Id))
	{
		return nullptr;
	}
	ImageSampler* resource = reinterpret_cast<ImageSampler*>(Allocator.Malloc(sizeof(ImageSampler)));
	Samplers.Insert(Id, resource);
	return resource;
}

ImageSampler* SRDeviceStore::GetImageSampler(size_t Id)
{
	if (!DoesImageSamplerExist(Id))
	{
		return nullptr;
	}
	return Samplers[Id];
}

bool SRDeviceStore::DeleteImageSampler(size_t Id)
{
	if (!DoesImageSamplerExist(Id))
	{
		return false;
	}
	Samplers.Remove(Id);
	return true;
}

SRPipeline* SRDeviceStore::NewPipeline(size_t Id)
{
	if (DoesPipelineExist(Id))
	{
		return nullptr;
	}
	SRPipeline* resource = reinterpret_cast<SRPipeline*>(Allocator.Malloc(sizeof(SRPipeline)));
	Pipelines.Insert(Id, resource);
	return resource;
}

SRPipeline* SRDeviceStore::GetPipeline(size_t Id)
{
	if (!DoesPipelineExist(Id))
	{
		return nullptr;
	}
	return Pipelines[Id];
}

bool SRDeviceStore::DeletePipeline(size_t Id)
{
	if (!DoesPipelineExist(Id))
	{
		return false;
	}
	Pipelines.Remove(Id);
	return true;
}

SRShader* SRDeviceStore::NewShader(size_t Id)
{
	if (DoesShaderExist(Id))
	{
		return nullptr;
	}
	SRShader* resource = reinterpret_cast<SRShader*>(Allocator.Malloc(sizeof(SRShader)));
	Shaders.Insert(Id, resource);
	return resource;
}

SRShader* SRDeviceStore::GetShader(size_t Id)
{
	if (!DoesShaderExist(Id))
	{
		return nullptr;
	}
	return Shaders[Id];
}

bool SRDeviceStore::DeleteShader(size_t Id)
{
	if (!DoesShaderExist(Id))
	{
		return false;
	}
	Shaders.Remove(Id);
	return true;
}

SRTexture* SRDeviceStore::NewTexture(size_t Id)
{
	if (DoesTextureExist(Id))
	{
		return nullptr;
	}
	SRTexture* resource = reinterpret_cast<SRTexture*>(Allocator.Malloc(sizeof(SRTexture)));
	Textures.Insert(Id, resource);
	return resource;
}

SRTexture* SRDeviceStore::GetTexture(size_t Id)
{
	if (!DoesTextureExist(Id))
	{
		return nullptr;
	}
	return Textures[Id];
}

bool SRDeviceStore::DeleteTexture(size_t Id)
{
	if (!DoesTextureExist(Id))
	{
		return false;
	}
	Textures.Remove(Id);
	return true;
}

