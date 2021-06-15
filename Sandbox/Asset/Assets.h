#pragma once
#ifndef LEARNVK_RENDERER_ASSETS
#define LEARNVK_RENDERER_ASSETS

#include "Library/Containers/Buffer.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderAbstracts/StagingManager.h"
#include "Renderer.h"

class ResourceManager;

namespace sandbox
{

	// Only the items below are considered as an asset to the graphics system.
	enum ERendererAsset : uint32
	{
		Renderer_Asset_Model	= 0x00,
		Renderer_Asset_Shader	= 0x01,
		Renderer_Asset_Texture	= 0x02
		//Renderer_Asset_Material = 0x02
	};

	enum EPbrTextureType : size_t
	{
		Pbr_Texture_Type_Albedo		= 0,
		Pbr_Texture_Type_Roughness	= 1,
		Pbr_Texture_Type_Metallic	= 2,
		Pbr_Texture_Type_Normal		= 3,
		Pbr_Texture_Type_Ao			= 4,
		Pbr_Texture_Type_Max		= 5
	};

	struct Vertex
	{
		math::vec3 Position;
		math::vec3 Normal;
		math::vec3 Tangent;
		math::vec3 Bitangent;
		math::vec2 TexCoord;
	};

	struct Mesh
	{
		Array<Vertex> Vertices;
		Array<uint32> Indices;
	};

	struct Model
	{
		Array<Mesh> Meshes;
		Array<VertexInformation> VertexInformations;
		Array<IndexInformation> IndexInformation;
		uint32 MaxVertices = 0;
		uint32 MaxIndices = 0;
		uint32 NumDrawables = 0 ;
	};

	struct Shader
	{
		String Code;
		EShaderType Type;
		Handle<SShader> Hnd;
	};

	struct Texture
	{
		BinaryBuffer Data;
		Handle<SImage> ImageHnd;
		uint32 Width;
		uint32 Height;
		uint32 Channels;
	};

	struct MaxTextureBindingSlot
	{
		enum { Value = 8 };
	};

	class Material
	{
	private:
		friend class MaterialDefinition;

		using TextureSlots = StaticArray<Ref<Texture>, MaxTextureBindingSlot::Value>;
		TextureSlots Textures;
		String128 Name;
	public:
		Material();
		Material(String128 Name);
		~Material();

		void AddTextureToSlot(Ref<Texture> pTexture, size_t Type);
	};

	class MaterialDefinition
	{
	private:

		using BindingSlot = uint32;
		using TypeToBindSlots = StaticArray<BindingSlot, MaxTextureBindingSlot::Value>;

		Array<Material> Materials;
		TypeToBindSlots Slots;
		Handle<SDescriptorSet> DescriptorSetHnd;
		Handle<SDescriptorSetLayout> DescriptorSetLayoutHnd;
		Handle<SPipeline> PipelineHnd;

	public:

		MaterialDefinition();
		~MaterialDefinition();

		Ref<Material> CreateMaterialFromDefinition(const String128& Name);
		Ref<Material> GetMaterial(const String128& Name);
		const Handle<SDescriptorSet> GetDescriptorSetHandle() const;
		const Handle<SDescriptorSetLayout> GetDescriptorSetLayoutHandle() const;
		const Handle<SPipeline> GetPipelineHandle() const;

	};

	struct MaterialDefinitionCreateInfo
	{
		Handle<SDescriptorSet> DescriptorSetHnd;
		Handle<SDescriptorSetLayout> DescriptorSetLayoutHnd;
		Handle<SPipeline> PipelineHnd;
		StaticArray<uint32, MaxTextureBindingSlot::Value> Slots;
	};

	//void SerializeModel(WriteMemoryStream& Stream, const Model& Src, const FilePath& DstPath);
	//void SerializeShader(WriteMemoryStream& Stream, const Shader& Src, const FilePath& DstPath);
	//void SerializeTexture(WriteMemoryStream& Stream, const Texture& Src, const FilePath& DstPath);

	//void DeserializeModel(ReadMemoryStream& Stream, const Model& Dst, const FilePath& SrcPath);
	//void DeserializeShader(ReadMemoryStream& Stream, const Shader& Dst, const FilePath& SrcPath);
	//void DeserializeTexture(ReadMemoryStream& Stream, const Texture& Dst, const FilePath& SrcPath);

	/**
	* Book keeps the engine definition of a resource and also the renderer's in this project.
	*/
	class IAssetManager
	{
	public:

		IAssetManager(EngineImpl& Engine);
		~IAssetManager();

		DELETE_COPY_AND_MOVE(IAssetManager)

		Handle<Shader> CreateNewShader(const Shader& Src);
		Ref<Shader> GetShaderWithHandle(Handle<Shader> Hnd);
		bool DeleteShader(Handle<Shader> Hnd);

		Handle<Model> CreateNewModel(const Model& Src);
		Ref<Model> GetModelWithHandle(Handle<Model> Hnd);
		bool DeleteModel(Handle<Model> Hnd);

		Handle<Texture> CreateNewTexture(const Texture& Src);
		Ref<Texture> GetTextureWithHandle(Handle<Texture> Hnd);
		bool DeleteTexture(Handle<Texture> Hnd);

	private:

		template <typename Type>
		using AssetContainer = Map<uint32, Type>;

		AssetContainer<Shader>	Shaders;
		AssetContainer<Model>	Models;
		AssetContainer<Texture>	Textures;
		EngineImpl&				Engine;
	};

}

#endif // !LEARNVK_RENDERER_ASSETS