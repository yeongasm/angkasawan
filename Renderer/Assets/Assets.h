#pragma once
#ifndef LEARNVK_RENDERER_ASSETS
#define LEARNVK_RENDERER_ASSETS

#include "RenderPlatform/API.h"
#include "Library/Containers/Map.h"
#include "SubSystem/Resource/Handle.h"
#include "Shader.h"
#include "Model.h"

// Only the items below are considered as an asset to the graphics system.
enum RendererResourceType : uint32
{
	Renderer_Resource_Mesh		= 0x00,
	Renderer_Resource_Model		= 0x01,
	Renderer_Resource_Shader	= 0x02,
	Renderer_Resource_Texture	= 0x03,
	Renderer_Resource_Material	= 0x04
};

/**
* Renderer's resource manager class.
* 
* Does not create the resources on the GPU tho. Still figuring out how to streamline asset creation and deletion.
*/
class RENDERER_API RendererAssetManager
{
public:

	RendererAssetManager();
	~RendererAssetManager();

	DELETE_COPY_AND_MOVE(RendererAssetManager)

	void Initialize();
	void Terminate();

	Handle<Shader>	CreateNewShader		(const ShaderCreateInfo& CreateInfo);
	Shader*			GetShaderWithHandle	(Handle<Shader> Hnd);
	bool			DeleteShader		(Handle<Shader> Hnd);

	Handle<Model>	CreateNewModel		(const ModelCreateInfo& CreateInfo);
	Model*			GetModelWithName	(const char* Identity);
	Model*			GetModelWithHandle	(Handle<Model> Hnd);
	bool			DeleteModel			(Handle<Model> Hnd);

	Handle<Mesh>	CreateNewMesh		(const MeshCreateInfo& CreateInfo);
	Mesh*			GetMeshWithHandle	(Handle<Mesh> Hnd);
	bool			DeleteMesh			(Handle<Mesh> Hnd);

	void			PushMeshIntoModel	(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd);
	bool			RemoveMeshFromModel	(Handle<Mesh> MeshHnd, Handle<Model> ModelHnd);

	//Handle<Model>	ImportModel(const ModelImportInfo& ImportInfo);

private:
	Map<uint32, Shader> ShaderStore;
	Map<uint32, Mesh>	MeshStore;
	Map<uint32, Model>	ModelStore;
	ResourceManager*	Manager;
};

#endif // !LEARNVK_RENDERER_ASSETS