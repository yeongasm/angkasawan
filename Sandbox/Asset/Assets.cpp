#include "Assets.h"
#include "Library/Algorithms/Hash.h"

static uint32 g_HashSeed = static_cast<uint32>(OS::GetPerfCounter());

namespace sandbox
{

	IAssetManager::IAssetManager(EngineImpl& Engine) :
		Shaders(),
		Models(),
		Textures(),
		Engine(Engine)
	{
		this->Engine.CreateNewResourceCache(Renderer_Asset_Model);
		this->Engine.CreateNewResourceCache(Renderer_Asset_Shader);
		this->Engine.CreateNewResourceCache(Renderer_Asset_Texture);

		Shaders.Reserve(32);
		Models.Reserve(32);
		Textures.Reserve(32);
	}

	IAssetManager::~IAssetManager()
	{
		Shaders.Release();
		Models.Release();
		Textures.Release();

		Engine.DeleteResourceCacheForType(Renderer_Asset_Model);
		Engine.DeleteResourceCacheForType(Renderer_Asset_Shader);
		Engine.DeleteResourceCacheForType(Renderer_Asset_Texture);
	}

	Handle<Shader> IAssetManager::CreateNewShader(const Shader& Src)
	{
		ResourceCache* shaderCache = Engine.FetchResourceCacheForType(Renderer_Asset_Shader);
		
		//if (shaderCache->Find(Src.Path))
		//{
		//	return -1;
		//}

		uint32 id = shaderCache->Create();
		Resource* resource = shaderCache->Get(id);
		//resource->Path = Src.Path;
		resource->Type = Renderer_Asset_Shader;

		Shaders.Add(id, Src);

		return Handle<Shader>(static_cast<size_t>(id));
	}

	Ref<Shader> IAssetManager::GetShaderWithHandle(Handle<Shader> Hnd)
	{
		return &Shaders[Hnd];
	}

	bool IAssetManager::DeleteShader(Handle<Shader> Hnd)
	{
		ResourceCache* shaderCache = Engine.FetchResourceCacheForType(Renderer_Asset_Shader);
		Resource* res = shaderCache->Get(Hnd);

		if (res->RefCount != 1)
		{
			return false;
		}

		Shader& shader = Shaders[Hnd];
		shaderCache->Delete(Hnd);
		Shaders.Remove(Hnd);

		return true;
	}

	Handle<Model> IAssetManager::CreateNewModel(const Model& Src)
	{
		ResourceCache* modelCache = Engine.FetchResourceCacheForType(Renderer_Asset_Model);

		//if (modelCache->Find(Src.Path))
		//{
		//	return -1;
		//}

		uint32 id = modelCache->Create();
		Resource* resource = modelCache->Get(id);
		//resource->Path = Src.Path;
		resource->Type = Renderer_Asset_Model;

		Models.Add(id, Src);

		return Handle<Model>(static_cast<size_t>(id));
	}

	Ref<Model> IAssetManager::GetModelWithHandle(Handle<Model> Hnd)
	{
		return &Models[Hnd];
	}

	bool IAssetManager::DeleteModel(Handle<Model> Hnd)
	{
		ResourceCache* modelCache = Engine.FetchResourceCacheForType(Renderer_Asset_Model);
		Resource* resource = modelCache->Get(Hnd);

		if (resource->RefCount != 1)
		{
			return false;
		}

		modelCache->Delete(Hnd);
		Models.Remove(Hnd);

		return true;
	}

	//Handle<Mesh> IAssetManager::CreateNewMesh(const MeshCreateInfo& CreateInfo)
	//{
	//	constexpr size_t sizeOfVertex = sizeof(SRVertex);
	//	constexpr size_t sizeOfUint32 = sizeof(uint32);

	//	SRMemoryBuffer* vtxBuf = Memory.GetBuffer(CreateInfo.VtxMemHandle);
	//	SRMemoryBuffer* idxBuf = Memory.GetBuffer(CreateInfo.IdxMemHandle);

	//	if (!vtxBuf || !idxBuf) { return INVALID_HANDLE; }

	//	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);

	//	uint32 id = meshCache->Create();
	//	Resource* resource = meshCache->Get(id);
	//	resource->Type = Renderer_Asset_Mesh;

	//	Mesh& mesh = MeshStore.Add(id, Mesh()).Value;

	//	size_t vertexCount = CreateInfo.Vertices.Length();
	//	size_t indexCount = CreateInfo.Indices.Length();
	//	size_t vtxSize = sizeOfVertex * vertexCount;
	//	size_t idxSize = sizeOfUint32 * indexCount;

	//	mesh.Name = CreateInfo.Name;
	//	mesh.NumOfVertices = static_cast<uint32>(vertexCount);
	//	mesh.NumOfIndices = static_cast<uint32>(indexCount);
	//	mesh.VtxOffset = static_cast<uint32>(vtxBuf->Offset / sizeOfVertex);

	//	SRMemoryBuffer vtxBuffer = {};
	//	vtxBuffer.Locality = Buffer_Locality_Cpu;
	//	vtxBuffer.Size = vtxSize;
	//	vtxBuffer.Type.Set(Buffer_Type_Transfer_Src);
	//	vtxBuffer.Type.Set(Buffer_Type_Vertex);

	//	SRVertex* vertices = const_cast<SRVertex*>(CreateInfo.Vertices.First());
	//	uint32* indices = const_cast<uint32*>(CreateInfo.Indices.First());

	//	gpu::CreateBuffer(vtxBuffer, vertices, sizeOfVertex * vertexCount);
	//	Memory.UploadData(vtxBuffer, CreateInfo.VtxMemHandle, false);

	//	for (size_t i = 0; i < indexCount; i++)
	//	{
	//		indices[i] += mesh.VtxOffset;
	//	}

	//	mesh.IdxOffset = static_cast<uint32>(idxBuf->Offset / sizeOfUint32);

	//	SRMemoryBuffer idxBuffer = {};
	//	idxBuffer.Locality = Buffer_Locality_Cpu;
	//	idxBuffer.Size = idxSize;
	//	idxBuffer.Type.Set(Buffer_Type_Transfer_Src);
	//	idxBuffer.Type.Set(Buffer_Type_Index);

	//	gpu::CreateBuffer(idxBuffer, indices, sizeOfUint32 * indexCount);
	//	Memory.UploadData(idxBuffer, CreateInfo.IdxMemHandle, false);

	//	mesh.Vbo = vtxBuf->Handle;
	//	mesh.Ebo = idxBuf->Handle;

	//	return Handle<Mesh>(static_cast<size_t>(id));
	//}

	//Mesh* IAssetManager::GetMeshWithHandle(Handle<Mesh> Hnd)
	//{
	//	return &MeshStore[Hnd];
	//}

	//bool IAssetManager::DeleteMesh(Handle<Mesh> Hnd)
	//{
	//	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);
	//	Resource* resource = meshCache->Get(Hnd);

	//	if (resource->RefCount != 1)
	//	{
	//		return false;
	//	}

	//	meshCache->Delete(Hnd);
	//	MeshStore.Remove(Hnd);

	//	return true;
	//}

	//void IAssetManager::PushMeshIntoModel(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd)
	//{
	//	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);

	//	meshCache->AddRef(MeshHnd);

	//	Mesh& mesh = MeshStore[MeshHnd];
	//	Model& model = ModelStore[ModelHnd];

	//	model.Push(&mesh);
	//}

	//bool IAssetManager::RemoveMeshFromModel(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd)
	//{
	//	ResourceCache* meshCache = Manager.FetchCacheForType(Renderer_Asset_Mesh);

	//	Mesh& toDelete = MeshStore[MeshHnd];
	//	Model& model = ModelStore[ModelHnd];

	//	size_t deleteIndex = -1;

	//	for (size_t i = 0; i < model.Length(); i++)
	//	{
	//		Mesh* mesh = model[i];

	//		if (mesh != &toDelete)
	//		{
	//			continue;
	//		}

	//		deleteIndex = i;
	//		break;
	//	}

	//	if (deleteIndex == -1)
	//	{
	//		return false;
	//	}

	//	meshCache->DeRef(MeshHnd);
	//	model.PopAt(deleteIndex);

	//	return true;
	//}

	Handle<Texture> IAssetManager::CreateNewTexture(const Texture& Src)
	{
		ResourceCache* textureCache = Engine.FetchResourceCacheForType(Renderer_Asset_Texture);

		//if (textureCache->Find(Src.Path))
		//{
		//	return -1;
		//}

		uint32 id = textureCache->Create();
		Resource* resource = textureCache->Get(id);
		//resource->Path = Src.Path;
		resource->Type = Renderer_Asset_Texture;

		Textures.Add(id, Src);

		return Handle<Texture>(static_cast<size_t>(id));
	}

	Ref<Texture> IAssetManager::GetTextureWithHandle(Handle<Texture> Hnd)
	{
		return &Textures[Hnd];
	}

	bool IAssetManager::DeleteTexture(Handle<Texture> Hnd)
	{
		ResourceCache* textureCache = Engine.FetchResourceCacheForType(Renderer_Asset_Texture);
		Resource* resource = textureCache->Get(Hnd);

		if (resource->RefCount != 1)
		{
			return false;
		}

		textureCache->Delete(Hnd);
		Textures.Remove(Hnd);

		return false;
	}

	//void IAssetManager::Destroy()
	//{
	//	// Meshes and models are not required to go through this since their data is uploaded to the GPU.

	//	Shader* shader = nullptr;
	//	Texture* texture = nullptr;

	//	for (auto& pair : ShaderStore)
	//	{
	//		shader = &pair.Value;
	//		shader->Attributes.Release();
	//		gpu::DestroyShader(*shader);
	//	}

	//	for (auto& pair : TextureStore)
	//	{
	//		texture = &pair.Value;
	//		gpu::DestroyTexture(*texture);
	//	}
	//}

	Material::Material() :
		Textures{}
	{}

	Material::Material(String128 Name) :
		Textures{}, Name(Name)
	{}

	Material::~Material() {}

	void Material::AddTextureToSlot(Ref<Texture> pTexture, size_t Type)
	{
		this->Textures[Type] = pTexture;
	}

	MaterialDefinition::MaterialDefinition() :
		Materials{},
		Slots{},
		DescriptorSetHnd{},
		DescriptorSetLayoutHnd{},
		PipelineHnd{}
	{}

	MaterialDefinition::~MaterialDefinition() {}

	Ref<Material> MaterialDefinition::CreateMaterialFromDefinition(const String128& Name)
	{
		Material& material = Materials.Insert(Material(Name));
		return &material;
	}

	Ref<Material> MaterialDefinition::GetMaterial(const String128& Name)
	{
		for (Material& material : Materials)
		{
			if (material.Name == Name)
			{
				return &material;
			}
		}
		return Ref<Material>();
	}

	const Handle<SDescriptorSet> MaterialDefinition::GetDescriptorSetHandle() const
	{
		return DescriptorSetHnd;
	}

	const Handle<SDescriptorSetLayout> MaterialDefinition::GetDescriptorSetLayoutHandle() const
	{
		return DescriptorSetLayoutHnd;
	}

	const Handle<SPipeline> MaterialDefinition::GetPipelineHandle() const
	{
		return PipelineHnd;
	}
}