#include "Engine/Interface.h"
#include "Assets.h"
#include "Library/Stream/Ifstream.h"
#include "Library/Containers/Buffer.h"
#include "SubSystem/Resource/ResourceManager.h"
#include "RenderAbstracts/RenderMemory.h"
#include "RenderAbstracts/TextureMemory.h"
#include "API/Context.h"
#include "Library/Algorithms/Hash.h"

static uint32 g_HashSeed = static_cast<uint32>(OS::GetPerfCounter());

IRAssetManager::IRAssetManager(ResourceManager& Manager, SRDeviceStore& InDeviceStore) :
	//ShaderStore(),
	MeshStore(),
	ModelStore(),
	//TextureStore(),
	Manager(Manager),
	Device(InDeviceStore)
{
	this->Manager.AddCache(Renderer_Asset_Mesh);
	this->Manager.AddCache(Renderer_Asset_Model);
	this->Manager.AddCache(Renderer_Asset_Shader);
	this->Manager.AddCache(Renderer_Asset_Texture);
	this->Manager.AddCache(Renderer_Asset_Material);

	//ShaderStore.Reserve(512);
	//MeshStore.Reserve(512);
	//ModelStore.Reserve(128);
}

IRAssetManager::~IRAssetManager() 
{
	MeshStore.Release();
	ModelStore.Release();

	

	Manager.RemoveCache(Renderer_Asset_Model);
	Manager.RemoveCache(Renderer_Asset_Mesh);
	Manager.RemoveCache(Renderer_Asset_Shader);
	Manager.RemoveCache(Renderer_Asset_Texture);
	Manager.RemoveCache(Renderer_Asset_Material);
}

//void IRAssetManager::Initialize()
//{
//
//}

//void IRAssetManager::Terminate()
//{
//
//}

Handle<Shader> IRAssetManager::CreateNewShader(const ShaderCreateInfo& CreateInfo)
{
	ResourceCache* shaderCache = Manager.FetchCacheForType(Renderer_Asset_Shader);
	FilePath path = CreateInfo.Name.C_Str();

	if (shaderCache->Find(path))
	{
		return -1;
	}

	uint32 id = shaderCache->Create();
	Resource* resource = shaderCache->Get(id);
	resource->Path = path;
	resource->Type = Renderer_Asset_Shader;

	Shader& shader = ShaderStore.Add(id, Shader()).Value;
	shader.Type = CreateInfo.Type;
	shader.Handle = -1;
	//shader.Name = CreateInfo.Name;

	gpu::CreateShader(shader, const_cast<String&>(CreateInfo.Code));

	return Handle<Shader>(static_cast<size_t>(id));
}

Shader* IRAssetManager::GetShaderWithHandle(Handle<Shader> Hnd)
{
	return &ShaderStore[Hnd];
}

bool IRAssetManager::DeleteShader(Handle<Shader> Hnd)
{
	ResourceCache* shaderCache = Manager.FetchCacheForType(Renderer_Asset_Shader);
	Resource* res = shaderCache->Get(Hnd);

	if (res->RefCount != 1)
	{
		return false;
	}
	
	Shader& shader = ShaderStore[Hnd];
	gpu::DestroyShader(shader);
	shaderCache->Delete(Hnd);
	ShaderStore.Remove(Hnd);

	return true;
}

Handle<Model> IRAssetManager::CreateNewModel(const ModelCreateInfo& Info)
{
	ResourceCache* modelCache = Manager.FetchCacheForType(Renderer_Asset_Model);
	FilePath path = Info.Name.C_Str();

	if (modelCache->Find(path))
	{
		return -1;
	}

	uint32 id = modelCache->Create();
	Resource* resource = modelCache->Get(id);
	resource->Path = path;
	resource->Type = Renderer_Asset_Model;

	Model& model = ModelStore.Add(id, Model()).Value;
	XXHash32(Info.Name.C_Str(), Info.Name.Length(), &model.Id, g_HashSeed);

	return Handle<Model>(static_cast<size_t>(id));
}

Model* IRAssetManager::GetModelWithName(const String128& Identity)
{
	uint32 id = 0;
	XXHash32(Identity.C_Str(), Identity.Length(), &id, g_HashSeed);
	Model* queried = nullptr;
	for (auto& pair : ModelStore)
	{
		Model* theModel = &pair.Value;
		if (theModel->Id != id) { continue; }
		queried = theModel;
		break;
	}
	return queried;
}

Model* IRAssetManager::GetModelWithHandle(Handle<Model> Hnd)
{
	return &ModelStore[Hnd];
}

bool IRAssetManager::DeleteModel(Handle<Model> Hnd)
{
	ResourceCache* modelCache = Manager.FetchCacheForType(Renderer_Asset_Model);
	Resource* resource = modelCache->Get(Hnd);

	if (resource->RefCount != 1)
	{
		return false;
	}

	Model* model = &ModelStore[Hnd];
	
	if (model->Length())
	{
		return false;
	}
	
	modelCache->Delete(Hnd);
	ModelStore.Remove(Hnd);

	return true;
}

Handle<Mesh> IRAssetManager::CreateNewMesh(const MeshCreateInfo& CreateInfo)
{
	constexpr size_t sizeOfVertex = sizeof(SRVertex);
	constexpr size_t sizeOfUint32 = sizeof(uint32);

	SRMemoryBuffer* vtxBuf = Memory.GetBuffer(CreateInfo.VtxMemHandle);
	SRMemoryBuffer* idxBuf = Memory.GetBuffer(CreateInfo.IdxMemHandle);

	if (!vtxBuf || !idxBuf) { return INVALID_HANDLE; }

	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);

	uint32 id = meshCache->Create();
	Resource* resource = meshCache->Get(id);
	resource->Type = Renderer_Asset_Mesh;

	Mesh& mesh = MeshStore.Add(id, Mesh()).Value;

	size_t vertexCount = CreateInfo.Vertices.Length();
	size_t indexCount = CreateInfo.Indices.Length();
	size_t vtxSize = sizeOfVertex * vertexCount;
	size_t idxSize = sizeOfUint32 * indexCount;

	mesh.Name = CreateInfo.Name;
	mesh.NumOfVertices = static_cast<uint32>(vertexCount);
	mesh.NumOfIndices = static_cast<uint32>(indexCount);
	mesh.VtxOffset = static_cast<uint32>(vtxBuf->Offset / sizeOfVertex);

	SRMemoryBuffer vtxBuffer = {};
	vtxBuffer.Locality = Buffer_Locality_Cpu;
	vtxBuffer.Size = vtxSize;
	vtxBuffer.Type.Set(Buffer_Type_Transfer_Src);
	vtxBuffer.Type.Set(Buffer_Type_Vertex);

	SRVertex* vertices = const_cast<SRVertex*>(CreateInfo.Vertices.First());
	uint32* indices = const_cast<uint32*>(CreateInfo.Indices.First());

	gpu::CreateBuffer(vtxBuffer, vertices, sizeOfVertex * vertexCount);
	Memory.UploadData(vtxBuffer, CreateInfo.VtxMemHandle, false);

	for (size_t i = 0; i < indexCount; i++)
	{
		indices[i] += mesh.VtxOffset;
	}

	mesh.IdxOffset = static_cast<uint32>(idxBuf->Offset / sizeOfUint32);

	SRMemoryBuffer idxBuffer = {};
	idxBuffer.Locality = Buffer_Locality_Cpu;
	idxBuffer.Size = idxSize;
	idxBuffer.Type.Set(Buffer_Type_Transfer_Src);
	idxBuffer.Type.Set(Buffer_Type_Index);

	gpu::CreateBuffer(idxBuffer, indices, sizeOfUint32 * indexCount);
	Memory.UploadData(idxBuffer, CreateInfo.IdxMemHandle, false);

	mesh.Vbo = vtxBuf->Handle;
	mesh.Ebo = idxBuf->Handle;

	return Handle<Mesh>(static_cast<size_t>(id));
}

Mesh* IRAssetManager::GetMeshWithHandle(Handle<Mesh> Hnd)
{
	return &MeshStore[Hnd];
}

bool IRAssetManager::DeleteMesh(Handle<Mesh> Hnd)
{
	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);
	Resource* resource = meshCache->Get(Hnd);

	if (resource->RefCount != 1)
	{
		return false;
	}

	meshCache->Delete(Hnd);
	MeshStore.Remove(Hnd);

	return true;
}

void IRAssetManager::PushMeshIntoModel(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd)
{
	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);

	meshCache->AddRef(MeshHnd);

	Mesh& mesh	 = MeshStore[MeshHnd];
	Model& model = ModelStore[ModelHnd];

	model.Push(&mesh);
}

bool IRAssetManager::RemoveMeshFromModel(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd)
{
	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);

	Mesh& toDelete	= MeshStore[MeshHnd];
	Model& model	= ModelStore[ModelHnd];

	size_t deleteIndex = -1;

	for (size_t i = 0; i < model.Length(); i++)
	{
		Mesh* mesh = model[i];

		if (mesh != &toDelete)
		{
			continue;
		}

		deleteIndex = i;
		break;
	}

	if (deleteIndex == -1)
	{
		return false;
	}

	meshCache->DeRef(MeshHnd);
	model.PopAt(deleteIndex);

	return true;
}

Handle<Texture> IRAssetManager::CreateNewTexture(const TextureCreateInfo& CreateInfo)
{
	ResourceCache* textureCache = Manager.FetchCacheForType(Renderer_Asset_Texture);

	uint32 id = textureCache->Create();
	Resource* resource = textureCache->Get(id);
	resource->Type = Renderer_Asset_Texture;

	Texture& texture = TextureStore.Add(id, Texture()).Value;
	FMemory::Memcpy(&texture, &CreateInfo, sizeof(TextureBase));

	texture.Usage.Set(Image_Usage_Sampled);
	texture.Usage.Set(Image_Usage_Transfer_Dst);
	gpu::CreateTexture(texture);

	const uint8* data = CreateInfo.TextureData.ConstData();
	
	TextureMemory.UploadTexture(&texture, const_cast<uint8*>(data), false);

	auto& buffer = const_cast<Buffer<uint8>&>(CreateInfo.TextureData);
	buffer.Release();

	//SRMemoryBuffer textureBuffer = {};
	//textureBuffer.Size = texture.Size;
	//textureBuffer.Locality = Buffer_Locality_Cpu;
	//textureBuffer.Type.Set(Buffer_Type_Transfer_Src);

	//gpu::CreateBuffer(textureBuffer, data, texture.Size);

	//gpu::BeginTransfer();
	//gpu::TransferTexture(texture, textureBuffer);
	//gpu::EndTransfer();
	//gpu::DestroyBuffer(textureBuffer);

	return Handle<Texture>(static_cast<size_t>(id));
}

Handle<Texture> IRAssetManager::GetTextureHandleWithName(const String128& Identity)
{
	size_t key = INVALID_HANDLE;
	Texture* texture = nullptr;
	for (auto& pair : TextureStore)
	{
		texture = &pair.Value;
		if (texture->Name != Identity)
		{
			continue;
		}
		key = pair.Key;
		break;
	}
	return Handle<Texture>(key);
}

Texture* IRAssetManager::GetTextureWithHandle(Handle<Texture> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE && "Invalid handle provided to function");
	return Hnd != INVALID_HANDLE ? &TextureStore[Hnd] : nullptr;
}

bool IRAssetManager::DeleteTexture(Handle<Texture> Hnd)
{
	ResourceCache* textureCache = Manager.FetchCacheForType(Renderer_Asset_Texture);
	Resource* resource = textureCache->Get(Hnd);

	if (resource->RefCount != 1)
	{
		return false;
	}

	Texture& texture = TextureStore[Hnd];
	gpu::DestroyTexture(texture);
	textureCache->Delete(Hnd);
	MeshStore.Remove(Hnd);

	return false;
}

void IRAssetManager::Destroy()
{
	// Meshes and models are not required to go through this since their data is uploaded to the GPU.

	Shader* shader = nullptr;
	Texture* texture = nullptr;

	for (auto& pair : ShaderStore)
	{
		shader = &pair.Value;
		shader->Attributes.Release();
		gpu::DestroyShader(*shader);
	}

	for (auto& pair : TextureStore)
	{
		texture = &pair.Value;
		gpu::DestroyTexture(*texture);
	}
}
