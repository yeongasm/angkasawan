#pragma once
#ifndef ANGKASA1_SANDBOX_MATERIAL_MATERIAL_CONTROLLER_H
#define ANGKASA1_SANDBOX_MATERIAL_MATERIAL_CONTROLLER_H

#include "MaterialObject.h"

namespace sandbox
{

  class MaterialController
  {
  private:

    astl::Map<size_t, Material> Materials;
    astl::Map<size_t, MaterialDef> MaterialDefinitions;
    astl::Ref<IAssetManager> pAssetManager;
    astl::Ref<EngineImpl> pEngine;
    astl::Ref<IRenderSystem> pRenderer;

    uint32 GetIndexForMaterialType(astl::Ref<MaterialDef> pDefinition, EMaterialTypeFlagBit Type);

  public:

    MaterialController(IAssetManager& InAssetManager, IRenderSystem& InRenderer, EngineImpl& InEngine);
    ~MaterialController();

    DELETE_COPY_AND_MOVE(MaterialController)

    void Initialize();
    void Terminate();

    Handle<MaterialDef> CreateMaterialDefinition(const MaterialDefCreateInfo& CreateInfo);
    astl::Ref<MaterialDef> GetMaterialDefinition(Handle<MaterialDef> Hnd);
    bool DestroyMaterialDefinition(Handle<MaterialDef>& Hnd);
    Handle<Material> CreateMaterial(const MaterialCreateInfo& CreateInfo);
    astl::Ref<Material> GetMaterial(Handle<Material> Hnd);
    bool DestroyMaterial(Handle<MaterialDef>& Hnd);

  };

}

#endif // !ANGKASA1_SANDBOX_MATERIAL_MATERIAL_CONTROLLER_H
