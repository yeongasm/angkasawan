#pragma once
#ifndef ANGKASA1_SANDBOX_MATERIAL_MATERIAL_OBJECT_H
#define ANGKASA1_SANDBOX_MATERIAL_MATERIAL_OBJECT_H

#include "Library/Containers/Array.h"
#include "RenderAbstracts/Primitives.h"
#include "SandboxApp/Definitions.h"
#include "Asset/Assets.h"

namespace sandbox
{
  using EPbrMaterialTypeFlagBit = uint32;
  using EMaterialTypeFlagBit = uint32;

  struct MaterialType
  {
    EMaterialTypeFlagBit Type;
    uint32 Binding;
    Array<RefHnd<Texture>> Textures;
  };

  using MaterialTypeTable = StaticArray<MaterialType, SANDBOX_MAX_MATERIAL_TYPE_IN_DEFINITION>;

  struct MaterialDef
  {
    MaterialTypeTable MatTypes;
    Handle<SPipeline> PipelineHnd;
    Handle<SDescriptorSet> SetHnd;
    Handle<SImageSampler> SamplerHnd;
    uint32 MatHash = 0;
  };

  /* Defines the relation between a material type and it's BINDING SLOT in the shader */
  struct MaterialTypeBindingInfo
  {
    EMaterialTypeFlagBit Type;
    uint32 Binding;
  };

  struct MaterialDefCreateInfo
  {
    MaterialTypeBindingInfo* pTypeBindings;
    uint32 NumOfTypeBindings;
    Handle<SPipeline> PipelineHnd;
    Handle<SDescriptorSet> SetHnd;
    Handle<SImageSampler> SamplerHnd;
  };

  /* Defines the relationship between a material type and it's INDEX in it's definition. */
  struct MaterialTypeIndexInfo
  {
    EMaterialTypeFlagBit Type;
    uint32 Slot;
  };

  using MaterialIndexTable = StaticArray<MaterialTypeIndexInfo, SANDBOX_MAX_MATERIAL_TYPE_IN_DEFINITION>;

  struct Material
  {
    MaterialIndexTable MatIndices;
    RefHnd<MaterialDef> pDefinition;
    union
    {
      uint8 _mem[128] = { 0 };
      uint8 Constants;
    };
  };

  struct TextureTypeInfo
  {
    ETextureType Type;
    RefHnd<Texture> Hnd;
  };

  struct MaterialCreateInfo
  {
    TextureTypeInfo* pInfo;
    uint32 NumTextureTypes; // Project will only handle up to 8 texture types. Rest are ignored.
    Handle<MaterialDef> DefinitionHnd;
  };
}

#endif // !ANGKASA1_SANDBOX_MATERIAL_MATERIAL_OBJECT_H
