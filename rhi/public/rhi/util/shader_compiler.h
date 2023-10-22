#pragma once
#ifndef RHI_UTIL_SHADER_COMPILER_H
#define RHI_UTIL_SHADER_COMPILER_H

#include <fstream>
#include <filesystem>
#include "lib/string.h"
#include "lib/map.h"
//#include "lib/set.h"
#include "lib/handle.h"
#include "rhi/common.h"

namespace rhi
{
namespace util
{
struct ShaderCompileInfo
{
	/**
	* \brief This should be the shader's path.
	*/
	std::string_view name;
	std::string_view path;
	ShaderType type;
	lib::string entryPoint = "main";
	lib::string sourceCode;
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

	RHI_API auto add_macro_definition(std::string_view key) -> void;
	RHI_API auto add_macro_definition(std::string_view key, std::string_view value) -> void;
	RHI_API auto add_macro_definition(std::string_view key, uint32 value) -> void;
	RHI_API auto clear_macro_definitions() -> void;
	RHI_API auto remove_macro_definition(std::string_view key) -> void;
};

class CompilationResult
{
public:
	CompilationResult() = default;
	CompilationResult(std::string_view);
	CompilationResult(CompiledShaderInfo&&);
	~CompilationResult() = default;

	RHI_API auto compiled_shader_info() const -> std::optional<CompiledShaderInfo>;
	RHI_API auto ok() const -> bool;
	RHI_API auto error_msg() const -> std::string_view;
	RHI_API explicit operator bool() const;
private:
	std::optional<CompiledShaderInfo> m_info;
	std::string_view m_msg;
};

class ShaderCompiler
{
public:
	using include_dir_index = lib::handle<struct IncludeDirectory, size_t, std::numeric_limits<size_t>::max()>;

	RHI_API ShaderCompiler() = default;
	RHI_API ~ShaderCompiler() = default;

	RHI_API auto add_include_directory(std::string_view dir) -> include_dir_index;
	RHI_API auto compile_shader(ShaderCompileInfo const& info) -> CompilationResult;
	RHI_API auto get_compiled_shader_info(std::string_view name) const -> std::optional<CompiledShaderInfo>;
	RHI_API auto add_macro_definition(std::string_view definition) -> void;
	RHI_API auto add_macro_definition(std::string_view key, std::string_view value) -> void;
	RHI_API auto add_macro_definition(std::string_view key, uint32 value) -> void;
	RHI_API auto clear_macro_definitions() -> void;
private:
	struct CompiledUnit
	{
		ShaderType type;
		std::string_view name;
		std::string_view entryPoint;
		std::string_view path;
		lib::array<uint32> binaries;
		lib::array<ShaderAttribute> inputVar;
	};
	lib::string_pool<64_KiB> m_string_pool;;
	lib::array<std::filesystem::path> m_include_dirs;
	lib::map<lib::hash_string_view, CompiledUnit> m_shader_compilations;
	lib::map<std::string_view, std::string_view> m_macro_definitions;

	auto reflect_compiled_unit(CompiledUnit& unit) -> void;
};
}
}

#endif // !RHI_UTIL_SHADER_COMPILER_H
