#pragma once
#ifndef LEARNVK_RENDERER_API_VK_SHADER_TO_SPIRV_COMPILER
#define LEARNVK_RENDERER_API_VK_SHADER_TO_SPIRV_COMPILER

#include "Library/Containers/Buffer.h"
#include "Library/Containers/Array.h"
#include "Library/Containers/Deque.h"
#include "Library/Containers/String.h"
#include "Src/shaderc.hpp"

/**
* TODO(Ygsm):
* 
* Should include SpirV-Cross for reflections and generate pipeline layout automatically.s
*/
class ShaderToSPIRVCompiler
{
public:
	bool		PreprocessShader(const char* ShaderName, shaderc_shader_kind Type, const char* Code, astl::String& Buf);
	bool		CompileShaderToAssembly(const char* ShaderName, shaderc_shader_kind Type, const char* Code, astl::Array<uint8>& Buf);
	bool		CompileShader(const char* ShaderName, shaderc_shader_kind Type, const char* Code, astl::Array<uint32>& Buf);
	const char* GetLastErrorMessage();
private:
	using CompileErrorLogs = astl::Deque<astl::String>;
	CompileErrorLogs ErrorLogs;

	void CacheToErrorLog(const char* Msg);
};

#endif // !LEARNVK_RENDERER_API_VK_SHADER_TO_SPIRV_COMPILER
