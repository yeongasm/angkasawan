#include "MaterialController.h"

namespace sandbox
{
  uint32 MaterialController::GetIndexForMaterialType(Ref<MaterialDef> pDefinition, EMaterialTypeFlagBit Type)
  {
    const uint32 length = static_cast<uint32>(pDefinition->MatTypes.Length());
    for (uint32 i = 0; i < length; i++)
    {
      if (pDefinition->MatTypes[i].Type == Type) { return i; }
    }
    return -1;
  }
  MaterialController::MaterialController(IAssetManager& InAssetManager, EngineImpl& InEngine) :
    Materials{},
    MaterialDefinitions{},
    pAssetManager(&InAssetManager),
    pEngine(&InEngine)
  {
    pEngine->CreateNewResourceCache(Sandbox_Asset_Material_Definition);
    pEngine->CreateNewResourceCache(Sandbox_Asset_Material);
    Materials.Reserve(32);
    MaterialDefinitions.Reserve(16);
  }

  MaterialController::~MaterialController()
  {
    Materials.Release();
    MaterialDefinitions.Release();
    pEngine->DeleteResourceCacheForType(Sandbox_Asset_Material_Definition);
    pEngine->DeleteResourceCacheForType(Sandbox_Asset_Material);
  }

  Handle<MaterialDef> MaterialController::CreateMaterialDefinition(const MaterialDefCreateInfo& CreateInfo)
  {
    if (CreateInfo.SamplerHnd == INVALID_HANDLE ||
      CreateInfo.PipelineHnd == INVALID_HANDLE ||
      CreateInfo.SetHnd == INVALID_HANDLE)
    {
      return INVALID_HANDLE;
    }

    uint32 numBindings = (CreateInfo.NumOfTypeBindings > SANDBOX_MAX_MATERIAL_TYPE_IN_DEFINITION) ?
      SANDBOX_MAX_MATERIAL_TYPE_IN_DEFINITION : CreateInfo.NumOfTypeBindings;

    uint32 hashSum = 0;
    MaterialTypeBindingInfo* pTypeBinding = nullptr;
    for (uint32 i = 0; i < numBindings; i++)
    {
      pTypeBinding = &CreateInfo.pTypeBindings[i];
      hashSum += (1 | ((pTypeBinding->Binding | (pTypeBinding->Type << 2)) < i));
    }
    uint32 outHash = 0;
    XXHash32(&hashSum, sizeof(uint32), &outHash);

    for (auto& [key, definition] : MaterialDefinitions)
    {
      if (definition.MatHash == outHash)
      {
        return INVALID_HANDLE;
      }
    }

    Ref<ResourceCache> pCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Material_Definition);
    uint32 id = pCache->Create();
    Ref<Resource> pResource = pCache->Get(id);
    pResource->Type = Sandbox_Asset_Material;

    Ref<MaterialDef> pDefinition = &MaterialDefinitions.Insert(id, MaterialDef());
    pDefinition->MatHash = outHash;
    pDefinition->PipelineHnd = CreateInfo.PipelineHnd;
    pDefinition->SamplerHnd = CreateInfo.SamplerHnd;
    pDefinition->SetHnd = CreateInfo.SetHnd;
    
    for (uint32 i = 0; i < numBindings; i++)
    {
      MaterialType& type = pDefinition->MatTypes.Insert(MaterialType());
      pTypeBinding = &CreateInfo.pTypeBindings[i];
      type.Type = pTypeBinding->Type;
      type.Binding = pTypeBinding->Binding;
    }

    return id;
  }

  Ref<MaterialDef> MaterialController::GetMaterialDefinition(Handle<MaterialDef> Hnd)
  {
    if (Hnd == INVALID_HANDLE) { return NULLPTR; }
    return Ref<MaterialDef>(&MaterialDefinitions[Hnd]);
  }

  bool MaterialController::DestroyMaterialDefinition(Handle<MaterialDef>& Hnd)
  {
    Ref<ResourceCache> pCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Material_Definition);
    Ref<Resource> pResource = pCache->Get(Hnd);

    if (pCache->IsReferenced(Hnd)) { return false; }

    Ref<MaterialDef> pDefinition = &MaterialDefinitions[Hnd];

    if (pDefinition->NumOfMaterials) { return false; }

    MaterialDefinitions.Remove(Hnd);
    Hnd = INVALID_HANDLE;

    return true;
  }

  Handle<Material> MaterialController::CreateMaterial(const MaterialCreateInfo& CreateInfo)
  {
    if (CreateInfo.DefinitionHnd == INVALID_HANDLE) { return INVALID_HANDLE; }
    Ref<MaterialDef> pDefinition = &MaterialDefinitions[CreateInfo.DefinitionHnd];
    uint32 numTextures = (CreateInfo.NumTextureTypes > SANDBOX_MAX_MATERIAL_TYPE_IN_DEFINITION) ?
      SANDBOX_MAX_MATERIAL_TYPE_IN_DEFINITION : CreateInfo.NumTextureTypes;

    Ref<ResourceCache> pTextureCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Texture);
    Ref<ResourceCache> pMatCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Material);
    TextureTypeInfo* pTexInfo = nullptr;

    uint32 id = pMatCache->Create();
    Material& material = Materials.Insert(id, Material());
    material.pDefinition = pDefinition;

    for (uint32 i = 0; i < numTextures; i++)
    {
      pTexInfo = &CreateInfo.pInfo[i];
      pTextureCache->AddRef(pTexInfo->Hnd);

      Ref<Texture> pTexture = pAssetManager->GetTextureWithHandle(pTexInfo->Hnd);
      uint32 index = GetIndexForMaterialType(pDefinition, pTexInfo->Type);
      VKT_ASSERT(pTexture && "Texture does not exist!");
      VKT_ASSERT((index != -1) && "Material type does not exist in material definition.");


      uint32 matSlot = static_cast<uint32>(pDefinition->MatTypes[index].Textures.Push(RefHnd<Texture>(pTexInfo->Hnd, pTexture)));
      material.MatIndices.Push({ pTexInfo->Type, matSlot });
    }

    pDefinition->NumOfMaterials++;

    return Handle<Material>(id);
  }

  Ref<Material> MaterialController::GetMaterial(Handle<Material> Hnd)
  {
    if (Hnd == INVALID_HANDLE) { return NULLPTR; }
    return Ref<Material>(&Materials[Hnd]);
  }

  bool MaterialController::DestroyMaterial(Handle<MaterialDef>& Hnd)
  {
    if (Hnd == INVALID_HANDLE) { return false; }

    Ref<ResourceCache> pMatCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Material);
    Ref<ResourceCache> pTexCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Texture);

    if (pMatCache->IsReferenced(Hnd)) { return false; }

    Ref<Material> pMaterial = &Materials[Hnd];
    const uint32 numTextures = static_cast<uint32>(pMaterial->MatIndices.Length());

    for (uint32 i = 0; i < numTextures; i++)
    {
      MaterialTypeIndexInfo* pMatTypeInfo = &pMaterial->MatIndices[i];
      uint32 indexOfTexture = pMatTypeInfo->Slot;
      uint32 indexOfTexType = GetIndexForMaterialType(pMaterial->pDefinition, pMatTypeInfo->Type);

      RefHnd<Texture> pTexture = pMaterial->pDefinition->MatTypes[indexOfTexType].Textures[indexOfTexture];
      pTexCache->DeRef(pTexture);
      pMaterial->pDefinition->MatTypes[indexOfTexType].Textures.PopAt(indexOfTexture, false);
    }

    return true;
  }



}
