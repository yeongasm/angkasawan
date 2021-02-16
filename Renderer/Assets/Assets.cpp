#include "Engine/Interface.h"
#include "Assets.h"
#include "Library/Stream/Ifstream.h"
#include "Library/Algorithms/Tokenizer.h"

IRAssetManager::IRAssetManager()  {}
IRAssetManager::~IRAssetManager() {}

void IRAssetManager::Initialize()
{
	Manager = &ao::FetchEngineCtx().Manager;
	Manager->AddCache(Renderer_Asset_Mesh);
	Manager->AddCache(Renderer_Asset_Model);
	Manager->AddCache(Renderer_Asset_Shader);
	Manager->AddCache(Renderer_Asset_Texture);
	Manager->AddCache(Renderer_Asset_Material);

	ShaderStore.Reserve(512);
	MeshStore.Reserve(512);
	ModelStore.Reserve(128);
}

void IRAssetManager::Terminate()
{
	ShaderStore.Release();
	MeshStore.Release();
	ModelStore.Release();

	Manager->RemoveCache(Renderer_Asset_Mesh);
	Manager->RemoveCache(Renderer_Asset_Model);
	Manager->RemoveCache(Renderer_Asset_Shader);
	Manager->RemoveCache(Renderer_Asset_Texture);
	Manager->RemoveCache(Renderer_Asset_Material);
}

Handle<Shader> IRAssetManager::CreateNewShader(const ShaderCreateInfo& CreateInfo)
{
	ResourceCache* shaderCache = Manager->FetchCacheForType(Renderer_Asset_Shader);
	
	if (shaderCache->Find(CreateInfo.Path))
	{
		return -1;
	}

	uint32 id = shaderCache->Create();
	Resource* resource = shaderCache->Get(id);
	resource->Path = CreateInfo.Path;
	resource->Type = Renderer_Asset_Shader;

	Shader& shader = ShaderStore.Add(id, Shader()).Value;
	shader.Type = CreateInfo.Type;
	shader.Handle = -1;
	shader.Name = CreateInfo.Name;

	Ifstream ifstream;
	if (!ifstream.Open(resource->Path.C_Str()))
	{
		VKT_ASSERT("Unable to open shader at specified path!" && false);
		return -1;
	}

	shader.Code.Reserve(ifstream.Size());
	if (!ifstream.Read(shader.Code.First(), shader.Code.Size()))
	{
		VKT_ASSERT("Unable to read stream contents into buffer!" && false);
		return -1;
	}
	shader.Code[ifstream.Size()] = '\0';
	ifstream.Close();

	//
	// TODO(Ygsm):
	// Shader tokenize system later.
	//

	return Handle<Shader>(static_cast<size_t>(id));
}

Shader* IRAssetManager::GetShaderWithHandle(Handle<Shader> Hnd)
{
	return &ShaderStore[Hnd];
}

bool IRAssetManager::DeleteShader(Handle<Shader> Hnd)
{
	ResourceCache* shaderCache = Manager->FetchCacheForType(Renderer_Asset_Shader);
	Resource* shader = shaderCache->Get(Hnd);

	if (shader->RefCount != 1)
	{
		return false;
	}
	
	shaderCache->Delete(Hnd);
	ShaderStore.Remove(Hnd);

	return true;
}

Handle<Model> IRAssetManager::CreateNewModel(const ModelCreateInfo& Info)
{
	ResourceCache* modelCache = Manager->FetchCacheForType(Renderer_Asset_Model);
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
	model.Name = Info.Name;

	return Handle<Model>(static_cast<size_t>(id));
}

Model* IRAssetManager::GetModelWithName(const char* Identity)
{
	Model* queried = nullptr;
	for (auto& pair : ModelStore)
	{
		Model* theModel = &pair.Value;
		if (theModel->Name != Identity) { continue; }
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
	ResourceCache* modelCache = Manager->FetchCacheForType(Renderer_Asset_Model);
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
	ResourceCache* meshCache = Manager->FetchCacheForType(Renderer_Asset_Mesh);

	uint32 id = meshCache->Create();
	Resource* resource = meshCache->Get(id);
	resource->Type = Renderer_Asset_Mesh;

	Mesh& mesh = MeshStore.Add(id, Mesh()).Value;
	mesh.Vertices	= CreateInfo.Vertices;
	mesh.Indices	= CreateInfo.Indices;
	mesh.Group		= INVALID_HANDLE;

	return Handle<Mesh>(static_cast<size_t>(id));
}

Mesh* IRAssetManager::GetMeshWithHandle(Handle<Mesh> Hnd)
{
	return &MeshStore[Hnd];
}

bool IRAssetManager::DeleteMesh(Handle<Mesh> Hnd)
{
	ResourceCache* meshCache = Manager->FetchCacheForType(Renderer_Asset_Mesh);
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
	ResourceCache* meshCache = Manager->FetchCacheForType(Renderer_Asset_Mesh);

	meshCache->AddRef(MeshHnd);

	Mesh& mesh	 = MeshStore[MeshHnd];
	Model& model = ModelStore[ModelHnd];

	model.Push(&mesh);
}

bool IRAssetManager::RemoveMeshFromModel(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd)
{
	ResourceCache* meshCache = Manager->FetchCacheForType(Renderer_Asset_Mesh);

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