#include "MaterialController.h"

namespace sandbox
{
  MaterialController::MaterialController(IAssetManager& InAssetManager, EngineImpl& InEngine) :
    Materials{},
    MaterialDefinitions{},
    pAssetManager(&InAssetManager),
    pEngine(&InEngine)
  {
    pEngine->CreateNewResourceCache(Sandbox_Asset_Material);
    Materials.Reserve(32);
    MaterialDefinitions.Reserve(16);
  }

  MaterialController::~MaterialController()
  {
    Materials.Release();
    MaterialDefinitions.Release();
    pEngine->DeleteResourceCacheForType(Sandbox_Asset_Material);
  }

  RefHnd<MaterialDef> MaterialController::CreateMaterialDefinition(const MaterialDefCreateInfo& CreateInfo)
  {
    if (CreateInfo.SamplerHnd == INVALID_HANDLE ||
      CreateInfo.PipelineHnd == INVALID_HANDLE ||
      CreateInfo.SetHnd == INVALID_HANDLE)
    {
      return NullPointer();
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
        return NullPointer();
      }
    }

    Ref<ResourceCache> pCache = pEngine->FetchResourceCacheForType(Sandbox_Asset_Material);
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

    return RefHnd<MaterialDef>(Handle<MaterialDef>(id), pDefinition);
  }

}
