#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#include "util/shader_compiler.hpp"

namespace gpu
{
namespace util
{
auto ShaderCompileInfo::add_macro_definition(std::string_view key) -> void
{
	if (defines.capacity() == defines.insitu_capacity())
	{
		defines.reserve((uint32)1_KiB);
	}
	auto start = defines.end();
	defines.append(key);
	macroDefinitions.emplace(std::string_view{ start.data(), static_cast<size_t>(defines.end() - start) }, std::string_view{});
}

auto ShaderCompileInfo::add_macro_definition(std::string_view key, std::string_view value) -> void
{
	if (defines.capacity() == defines.insitu_capacity())
	{
		defines.reserve((uint32)1_KiB);
	}
	auto kstart = defines.end();
	defines.append(key);
	std::string_view k{ kstart.data(), key.size() };

	auto vstart = defines.end();
	defines.append(value);
	std::string_view v{ vstart.data(), value.size() };

	macroDefinitions.emplace(std::move(k), std::move(v));
}

auto ShaderCompileInfo::add_macro_definition(std::string_view key, uint32 value) -> void
{
	if (defines.capacity() == defines.insitu_capacity())
	{
		defines.reserve((uint32)1_KiB);
	}
	auto kstart = defines.end();
	defines.append(key);
	std::string_view k{ kstart.data(), key.size() };

	auto vstart = defines.end();
	defines.append("{}", value);
	std::string_view v{ vstart.data(), (std::string_view::size_type)std::distance(vstart, defines.end()) };

	macroDefinitions.emplace(std::move(k), std::move(v));
}

auto ShaderCompileInfo::clear_macro_definitions() -> void
{
	macroDefinitions.clear();
	defines.clear();
}

auto ShaderCompileInfo::remove_macro_definition(std::string_view key) -> void
{
	macroDefinitions.erase(key);
}

CompilationResult::CompilationResult(
	std::string_view msg
) :
	m_info{ std::nullopt },
	m_msg{ msg }
{}

CompilationResult::CompilationResult(
	CompiledShaderInfo&& info
) :
	m_info{ std::move(info) },
	m_msg{}
{}

auto CompilationResult::compiled_shader_info() const -> std::optional<CompiledShaderInfo>
{
	return m_info;
}

auto CompilationResult::ok() const -> bool
{
	return m_msg.size() == 0;
}

auto CompilationResult::error_msg() const -> std::string_view
{
	return m_msg;
}

CompilationResult::operator bool() const
{
	return ok();
}

class ShadercDefaultIncluder : public shaderc::CompileOptions::IncluderInterface
{
private:
	struct IncludeData
	{
		std::string source;
		std::string content;
	};
	//std::filesystem::path base_path = std::filesystem::current_path();
	std::span<std::filesystem::path> m_include_directories;
public:
	ShadercDefaultIncluder(std::span<std::filesystem::path> includeDirs) :
		m_include_directories{ includeDirs }
	{}

	// Handles shaderc_include_resolver_fn callbacks.
	virtual auto GetInclude(const char* requested_source,
		[[maybe_unused]] shaderc_include_type type,
		const char* requesting_source,
		[[maybe_unused]] size_t include_depth) -> shaderc_include_result* override
	{
		IncludeData* data = new IncludeData;
		std::ostringstream buf;

		auto requester_path = std::filesystem::absolute(std::filesystem::path{ requesting_source }).remove_filename();
		auto alternative_path = std::filesystem::current_path() / requested_source;
		auto path = requester_path / requested_source;

		if (std::ifstream input{ path, std::ios::in | std::ios::binary }; input)
		{
			// 1. Look for the file in the same directory as the requesting source.
			buf << input.rdbuf();
			data->content = buf.str();
			data->source = path.string();
		}
		else if (m_include_directories.size())
		{
			// 2. Look for list of supplied include directories.
			for (auto const& include_dir : m_include_directories)
			{
				path = include_dir / requested_source;
				if (input = std::ifstream{ path / requested_source, std::ios::in | std::ios::binary }; input)
				{
					buf << input.rdbuf();
					data->content = buf.str();
					data->source = path.string();
					break;
				}
			}
		}
		else if (input = std::ifstream{ alternative_path, std::ios::in | std::ios::binary }; input)
		{
			// 3. Look for file in current working directory.
			buf << input.rdbuf();
			data->content = buf.str();
			data->source = path.string();
		}
		else
		{	
			// 4. Force compilation error.
			data->content = "file could not be read.";
		}

		shaderc_include_result* result = new shaderc_include_result;
		result->user_data = data;
		result->content = data->content.data();
		result->content_length = data->content.size();
		result->source_name = data->source.data();
		result->source_name_length = data->source.size();

		return result;
	}

	// Handles shaderc_include_result_release_fn callbacks.
	virtual auto ReleaseInclude(shaderc_include_result* data) -> void override
	{
		delete static_cast<IncludeData*>(data->user_data);
		delete data;
	}
};

auto translate_shader_type_to_shaderc_kind(ShaderType type) -> shaderc_shader_kind
{
	switch (type)
	{
	case ShaderType::Fragment:
		return shaderc_fragment_shader;
	case ShaderType::Geometry:
		return shaderc_geometry_shader;
	case ShaderType::Tesselation_Control:
		return shaderc_tess_control_shader;
	case ShaderType::Tesselation_Evaluation:
		return shaderc_tess_evaluation_shader;
	case ShaderType::Compute:
		return shaderc_compute_shader;
	case ShaderType::None:
	case ShaderType::Vertex:
	default:
		return shaderc_vertex_shader;
	}
}

auto translate_to_shader_attribute_format(SpvReflectFormat spvFormat) -> Format
{
	switch (spvFormat)
	{
	case SPV_REFLECT_FORMAT_R32_UINT:
		return Format::R32_Uint;
	case SPV_REFLECT_FORMAT_R32_SINT:
		return Format::R32_Int;
	case SPV_REFLECT_FORMAT_R32_SFLOAT:
		return Format::R32_Float;
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
		return Format::R32G32_Float;
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
		return Format::R32G32B32_Float;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
		return Format::R32G32B32A32_Float;
	case SPV_REFLECT_FORMAT_R64_SINT:
		return Format::R64_Int;
	case SPV_REFLECT_FORMAT_R64_UINT:
		return Format::R64_Uint;
	case SPV_REFLECT_FORMAT_R64_SFLOAT:
		return Format::R64_Float;
	case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
		return Format::R64G64_Float;
	case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
		return Format::R64G64B64_Float;
	case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
		return Format::R64G64B64A64_Float;
	case SPV_REFLECT_FORMAT_UNDEFINED:
	default:
		return Format::Undefined;
	}
}

auto ShaderCompiler::add_include_directory(std::string_view dir) -> include_dir_index
{
	include_dir_index handle = {};
	auto path = std::filesystem::absolute(std::filesystem::path{ dir }).remove_filename();

	for (auto const& existing_dir : m_include_dirs)
	{
		if (existing_dir == path)
		{
			new (&handle) include_dir_index{ m_include_dirs.index_of(existing_dir) };
			return handle;
		}
	}

	size_t index = m_include_dirs.push(path);

	new (&handle) include_dir_index{ index };

	return handle;
}

auto ShaderCompiler::compile_shader(ShaderCompileInfo const& info) -> CompilationResult
{
	static constexpr std::string_view shader_types[] = {
		"vertex",
		"pixel",
		"geometry",
		"tesselation_control",
		"tesselation_evaluation",
		"compute"
	};

	shaderc::Compiler compiler = {};
	shaderc::CompileOptions options = {};

	// These are macro that are defined in the shader compiler instance.
	for (auto const& macro : m_macro_definitions)
	{
		if (macro.second.size())
		{
			options.AddMacroDefinition(std::string{ macro.first }, std::string{ macro.second });
		}
		else
		{
			options.AddMacroDefinition(std::string{ macro.first });
		}
	}

	// These are macros defined in the ShaderCompileInfo struct.
	for (auto const& macro : info.macroDefinitions)
	{
		if (macro.second.size())
		{
			options.AddMacroDefinition(std::string{ macro.first }, std::string{ macro.second });
		}
		else
		{
			options.AddMacroDefinition(std::string{ macro.first });
		}
	}

	shaderc_shader_kind type = translate_shader_type_to_shaderc_kind(info.type);

	options.SetIncluder(std::make_unique<ShadercDefaultIncluder>(std::span{ m_include_dirs.data(), m_include_dirs.size() }));
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	options.SetSourceLanguage(shaderc_source_language_glsl);
	//auto foo = compiler.PreprocessGlsl(info.sourceCode.c_str(), type, info.path.data(), options);
	//fmt::print("\n{}", foo.cbegin());
	// Unoptimized compilation to enable robust reflection.
	auto unoptimized = compiler.CompileGlslToSpv(info.sourceCode.c_str(), type, info.path.data(), options);

	// Set optimization level.
	for (uint32 i = 1; i < info.optimizationLevel; ++i)
	{
		options.SetOptimizationLevel((shaderc_optimization_level)i);
	}

	// Set debug info generation. 
	if (info.enableDebugSymbols == 1)
	{
		options.SetGenerateDebugInfo();
	}

	// Final compilation output that will be used to create the shader module.
	auto optimized = compiler.CompileGlslToSpv(info.sourceCode.c_str(), type, info.path.data(), options);

	if (unoptimized.GetCompilationStatus() != shaderc_compilation_status_success ||
		optimized.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		std::string_view errorMessage = m_string_pool.append(optimized.GetErrorMessage().c_str());
		return CompilationResult{ errorMessage };
	}

	std::string_view name = m_string_pool.append(info.name);
	std::string_view path = m_string_pool.append(info.path);
	std::string_view entryPoint = m_string_pool.append((std::string_view)info.entryPoint);

	lib::hash_string_view key = m_string_pool.append("<shader:{}>:{}", shader_types[(size_t)info.type], path);

	auto&& [_, compiledUnit] = m_shader_compilations.emplace(key, CompiledUnit{ .type = info.type, .name = name, .entryPoint = entryPoint, .path = path });
	
	compiledUnit.binaries.assign(unoptimized.begin(), unoptimized.end());

	reflect_compiled_unit(compiledUnit);

	compiledUnit.binaries.clear();
	compiledUnit.binaries.assign(optimized.begin(), optimized.end());

	CompiledShaderInfo compiledInfo{
		.name = name,
		.path = path,
		.type = info.type,
		.entryPoint = compiledUnit.entryPoint,
		.binaries = std::span{ compiledUnit.binaries.data(), compiledUnit.binaries.size() },
		.vertexInputAttributes = std::span{ compiledUnit.inputVar.data(), compiledUnit.inputVar.size() }
	};

	return CompilationResult{ std::move(compiledInfo) };
}

auto ShaderCompiler::get_compiled_shader_info(std::string_view name) const -> std::optional<CompiledShaderInfo>
{
	if (auto result = m_shader_compilations.at(name); result)
	{
		auto&& compiledUnit = result.value()->second;

		CompiledShaderInfo compiledInfo{
			.name = result.value()->first,
			.type = compiledUnit.type,
			.entryPoint = compiledUnit.entryPoint,
			.binaries = std::span{ compiledUnit.binaries.data(), compiledUnit.binaries.size() },
			.vertexInputAttributes = std::span{ compiledUnit.inputVar.data(), compiledUnit.inputVar.size() }
		};
		return std::optional{ std::move(compiledInfo) };
	}
	return std::nullopt;
}

auto ShaderCompiler::add_macro_definition(std::string_view definition) -> void
{
	auto def = m_string_pool.append(definition);
	m_macro_definitions.try_insert(def, std::string_view{});
}

auto ShaderCompiler::add_macro_definition(std::string_view key, std::string_view value) -> void
{
	auto k = m_string_pool.append(key);
	auto v = m_string_pool.append(value);
	m_macro_definitions.try_insert(k, k);
}

auto ShaderCompiler::add_macro_definition(std::string_view key, uint32 value) -> void
{
	auto k = m_string_pool.append(key);
	auto v = m_string_pool.append(value);
	m_macro_definitions.try_insert(k, v);
}

auto ShaderCompiler::clear_macro_definitions() -> void
{
	m_macro_definitions.clear();
}

auto ShaderCompiler::reflect_compiled_unit(CompiledUnit& unit) -> void
{
	spv_reflect::ShaderModule module{ unit.binaries.bytes(), unit.binaries.data() };

	uint32 numInputs = 0;

	module.EnumerateInputVariables(&numInputs, nullptr);

	SpvReflectInterfaceVariable* reflectionInputVariables[32] = {};

	if (numInputs)
	{
		unit.inputVar.reserve(size_t{ numInputs });
		module.EnumerateInputVariables(&numInputs, reflectionInputVariables);
	}

	for (uint32 i = 0; i < numInputs; ++i)
	{
		if (!reflectionInputVariables[i])
		{
			continue;
		}

		SpvReflectInterfaceVariable const& data = *reflectionInputVariables[i];

		if (data.location == std::numeric_limits<decltype(data.location)>::max())
		{
			continue;
		}

		std::string_view name = m_string_pool.append(data.name);

		ShaderAttribute attribute = {
			.name = name,
			.location = data.location,
			.format = translate_to_shader_attribute_format(data.format)
		};

		if (data.type_description->op == SpvOpTypeMatrix)
		{
			attribute.format = Format::R32G32B32A32_Float;
			for (uint32 j = 0; j < 4; ++j)
			{
				attribute.name = m_string_pool.append("{}_{}", attribute.name.data(), j);
				attribute.location = data.location + j;
				unit.inputVar.push_back(attribute);
			}
		}
		else if (data.type_description->op == SpvOpTypeArray)
		{
			for (uint32 j = 0; j < data.array.dims_count; ++j)
			{
				for (uint32 k = 0; k < data.array.dims[j]; ++k)
				{
					attribute.name = m_string_pool.append("{}_{}_{}", attribute.name.data(), j, k);
					attribute.location = data.location + k;
					unit.inputVar.push_back(attribute);
				}
			}
		}
		else
		{
			unit.inputVar.push_back(attribute);
		}
	}
	// Sort to ascending order.
	std::sort(
		unit.inputVar.begin(),
		unit.inputVar.end(),
		[](ShaderAttribute const& a, ShaderAttribute const& b) -> bool
		{
			return a.location < b.location;
		}
	);
}
}
}