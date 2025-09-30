#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto CommandPool::info() const -> CommandPoolInfo const&
{
	return __self().info;
}

auto CommandPool::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto CommandPool::reset() const -> void
{
	if (valid())
	{
		return;
	}
	vkResetCommandPool(__device().device, __self().handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

auto CommandPool::from(Device& device, CommandPoolInfo&& info) -> CommandPool
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	Queue* pQueue = nullptr;

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

	auto&& vkcommandpool = *vkdevice.gpuResourcePool.stores.commandPools.emplace();

	vkcommandpool.handle = handle;
	vkcommandpool.info = std::move(info);

	vkcommandpool.commandBufferPool.commandBuffers.reserve(MAX_COMMAND_BUFFER_PER_POOL);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkcommandpool);
	}

	return CommandPool{ &vkcommandpool, &vkdevice };
}

auto CommandPool::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	DeviceImpl& device = static_cast<DeviceImpl&>(dvc);
	CommandPoolImpl& commandPool = static_cast<CommandPoolImpl&>(resource);

	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&commandPool](DeviceImpl& device) -> void
		{
			lib::array<VkCommandBuffer> cmdBuffers{ commandPool.commandBufferPool.commandBuffers.size() };
			
			for (auto const& cmdBuffer : commandPool.commandBufferPool.commandBuffers)
			{
				cmdBuffers.push_back(cmdBuffer.handle);
			}

			vkFreeCommandBuffers(device.device, commandPool.handle, static_cast<uint32>(cmdBuffers.size()), cmdBuffers.data());
			vkDestroyCommandPool(device.device, commandPool.handle, nullptr);

			auto it = device.gpuResourcePool.stores.commandPools.get_iterator(&commandPool);
			device.gpuResourcePool.stores.commandPools.erase(it);	
		}
	);
}
}