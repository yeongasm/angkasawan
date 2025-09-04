#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Shader::info() const -> ShaderInfo const&
{
	return m_info;
}

auto Shader::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
}

auto Shader::from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> handle_type
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

	auto it = vkdevice.gpuResourcePool.stores.shaders.emplace();

	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::ShaderImpl>,
		.id 	= ++vkdevice.gpuResourcePool.idCounter.others
	};

	auto id = std::bit_cast<uint64>(meta);

	vkdevice.gpuResourcePool.caches.shader.emplace(id, it);
	vkdevice.begin_referencing(id);

	auto&& vkshader = *it;

	vkshader.handle = handle;
	vkshader.stage = vk::translate_shader_stage(compiledShaderInfo.type);
	vkshader.m_info.type = compiledShaderInfo.type;
	vkshader.m_info.entryPoint = compiledShaderInfo.entryPoint;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkshader);
	}

	return { vkdevice, id };
}

auto Shader::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a Shader with an invalid id");
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(device);
	auto&& vkshader = *vkdevice.gpuResourcePool.caches.shader[id];

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkshader, id](vk::DeviceImpl& device) -> void
		{
			vkDestroyShaderModule(device.device, vkshader.handle, nullptr);

			auto it = device.gpuResourcePool.caches.shader[id];

			device.gpuResourcePool.caches.shader.erase(id);
			device.gpuResourcePool.stores.shaders.erase(it);
		}
	);
}
}