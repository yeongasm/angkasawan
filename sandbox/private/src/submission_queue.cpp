#include "submission_queue.h"

namespace sandbox
{
SubmissionQueue::SubmissionGroup::SubmissionGroup(SubmissionQueue::Data& data) :
	m_data{ data }
{}

auto SubmissionQueue::SubmissionGroup::submit_command_buffer(rhi::CommandBuffer& commandBuffer) -> void
{
	if (!commandBuffer.valid() ||
		commandBuffer.current_state() != rhi::CommandBuffer::State::Executable)
	{
		return;
	}
	ASSERTION(
		m_data.numCommandBuffers < Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP
		&& "Submitting command buffer will overflow beyond the capacity allocated for this submission group."
	);
	m_data.commandBuffers[m_data.numCommandBuffers++] = &commandBuffer;
}

auto SubmissionQueue::SubmissionGroup::signal_fence(rhi::Fence const& fence, uint64 value) -> void
{
	if (!fence.valid())
	{
		return;
	}
	ASSERTION(
		m_data.numSignalFences < Queue::HALF_FENCE_SUBMISSION_COUNT
		&& "Submitting fence for signalling will overflow beyond the capacity allocated for this submission group."
	);
	m_data.signalFences[m_data.numSignalFences++] = std::pair{ &fence, value };
}

auto SubmissionQueue::SubmissionGroup::wait_on_fence(rhi::Fence const& fence, uint64 value) -> void
{
	if (!fence.valid())
	{
		return;
	}
	ASSERTION(
		m_data.numWaitFences < Queue::HALF_FENCE_SUBMISSION_COUNT
		&& "Submitting fence for wait will overflow beyond the capacity allocated for this submission group."
	);
	m_data.waitFences[m_data.numWaitFences++] = std::pair{ &fence, value };
}

auto SubmissionQueue::SubmissionGroup::signal_semaphore(rhi::Semaphore const& semaphore) -> void
{
	if (!semaphore.valid())
	{
		return;
	}
	ASSERTION(
		m_data.numSignalSemaphores < Queue::HALF_SEMAPHORE_SUBMISSION_COUNT
		&& "Submitting semaphore for signalling will overflow beyond the capacity allocated for this submission group."
	);
	m_data.signalSemaphores[m_data.numSignalSemaphores++] = &semaphore;
}

auto SubmissionQueue::SubmissionGroup::wait_on_semaphore(rhi::Semaphore const& semaphore) -> void
{
	if (!semaphore.valid())
	{
		return;
	}
	ASSERTION(
		m_data.numWaitSemaphores < Queue::HALF_SEMAPHORE_SUBMISSION_COUNT
		&& "Submitting semaphore for wait will overflow beyond the capacity allocated for this submission group."
	);
	m_data.waitSemaphores[m_data.numWaitSemaphores++] = &semaphore;
}

auto SubmissionQueue::SubmissionGroup::limits() const -> Limits
{
	return Limits{
		.signalFenceCount = Queue::HALF_FENCE_SUBMISSION_COUNT,
		.waitFenceCount = Queue::HALF_FENCE_SUBMISSION_COUNT,
		.signalSemaphoreCount = Queue::HALF_SEMAPHORE_SUBMISSION_COUNT,
		.waitSemaphoreCount = Queue::HALF_SEMAPHORE_SUBMISSION_COUNT,
		.commandBufferCount = Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP
	};
}

SubmissionQueue::SubmissionQueue(rhi::Device& device) :
	m_device{ device },
	m_mainQueueSubmissions{},
	m_transferQueueSubmissions{}
{
	m_mainQueueSubmissions = lib::make_unique<Queue>();
	m_mainQueueSubmissions->type = rhi::DeviceQueueType::Main;
	setup_queue(*m_mainQueueSubmissions);

	m_transferQueueSubmissions = lib::make_unique<Queue>();
	m_transferQueueSubmissions->type = rhi::DeviceQueueType::Transfer;
	setup_queue(*m_transferQueueSubmissions);
}

auto SubmissionQueue::new_submission_group(rhi::DeviceQueueType queueType) -> SubmissionGroup
{
	Queue& queue = get_queue(queueType);
	/**
	* NOTE(afiq):
	* Doing this will require us to sync resources between queues which ideally should be done as minimally as possible.
	* Need to figure out a solution that does not involve submissions when the maximum capacity for the submissionGroup is reached.
	* lib::paged_array maybe?
	*/
	if (queue.numSubmissionGroups >= Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP)
	{
		if (queue.type == rhi::DeviceQueueType::Main)
		{
			send_to_gpu(*m_mainQueueSubmissions);
			clear(*m_mainQueueSubmissions);
		}
		else if (queue.type == rhi::DeviceQueueType::Transfer)
		{
			send_to_gpu(*m_transferQueueSubmissions);
			clear(*m_transferQueueSubmissions);
		}
	}
	Data& data = queue.submissionGroups[queue.numSubmissionGroups++];

	return SubmissionGroup{ data };
}

auto SubmissionQueue::clear() -> void
{
	clear_main_submissions();
	clear_transfer_submissions();
}

auto SubmissionQueue::clear_transfer_submissions() -> void
{
	clear(*m_transferQueueSubmissions);
}

auto SubmissionQueue::clear_main_submissions() -> void
{
	clear(*m_mainQueueSubmissions);
}

auto SubmissionQueue::send_to_gpu() -> void
{
	send_to_gpu_transfer_submissions();
	send_to_gpu_main_submissions();
}

auto SubmissionQueue::send_to_gpu_transfer_submissions() -> void
{
	send_to_gpu(*m_transferQueueSubmissions);
}

auto SubmissionQueue::send_to_gpu_main_submissions() -> void
{
	send_to_gpu(*m_mainQueueSubmissions);
}

auto SubmissionQueue::device() -> rhi::Device&
{
	return m_device;
}

auto SubmissionQueue::get_queue(rhi::DeviceQueueType type) -> Queue&
{
	Queue* pQueue = m_mainQueueSubmissions.get();

	if (type == rhi::DeviceQueueType::Transfer)
	{
		pQueue = m_transferQueueSubmissions.get();
	}
	else if (type == rhi::DeviceQueueType::Compute)
	{
		// TODO(afiq): ...
	}
	return *pQueue;
}

auto SubmissionQueue::send_to_gpu(Queue& queue) -> void
{
	for (uint32 i = 0; i < queue.numSubmissionGroups; ++i)
	{
		Data& data = queue.submissionGroups[i];

		if (!data.numCommandBuffers)
		{
			continue;
		}
		rhi::SubmitInfo info{
			.queue = queue.type,
			.commandBuffers = std::span{ data.commandBuffers.front(), data.numCommandBuffers },
			.waitSemaphores = std::span{ data.waitSemaphores.front(), data.numWaitSemaphores },
			.signalSemaphores = std::span{ data.signalSemaphores.front(), data.numSignalSemaphores },
			.waitFences = std::span{ data.waitFences.data(), data.numWaitFences },
			.signalFences = std::span{ data.signalFences.data(), data.numSignalFences }
		};
		m_device.submit(info);
	}
	clear(queue);
}

auto SubmissionQueue::clear(Queue& queue) -> void
{
	for (uint32 i = 0; i < queue.numSubmissionGroups; ++i)
	{
		Data& data = queue.submissionGroups[i];

		data.numSignalFences = 0;
		data.numWaitFences = 0;
		data.numSignalSemaphores = 0;
		data.numWaitSemaphores = 0;
		data.numCommandBuffers = 0;
	}
	queue.numSubmissionGroups = 0;
}

auto SubmissionQueue::setup_queue(Queue& queue) -> void
{
	for (uint32 i = 0; i < Queue::MAX_SUBMISSION_GROUPS; ++i)
	{
		uint32 const fenceOffset = (Queue::NUM_FENCE_SUBMISSION_PER_GROUP - 1u) * i;
		uint32 const semaphoreOffset = (Queue::NUM_SEMAPHORE_SUBMISSION_PER_GROUP - 1u) * i;
		uint32 const commandBufferOffset = (Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP - 1u) * i;

		Data& data = queue.submissionGroups[i];

		new (&data) Data{};

		data.signalFences = std::span{ &queue.submittedFences[fenceOffset], Queue::HALF_FENCE_SUBMISSION_COUNT };
		data.waitFences = std::span{ &queue.submittedFences[fenceOffset + Queue::HALF_FENCE_SUBMISSION_COUNT], Queue::HALF_FENCE_SUBMISSION_COUNT };
		data.signalSemaphores = std::span{ &queue.submittedSemaphores[semaphoreOffset], Queue::HALF_SEMAPHORE_SUBMISSION_COUNT };
		data.waitSemaphores = std::span{ &queue.submittedSemaphores[semaphoreOffset + Queue::HALF_SEMAPHORE_SUBMISSION_COUNT], Queue::HALF_SEMAPHORE_SUBMISSION_COUNT };
		data.commandBuffers = std::span{ &queue.submittedCommandBuffers[commandBufferOffset], Queue::NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP };
	}
}

}