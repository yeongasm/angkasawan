#include "constants.hpp"
#include "vulkan/vkgpu.hpp"

namespace gpu
{
namespace vk
{
CommandPoolImpl::CommandPoolImpl(DeviceImpl& device) :
	vkdevice{ &device }
{}

auto CommandPoolImpl::allocate_new_command_buffer() -> uint64
{
	auto index = commandBufferPool.commandBuffers.size();
	auto&& vkcmdbuffer = commandBufferPool.commandBuffers.emplace_back();

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = handle,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer handle = VK_NULL_HANDLE;

	CHECK_OP(vkAllocateCommandBuffers(vkdevice->device, &allocateInfo, &handle))

	vkcmdbuffer.handle = handle;
	vkcmdbuffer.gpuTimeline = Fence::from(*vkdevice, {
		.name = fmt::format("<fence>:{}:cmd_buffer_{}", m_info.name, index)
	});

	return index;
}
}

auto CommandPool::info() const -> CommandPoolInfo const&
{
	return m_info;
}

auto CommandPool::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
}

auto CommandPool::reset() const -> void
{
	auto const& self = to_impl(*this);

	if (valid())
	{
		return;
	}

	vkResetCommandPool(self.vkdevice->device, self.handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

auto CommandPool::from(Device& device, CommandPoolInfo&& info) -> handle_type
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

	auto it = vkdevice.gpuResourcePool.stores.commandPools.emplace(vkdevice);

	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::CommandPoolImpl>,
		.id 	= ++vkdevice.gpuResourcePool.idCounter.others
	};

	auto id = std::bit_cast<uint64>(meta);

	vkdevice.gpuResourcePool.caches.commandPool.emplace(id, it);
	vkdevice.begin_referencing(id);

	auto&& vkcommandpool = *it;

	vkcommandpool.handle = handle;
	vkcommandpool.m_info = std::move(info);

	vkcommandpool.commandBufferPool.commandBuffers.reserve(MAX_COMMAND_BUFFER_PER_POOL);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkcommandpool);
	}

	return { vkdevice, id };
}

auto CommandPool::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a CommandPool with an invalid id");
	/*
	* At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	*/
	auto&& vkdevice = to_device(device);
	auto&& vkcommandpool = *vkdevice.gpuResourcePool.caches.commandPool[id];

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkcommandpool, id](vk::DeviceImpl& device) -> void
		{
			lib::array<VkCommandBuffer> cmdBuffers{ vkcommandpool.commandBufferPool.commandBuffers.size() };
			
			for (auto const& cmdBuffer : vkcommandpool.commandBufferPool.commandBuffers)
			{
				cmdBuffers.push_back(cmdBuffer.handle);
			}

			vkFreeCommandBuffers(device.device, vkcommandpool.handle, cmdBuffers.size(), cmdBuffers.data());
			vkDestroyCommandPool(device.device, vkcommandpool.handle, nullptr);

			auto it = device.gpuResourcePool.caches.commandPool[id];

			device.gpuResourcePool.caches.commandPool.erase(id);
			device.gpuResourcePool.stores.commandPools.erase(it);		
		}
	);
}
}