module;

#include <mutex>
#include "vulkan/vk.h"

module forge;

namespace frg
{
auto CommandPool::info() const -> CommandPoolInfo const&
{
	return m_info;
}

auto CommandPool::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto CommandPool::reset() const -> void
{
	if (m_device == nullptr || m_impl.handle == VK_NULL_HANDLE) [[unlikely]]
	{
		return;
	}
	vkResetCommandPool(m_device->m_apiContext.device, m_impl.handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

auto CommandPool::from(Device& device, CommandPoolInfo&& info) -> Resource<CommandPool>
{
	auto&& [index, commandPool] = device.m_gpuResourcePool.commandPools.emplace(device);

	api::Queue* pQueue = nullptr;

	switch (info.queue)
	{
	case DeviceQueue::Transfer:
		pQueue = &device.m_apiContext.transferQueue;
		break;
	case DeviceQueue::Compute:
		pQueue = &device.m_apiContext.computeQueue;
		break;
	case DeviceQueue::Main:
	default:
		pQueue = &device.m_apiContext.mainQueue;
		break;
	}

	VkCommandPoolCreateInfo commandPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = pQueue->familyIndex 
	};

	VkResult result = vkCreateCommandPool(device.m_apiContext.device, &commandPoolInfo, nullptr, &commandPool.m_impl.handle);
	
	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.commandPools.erase(index);

		return null_resource;
	}

	info.name.format("<command_pool>:{}", info.name.c_str());

	commandPool.m_info = std::move(info);

	return Resource{ index, commandPool };
}

auto CommandPool::destroy(CommandPool const& resource, id_type id) -> void
{
	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<CommandPool> });
}

CommandPool::CommandPool(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{}
{}
}