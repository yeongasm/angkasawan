#pragma once
#ifndef LEANRVK_RENDERER_RENDER_ABSTRACTS_MATERIAL_MANAGER_H
#define LEANRVK_RENDERER_RENDER_ABSTRACTS_MATERIAL_MANAGER_H

#include "Library/Containers/String.h"
#include "Library/Containers/Pair.h"
#include "Library/Allocators/LinearAllocator.h"
#include "Primitives.h"
#include "RenderPlatform/API.h"

class IRPipelineManager;
class IRDescriptorManager;
class IRAssetManager;
class IRPushConstantManager;
struct DescriptorBinding;
struct DescriptorSet;
struct DescriptorLayoutCreateInfo;
struct DescriptorSetCreateInfo;
struct SRPipeline;
struct SRPushConstant;

struct MaterialType
{
	uint32 Type;
	uint32 Binding;
	Array<Texture*>	Textures;
};

struct SRMaterialId
{
	uint32 Id;
};

struct MaterialDefinition : public SRMaterialId
{
	Array<MaterialType*> Types;
	Handle<SRPipeline> PipelineHandle;
	Handle<DescriptorSet> DescriptorSetHandle;
	//Handle<DescriptorLayout> LayoutHandle;
};

struct MaterialDefinitionCreateInfo : public SRMaterialId
{
	using Type = uint32;
	using Binding = uint32;

	Array<Pair<Type, Binding>> Types;
	Handle<SRPipeline> PipelineHandle;
	Handle<DescriptorLayout> LayoutHandle;
	Handle<DescriptorSet> SetHandle;
};

struct SRMaterial : public SRMaterialId
{
	using Type = uint32;
	Array<Pair<Type, uint32>> TextureIndices;
	Handle<SRPushConstant> PushConstantHandle;
};

struct MaterialCreateInfo
{
	using Type = uint32;
	using TextureTypePairs = Array<Pair<Type, Handle<Texture>>>;

	String128 Name;
	TextureTypePairs Textures;
	Handle<MaterialDefinition> DefinitionHandle;
	Handle<ImageSampler> SamplerHandle;
};

/**
* Bindless material system.
*/
class RENDERER_API IRMaterialManager
{
private:

	friend class IRDrawManager;

	LinearAllocator& Allocator;
	IRPipelineManager& PipelineManager;
	IRDescriptorManager& DescriptorManager;
	IRAssetManager& AssetManager;
	IRPushConstantManager& PushConstantManager;

	Array<MaterialDefinition*> MatDefinitionContainer;
	Array<ImageSampler*> ImageSamplerContainer;
	Array<SRMaterial*> MaterialContainer;

	MaterialType* GetMaterialTypeFromDefinition(Handle<MaterialDefinition> Hnd, uint32 Type);

public:

	IRMaterialManager(
		LinearAllocator& InAllocator,
		IRPipelineManager& InPipelineManager,
		IRDescriptorManager& InDescriptorManager,
		IRAssetManager& InAssetManager,
		IRPushConstantManager& InPushConstantManager
	);

	~IRMaterialManager();

	DELETE_COPY_AND_MOVE(IRMaterialManager)

	Handle<MaterialDefinition> CreateMaterialDefinition(const MaterialDefinitionCreateInfo& CreateInfo);
	Handle<MaterialDefinition> GetMaterialDefinitionHandleWithId(uint32 Id);

	//size_t PushTextureToBinding(Handle<Texture> TextureHandle, Handle<MaterialDefinition> DefinitionHandle, uint32 Binding, Handle<ImageSampler> SamplerHandle);
	Handle<ImageSampler> CreateImageSampler(const ImageSamplerCreateInfo& CreateInfo);
	Handle<ImageSampler> GetImageSamplerHandleWithName(const String128& Name);
	bool UpdateMaterialDefinition(Handle<MaterialDefinition> Hnd);

	Handle<SRMaterial> CreateNewMaterial(const MaterialCreateInfo& CreateInfo);
	Handle<SRMaterial> GetMaterialHandleWithName(const String128 Name);
	SRMaterial* GetMaterialWithHandle(Handle<SRMaterial> Hnd);

	void Destroy();
};

#endif // !LEANRVK_RENDERER_RENDER_ABSTRACTS_MATERIAL_MANAGER_H