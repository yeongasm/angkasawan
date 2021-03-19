#include <new>
#include "Importer.h"
#include "Library/Containers/String.h"
#include "Library/Stream/Ifstream.h"
#include "Assets.h"

void ModelImporter::FlushBuffers()
{
	Position.Flush();
	PosCount = 0;
	Normals.Flush();
	NormalsCount = 0;
	Tangent.Flush();
	TangentCount = 0;
	TexCoords.Flush();
	TexCoordsCount = 0;
}

void ModelImporter::LoadBufferData(cgltf_attribute* Attribute, size_t& Count, Buffer<float32>& Buffer)
{
	size_t dataCount = cgltf_accessor_unpack_floats(Attribute->data, nullptr, 0);
	Count = dataCount;
	if (dataCount > Buffer.Count())
	{
		Buffer.Alloc(dataCount);
	}
	cgltf_accessor_unpack_floats(Attribute->data, Buffer, dataCount);
}

void ModelImporter::LoadGLTFNodeMeshData(cgltf_mesh* InMesh, Handle<Model>& ModelHandle, Handle<SRMemoryBuffer> VtxBuf, Handle<SRMemoryBuffer> IdxBuf, IRAssetManager& AssetManager)
{
	cgltf_mesh* mesh = InMesh;
	cgltf_primitive* primitive = nullptr;
	cgltf_attribute* attribute = nullptr;
	cgltf_accessor* indices = nullptr;

	for (size_t i = 0; i < mesh->primitives_count; i++)
	{
		primitive = &mesh->primitives[i];
		indices = primitive->indices;

		if (primitive->type != cgltf_primitive_type_triangles) { continue; }

		for (size_t j = 0; j < primitive->attributes_count; j++)
		{
			attribute = &primitive->attributes[j];

			switch (attribute->type)
			{
				case cgltf_attribute_type_position:
					LoadBufferData(attribute, PosCount, Position);
					break;
				case cgltf_attribute_type_normal:
					LoadBufferData(attribute, NormalsCount, Normals);
					break;
				case cgltf_attribute_type_tangent:
					LoadBufferData(attribute, TangentCount, Tangent);
					break;
				case cgltf_attribute_type_texcoord:
					LoadBufferData(attribute, TexCoordsCount, TexCoords);
					break;
				default:
					break;
			}
		}

		if (!PosCount) { continue; }

		MeshCreateInfo createInfo = {};
		SRVertex vertex = {};

		createInfo.IdxMemHandle = IdxBuf;
		createInfo.VtxMemHandle = VtxBuf;
		createInfo.Name = mesh->name;
		createInfo.Vertices.Reserve(PosCount / 3);

		for (size_t j = 0; j < createInfo.Vertices.Size(); j++)
		{
			// Position ...
			if (PosCount)
			{
				vertex.Position.x = Position[j * 3 + 0];
				vertex.Position.y = Position[j * 3 + 1];
				vertex.Position.z = Position[j * 3 + 2];
			}

			// Normals ...
			if (NormalsCount)
			{
				vertex.Normal.x = Normals[j * 3 + 0];
				vertex.Normal.y = Normals[j * 3 + 1];
				vertex.Normal.z = Normals[j * 3 + 2];
			}

			// Tangent ...
			if (TangentCount)
			{
				vertex.Tangent.x = Tangent[j * 3 + 0];
				vertex.Tangent.y = Tangent[j * 3 + 1];
				vertex.Tangent.z = Tangent[j * 3 + 2];
			}

			// TexCoords ...
			if (TexCoordsCount)
			{
				vertex.TexCoord.x = TexCoords[j * 2 + 0];
				vertex.TexCoord.y = TexCoords[j * 2 + 1];
			}

			createInfo.Vertices.Push(Move(vertex));
		}

		if (indices)
		{
			if (indices->count)
			{
				createInfo.Indices.Reserve(indices->count);
			}

			for (size_t j = 0; j < indices->count; j++)
			{
				uint32 index = 0;
				cgltf_accessor_read_uint(indices, j, &index, 1);
				createInfo.Indices.Push(index);
			}
		}

		uint32 vertexOffset = 0;
		Model* currentModel = AssetManager.GetModelWithHandle(ModelHandle);
		for (Mesh* mesh : *currentModel)
		{
			vertexOffset += mesh->NumOfVertices;
		}

		Handle<Mesh> meshHandle = AssetManager.CreateNewMesh(createInfo);
		AssetManager.PushMeshIntoModel(meshHandle, ModelHandle);
		Mesh* currentMesh = AssetManager.GetMeshWithHandle(meshHandle);

		FlushBuffers();
	}
}

void ModelImporter::LoadGLTFNode(cgltf_node* Node, Handle<Model>& ModelHandle, Handle<SRMemoryBuffer> VtxBuf, Handle<SRMemoryBuffer> IdxBuf, IRAssetManager& AssetManager)
{
	if (Node->mesh)
	{
		LoadGLTFNodeMeshData(Node->mesh, ModelHandle, VtxBuf, IdxBuf, AssetManager);
	}

	for (size_t i = 0; i < Node->children_count; i++)
	{
		LoadGLTFNode(Node->children[i], ModelHandle, VtxBuf, IdxBuf, AssetManager);
	}
}

void ModelImporter::LoadTexturePathsFromGLTF()
{
	cgltf_image* image = nullptr;
	if (!Data->images_count) { return; }
	//String imagePath;
	for (size_t i = 0; i < Data->images_count; i++)
	{
		image = &Data->images[i];
		//imagePath.Format("%s%s", Directory.C_Str(), image->uri);
		TextureFiles.Push(FilePath(image->uri));
		//imagePath.Empty();
	}
}

//size_t ModelImporter::FindMeshTextureMapIndex(Handle<Mesh> MeshHandle)
//{
//	size_t index = -1;
//	Pair<Handle<Mesh>, Array<FilePath*>>* pair = nullptr;
//	for (size_t i = 0; i < MeshTextureMap.Length(); i++)
//	{
//		pair = &MeshTextureMap[i];
//		if (pair->Key == MeshHandle)
//		{
//			index = i;
//			break;
//		}
//	}
//	return index;
//}

ModelImporter::ModelImporter() :
	Data(nullptr),
	Position(), 
	Normals(),
	Tangent(),
	TexCoords(),
	TextureFiles(),
	//MeshTextureMap(),
	Directory(),
	PosCount(0),
	NormalsCount(0),
	TangentCount(0),
	TexCoordsCount(0)
{}

ModelImporter::ModelImporter(const ModelImportInfo& ImportInfo) :
	ModelImporter()
{
	ImportModelFromPath(ImportInfo);
}

ModelImporter::~ModelImporter()
{
	Release();
}

Handle<Model> ModelImporter::ImportModelFromPath(const ModelImportInfo& ImportInfo)
{
	Ifstream ifstream;
	String buf;

	if (!ImportInfo.AssetManager || !ImportInfo.Name.Length() || !ImportInfo.Path.Length())
	{
		return INVALID_HANDLE;
	}

	if (!ifstream.Open(ImportInfo.Path.C_Str()))
	{
		return INVALID_HANDLE;
	}

	size_t fileSize = ifstream.Size();
	buf.Reserve(fileSize);
	ifstream.Read(buf.First(), fileSize);
	buf[fileSize - 1] = '\0';
	ifstream.Close();

	cgltf_options options = {};
	cgltf_result result = cgltf_parse(&options, buf.First(), fileSize, &Data);

	if (result != cgltf_result_success) { return false; }

	result = cgltf_load_buffers(&options, Data, ImportInfo.Path.C_Str());

	if (result != cgltf_result_success) { return false; }

	result = cgltf_validate(Data);

	if (result != cgltf_result_success) { return false; }

	if (!Data->meshes_count) { return false; }

	Directory = ImportInfo.Path.Directory();

	LoadTexturePathsFromGLTF();

	IRAssetManager& assetManager = *ImportInfo.AssetManager;
	Handle<Model> modelHandle = assetManager.CreateNewModel(ImportInfo);

	cgltf_node* node = nullptr;
	cgltf_scene* scene = nullptr;

	for (size_t i = 0; i < Data->scenes_count; i++)
	{
		scene = &Data->scenes[i];
		for (size_t j = 0; j < scene->nodes_count; j++)
		{
			node = scene->nodes[j];
			LoadGLTFNode(node, modelHandle, ImportInfo.VtxMemHandle, ImportInfo.IdxMemHandle, assetManager);
		}
	}

	return modelHandle;
}

void ModelImporter::Release()
{
	if (Data)
	{
		cgltf_free(Data);
	}
	Position.Release();
	Normals.Release();
	Tangent.Release();
	TexCoords.Release();
}

size_t ModelImporter::PathsToTextures(Array<FilePath>* Out)
{
	String texturePath;
	if (!Out) { return TextureFiles.Length(); }
	for (const FilePath& path : TextureFiles)
	{
		texturePath.Format("%s%s", Directory.C_Str(), path.C_Str());
		Out->Push(texturePath.C_Str());
	}
	return 1;
}

//bool ModelImporter::TexturesReferencedByMesh(Handle<Mesh> MeshHandle, Array<FilePath>& Out)
//{
//	String texturePath;
//	size_t mapIndex = FindMeshTextureMapIndex(MeshHandle);
//	if (mapIndex == -1) { return false; }
//	auto& mapPair = MeshTextureMap[mapIndex];
//	for (const FilePath* path : mapPair.Value)
//	{
//		texturePath.Format("%s%s", Directory.C_Str(), path->C_Str());
//		Out.Push(texturePath.C_Str());
//	}
//	return true;
//}

TextureImporter::TextureImporter() :
	//Buf(),
	Width(0), 
	Height(0),
	Channels(0)
{}

TextureImporter::TextureImporter(const TextureImportInfo& ImportInfo) :
	TextureImporter()
{
	ImportTextureFromPath(ImportInfo);
}

TextureImporter::~TextureImporter()
{
	Release();
}

Handle<Texture> TextureImporter::ImportTextureFromPath(const TextureImportInfo& ImportInfo)
{
	if (!ImportInfo.AssetManager || !ImportInfo.Path.Length() || !ImportInfo.Name.Length()) 
	{ 
		return INVALID_HANDLE; 
	}

	Ifstream ifstream;

 	if (!ifstream.Open(ImportInfo.Path.C_Str())) { return INVALID_HANDLE; }

	const size_t fileSize = ifstream.Size();

	Buffer<uint8> textureFile;
	textureFile.Alloc(ifstream.Size());
	ifstream.Read(textureFile, fileSize);

	uint8* data = stbi_load_from_memory(textureFile, static_cast<int32>(fileSize), &Width, &Height, &Channels, STBI_rgb_alpha);

	if (!data) { return false; }

	TextureCreateInfo createInfo = {};
	IRAssetManager* assetManager = ImportInfo.AssetManager;

	createInfo.Name = ImportInfo.Name;
	createInfo.Width = Width;
	createInfo.Height = Height;
	createInfo.Channels = Channels;
	createInfo.Size = static_cast<size_t>(Width) * static_cast<size_t>(Height) * 4;
	createInfo.TextureData.Alloc(createInfo.Size);

	for (size_t i = 0; i < createInfo.Size; i++)
	{
		createInfo.TextureData[i] = data[i];
	}

	Handle<Texture> textureHandle = assetManager->CreateNewTexture(createInfo);
	//Texture* texture = assetManager->GetTextureWithHandle(textureHandle);

	stbi_image_free(data);
	//Buf.Flush();
	//new (this) TextureImporter();

	return textureHandle;
}

void TextureImporter::Release()
{
	//if (Buf.Size())
	//{
	//	Buf.Release();
	//	new (this) TextureImporter();
	//}
}

ShaderImporter::ShaderImporter()
{}

ShaderImporter::ShaderImporter(const ShaderImportInfo& ImportInfo)
{
	ImportShaderFromPath(ImportInfo);
}

ShaderImporter::~ShaderImporter()
{
	Release();
}

Handle<Shader> ShaderImporter::ImportShaderFromPath(const ShaderImportInfo& ImportInfo)
{
	if (!ImportInfo.AssetManager || !ImportInfo.Path.Length() || !ImportInfo.Name.Length())
	{
		return INVALID_HANDLE;
	}

	if (Code.Length()) { Code.Empty(); }

	IRAssetManager* assetManager = ImportInfo.AssetManager;
	ShaderCreateInfo createInfo = {};
	FMemory::Memcpy(&createInfo, &ImportInfo, sizeof(ShaderBase));

	Ifstream ifstream;
	if (!ifstream.Open(ImportInfo.Path.C_Str())) { return INVALID_HANDLE; }

	const size_t fileSize = ifstream.Size();

	if (Code.Size() < fileSize)
	{
		createInfo.Code.Reserve(fileSize + 1);
	}

	ifstream.Read(createInfo.Code.First(), fileSize);
	createInfo.Code[fileSize] = '\0';
	ifstream.Close();

	Handle<Shader> shaderHandle = assetManager->CreateNewShader(createInfo);

	return shaderHandle;
}

void ShaderImporter::Release()
{
	if (Code.Length())
	{
		Code.Release();
	}
}