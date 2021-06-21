#pragma once
#ifndef ANGKASA1_SANDBOX_SANDBOX_APP_DEFINITIONS_H
#define ANGKASA1_SANDBOX_SANDBOX_APP_DEFINITIONS_H

namespace sandbox
{
#define SANDBOX_MAX_MATERIAL_TYPE_IN_DEFINITION 8

  enum ESandboxAsset : uint32
  {
    Sandbox_Asset_Model   = 0,
    Sandbox_Asset_Shader  = 1,
    Sandbox_Asset_Texture = 2,
    Sandbox_Asset_Material = 3
  };

  enum class EPbrMaterialType : uint32
  {
    Material_Type_Albedo = 0,
    Material_Type_Roughness = 1,
    Material_Type_Metallic = 2,
    Material_Type_Ao = 3
  };

}

#endif // !ANGKASA1_SANDBOX_SANDBOX_APP_DEFINITIONS_H