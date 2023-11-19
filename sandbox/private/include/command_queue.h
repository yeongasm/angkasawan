#pragma once
#ifndef SANDBOX_COMMAND_QUEUE_H
#define SANDBOX_COMMAND_QUEUE_H

#include <thread>
#include "submission_queue.h"

namespace sandbox
{
class CommandQueue
{
	private:
	struct CommandBufferStore
	{
		using CommandBufferArr = std::array<rhi::CommandBuffer*, rhi::MAX_COMMAND_BUFFER_PER_POOL>;
		using CommandBufferStateArr = std::array<rhi::CommandBuffer::State, rhi::MAX_COMMAND_BUFFER_PER_POOL>;

		CommandBufferArr commandBuffers;
		CommandBufferStateArr states;
		uint32 count;
		uint32 index;
	};

	rhi::Device& m_device;
	SubmissionQueue& m_submissionQueue;
	rhi::DeviceQueueType m_type;
	lib::map<std::thread::id, rhi::CommandPool> m_commandPool;
	lib::map<std::thread::id, CommandBufferStore> m_commandStore;
public:
	CommandQueue(rhi::Device& device, SubmissionQueue& submissionQueue, rhi::DeviceQueueType type);
	~CommandQueue() = default;

	auto next_free_command_buffer(std::thread::id tid) -> lib::ref<rhi::CommandBuffer>;
	auto terminate() -> void;
};
}

#endif // !SANDBOX_COMMAND_QUEUE_H

