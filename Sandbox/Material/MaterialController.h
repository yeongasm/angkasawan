#pragma once
#ifndef ANGKASA1_SANDBOX_MATERIAL_MATERIAL_CONTROLLER_H
#define ANGKASA1_SANDBOX_MATERIAL_MATERIAL_CONTROLLER_H

#include "MaterialObject.h"

namespace sandbox
{

  class MaterialController
  {
  private:

    Map<size_t, Material> Materials;
    Map<size_t, MaterialDef> MaterialDefinitions;
    Ref<IAssetManager> pAssetManager;
    Ref<EngineImpl> pEngine;

    uint32 GetIndexForMaterialType(Ref<MaterialDef> pDefinition, EMaterialTypeFlagBit Type);

  public:

    MaterialController(IAssetManager& InAssetManager, EngineImpl& InEngine);
    ~MaterialController();

    DELETE_COPY_AND_MOVE(MaterialController)

    Handle<MaterialDef> CreateMaterialDefinition(const MaterialDefCreateInfo& CreateInfo);
    Ref<MaterialDef> GetMaterialDefinition(Handle<MaterialDef> Hnd);
    bool DestroyMaterialDefinition(Handle<MaterialDef>& Hnd);
    Handle<Material> CreateMaterial(const MaterialCreateInfo& CreateInfo);
    Ref<Material> GetMaterial(Handle<Material> Hnd);
    bool DestroyMaterial(Handle<MaterialDef>& Hnd);

  };

}

#endif // !ANGKASA1_SANDBOX_MATERIAL_MATERIAL_CONTROLLER_H
