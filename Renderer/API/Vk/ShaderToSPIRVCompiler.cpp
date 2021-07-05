#include "ShaderToSPIRVCompiler.h"

/**
* TODO(Ygsm):
* There is a function called AddMacroDefinition in shaderc.
* Figure out what it does and if required, write a parser that searches for preprocessor definitions.
*/

bool ShaderToSPIRVCompiler::PreprocessShader(const char* ShaderName, shaderc_shader_kind Type, const char* Code, astl::String& Buf)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl(Code, Type, ShaderName, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		CacheToErrorLog(result.GetErrorMessage().c_str());
		return false;
	}

	Buf.Reserve(KILOBYTES(128));
	Buf.Write(result.cbegin());

	return true;
}

bool ShaderToSPIRVCompiler::CompileShaderToAssembly(const char* ShaderName, shaderc_shader_kind Type, const char* Code, astl::Array<uint8>& Buf)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetOptimizationLevel(shaderc_optimization_level_size);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);

	shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(Code, Type, ShaderName, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		CacheToErrorLog(result.GetErrorMessage().c_str());
		return false;
	}

	if (!Buf.Size())
	{
		Buf.Reserve(KILOBYTES(128));
	}

	for (auto binary : result)
	{
		Buf.Push(binary);
	}
	return true;
}

bool ShaderToSPIRVCompiler::CompileShader(const char* ShaderName, shaderc_shader_kind Type, const char* Code, astl::Array<uint32>& Buf)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetOptimizationLevel(shaderc_optimization_level_size);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);

	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(Code, Type, ShaderName, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		CacheToErrorLog(result.GetErrorMessage().c_str());
		return false;
	}

	if (!Buf.Size())
	{
		Buf.Reserve(KILOBYTES(32));
	}

	for (auto word : result)
	{
		Buf.Push(word);
	}
	return true;
}

const char* ShaderToSPIRVCompiler::GetLastErrorMessage()
{
	return ErrorLogs.Front().C_Str();
}

void ShaderToSPIRVCompiler::CacheToErrorLog(const char* Msg)
{
	if (ErrorLogs.Length() == ErrorLogs.Size() - 1)
	{
		ErrorLogs.PopBack();
	}
	ErrorLogs.PushFront(Msg);
}
