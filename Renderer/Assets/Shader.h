#pragma once
#ifndef LEARNVK_RENDERER_ASSETS_SHADER_H
#define LEARNVK_RENDERER_ASSETS_SHADER_H

#include "API/RendererFlagBits.h"
#include "GPUHandles.h"
#include "Library/Containers/Path.h"

struct ShaderCreateInfo
{
	FilePath	Path;
	String128	Name;
	ShaderType	Type;
};


struct ShaderAttribs
{
	size_t Binding;
	size_t Location;
};


/**
* TODO(Ygsm):
* 1. Tokenize shader code and have it look up at certain keywords (High priority) [Done].
* 2. Fetch vertex attributes for vertex shaders and unifr
* 3. Derive this from a streamable class (Super low priority).
*/
struct Shader
{
	String			Code;
	String128		Name;
	ShaderType		Type;
	Handle<HShader>	Handle;
};



#endif // !LEARNVK_RENDERER_ASSETS_SHADER_H