#include <new>
#include "Importer.h"
#include "Library/Containers/String.h"
#include "Library/Stream/Ifstream.h"
#include <memory>

namespace sandbox
{

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

	void ModelImporter::LoadBufferData(cgltf_attribute* Attribute, size_t& Count, astl::Buffer<float32>& Buffer)
	{
		size_t dataCount = cgltf_accessor_unpack_floats(Attribute->data, nullptr, 0);
		Count = dataCount;
		if (dataCount > Buffer.Count())
		{
			Buffer.Alloc(dataCount);
		}
		cgltf_accessor_unpack_floats(Attribute->data, Buffer, dataCount);
	}

	void ModelImporter::LoadGLTFNodeMeshData(cgltf_mesh* InMesh, astl::Ref<Model> pModel, astl::Ref<IAssetManager> pAssetManager)
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

			Mesh temp;
			temp.Indices.Reserve(indices->count);
			temp.Vertices.Reserve(PosCount / 3);

			Vertex vertex;

			for (size_t j = 0; j < temp.Vertices.Size(); j++)
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
				//if (TangentCount)
				//{
				//	vertex.Tangent.x = Tangent[j * 3 + 0];
				//	vertex.Tangent.y = Tangent[j * 3 + 1];
				//	vertex.Tangent.z = Tangent[j * 3 + 2];
				//}

				// TexCoords ...
				if (TexCoordsCount)
				{
					vertex.TexCoord.x = TexCoords[j * 2 + 0];
					vertex.TexCoord.y = TexCoords[j * 2 + 1];
				}

				temp.Vertices.Push(astl::Move(vertex));
			}

			if (indices)
			{
				if (indices->count)
				{
					temp.Indices.Reserve(indices->count);
				}

				for (size_t j = 0; j < indices->count; j++)
				{
					uint32 index = 0;
					cgltf_accessor_read_uint(indices, j, &index, 1);
					temp.Indices.Push(index);
				}
			}

			pModel->Meshes.Push(astl::Move(temp));

			FlushBuffers();
		}
	}

	void ModelImporter::LoadGLTFNode(cgltf_node* Node, astl::Ref<Model> pModel, astl::Ref<IAssetManager> pAssetManager)
	{
		if (Node->mesh)
		{
			LoadGLTFNodeMeshData(Node->mesh, pModel, pAssetManager);
		}

		for (size_t i = 0; i < Node->children_count; i++)
		{
			LoadGLTFNode(Node->children[i], pModel, pAssetManager);
		}
	}

	void ModelImporter::LoadTexturePathsFromGLTF()
	{
    TextureFiles.Empty();
		cgltf_image* image = nullptr;
		if (!Data->images_count) { return; }

		for (size_t i = 0; i < Data->images_count; i++)
		{
			image = &Data->images[i];
      TextureFiles.Push(MeshTexture(static_cast<uint32>(i), image->uri));
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
    Directory(),
		PosCount(0),
		NormalsCount(0),
		TangentCount(0),
		TexCoordsCount(0)
	{}

	ModelImporter::ModelImporter(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager) :
		ModelImporter()
	{
		ImportModelFromPath(Path, pAssetManager);
	}

	ModelImporter::~ModelImporter()
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

	RefHnd<Model> ModelImporter::ImportModelFromPath(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager)
	{
		astl::Ifstream ifstream;
		astl::String buf;

		if (!pAssetManager || !Path.Length())
		{
			return NULLPTR;
		}

		if (!ifstream.Open(Path.C_Str()))
		{
			return NULLPTR;
		}

		size_t fileSize = ifstream.Size();
		buf.Reserve(fileSize);
		ifstream.Read(buf.First(), fileSize);
		buf[fileSize - 1] = '\0';
		ifstream.Close();

		cgltf_options options = {};
		cgltf_result result = cgltf_parse(&options, buf.First(), fileSize, &Data);

		if (result != cgltf_result_success) { return NULLPTR; }

		result = cgltf_load_buffers(&options, Data, Path.C_Str());

		if (result != cgltf_result_success) { return NULLPTR; }

		result = cgltf_validate(Data);

		if (result != cgltf_result_success) { return NULLPTR; }

		if (!Data->meshes_count) { return NULLPTR; }

    Directory.~FilePath();
    Directory = Path.Directory();
		LoadTexturePathsFromGLTF();

		Handle<Model> hnd = pAssetManager->CreateModel(Model());
		astl::Ref<Model> pModel = pAssetManager->GetModelWithHandle(hnd);

		cgltf_node* node = nullptr;
		cgltf_scene* scene = nullptr;

		for (size_t i = 0; i < Data->scenes_count; i++)
		{
			scene = &Data->scenes[i];
			for (size_t j = 0; j < scene->nodes_count; j++)
			{
				node = scene->nodes[j];
				LoadGLTFNode(node, pModel, pAssetManager);
			}
		}

		return RefHnd<Model>(hnd, pModel);
	}

	size_t ModelImporter::PathsToTextures(astl::Array<astl::FilePath>* Out)
	{
    astl::String256 texturePath;
		if (!Out) { return TextureFiles.Length(); }
		for (auto& [i, path] : TextureFiles)
		{
			texturePath.Format("%s%s", Directory.C_Str(), path.C_Str());
			Out->Push(texturePath.C_Str());
		}
		return 1;
	}

	//bool ModelImporter::TexturesReferencedByMesh(Handle<Mesh> MeshHandle, Array<astl::FilePath>& Out)
	//{
	//	String texturePath;
	//	size_t mapIndex = FindMeshTextureMapIndex(MeshHandle);
	//	if (mapIndex == -1) { return false; }
	//	auto& mapPair = MeshTextureMap[mapIndex];
	//	for (const astl::FilePath* path : mapPair.Value)
	//	{
	//		texturePath.Format("%s%s", Directory.C_Str(), path->C_Str());
	//		Out.Push(texturePath.C_Str());
	//	}
	//	return true;
	//}

	TextureImporter::TextureImporter() {}

	TextureImporter::TextureImporter(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager) :
		TextureImporter()
	{
		ImportTextureFromPath(Path, pAssetManager);
	}

	TextureImporter::~TextureImporter() {}

	RefHnd<Texture> TextureImporter::ImportTextureFromPath(const astl::FilePath& Path, astl::Ref<IAssetManager> pAssetManager)
	{
		if (!pAssetManager || !Path.Length())
		{
			return NULLPTR;
		}

		astl::Ifstream ifstream;

		if (!ifstream.Open(Path.C_Str())) { return NULLPTR; }

		const size_t fileSize = ifstream.Size();

		astl::Buffer<uint8> temp(ifstream.Size());

		ifstream.Read(temp, fileSize);
		ifstream.Close();

		int32 width = 0;
		int32 height = 0;
		int32 channels = 0;

		uint8* data = stbi_load_from_memory(temp, static_cast<int32>(fileSize), &width, &height, &channels, STBI_rgb_alpha);

		if (!data) { return NULLPTR; }

		Handle<Texture> hnd = pAssetManager->CreateTexture(Texture());
		astl::Ref<Texture> pTexture = pAssetManager->GetTextureWithHandle(hnd);

		pTexture->Width = width;
		pTexture->Height = height;
		pTexture->Channels = channels;

    if (pTexture->Channels == 3)
    {
      pTexture->Channels = 4;
    }

    pTexture->Size = (size_t)pTexture->Width * (size_t)pTexture->Height * (size_t)pTexture->Channels;

    // TODO(Ygsm):
    // Fix broken buffer class.
    new (&pTexture->Data) astl::BinaryBuffer();
    pTexture->Data.Alloc(pTexture->Size);
		astl::IMemory::Memcpy(pTexture->Data.Data(), data, pTexture->Size);

		stbi_image_free(data);

		return RefHnd<Texture>(hnd, pTexture);
	}

	ShaderImporter::ShaderImporter() {}

	ShaderImporter::ShaderImporter(const astl::FilePath& Path, EShaderType Type, astl::Ref<IAssetManager> pAssetManager)
	{
		ImportShaderFromPath(Path, Type, pAssetManager);
	}

	ShaderImporter::~ShaderImporter() {}

	RefHnd<Shader> ShaderImporter::ImportShaderFromPath(const astl::FilePath& Path, EShaderType Type, astl::Ref<IAssetManager> pAssetManager)
	{
		if (!pAssetManager || !Path.Length())
		{
			return NULLPTR;
		}

		astl::Ifstream ifstream;
		if (!ifstream.Open(Path.C_Str())) { return NULLPTR; }
		
		const size_t fileSize = ifstream.Size();
		Handle<Shader> hnd = pAssetManager->CreateShader(Shader());
		astl::Ref<Shader> pShader = pAssetManager->GetShaderWithHandle(hnd);

		pShader->Code.Reserve(fileSize + 1);

		ifstream.Read(pShader->Code.First(), fileSize);
		ifstream.Close();

		pShader->Type = Type;
		pShader->Code[fileSize] = '\0';

		return RefHnd<Shader>(hnd, pShader);
	}

  ModelImporter::MeshTexture::MeshTexture() :
    MeshIndex(0), TexturePath()
  {}

  ModelImporter::MeshTexture::MeshTexture(uint32 Index, const char* Uri) :
    MeshIndex(Index), TexturePath(Uri)
  {}
}
