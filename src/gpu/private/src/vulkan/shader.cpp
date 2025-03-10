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
		return {};
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

	CHECK_OP(vkCreateShaderModule(vkdevice.device, &shaderInfo, nullptr, &handle))

	auto it = vkdevice.gpuResourcePool.shaders.emplace(vkdevice);

	auto&& vkshader = *it;

	vkshader.handle = handle;
	vkshader.stage = vk::translate_shader_stage(compiledShaderInfo.type);

	vkshader.m_info.name.format("<shader:{}>:{}", shaderTypes[std::to_underlying(compiledShaderInfo.type)].data(), compiledShaderInfo.name);

	vkshader.m_info.type = compiledShaderInfo.type;
	vkshader.m_info.entryPoint = compiledShaderInfo.entryPoint;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkshader);
	}

	return Resource<Shader>{ vkshader, vk::to_id(it) };
}

auto Shader::destroy(Shader& resource, Id id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(resource.m_device);
	auto&& vkshader = to_impl(resource);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkshader, id](vk::DeviceImpl& device) -> void
		{
			vkDestroyShaderModule(device.device, vkshader.handle, nullptr);

			using iterator = typename lib::hive<vk::ShaderImpl>::iterator;

			auto const it = vk::to_hive_it<iterator>(id);

			device.gpuResourcePool.shaders.erase(it);
		}
	);
}

namespace vk
{
ShaderImpl::ShaderImpl(DeviceImpl& device) :
	Shader{ device }
{}
}
}