#pragma once
#ifndef LEARNVK_RENDERER_API_VK_SHADER_TO_SPIRV_COMPILER
#define LEARNVK_RENDERER_API_VK_SHADER_TO_SPIRV_COMPILER

#include "Library/Containers/Array.h"
#include "Library/Containers/Deque.h"
#include "Library/Containers/String.h"
#include "Src/shaderc.hpp"
#include <vector>

namespace vk
{
	using BinaryBuffer = Array<uint8>;
	using DWordBuffer = Array<uint32>;

	class ShaderToSPIRVCompiler
	{
	public:
		bool		PreprocessShader(const char* ShaderName, shaderc_shader_kind Type, const char* Code, String& Buf);
		bool		CompileShaderToAssembly(const char* ShaderName, shaderc_shader_kind Type, const char* Code, BinaryBuffer& Buf);
		bool		CompileShader(const char* ShaderName, shaderc_shader_kind Type, const char* Code, DWordBuffer& Buf);
		const char* GetLastErrorMessage();
	private:
		using CompileErrorLogs = Deque<String>;
		CompileErrorLogs ErrorLogs;

		void CacheToErrorLog(const char* Msg);
	};
}

#endif // !LEARNVK_RENDERER_API_VK_SHADER_TO_SPIRV_COMPILER