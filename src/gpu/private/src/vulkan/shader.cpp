#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Shader::info() const -> ShaderInfo const&
{
	return __self().info;
}

auto Shader::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

Shader::operator bool() const
{
	return valid();
}

auto Shader::from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> Shader
{
	if (compiledShaderInfo.binaries.empty())
	{
		return {};
	}

	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	static constexpr std::string_view shaderTypes[] = {
		"vertex",
		"pixel",
		"geometry",
		"tesselation_control",
		"tesselation_evaluation",
		"compute"
	};

	VkShaderModuleCreateInfo shaderInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = compiledShaderInfo.binaries.size_bytes(),
		.pCode = compiledShaderInfo.binaries.data()
	};

	VkShaderModule handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateShaderModule(vkdevice.device, &shaderInfo, nullptr, &handle))

	auto&& vkshader = *vkdevice.gpuResourcePool.stores.shaders.emplace();

	vkshader.handle = handle;
	vkshader.stage = translate_shader_stage(compiledShaderInfo.type);
	vkshader.info.type = compiledShaderInfo.type;
	vkshader.info.entryPoint = std::string{ compiledShaderInfo.entryPoint };

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkshader);
	}

	return Shader{ &vkshader, &vkdevice };
}

auto Shader::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	auto&& device = static_cast<DeviceImpl&>(dvc);
	auto&& shader = static_cast<ShaderImpl&>(resource);
	
	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&shader](DeviceImpl& device) -> void
		{
			vkDestroyShaderModule(device.device, shader.handle, nullptr);

			auto it = device.gpuResourcePool.stores.shaders.get_iterator(&shader);
			device.gpuResourcePool.stores.shaders.erase(it);
		}
	);
}
}