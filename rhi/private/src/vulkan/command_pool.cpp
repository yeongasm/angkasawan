#include "command_pool.h"
#include "vulkan/vk_device.h"

namespace rhi
{

CommandPool::CommandPool(
	CommandPoolInfo&& info,
	APIContext* context,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) },
	m_command_buffers{}
{
	m_command_buffers.emplace(INVALID_COMMAND_BUFFER_INDEX, CommandBuffer{});
	m_context->setup_debug_name(*this);
}

CommandPool::CommandPool(CommandPool&& rhs) noexcept
{
	*this = std::move(rhs);
}

auto CommandPool::operator=(CommandPool&& rhs) noexcept -> CommandPool&
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		m_command_buffers = std::move(rhs.m_command_buffers);
		Resource::operator=(std::move(rhs));
		new (&rhs) CommandPool{};
	}
	return *this;
}

auto CommandPool::info() const -> CommandPoolInfo const&
{
	return m_info;
}

auto CommandPool::reset() -> void
{
	VkCommandPool& handle = as<VkCommandPool>();
	vkResetCommandPool(m_context->device, handle, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
}

auto CommandPool::allocate_command_buffer(CommandBufferInfo&& info) -> CommandBuffer&
{
	vulkan::CommandPool& cmdPool = as<vulkan::CommandPool>();
	std::uintptr_t cmdPoolAddress = reinterpret_cast<std::uintptr_t>(&cmdPool);
	vulkan::CommandBufferPool& cmdBufferPool = m_context->gpuResourcePool.commandBufferPools[cmdPoolAddress];

	if (cmdBufferPool.commandBufferCount + 1 >= MAX_COMMAND_BUFFER_PER_POOL)
	{
		return m_command_buffers[INVALID_COMMAND_BUFFER_INDEX];
	}

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = cmdPool.handle,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer cmdBufferHandle = VK_NULL_HANDLE;
	VkResult result = vkAllocateCommandBuffers(m_context->device, &allocateInfo, &cmdBufferHandle);
	if (result != VK_SUCCESS)
	{
		return m_command_buffers[INVALID_COMMAND_BUFFER_INDEX];
	}
	
	size_t index = cmdBufferPool.commandBufferCount;
	if (cmdBufferPool.freeSlotCount)
	{
		index = cmdBufferPool.freeSlots[cmdBufferPool.currentFreeSlot];
		cmdBufferPool.currentFreeSlot = (cmdBufferPool.currentFreeSlot - 1) % MAX_COMMAND_BUFFER_PER_POOL;
		--cmdBufferPool.freeSlotCount;
	}
	else
	{
		++cmdBufferPool.commandBufferCount;
	}
	new (&cmdBufferPool.commandBuffers[index]) vulkan::CommandBuffer{ .handle = cmdBufferHandle };

	info.name.format("<command_buffer>:{}", m_info.name.c_str(), info.name.c_str());

	CommandBuffer cmdBuffer{
		std::move(info),
		m_context,
		&cmdBufferPool.commandBuffers[index],
		resource_type_id_v<vulkan::CommandBuffer>
	};
	auto& [key, value] = m_command_buffers.emplace(index, std::move(cmdBuffer));
	return value;
}

auto CommandPool::free_command_buffer(CommandBuffer& commandBuffer) -> void
{
	if (!commandBuffer.valid() ||
		commandBuffer.m_state != CommandBuffer::State::Initial)
	{
		return;
	}

	vulkan::CommandPool& cmdPool = as<vulkan::CommandPool>();
	std::uintptr_t cmdPoolAddress = reinterpret_cast<std::uintptr_t>(&cmdPool);
	vulkan::CommandBufferPool& cmdBufferPool = m_context->gpuResourcePool.commandBufferPools[cmdPoolAddress];
	vulkan::CommandBuffer& cmdBufferResource = commandBuffer.as<vulkan::CommandBuffer>();

	if (&cmdBufferResource >= cmdBufferPool.commandBuffers.data() && 
		&cmdBufferResource < cmdBufferPool.commandBuffers.data() + MAX_COMMAND_BUFFER_PER_POOL)
	{
		size_t index = static_cast<size_t>(&cmdBufferResource - cmdBufferPool.commandBuffers.data());
		m_context->gpuResourcePool.zombies.emplace_back(cmdPoolAddress, static_cast<uint32>(index), vulkan::Resource::CommandBuffer);
		m_command_buffers.erase(index);
	}
}

}