#include <fmt/std.h>
#include "command_queue.h"

namespace sandbox
{
CommandQueue::CommandQueue(rhi::Device& device, SubmissionQueue& submissionQueue, rhi::DeviceQueueType type) :
	m_device{ device },
	m_submissionQueue{ submissionQueue },
	m_type{ type },
	m_commandPool{},
	m_commandStore{}
{}

auto CommandQueue::next_free_command_buffer(std::thread::id tid) -> lib::ref<rhi::CommandBuffer>
{
	auto queue_type_name = [](rhi::DeviceQueueType type) -> const char*
	{
		switch (type)
		{
		case rhi::DeviceQueueType::Main:
			return "main";
		case rhi::DeviceQueueType::Transfer:
			return "transfer";
		case rhi::DeviceQueueType::Compute:
			return "compute";
		default:
			return "none";
		}
	};

	auto queueID = queue_type_name(m_type);

	if (!m_commandPool.contains(tid))
	{
		lib::string name = lib::format("<tid:{}>:{}_cmd_pool", tid, queueID);
		auto pool = m_device.create_command_pool({ .name = std::move(name), .queue = m_type });
		m_commandPool.emplace(tid, std::move(pool));
		m_commandStore.emplace(tid, CommandBufferStore{});
	}

	rhi::CommandPool& pool = m_commandPool[tid];
	if (!pool.valid())
	{
		return lib::ref<rhi::CommandBuffer>{};
	}

	uint32 index = std::numeric_limits<uint32>::max();
	CommandBufferStore& cmdStore = m_commandStore[tid];

	// Find a free command buffer.
	for (uint32 i = cmdStore.index; i < cmdStore.count; ++i)
	{
		rhi::CommandBuffer* pCmdBuffer = cmdStore.commandBuffers[i];
		if (pCmdBuffer &&
			pCmdBuffer->is_completed())
		{
			index = i;
			break;
		}
	}

	// If none exist, we allocate one.
	if (index == std::numeric_limits<uint32>::max() &&
		cmdStore.count < rhi::MAX_COMMAND_BUFFER_PER_POOL)
	{
		lib::string name = lib::format("<tid:{}>:{}_cmd_buffer_{}", tid, queueID, cmdStore.count);
		rhi::CommandBuffer& cmdBuffer = pool.allocate_command_buffer({ .name = std::move(name) });

		index = cmdStore.count;
		cmdStore.commandBuffers[cmdStore.count++] = &cmdBuffer;
	}
	
	// If we still couldn't get one, return a null reference. Ideally, this should be taken care of.
	// Maybe allocate on the heap?
	if (index == std::numeric_limits<uint32>::max())
	{
		return lib::ref<rhi::CommandBuffer>{};
	}

	cmdStore.index = (cmdStore.index + 1) % cmdStore.count;

	return lib::ref<rhi::CommandBuffer>{ cmdStore.commandBuffers[index] };
}

auto CommandQueue::terminate() -> void
{
	// Ideally, m_device.wait_idle() should be called here but because the device queues resources into the zombie pool, there's no need.

	for (auto&& [tid, pool] : m_commandPool)
	{
		CommandBufferStore& cmdStore = m_commandStore[tid];
		for (uint32 i = 0; i < cmdStore.count; ++i)
		{
			rhi::CommandBuffer& cmdBuffer = *cmdStore.commandBuffers[i];
			pool.free_command_buffer(cmdBuffer);
		}
		m_device.destroy_command_pool(pool);
	}

	m_commandStore.clear();
	m_commandPool.clear();
}
}