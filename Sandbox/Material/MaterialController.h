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

  public:

    MaterialController(IAssetManager& InAssetManager, EngineImpl& InEngine);
    ~MaterialController();

    DELETE_COPY_AND_MOVE(MaterialController)

    RefHnd<MaterialDef> CreateMaterialDefinition(const MaterialDefCreateInfo& CreateInfo);
    bool DestroyMaterialDefinition(RefHnd<MaterialDef> Hnd);
    RefHnd<Material> CreateMaterial(const MaterialCreateInfo& CreateInfo);
    bool DestroyMaterial(RefHnd<MaterialDef> Hnd);

  };

}

#endif // !ANGKASA1_SANDBOX_MATERIAL_MATERIAL_CONTROLLER_H
