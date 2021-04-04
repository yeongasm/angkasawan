#pragma once
#ifndef LEARNVK_RENDERER_ASSETS
#define LEARNVK_RENDERER_ASSETS

#include "RenderPlatform/API.h"
#include "Library/Containers/Map.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderAbstracts/Primitives.h"

class RenderContext;
class ResourceManager;
class IRenderMemoryManager;
class IRTextureMemoryManager;

// Only the items below are considered as an asset to the graphics system.
enum ERendererAsset : uint32
{ 
	Renderer_Asset_Mesh		= 0x00,
	Renderer_Asset_Model	= 0x01,
	Renderer_Asset_Shader	= 0x02,
	Renderer_Asset_Texture	= 0x03,
	Renderer_Asset_Material	= 0x04
};

/**
* Definition of an asset in Angkasawan:
*	- External resource required by the engine and is streamable into the engine.
* 
* TODO(Ygsm):
* [ ] Only return the handle to the resource if fetching my name.
*/
class RENDERER_API IRAssetManager
{
public:

	IRAssetManager(IRenderMemoryManager& Memory, IRTextureMemoryManager& TextureMemory, ResourceManager& Manager);
	~IRAssetManager();

	DELETE_COPY_AND_MOVE(IRAssetManager)

	Handle<Shader>	CreateNewShader		(const ShaderCreateInfo& CreateInfo);
	Shader*			GetShaderWithHandle	(Handle<Shader> Hnd);
	bool			DeleteShader		(Handle<Shader> Hnd);

	Handle<Model>	CreateNewModel		(const ModelCreateInfo& CreateInfo);
	Model*			GetModelWithName	(const String128& Identity);
	Model*			GetModelWithHandle	(Handle<Model> Hnd);
	bool			DeleteModel			(Handle<Model> Hnd);

	Handle<Mesh>	CreateNewMesh		(const MeshCreateInfo& CreateInfo);
	Mesh*			GetMeshWithHandle	(Handle<Mesh> Hnd);
	bool			DeleteMesh			(Handle<Mesh> Hnd);

	void			PushMeshIntoModel	(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd);
	bool			RemoveMeshFromModel	(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd);

	Handle<Texture> CreateNewTexture		(const TextureCreateInfo& CreateInfo);
	Handle<Texture> GetTextureHandleWithName(const String128& Identity);
	Texture*		GetTextureWithHandle	(Handle<Texture> Hnd);
	bool			DeleteTexture			(Handle<Texture> Hnd);

	void			Destroy();

private:
	Map<uint32, Shader> ShaderStore;
	Map<uint32, Mesh> MeshStore;
	Map<uint32, Model> ModelStore;
	Map<uint32, Texture> TextureStore;
	ResourceManager& Manager;
	IRenderMemoryManager& Memory;
	IRTextureMemoryManager& TextureMemory;
};

#endif // !LEARNVK_RENDERER_ASSETS