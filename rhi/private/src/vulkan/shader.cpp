#include <span>
#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#include "shader.h"
#include "vulkan/vk_device.h"

namespace rhi
{

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

auto get_shader_name(ShaderType type) -> literal_t
{
	switch (type)
	{
	case ShaderType::None:
	case ShaderType::Vertex:
		return "VertexShader";
	case ShaderType::Fragment:
		return "PixelShader";
	case ShaderType::Geometry:
		return "GeometryShader";
	case ShaderType::Tesselation_Control:
		return "TesselationControlShader";
	case ShaderType::Tesselation_Evaluation:
		return "TesselationEvaluationShader";
	case ShaderType::Compute:
		return "ComputeShader";
	default:
		return "UnkownShader";
	}
}

auto compile_to_shader_binaries(ShaderCompileInfo const& compileInfo, lib::string* error) -> std::pair<bool, ShaderInfo>
{
	shaderc::Compiler compiler{};
	shaderc::CompileOptions options{};

	options.SetOptimizationLevel(shaderc_optimization_level_size);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);
	options.SetWarningsAsErrors();

	for (auto const& macro : compileInfo.preprocessors)
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

	shaderc_shader_kind const shaderKind = translate_shader_type_to_shaderc_kind(compileInfo.type);
	auto shaderName = get_shader_name(compileInfo.type);

	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(compileInfo.sourceCode.c_str(), shaderKind, shaderName, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		if (error)
		{
			(*error).append("\n\n{}", result.GetErrorMessage());
		}
		return std::pair{ false, ShaderInfo{} };
	}

	ShaderInfo info = {
		.name = compileInfo.name,
		.type = compileInfo.type,
		.entryPoint = compileInfo.entryPoint
	};
	info.binaries.assign(result.begin(), result.end());

	return std::pair{ true, std::move(info) };
}

auto translate_to_shader_attribute_format(SpvReflectFormat spvFormat) -> ShaderAttribute::Format
{
	switch (spvFormat)
	{
	case SPV_REFLECT_FORMAT_UNDEFINED:
		return ShaderAttribute::Format::Undefined;
	case SPV_REFLECT_FORMAT_R32_UINT:
		return ShaderAttribute::Format::Uint;
	case SPV_REFLECT_FORMAT_R32_SINT:
		return ShaderAttribute::Format::Int;
	case SPV_REFLECT_FORMAT_R32_SFLOAT:
		return ShaderAttribute::Format::Float;
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
		return ShaderAttribute::Format::Vec2;
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
		return ShaderAttribute::Format::Vec3;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
		return ShaderAttribute::Format::Vec4;
	case SPV_REFLECT_FORMAT_R64_SINT:
		return ShaderAttribute::Format::Int64;
	case SPV_REFLECT_FORMAT_R64_UINT:
		return ShaderAttribute::Format::Uint64;
	case SPV_REFLECT_FORMAT_R64_SFLOAT:
		return ShaderAttribute::Format::Float64;
	case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
		return ShaderAttribute::Format::DVec2;
	case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
		return ShaderAttribute::Format::DVec3;
	case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
		return ShaderAttribute::Format::DVec4;
	default:
		return ShaderAttribute::Format::Undefined;
	}
}

auto reflect_shader_binaries(std::span<uint32> binaries) -> lib::array<ShaderAttribute>
{
	lib::array<ShaderAttribute> attributes;
	spv_reflect::ShaderModule module{ binaries.size_bytes(), binaries.data()};
	uint32 count = 0u;
	module.EnumerateInputVariables(&count, nullptr);

	if (count)
	{
		attributes.reserve(count);

		SpvReflectInterfaceVariable* variables[1024] = {};
		module.EnumerateInputVariables(&count, variables);

		for (auto it = std::begin(variables); it != (std::begin(variables) + count); ++it)
		{
			SpvReflectInterfaceVariable const* var = *it;
			ShaderAttribute attrib{};

			if (var->built_in == -1 || var->location == -1)
			{
				continue;
			}

			if (var->type_description->op == SpvOpTypeMatrix)
			{
				attrib.format = ShaderAttribute::Format::Vec4;
				for (uint32 j = 0; j < 4; ++j)
				{
					attrib.location = var->location + j;
					attributes.emplace_back(attrib);
				}
				continue;
			}

			attrib.format = translate_to_shader_attribute_format(var->format);
			attrib.location = var->location;

			if (var->type_description->op == SpvOpTypeArray)
			{
				for (uint32 j = 0; j < var->array.dims_count; ++j)
				{
					for (uint32 k = 0; k < var->array.dims[j]; ++k)
					{
						attrib.location = var->location + k;
						attributes.emplace_back(attrib);
					}
				}
				continue;
			}

			if (var->name)
			{
				attrib.name = var->name;
			}
			attributes.emplace_back(std::move(attrib));
		}
		std::sort(
			attributes.begin(),
			attributes.end(),
			[](ShaderAttribute const& a, ShaderAttribute const& b) -> bool
			{
				return a.location < b.location;
			}
		);
	}
	return attributes;
}

Shader::Shader(
	ShaderInfo&& info,
	APIContext* context,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) },
	m_attributes{}
{
	// TODO(afiq):
	// Work on this even more.
	// The end goal is to get full reflection of the shader.
	m_attributes = reflect_shader_binaries(std::span<uint32>{ m_info.binaries.data(), m_info.binaries.size() });
	// Once we finish reflecting, the binaries are no longer useful.
	m_info.binaries.release();
	m_context->setup_debug_name(*this);
}

Shader::Shader(Shader&& rhs) noexcept
{
	*this = std::move(rhs);
}

auto Shader::operator=(Shader&& rhs) noexcept -> Shader&
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		m_attributes = std::move(rhs.m_attributes);
		Resource::operator=(std::move(rhs));
		new (&rhs) Shader{};
	}
	return *this;
}

auto Shader::info() const -> ShaderInfo const&
{
	return m_info;
}

auto Shader::attributes() const -> std::span<ShaderAttribute const>
{
	return std::span{ m_attributes.data(), m_attributes.size() };
}

}