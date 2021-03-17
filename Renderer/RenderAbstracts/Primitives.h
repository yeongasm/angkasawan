#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H

#include "Library/Containers/Buffer.h"
#include "Library/Containers/Array.h"
#include "Library/Containers/Path.h"
#include "Library/Containers/Bitset.h"
#include "Library/Math/Math.h"
#include "API/RendererFlagBits.h"
#include "API/ShaderAttribute.h"
#include "Assets/GPUHandles.h"

/**
* TODO(Ygsm):
* [x] Move models and meshes into this file.
* [x] Move textures into this file.
* [x] Move shaders into this file.
*/

class IRAssetManager;
class IRenderMemoryManager;

struct SRMemoryBufferLocality
{
	EBufferLocality Locality;
};

struct SRMemoryBufferBase
{
	uint32			Id;
	BitSet<uint32>	Type;
	size_t			Size;
};

struct SRMemoryBuffer : public SRMemoryBufferBase, public SRMemoryBufferLocality
{
	size_t Offset;
	Handle<HBuffer> Handle;
};

struct SRMemoryTransferContext
{
	SRMemoryBuffer SrcBuffer;
	size_t SrcOffset;
	size_t SrcSize;
	SRMemoryBuffer* DstBuffer;
	size_t DstOffset;
};

struct SRVertex
{
	math::vec3 Position;
	math::vec3 Normal;
	math::vec3 Tangent;
	math::vec3 Bitangent;
	math::vec2 TexCoord;
};

struct ModelCreateInfo
{
	String128 Name;
};

struct ModelImportInfo : public ModelCreateInfo
{
	FilePath Path;
	IRAssetManager* AssetManager;
	Handle<SRMemoryBuffer> VtxMemHandle;
	Handle<SRMemoryBuffer> IdxMemHandle;
};


struct MeshCreateInfo : public ModelCreateInfo
{
	Array<SRVertex> Vertices;
	Array<uint32> Indices;
	Handle<SRMemoryBuffer> VtxMemHandle;
	Handle<SRMemoryBuffer> IdxMemHandle;
};

struct Mesh : public ModelCreateInfo
{
	uint32 VtxOffset;
	uint32 IdxOffset;
	uint32 NumOfVertices;
	uint32 NumOfIndices;
	Handle<HBuffer> Vbo;
	Handle<HBuffer> Ebo;
};


class Model : Array<Mesh*>
{
private:
	using Super = Array<Mesh*>;
public:
	uint32 Id;

	using Super::Push;
	using Super::PopAt;
	using Super::Length;
	using Super::begin;
	using Super::end;
	using Super::operator[];
	using Super::Reserve;
};


struct TextureBase
{
	String128 Name;
	size_t Size;
	uint32 Width;
	uint32 Height;
	uint32 Channels;
	BitSet<uint32> Usage; // EImageUsageFlags;
};


struct TextureCreateInfo : public TextureBase
{
	Buffer<uint8> TextureData;
};


struct TextureImportInfo
{
	String128 Name;
	FilePath Path;
	IRAssetManager* AssetManager;
};


struct Texture : public TextureBase
{
	Handle<HImage> Handle;
};

struct ShaderBase
{
	EShaderType Type;
	String128 Name;
	//uint32 Id;
};

struct ShaderImportInfo : public ShaderBase
{
	FilePath Path;
	IRAssetManager* AssetManager;
};

struct ShaderCreateInfo : public ShaderBase
{
	String Code;
};

struct Shader : public ShaderBase
{
	Handle<HShader> Handle;
	Array<ShaderAttrib> Attributes;
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H