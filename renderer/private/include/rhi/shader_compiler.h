#pragma once
#ifndef RENDERER_RHI_VULKAN_VK_SHADER_COMPILER_H
#define RENDERER_RHI_VULKAN_VK_SHADER_COMPILER_H

#include "rhi.h"

namespace rhi
{

namespace shader_compiler
{

bool compile_glsl_to_spirv(ShaderCompileInfo const& compileInfo, ShaderInfo& shaderInfo, std::string* error);

}

}

#endif // !RENDERER_RHI_VULKAN_VK_SHADER_COMPILER_H
