#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#include "fmt/format.h"
#include "rhi/shader_compiler.h"

namespace rhi
{

namespace shader_compiler
{

namespace glsl_shaderc_compiler
{

shaderc_shader_kind translate_shader_type_to_shaderc_kind(ShaderType type)
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

literal_t get_shader_name(ShaderType type)
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

ShaderAttribute::Format translate_to_shader_attribute_format(SpvReflectFormat spvFormat)
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
	default:
		return ShaderAttribute::Format::Undefined;
	}
}

std::vector<ShaderAttribute> reflect_spirv_shader_attributes(spv_reflect::ShaderModule const& shaderModule)
{
	std::vector<ShaderAttribute> attributes;
	uint32 count = 0u;
	shaderModule.EnumerateInputVariables(&count, nullptr);

	if (count)
	{
		attributes.reserve(count);

		std::vector<SpvReflectInterfaceVariable*> variables{ static_cast<size_t>(count) + 1 };
		variables.resize(count);

		shaderModule.EnumerateInputVariables(&count, variables.data());

		for (size_t i = 0; i < variables.size(); ++i)
		{	
			SpvReflectInterfaceVariable const* var = variables[i];
			ShaderAttribute attrib{};

			if (var->built_in == -1 || var->location == -1)
			{
				--count;
				variables.pop_back();
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

			attrib.format = glsl_shaderc_compiler::translate_to_shader_attribute_format(var->format);
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

			attributes.emplace_back(attrib);
		}

		// We check here again since there might not actually be any variables.
		if (count)
		{
			std::sort(
				attributes.begin(),
				attributes.end(),
				[](const ShaderAttribute& a, const ShaderAttribute& b) -> bool
				{
					return a.location < b.location;
				}
			);
		}
	}

	return attributes;
}

}

bool compile_glsl_to_spirv(ShaderCompileInfo const& compileInfo, ShaderInfo& shaderInfo, std::string* error)
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

	shaderc_shader_kind const shaderKind = glsl_shaderc_compiler::translate_shader_type_to_shaderc_kind(compileInfo.type);
	auto shaderName = glsl_shaderc_compiler::get_shader_name(compileInfo.type);

	/*shaderc::PreprocessedSourceCompilationResult preprocessResult = compiler.PreprocessGlsl(compileInfo.sourceCode.c_str(), shaderKind, shaderName, options);
	if (preprocessResult.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		if (error)
		{
			(*error).append(fmt::format("\n\n{}", preprocessResult.GetErrorMessage()));
		}
		return false;
	}

	std::string sourceCode{ preprocessResult.begin(), preprocessResult.end() };
	auto it = std::remove_if(
		sourceCode.begin(), 
		sourceCode.end(), 
		[](char& c) -> bool 
		{ 
			if (c == '\n' || c == '\r\n')
			{
				char* ptr = &c;
				++ptr;
				char next = *ptr;
				return next == '\n' || next == '\r\n';
			}
			return false;
		}
	);
	sourceCode.erase(it, sourceCode.end());*/

	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(compileInfo.sourceCode.c_str(), shaderKind, shaderName, options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		if (error)
		{
			(*error).append(fmt::format("\n\n{}", result.GetErrorMessage()));
		}
		return false;
	}

	shaderInfo.type			= compileInfo.type;
	shaderInfo.filename		= compileInfo.filename;
	shaderInfo.entryPoint	= compileInfo.entryPoint;

	shaderInfo.binaries.assign(result.begin(), result.end());

	spv_reflect::ShaderModule shaderModule{ shaderInfo.binaries };

	/**
	* TODO(afiq):
	* reflect descriptors, uniforms, storage buffers and etc.
	*/
	shaderInfo.attributes = glsl_shaderc_compiler::reflect_spirv_shader_attributes(shaderModule);

	return true;
}

}

}