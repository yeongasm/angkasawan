#pragma once
#ifndef LEARNVK_RENDERER_ASSETS_IMPORTER_H
#define LEARNVK_RENDERER_ASSETS_IMPORTER_H

#include "External/cgltf/cgltf.h"
#include "External/stb/stb_image.h"
#include "Library/Containers/Buffer.h"
#include "Library/Containers/Path.h"
#include "Library/Containers/Array.h"
#include "Asset/Assets.h"

namespace sandbox
{

	/**
	* TODO(Ygsm):
	* [x] Include functions to get textures specified in GLTF file (17.02.2021, 2251Hrs).
	* [x] Need to store mesh to texture data.
	*/
	class ModelImporter
	{
	private:

		//struct MeshTexture
		//{
		//	uint32 MeshIndex;
		//	astl::FilePath TexturePath;

  //    MeshTexture();
  //    MeshTexture(uint32 Index, const char* Uri);
		//};

    // GTLF 2.0 stores metallic and roughness in the same texture.
    // Roughness is in the green channel.
    // Metalness is in the blue channel.
    struct MeshTextureReference
    {
      EPbrTextureType Type = Pbr_Texture_Type_None;
      float32 Roughness = 0.0f;
      float32 Metallic = 0.0f;
      astl::Array<size_t> MeshIndices = {};
    };

		cgltf_data* Data;
		astl::Buffer<float32> Position;
		astl::Buffer<float32> Normals;
		astl::Buffer<float32> Tangent;
		astl::Buffer<float32> TexCoords;
    astl::Map<astl::FilePath, MeshTextureReference> TextureReferences;
		//astl::Array<MeshTexture> TextureFiles;
    astl::FilePath Directory;
		size_t PosCount;
		size_t NormalsCount;
		size_t TangentCount;
		size_t TexCoordsCount;

		void FlushBuffers();
		void LoadBufferData(cgltf_attribute* Attribute, size_t& Count, astl::Buffer<float32>& Buffer);
		void LoadGLTFNodeMeshData(cgltf_mesh* InMesh, astl::Ref<Model> pModel, astl::Ref<IAssetManager> pAssetManager);
		void LoadGLTFNode(cgltf_node* Node, astl::Ref<Model> pModel, astl::Ref<IAssetManager> pAssetManager);
		//void LoadTexturePathsFromGLTF();

    void LoadMeshVertexData(Mesh& InMesh);
    void LoadMeshIndexData(Mesh& InMesh, cgltf_accessor* pIndices);
    void AppendMeshToModel(astl::Ref<Model> pModel, cgltf_primitive* pPrimitive);
    void LoadMeshMaterial(size_t MeshIndex, cgltf_material* InMaterial);

		//size_t FindMeshTextureMapIndex(Handle<Mesh> MeshHandle);

	public:

		ModelImporter();
		ModelImporter(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager);
		~ModelImporter();

		DELETE_COPY_AND_MOVE(ModelImporter)

		RefHnd<Model> ImportModelFromPath(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager);

		/**
		* Retrieves the relative path to the textures referenced by the imported model.
		* If nullptr is specified for the container, only the number of textures referenced by the imported model is returned.
		*/
		//size_t PathsToTextures(astl::Array<astl::FilePath>* Out);

		//bool TexturesReferencedByMesh(Handle<Mesh> MeshHandle, Array<astl::FilePath>& Out);
	};

	class TextureImporter
	{
	public:
		TextureImporter();
		TextureImporter(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager);
		~TextureImporter();

		DELETE_COPY_AND_MOVE(TextureImporter)

		RefHnd<Texture> ImportTextureFromPath(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager);
	};

	class ShaderImporter
	{
	public:
		ShaderImporter();
		ShaderImporter(const astl::FilePath& Path, EShaderType Type, astl::Ref<IAssetManager> pAssetManager);
		~ShaderImporter();

		DELETE_COPY_AND_MOVE(ShaderImporter)

		RefHnd<Shader> ImportShaderFromPath(const astl::FilePath& Path, EShaderType Type, astl::Ref<IAssetManager> pAssetManager);
	};

}

#endif // !LEARNVK_RENDERER_ASSETS_IMPORTER_H
