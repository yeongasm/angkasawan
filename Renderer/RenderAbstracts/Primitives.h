#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H

#include "Library/Containers/Buffer.h"
#include "Library/Containers/Array.h"
#include "Library/Containers/Path.h"
#include "Library/Containers/Bitset.h"
#include "Library/Math/Math.h"
#include "API/RendererFlagBits.h"
#include "API/Definitions.h"
#include "API/ShaderAttribute.h"
#include "Assets/GPUHandles.h"

class IRAssetManager;
class IRenderMemoryManager;
struct RenderPass;
struct DescriptorLayout;

//struct SRMemoryBufferLocality
//{
//	EBufferLocality Locality;
//};

struct SRMemoryBufferBase
{
	EBufferLocality Locality;
	BitSet<EBufferTypeFlagBits>	Type;
	size_t Size;
};

//struct MemoryAllocateInfo : SRMemoryBufferBase
//{
//	String128 Name;
//};

struct SRMemoryBuffer : SRMemoryBufferBase
{
	size_t Offset;
	Handle<HBuffer> Handle;
};

//struct SRMemoryTransferContext
//{
//	SRMemoryBuffer SrcBuffer;
//	size_t SrcOffset;
//	size_t SrcSize;
//	SRMemoryBuffer* DstBuffer;
//	size_t DstOffset;
//};

//struct SRVertex
//{
//	math::vec3 Position;
//	math::vec3 Normal;
//	math::vec3 Tangent;
//	math::vec3 Bitangent;
//	math::vec2 TexCoord;
//};

//struct ModelCreateInfo
//{
//	String128 Name;
//};
//
//struct ModelImportInfo : ModelCreateInfo
//{
//	FilePath Path;
//	IRAssetManager* AssetManager;
//	Handle<SRMemoryBuffer> VtxMemHandle;
//	Handle<SRMemoryBuffer> IdxMemHandle;
//};
//
//struct MeshCreateInfo : ModelCreateInfo
//{
//	Array<SRVertex> Vertices;
//	Array<uint32> Indices;
//	Handle<SRMemoryBuffer> VtxMemHandle;
//	Handle<SRMemoryBuffer> IdxMemHandle;
//};
//
//struct Mesh : ModelCreateInfo
//{
//	uint32 VtxOffset;
//	uint32 IdxOffset;
//	uint32 NumOfVertices;
//	uint32 NumOfIndices;
//	Handle<HBuffer> Vbo;
//	Handle<HBuffer> Ebo;
//};
//
//class Model : Array<Mesh*>
//{
//private:
//	using Super = Array<Mesh*>;
//public:
//	//uint32 Id;
//
//	using Super::Push;
//	using Super::PopAt;
//	using Super::Length;
//	using Super::begin;
//	using Super::end;
//	using Super::operator[];
//	using Super::Reserve;
//};

struct SRImageSamplerBase
{
	ESamplerFilter MinFilter;
	ESamplerFilter MagFilter;
	ESamplerAddressMode AddressModeU;
	ESamplerAddressMode AddressModeV;
	ESamplerAddressMode AddressModeW;
	ECompareOp CompareOp;
	float32 AnisotropyLvl;
};

//struct ImageSamplerCreateInfo : ImageSamplerBase
//{
//	String128 Name;
//};

struct SRImageSampler : SRImageSamplerBase
{
	Handle<HSampler> Handle;
};

struct SRTextureBase
{
	size_t Size;
	uint32 Width;
	uint32 Height;
	uint32 Channels;
	ETextureType Type;
	BitSet<EImageUsageFlagBits> Usage;
};

//struct TextureCreateInfo : TextureBase
//{
//	String128 Name;
//	Buffer<uint8> TextureData;
//};

//struct TextureImportInfo
//{
//	String128 Name;
//	FilePath Path;
//	IRAssetManager* AssetManager;
//};

struct SRTexture : SRTextureBase
{
	ImageSampler* Sampler;
	Handle<HImage> Handle;
};

//struct SRTextureTransferContext
//{
//	SRMemoryBuffer Buffer;
//	SRTexture* Texture;
//};

struct ShaderBase
{
	EShaderType Type;
	//String128 Name;
	//uint32 Id;
};

struct ShaderImportInfo : ShaderBase
{
	FilePath Path;
	String128 Name;
	IRAssetManager* AssetManager;
};

struct ShaderCreateInfo : ShaderBase
{
	String Code;
	//String128 Name;
};

struct SRShader : ShaderBase
{
	using AttribsContainer = StaticArray<ShaderAttrib, MAX_SHADER_ATTRIBUTES>;
	Handle<HShader> Handle;
	AttribsContainer Attributes;
};

struct VertexInputBinding
{
	uint32 Binding;
	uint32 From;
	uint32 To;
	uint32 Stride;
	EVertexInputRateType Type;
};

struct PipelineBase
{
	Array<VertexInputBinding> VertexInputBindings;
	ESampleCount Samples;
	ETopologyType Topology;
	EFrontFaceDir FrontFace;
	ECullingMode CullMode;
	EPolygonMode PolygonalMode;
	uint32 Id;
};

struct SRPipeline : PipelineBase
{
	Array<DescriptorLayout*> DescriptorLayouts;
	Handle<HPipeline> Handle;
	RenderPass* Renderpass;
	SRShader* VertexShader;
	SRShader* FragmentShader;
	uint32 ColorOutputCount;
	bool HasDefaultColorOutput;
	bool HasDepthStencil;
};

struct SRPushConstant
{
	uint8 Data[Push_Constant_Size];
	size_t Offset;
	SRPipeline* Pipeline;
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_PRIMITIVES_H