#include "Engine/Interface.h"
#include "Assets.h"
#include "Library/Stream/Ifstream.h"

RendererAssetManager::RendererAssetManager()  {}
RendererAssetManager::~RendererAssetManager() {}

void RendererAssetManager::Initialize()
{
	Manager = &ao::FetchEngineCtx().Manager;
	Manager->AddCache(Renderer_Resource_Mesh);
	Manager->AddCache(Renderer_Resource_Model);
	Manager->AddCache(Renderer_Resource_Shader);
	Manager->AddCache(Renderer_Resource_Texture);
	Manager->AddCache(Renderer_Resource_Material);

	ShaderStore.Reserve(512);
}

void RendererAssetManager::Terminate()
{
	ShaderStore.Release();

	Manager->RemoveCache(Renderer_Resource_Mesh);
	Manager->RemoveCache(Renderer_Resource_Model);
	Manager->RemoveCache(Renderer_Resource_Shader);
	Manager->RemoveCache(Renderer_Resource_Texture);
	Manager->RemoveCache(Renderer_Resource_Material);
}

Handle<Shader> RendererAssetManager::CreateNewShader(const ShaderCreateInfo& CreateInfo)
{
	ResourceCache* shaderCache = Manager->FetchCacheForType(Renderer_Resource_Shader);
	
	if (shaderCache->Find(CreateInfo.Path))
	{
		return -1;
	}

	uint32 id = shaderCache->Create();
	Resource* resource = shaderCache->Get(id);
	resource->Path = CreateInfo.Path;
	resource->Type = Renderer_Resource_Shader;

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

	return Handle<Shader>(static_cast<size_t>(id));
}

Shader* RendererAssetManager::GetShaderWithHandle(Handle<Shader> Handle)
{
	return &ShaderStore[Handle];
}

bool RendererAssetManager::DeleteShader(Handle<Shader> Id)
{
	ResourceCache* shaderCache = Manager->FetchCacheForType(Renderer_Resource_Shader);
	Resource* shader = shaderCache->Get(Id);

	if (shader->RefCount != 1)
	{
		return false;
	}
	
	shaderCache->Delete(Id);
	ShaderStore.Remove(Id);
	return true;
}