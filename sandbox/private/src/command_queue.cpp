#include <fmt/std.h>
#include "command_queue.h"

namespace sandbox
{
SubmissionGroup::SubmissionGroup(Queue& queue, Queue::SubmissionData& data) :
	m_queue{ queue },
	m_data{ data }
{}

auto SubmissionGroup::submit(gpu::command_buffer const& commandBuffer) -> void
{
	if (!commandBuffer.valid())
	{
		return;
	}

	ASSERTION(
		m_data.numCommandBuffers < Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP
		&& "Submitting command buffer will overflow beyond the capacity allocated for this submission group."
	);

	const uint32 offset = (Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP - 1u) * m_data.id;

	m_queue.submittedCommandBuffers[offset + m_data.numCommandBuffers++] = commandBuffer;
}

auto SubmissionGroup::signal(gpu::fence const& fence, uint64 value) -> void
{
	if (!fence.valid())
	{
		return;
	}

	ASSERTION(
		m_data.numSignalFences < Queue::HALF_FENCE_SUBMISSION_COUNT
		&& "Submitting fence for signalling will overflow beyond the capacity allocated for this submission group."
	);

	const uint32 offset = (Queue::NUM_FENCE_SUBMISSION_PER_GROUP - 1u) * m_data.id;

	m_queue.submittedFences[offset + m_data.numSignalFences++] = std::pair{ fence, value };
}

auto SubmissionGroup::signal(gpu::semaphore const& semaphore) -> void
{
	if (!semaphore.valid())
	{
		return;
	}

	ASSERTION(
		m_data.numSignalSemaphores < Queue::HALF_SEMAPHORE_SUBMISSION_COUNT
		&& "Submitting semaphore for signalling will overflow beyond the capacity allocated for this submission group."
	);

	const uint32 offset = (Queue::NUM_SEMAPHORE_SUBMISSION_PER_GROUP - 1u) * m_data.id;

	m_queue.submittedSemaphores[offset + m_data.numSignalSemaphores++] = semaphore;
}

auto SubmissionGroup::wait(gpu::fence const& fence, uint64 value) -> void
{
	if (!fence.valid())
	{
		return;
	}

	ASSERTION(
		m_data.numWaitFences < Queue::HALF_FENCE_SUBMISSION_COUNT
		&& "Submitting fence for wait will overflow beyond the capacity allocated for this submission group."
	);

	const uint32 offset = (Queue::NUM_FENCE_SUBMISSION_PER_GROUP - 1u) * m_data.id + Queue::HALF_FENCE_SUBMISSION_COUNT;

	m_queue.submittedFences[offset + m_data.numWaitFences++] = std::pair{ fence, value };
}

auto SubmissionGroup::wait(gpu::semaphore const& semaphore) -> void
{
	if (!semaphore.valid())
	{
		return;
	}

	ASSERTION(
		m_data.numWaitSemaphores < Queue::HALF_SEMAPHORE_SUBMISSION_COUNT
		&& "Submitting semaphore for wait will overflow beyond the capacity allocated for this submission group."
	);

	const uint32 offset = (Queue::NUM_SEMAPHORE_SUBMISSION_PER_GROUP - 1u) * m_data.id + Queue::HALF_SEMAPHORE_SUBMISSION_COUNT;

	m_queue.submittedSemaphores[offset + m_data.numWaitSemaphores++] = semaphore;
}

CommandQueue::CommandQueue(gpu::Device& device) :
	m_mainQueue{ std::make_unique<Queue>() },
	m_transferQueue{ std::make_unique<Queue>() },
	m_device{ device }
{}

auto CommandQueue::new_submission_group(gpu::DeviceQueue deviceQueue) -> SubmissionGroup
{
	Queue& queue = get_queue(deviceQueue);

	if (queue.numSubmissionGroups >= Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP)
	{
		send_to_gpu(queue, deviceQueue);
	}

	uint32 const id = queue.numSubmissionGroups++;
	Queue::SubmissionData& data = queue.submissionGroupData[id];

	data.id = id;

	return SubmissionGroup{ queue, data };
}

auto CommandQueue::next_free_command_buffer(RequestCommandBufferInfo&& info) -> gpu::command_buffer
{
	Queue& queue = get_queue(info.queue);

	if (!queue.commandPoolStore.contains(info.tid))
	{
		auto queue_type_name = [](gpu::DeviceQueue type) -> const char*
		{
			switch (type)
			{
			case gpu::DeviceQueue::Transfer:
				return "transfer";
			case gpu::DeviceQueue::Compute:
				return "compute";
			case gpu::DeviceQueue::Main:
			default:
				return "main";
			}
		};

		auto commandPool = gpu::CommandPool::from(m_device, { .name = lib::format("type={}, tid={}", queue_type_name(info.queue), info.tid), .queue = info.queue });

		if (!commandPool.valid())
		{
			return gpu::null_resource;
		}

		queue.commandPoolStore.emplace(info.tid, std::move(commandPool));
	}

	auto& commandPool = queue.commandPoolStore[info.tid];

	return gpu::CommandBuffer::from(commandPool);
}

auto CommandQueue::clear(gpu::DeviceQueue queue) -> void
{
	clear(get_queue(queue));
}

auto CommandQueue::send_to_gpu(gpu::DeviceQueue queue) -> void
{
	send_to_gpu(get_queue(queue), queue);
}

auto CommandQueue::get_queue(gpu::DeviceQueue type) -> Queue&
{
	switch (type)
	{
	case gpu::DeviceQueue::Transfer:
		return *(m_transferQueue.get());
	case gpu::DeviceQueue::Main:
	default:
		return *(m_mainQueue.get());
	}
}

auto CommandQueue::send_to_gpu(Queue& queue, gpu::DeviceQueue type) -> void
{
	for (uint32 i = 0; i < queue.numSubmissionGroups; ++i)
	{
		Queue::SubmissionData& data = queue.submissionGroupData[i];

		if (!data.numCommandBuffers)
		{
			continue;
		}

		uint32 const commandBufferOffset	= (Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP - 1u) * data.id;
		uint32 const waitSemaphoreOffset	= (Queue::NUM_SEMAPHORE_SUBMISSION_PER_GROUP - 1u) * data.id + Queue::HALF_SEMAPHORE_SUBMISSION_COUNT;;
		uint32 const signalSemaphoreOffset	= (Queue::NUM_SEMAPHORE_SUBMISSION_PER_GROUP - 1u) * data.id;
		uint32 const waitFenceOffset		= (Queue::NUM_FENCE_SUBMISSION_PER_GROUP - 1u) * data.id + Queue::HALF_FENCE_SUBMISSION_COUNT;
		uint32 const signalFenceOffset		= (Queue::NUM_FENCE_SUBMISSION_PER_GROUP - 1u) * data.id;

		gpu::SubmitInfo info{
			.queue				= type,
			.commandBuffers		= std::span{ &queue.submittedCommandBuffers[commandBufferOffset],	data.numCommandBuffers },
			.waitSemaphores		= std::span{ &queue.submittedSemaphores[waitSemaphoreOffset],		data.numWaitSemaphores },
			.signalSemaphores	= std::span{ &queue.submittedSemaphores[signalSemaphoreOffset],		data.numSignalSemaphores },
			.waitFences			= std::span{ &queue.submittedFences[waitFenceOffset],				data.numWaitFences },
			.signalFences		= std::span{ &queue.submittedFences[signalFenceOffset],				data.numSignalFences }
		};

		m_device.submit(info);
	}

	clear(queue);
}

auto CommandQueue::clear(Queue& queue) -> void
{
	for (uint32 i = 0; i < queue.numSubmissionGroups; ++i)
	{
		Queue::SubmissionData& data = queue.submissionGroupData[i];

		uint32 const commandBufferOffset	= (Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP - 1u) * data.id;
		uint32 const waitSemaphoreOffset	= (Queue::NUM_SEMAPHORE_SUBMISSION_PER_GROUP - 1u) * data.id + Queue::HALF_SEMAPHORE_SUBMISSION_COUNT;;
		uint32 const signalSemaphoreOffset	= (Queue::NUM_SEMAPHORE_SUBMISSION_PER_GROUP - 1u) * data.id;
		uint32 const waitFenceOffset		= (Queue::NUM_FENCE_SUBMISSION_PER_GROUP - 1u) * data.id + Queue::HALF_FENCE_SUBMISSION_COUNT;
		uint32 const signalFenceOffset		= (Queue::NUM_FENCE_SUBMISSION_PER_GROUP - 1u) * data.id;

		for (uint32 j = 0; j < data.numCommandBuffers; ++j)
		{
			queue.submittedCommandBuffers[commandBufferOffset + j].destroy();
		}

		for (uint32 j = 0; j < data.numWaitFences; ++j)
		{
			queue.submittedFences[waitFenceOffset + j].first.destroy();
		}

		for (uint32 j = 0; j < data.numWaitSemaphores; ++j)
		{
			queue.submittedSemaphores[waitSemaphoreOffset + j].destroy();
		}

		for (uint32 j = 0; j < data.numSignalFences; ++j)
		{
			queue.submittedFences[signalFenceOffset + j].first.destroy();
		}

		for (uint32 j = 0; j < data.numSignalSemaphores; ++j)
		{
			queue.submittedSemaphores[signalSemaphoreOffset + j].destroy();
		}

		data.id = 0;
		data.numWaitFences = 0;
		data.numSignalFences = 0;
		data.numSignalSemaphores = 0;
		data.numWaitSemaphores = 0;
		data.numCommandBuffers = 0;
	}
	queue.numSubmissionGroups = 0;
}
}