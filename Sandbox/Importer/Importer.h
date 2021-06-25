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
	* [ ] To calculate tangent and bitangent.
	*/
	class ModelImporter
	{
	private:

		struct MeshTexture
		{
			uint32		MeshIndex;
			FilePath	TexturePath;

      MeshTexture();
      MeshTexture(uint32 Index, const char* Uri);
		};

		cgltf_data* Data;
		Buffer<float32> Position;
		Buffer<float32> Normals;
		Buffer<float32> Tangent;
		Buffer<float32> TexCoords;
		Array<MeshTexture> TextureFiles;
    FilePath Directory;
		size_t PosCount;
		size_t NormalsCount;
		size_t TangentCount;
		size_t TexCoordsCount;

		void FlushBuffers();
		void LoadBufferData(cgltf_attribute* Attribute, size_t& Count, Buffer<float32>& Buffer);
		void LoadGLTFNodeMeshData(cgltf_mesh* InMesh, Ref<Model> pModel, Ref<IAssetManager> pAssetManager);
		void LoadGLTFNode(cgltf_node* Node, Ref<Model> pModel, Ref<IAssetManager> pAssetManager);
		void LoadTexturePathsFromGLTF();

		//size_t FindMeshTextureMapIndex(Handle<Mesh> MeshHandle);

	public:

		ModelImporter();
		ModelImporter(const FilePath& Path, Ref<IAssetManager> pAssetManager);
		~ModelImporter();

		DELETE_COPY_AND_MOVE(ModelImporter)

		RefHnd<Model> ImportModelFromPath(const FilePath& Path, Ref<IAssetManager> pAssetManager);

		/**
		* Retrieves the relative path to the textures referenced by the imported model.
		* If nullptr is specified for the container, only the number of textures referenced by the imported model is returned.
		*/
		size_t PathsToTextures(Array<FilePath>* Out);

		//bool TexturesReferencedByMesh(Handle<Mesh> MeshHandle, Array<FilePath>& Out);
	};

	class TextureImporter
	{
	public:
		TextureImporter();
		TextureImporter(const FilePath& Path, Ref<IAssetManager> pAssetManager);
		~TextureImporter();

		DELETE_COPY_AND_MOVE(TextureImporter)

		RefHnd<Texture> ImportTextureFromPath(const FilePath& Path, Ref<IAssetManager> pAssetManager);
	};

	class ShaderImporter
	{
	public:
		ShaderImporter();
		ShaderImporter(const FilePath& Path, EShaderType Type, Ref<IAssetManager> pAssetManager);
		~ShaderImporter();

		DELETE_COPY_AND_MOVE(ShaderImporter)

		RefHnd<Shader> ImportShaderFromPath(const FilePath& Path, EShaderType Type, Ref<IAssetManager> pAssetManager);
	};

}

#endif // !LEARNVK_RENDERER_ASSETS_IMPORTER_H
