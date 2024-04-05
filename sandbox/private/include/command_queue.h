#pragma once
#ifndef SANDBOX_COMMAND_QUEUE_H
#define SANDBOX_COMMAND_QUEUE_H

#include <thread>
#include "submission_queue.h"

namespace sandbox
{
class CommandQueue
{
public:
	CommandQueue(SubmissionQueue& submissionQueue, rhi::DeviceQueue type);
	~CommandQueue() = default;

	auto next_free_command_buffer(std::thread::id tid) -> lib::ref<rhi::CommandBuffer>;
	auto terminate() -> void;
	auto new_submission_group() -> SubmissionQueue::SubmissionGroup;
	auto submission_queue() -> SubmissionQueue&;
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
	SubmissionQueue& m_submissionQueue;
	rhi::DeviceQueue m_type;
	lib::map<std::thread::id, rhi::CommandPool> m_commandPool;
	lib::map<std::thread::id, CommandBufferStore> m_commandStore;
};
}

#endif // !SANDBOX_COMMAND_QUEUE_H

