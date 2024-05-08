module;

#include <mutex>
#include "vulkan/vk.h"

module forge;

namespace frg
{
auto Shader::info() const -> ShaderInfo const&
{
	return m_info;
}

auto Shader::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto Shader::from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> Resource<Shader>
{
	if (!compiledShaderInfo.binaries.size())
	{
		return null_resource;
	}

	static constexpr std::string_view shaderTypes[] = {
		"vertex",
		"pixel",
		"geometry",
		"tesselation_control",
		"tesselation_evaluation",
		"compute"
	};

	auto&& [index, shader] = device.m_gpuResourcePool.shaders.emplace(device);

	VkShaderModuleCreateInfo shaderInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = compiledShaderInfo.binaries.size_bytes(),
		.pCode = compiledShaderInfo.binaries.data()
	};

	VkResult result = vkCreateShaderModule(device.m_apiContext.device, &shaderInfo, nullptr, &shader.m_impl.handle);

	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.shaders.erase(index);

		return null_resource;
	}

	shader.m_impl.stage = api::translate_shader_stage(compiledShaderInfo.type);

	using shader_index_t = std::underlying_type_t<frg::ShaderType>;

	shader.m_info.name.format("<shader:{}>:{}", shaderTypes[static_cast<shader_index_t>(compiledShaderInfo.type)].data(), compiledShaderInfo.path);

	shader.m_info.type			= compiledShaderInfo.type;
	shader.m_info.entryPoint	= compiledShaderInfo.entryPoint;

	return Resource{ index, shader };
}

auto Shader::destroy(Shader const& resource, id_type id) -> void
{
	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<Shader> });
}

Shader::Shader(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{}
{}

}