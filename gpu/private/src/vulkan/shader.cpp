#include "vulkan/vkgpu.hpp"

namespace gpu
{
Shader::Shader(Device& device) :
	DeviceResource{ device },
	m_info{}
{}

auto Shader::info() const -> ShaderInfo const&
{
	return m_info;
}

auto Shader::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto Shader::from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> Resource<Shader>
{
	if (compiledShaderInfo.binaries.empty())
	{
		return null_resource;
	}

	auto&& vkdevice = to_device(device);

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

	VkResult result = vkCreateShaderModule(vkdevice.device, &shaderInfo, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		return null_resource;
	}

	auto&& [id, vkshader] = vkdevice.gpuResourcePool.shaders.emplace(vkdevice);

	vkshader.handle = handle;
	vkshader.stage = vk::translate_shader_stage(compiledShaderInfo.type);

	using shader_index_t = std::underlying_type_t<ShaderType>;

	vkshader.m_info.name.format("<shader:{}>:{}", shaderTypes[static_cast<shader_index_t>(compiledShaderInfo.type)].data(), compiledShaderInfo.path);

	vkshader.m_info.type = compiledShaderInfo.type;
	vkshader.m_info.entryPoint = compiledShaderInfo.entryPoint;

	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		vkdevice.setup_debug_name(vkshader);
	}

	return Resource<Shader>{ id.to_uint64(), vkshader };
}

auto Shader::destroy(Shader& resource, uint64 id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(resource.m_device);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, id, vk::ResourceType::Shader);
}

namespace vk
{
ShaderImpl::ShaderImpl(DeviceImpl& device) :
	Shader{ device }
{}
}
}