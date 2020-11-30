#pragma once
#ifndef LEARNVK_RENDERER_ASSETS
#define LEARNVK_RENDERER_ASSETS

#include "RenderPlatform/API.h"
#include "Library/Containers/Map.h"
#include "SubSystem/Resource/Handle.h"
#include "Shader.h"

// Only the items below are considered as an asset to the graphics system.
enum RendererResourceType : uint32
{
	Renderer_Resource_Mesh		= 0x00,
	Renderer_Resource_Model		= 0x01,
	Renderer_Resource_Shader	= 0x02,
	Renderer_Resource_Texture	= 0x03,
	Renderer_Resource_Material	= 0x04
};

// TODO(Ygsm):
// IMPORTANT before proceeding to shader and pipeline creation in Vulkan!!!

// This is how resources will be loaded and built in the engine.
// Users will call LoadAssetType that takes in an AssetTypeCreateInfo struct.
// LoadAssetType function creates a new entry in the Resource for it's type inside of ResourceManager.
// It also stores a copy of the asset into it's own store.
// By default, signals the driver to build the asset (something like Gravity's).
// Driver watches for new assets to be built every frame.
class RENDERER_API RendererAssetManager
{
public:

	RendererAssetManager();
	~RendererAssetManager();

	DELETE_COPY_AND_MOVE(RendererAssetManager)

	void Initialize();
	void Terminate();

	/**
	* Loads the contents of the shader file into memory.
	*/
	Handle<Shader>	CreateNewShader		(const ShaderCreateInfo& CreateInfo);
	Shader*			GetShaderWithHandle	(Handle<Shader> Handle);
	/**
	* Removes the contents of the shader file from memory.
	*/
	bool			DeleteShader		(Handle<Shader> Id);

private:
	Map<uint32, Shader> ShaderStore;
	ResourceManager*	Manager;
};

#endif // !LEARNVK_RENDERER_ASSETS