#pragma once
#ifndef LEARNVK_RENDERER_ASSETS_IMPORTER_H
#define LEARNVK_RENDERER_ASSETS_IMPORTER_H

#include "External/cgltf/cgltf.h"
#include "External/stb/stb_image.h"
#include "RenderPlatform/API.h"
#include "Library/Containers/Buffer.h"
#include "Library/Containers/Pair.h"
#include "RenderAbstracts/Primitives.h"

class IRAssetManager;

/**
* TODO(Ygsm):
* [x] Include functions to get textures specified in GLTF file (17.02.2021, 2251Hrs).
* [ ] Need to store mesh to texture data.
* [ ] To calculate tangent and bitangent.
*/
class RENDERER_API ModelImporter
{
private:
	//using MeshTextureMaps = Array<Pair<Handle<Mesh>, Array<FilePath*>>>;

	cgltf_data* Data;
	Buffer<float32> Position;
	Buffer<float32> Normals;
	Buffer<float32> Tangent;
	Buffer<float32> TexCoords;
	Array<FilePath> TextureFiles;
	//MeshTextureMaps MeshTextureMap;
	FilePath Directory;
	size_t PosCount;
	size_t NormalsCount;
	size_t TangentCount;
	size_t TexCoordsCount;

	void FlushBuffers			();
	void LoadBufferData			(cgltf_attribute* Attribute, size_t& Count, Buffer<float32>& Buffer);
	void LoadGLTFNodeMeshData	(cgltf_mesh* InMesh, Handle<Model>& ModelHandle, Handle<SRMemoryBuffer> VtxBuf, Handle<SRMemoryBuffer> IdxBuf, IRAssetManager& AssetManager);
	void LoadGLTFNode			(cgltf_node* Node, Handle<Model>& ModelHandle, Handle<SRMemoryBuffer> VtxBuf, Handle<SRMemoryBuffer> IdxBuf, IRAssetManager& AssetManager);
	void LoadTexturePathsFromGLTF();

	//size_t FindMeshTextureMapIndex(Handle<Mesh> MeshHandle);

public:

	ModelImporter();
	ModelImporter(const ModelImportInfo& ImportInfo);
	~ModelImporter();

	DELETE_COPY_AND_MOVE(ModelImporter)

	Handle<Model> ImportModelFromPath(const ModelImportInfo& ImportInfo);
	void Release();

	/**
	* Retrieves the relative path to the textures referenced by the imported model.
	* If nullptr is specified for the container, only the number of textures referenced by the imported model is returned.
	*/
	size_t PathsToTextures(Array<FilePath>* Out);

	//bool TexturesReferencedByMesh(Handle<Mesh> MeshHandle, Array<FilePath>& Out);
};


class RENDERER_API TextureImporter
{
private:
	//Buffer<uint8> Buf;
	int32 Width;
	int32 Height;
	int32 Channels;
public:
	TextureImporter();
	TextureImporter(const TextureImportInfo& ImportInfo);
	~TextureImporter();

	DELETE_COPY_AND_MOVE(TextureImporter)

	Handle<Texture> ImportTextureFromPath(const TextureImportInfo& ImportInfo);
	void Release();
};

class RENDERER_API ShaderImporter
{
private:
	String Code;
public:
	ShaderImporter();
	ShaderImporter(const ShaderImportInfo& ImportInfo);
	~ShaderImporter();

	DELETE_COPY_AND_MOVE(ShaderImporter)

	Handle<Shader> ImportShaderFromPath(const ShaderImportInfo& ImportInfo);
	void Release();
};

#endif // !LEARNVK_RENDERER_ASSETS_IMPORTER_H