#include "vulkan/vkgpu.hpp"

namespace gpu
{
CommandPool::CommandPool(Device& device) :
	DeviceResource{ device },
	m_info{}
{}

auto CommandPool::info() const -> CommandPoolInfo const&
{
	return m_info;
}

auto CommandPool::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto CommandPool::reset() const -> void
{
	auto const& self = to_impl(*this);
	auto const& vkdevice = to_device(m_device);

	if (valid())
	{
		return;
	}

	vkResetCommandPool(vkdevice.device, self.handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

auto CommandPool::from(Device& device, CommandPoolInfo&& info) -> Resource<CommandPool>
{
	auto&& vkdevice = to_device(device);

	vk::Queue* pQueue = nullptr;

	switch (info.queue)
	{
	case DeviceQueue::Transfer:
		pQueue = &vkdevice.transferQueue;
		break;
	case DeviceQueue::Compute:
		pQueue = &vkdevice.computeQueue;
		break;
	case DeviceQueue::Main:
	default:
		pQueue = &vkdevice.mainQueue;
		break;
	}

	VkCommandPoolCreateInfo commandPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = pQueue->familyIndex
	};

	VkCommandPool handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateCommandPool(vkdevice.device, &commandPoolInfo, nullptr, &handle))

	auto&& [id, vkcommandpool] = vkdevice.gpuResourcePool.commandPools.emplace(std::make_unique<vk::CommandPoolImpl>(vkdevice));

	if (!info.name.empty())
	{
		info.name.format("<command_pool>:{}", info.name.c_str());
	}

	vkcommandpool->handle = handle;
	vkcommandpool->m_info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(*vkcommandpool);
	}

	return Resource<CommandPool>{ id.to_uint64(), *vkcommandpool.get() };
}

auto CommandPool::destroy(CommandPool& resource, uint64 id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(resource.m_device);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, id, vk::ResourceType::Command_Pool);
}

namespace vk
{
CommandPoolImpl::CommandPoolImpl(DeviceImpl& device) :
	CommandPool{ device }
{}
}
}