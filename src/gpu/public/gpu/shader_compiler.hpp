#pragma once
#ifndef GPU_SHADER_COMPILER_H
#define GPU_SHADER_COMPILER_H

#include "lib/string.hpp"
#include "lib/map.hpp"
#include "gpu/common.hpp"

namespace gpu
{
namespace util
{
struct ShaderCompileInfo
{
	/**
	* \brief This should be the shader's path.
	*/
	std::string_view name;
	ShaderType type;
	/*
	* Deprecated, don't use! Only here for legacy reasons and until glsl is removed.
	*/
	std::string_view entryPoint;
	std::string_view sourceCode;
	/**
	* \brief Do not use. Use add_macro_definition rather than appending manually.
	*/
	lib::string defines;
	/**
	* \brief Do not use. Use add_macro_definition rather than appending manually.
	*/
	lib::map<std::string_view, std::string_view> macroDefinitions;
	/**
	* \brief 0 - no optimization
	* \brief 1 - performance
	* \brief 2 - size
	* \brief 3 - performance and size
	*/
	uint32 optimizationLevel;
	/**
	* \brief Compiled with debug symbol if 1.
	*/
	uint32 enableDebugSymbols : 1;

	auto add_macro_definition(std::string_view key) -> void;
	auto add_macro_definition(std::string_view key, std::string_view value) -> void;
	auto add_macro_definition(std::string_view key, uint32 value) -> void;
	auto clear_macro_definitions() -> void;
	auto remove_macro_definition(std::string_view key) -> void;
};

struct ShaderCompilerBackend;

struct ShaderCompiledUnit
{
	ShaderType type;
	lib::string name;
	lib::string entryPoint;
	lib::array<uint32> byteCode;

	auto compiled_info() const -> CompiledShaderInfo
	{
		return { 
			.type = type, 
			.entryPoint = "main", 
			.binaries = std::span{ byteCode.data(), byteCode.size() }
		};
	}
};

/**
* TODO(afiq):
* 1. Allow file watching when enabled.
* 2. Add / Remove compiled shaders.
* 3. Cache the result of the compilation and only recompile if the filewatcher kicks in or if the driver version does not match.
* 4. Add some sort of scratch buffer for string allocation and replace lib::string with std::string_view
*/
class ShaderCompiler : public lib::non_copyable_non_movable
{
public:
	ShaderCompiler() = default;
	~ShaderCompiler() = default;

	static auto create() -> std::expected<std::unique_ptr<ShaderCompiler>, std::string_view>;

	auto add_include_directory(std::string_view dir) -> void;
	auto remove_include_directory(std::string_view dir) -> void;
	auto compile(ShaderCompileInfo const& info) -> std::expected<ShaderCompiledUnit const, lib::string>;
	auto add_macro_definition(std::string_view definition) -> void;
	auto add_macro_definition(std::string_view key, std::string_view value) -> void;
	auto add_macro_definition(std::string_view key, uint32 value) -> void;
	auto remove_macro_definition(std::string_view key) -> void;
	auto clear_macro_definitions() -> void;

protected:
	lib::array<lib::string> 			m_includeDirectories;
	lib::map<lib::string, lib::string> 	m_macroDefinitions;
};
}
}

#endif // !GPU_SHADER_COMPILER_H
