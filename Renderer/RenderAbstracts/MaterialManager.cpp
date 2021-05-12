#include "MaterialManager.h"
#include "Library/Algorithms/Hash.h"
#include "API/Context.h"
#include "DescriptorSets.h"
#include "PipelineManager.h"
#include "PushConstant.h"
#include "Assets/Assets.h"

static uint32 g_ImgSamplerSeed = static_cast<uint32>(OS::GetPerfCounter());

MaterialType* IRMaterialManager::GetMaterialTypeFromDefinition(Handle<MaterialDefinition> Hnd, uint32 Type)
{
	if (Hnd == INVALID_HANDLE) { return nullptr; }
	MaterialDefinition* definition = MatDefinitionContainer[Hnd];
	if (!definition) { return nullptr; }
	MaterialType* type = nullptr;
	for (MaterialType* t : definition->Types)
	{
		if (t->Type != Type) { continue; }
		type = t;
		break;
	}
	return type;
}

IRMaterialManager::IRMaterialManager(
	LinearAllocator& InAllocator, 
	IRPipelineManager& InPipelineManager, 
	IRDescriptorManager& InDescriptorManager,
	IRAssetManager& InAssetManager,
	IRPushConstantManager& InPushConstantManager
) :
	Allocator(InAllocator),
	PipelineManager(InPipelineManager),
	DescriptorManager(InDescriptorManager),
	AssetManager(InAssetManager),
	PushConstantManager(InPushConstantManager),
	MatDefinitionContainer(),
	ImageSamplerContainer(),
	MaterialContainer()
{}

IRMaterialManager::~IRMaterialManager()
{
	MatDefinitionContainer.Release();
	ImageSamplerContainer.Release();
	MaterialContainer.Release();
}

Handle<MaterialDefinition> IRMaterialManager::CreateMaterialDefinition(const MaterialDefinitionCreateInfo& CreateInfo)
{
	if (CreateInfo.PipelineHandle == INVALID_HANDLE ||
		CreateInfo.LayoutHandle == INVALID_HANDLE ||
		CreateInfo.SetHandle == INVALID_HANDLE)
		//CreateInfo.SamplerHandle == INVALID_HANDLE ||
		//!CreateInfo.pDescriptorSetCreateInfo ||
		//!CreateInfo.pLayoutCreateInfo)
	{
		return INVALID_HANDLE;
	}

	MaterialDefinition* definition = reinterpret_cast<MaterialDefinition*>(Allocator.Malloc(sizeof(MaterialDefinition)));
	IMemory::InitializeObject(definition);

	definition->Id = CreateInfo.Id;
	definition->PipelineHandle = CreateInfo.PipelineHandle;
	definition->DescriptorSetHandle = CreateInfo.SetHandle;
	//definition->LayoutHandle = CreateInfo.LayoutHandle;

	//definition->Sampler = ImageSamplerContainer[CreateInfo.SamplerHandle];

	//Handle<DescriptorLayout> layoutHnd = DescriptorManager.CreateDescriptorLayout(*CreateInfo.pLayoutCreateInfo);

	//uint32 countOfTypes = static_cast<uint32>(CreateInfo.Types.Length());
	for (auto& [t, binding] : CreateInfo.Types)
	{
		//auto& [t, binding] = CreateInfo.Types[i];

		DescriptorLayoutBindingCreateInfo bindingInfo = {};
		bindingInfo.Binding = binding;
		bindingInfo.DescriptorCount = 64; // NOTE(Ygsm): hard coded for now ...
		bindingInfo.LayoutHandle = CreateInfo.LayoutHandle;
		bindingInfo.ShaderStages = Shader_Type_Fragment;
		bindingInfo.Type = Descriptor_Type_Sampled_Image;

		DescriptorManager.AddDescriptorSetLayoutBinding(bindingInfo);

		MaterialType* type = reinterpret_cast<MaterialType*>(Allocator.Malloc(sizeof(MaterialType)));
		type->Binding = binding;
		type->Type = t;
		definition->Types.Push(Move(type));
	}

	//DescriptorManager.AddDescriptorSetLayoutBinding(layoutHnd, countOfTypes, 0, 0, Descriptor_Type_Sampler, Shader_Type_Fragment);
	//CreateInfo.pDescriptorSetCreateInfo->DescriptorLayoutHandle = layoutHnd;

	//Handle<DescriptorSet> setHandle = DescriptorManager.CreateDescriptorSet(*CreateInfo.pDescriptorSetCreateInfo);


	size_t idx = MatDefinitionContainer.Push(definition);

	return Handle<MaterialDefinition>(idx);
}

Handle<MaterialDefinition> IRMaterialManager::GetMaterialDefinitionHandleWithId(uint32 Id)
{
	size_t hnd = INVALID_HANDLE;
	MaterialDefinition* definition = nullptr;
	for (size_t i = 0; i < MatDefinitionContainer.Length(); i++)
	{
		definition = MatDefinitionContainer[i];
		if (definition->Id != Id) { continue; }
		hnd = i;
		break;
	}
	return Handle<MaterialDefinition>(hnd);
}

//size_t IRMaterialManager::PushTextureToBinding(Handle<Texture> TextureHandle, Handle<MaterialDefinition> DefinitionHandle, uint32 Binding, Handle<ImageSampler> SamplerHandle)
//{
//	if (TextureHandle == INVALID_HANDLE ||
//		DefinitionHandle == INVALID_HANDLE ||
//		SamplerHandle == INVALID_HANDLE)
//	{
//		return -1;
//	}
//
//	Texture* texture = AssetManager.GetTextureWithHandle(TextureHandle);
//	MaterialDefinition* definition = MatDefinitionContainer[DefinitionHandle];
//	ImageSampler* sampler = ImageSamplerContainer[SamplerHandle];
//	MaterialType* type = nullptr;
//
//	if (!texture || !definition || !sampler) { return -1; }
//
//	texture->Sampler = sampler;
//
//	for (MaterialType* t : definition->Types)
//	{
//		if (t->Binding != Binding) { continue; }
//		type = t;
//		break;
//	}
//
//	if (!type) { return -1; }
//
//	return type->Textures.Push(texture);;
//}

Handle<ImageSampler> IRMaterialManager::CreateImageSampler(const ImageSamplerCreateInfo& CreateInfo)
{
	uint32 id = 0;
	XXHash32(CreateInfo.Name.First(), CreateInfo.Name.Length(), &id, g_ImgSamplerSeed);

	ImageSampler* sampler = reinterpret_cast<ImageSampler*>(Allocator.Malloc(sizeof(ImageSampler)));
	IMemory::InitializeObject(sampler);
	IMemory::Memcpy(sampler, &CreateInfo, sizeof(ImageSamplerBase));
	sampler->Id = id;

	gpu::CreateSampler(*sampler);
	size_t idx = ImageSamplerContainer.Push(sampler);

	return Handle<ImageSampler>(idx);
}

Handle<ImageSampler> IRMaterialManager::GetImageSamplerHandleWithName(const String128& Name)
{
	uint32 id = 0;
	size_t hnd = INVALID_HANDLE;
	XXHash32(Name.First(), Name.Length(), &id, g_ImgSamplerSeed);

	ImageSampler* sampler = nullptr;
	for (size_t i = 0; i < ImageSamplerContainer.Length(); i++)
	{
		sampler = ImageSamplerContainer[i];
		if (sampler->Id != id) { continue; }
		hnd = i;
		break;
	}

	return Handle<ImageSampler>(hnd);
}

bool IRMaterialManager::UpdateMaterialDefinition(Handle<MaterialDefinition> Hnd)
{
	if (Hnd == INVALID_HANDLE) { return false; }

	MaterialDefinition* definition = MatDefinitionContainer[Hnd];

	if (!definition) { return false; }

	for (MaterialType* type : definition->Types)
	{
		DescriptorManager.QueueTexturesForUpdate(
			definition->DescriptorSetHandle, 
			type->Binding, 
			type->Textures.First(), 
			static_cast<uint32>(type->Textures.Length())
		);
	}

	return true;
}

Handle<SRMaterial> IRMaterialManager::CreateNewMaterial(const MaterialCreateInfo& CreateInfo)
{
	if (CreateInfo.DefinitionHandle == INVALID_HANDLE || !CreateInfo.Textures.Length())
	{
		return INVALID_HANDLE;
	}

	MaterialDefinition* definition = MatDefinitionContainer[CreateInfo.DefinitionHandle];
	ImageSampler* sampler = ImageSamplerContainer[CreateInfo.SamplerHandle];

	if (!definition || !sampler) { return INVALID_HANDLE; }

	uint32 id = 0;
	XXHash32(CreateInfo.Name.C_Str(), CreateInfo.Name.Length(), &id, g_ImgSamplerSeed);
	
	SRMaterial* material = reinterpret_cast<SRMaterial*>(Allocator.Malloc(sizeof(SRMaterial)));
	IMemory::InitializeObject(material);

	material->Id = id;

	for (auto& [t, hnd] : CreateInfo.Textures)
	{
		MaterialType* type = GetMaterialTypeFromDefinition(CreateInfo.DefinitionHandle, t);
		Texture* texture = AssetManager.GetTextureWithHandle(hnd);
		texture->Sampler = sampler;
		uint32 textureIdx = static_cast<uint32>(type->Textures.Push(texture));
		material->TextureIndices.Push({ t, textureIdx });
	}

	size_t idx = MaterialContainer.Push(material);

	PushConstantCreateInfo info = {};
	info.PipelineHandle = definition->PipelineHandle;

	material->PushConstantHandle = PushConstantManager.CreateNewPushConstant(info);

	for (auto& [type, index] : material->TextureIndices)
	{
		PushConstantManager.UpdatePushConstantData(material->PushConstantHandle, &index, sizeof(index));
	}

	return Handle<SRMaterial>(idx);
}

Handle<SRMaterial> IRMaterialManager::GetMaterialHandleWithName(const String128 Name)
{
	uint32 id = 0;
	XXHash32(Name.C_Str(), Name.Length(), &id, g_ImgSamplerSeed);

	SRMaterial* material = nullptr;

	for (size_t i = 0; i < MaterialContainer.Length(); i++)
	{
		material = MaterialContainer[i];
		if (material->Id != id) { continue; }
		return Handle<SRMaterial>(i);
	}

	return Handle<SRMaterial>(INVALID_HANDLE);
}

SRMaterial* IRMaterialManager::GetMaterialWithHandle(Handle<SRMaterial> Hnd)
{
	SRMaterial* material = MaterialContainer[Hnd];
	return material;
}

void IRMaterialManager::Destroy()
{
	for (ImageSampler* sampler : ImageSamplerContainer)
	{
		gpu::DestroySampler(*sampler);
	}
}

