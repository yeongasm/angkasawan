#pragma once
#ifndef RHI_RHI_H
#define RHI_RHI_H

#include "device.h"
#include "swapchain.h"
#include "buffer.h"
#include "image.h"
#include "shader.h"
#include "pipeline.h"

namespace rhi
{

using Instance = class _Instance_*;

RHI_API auto create_instance() -> Instance;
RHI_API auto destroy_instance() -> void;
RHI_API auto create_device(Instance instance, DeviceInitInfo const& info) -> Device&;
RHI_API auto destroy_device(Device& device) -> void;
RHI_API auto compile_shader(ShaderCompileInfo const& info, lib::string* error = nullptr) -> std::pair<bool, ShaderInfo>;

}

#endif // !RHI_RHI_H
