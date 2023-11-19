#pragma once
#ifndef SANDBOX_SUBMISSION_QUEUE
#define SANDBOX_SUBMISSION_QUEUE

#include "rhi/rhi.h"

namespace sandbox
{
class SubmissionQueue
{
public:
	struct Data
	{
		std::span<std::pair<rhi::Fence const*, uint64>> signalFences;
		std::span<std::pair<rhi::Fence const*, uint64>> waitFences;
		std::span<rhi::Semaphore const*> signalSemaphores;
		std::span<rhi::Semaphore const*> waitSemaphores;
		std::span<rhi::CommandBuffer*> commandBuffers;

		uint32 numSignalFences;
		uint32 numWaitFences;
		uint32 numSignalSemaphores;
		uint32 numWaitSemaphores;
		uint32 numCommandBuffers;
	};

	class SubmissionGroup
	{
	public:
		struct Limits
		{
			uint32 signalFenceCount;
			uint32 waitFenceCount;
			uint32 signalSemaphoreCount;
			uint32 waitSemaphoreCount;
			uint32 commandBufferCount;
		};

		SubmissionGroup() = default;
		SubmissionGroup(Data& data);

		~SubmissionGroup() = default;

		auto submit_command_buffer(rhi::CommandBuffer& commandBuffer) -> void;
		auto signal_fence(rhi::Fence const& fence, uint64 value) -> void;
		auto wait_on_fence(rhi::Fence const& fence, uint64 value) -> void;
		auto signal_semaphore(rhi::Semaphore const& semaphore) -> void;
		auto wait_on_semaphore(rhi::Semaphore const& semaphore) -> void;
		auto limits() const -> Limits;
	private:
		Data& m_data;
	};

	SubmissionQueue(rhi::Device& device);
	~SubmissionQueue() = default;

	auto new_submission_group(rhi::DeviceQueueType queueType = rhi::DeviceQueueType::Main) -> SubmissionGroup;
	auto clear() -> void;
	auto send_to_gpu() -> void;
private:
	struct Queue
	{
		static constexpr uint32 MAX_FENCE_SUBMISSION_COUNT				= 128;
		static constexpr uint32 MAX_SEMAPHORE_SUBMISSION_COUNT			= 128;
		static constexpr uint32 MAX_COMMAND_BUFFER_SUBMISSION_COUNT		= 128;
		static constexpr uint32 MAX_SUBMISSION_GROUPS					= 8;

		static constexpr uint32 NUM_FENCE_SUBMISSION_PER_GROUP			= MAX_FENCE_SUBMISSION_COUNT / MAX_SUBMISSION_GROUPS;
		static constexpr uint32 NUM_SEMAPHORE_SUBMISSION_PER_GROUP		= MAX_SEMAPHORE_SUBMISSION_COUNT / MAX_SUBMISSION_GROUPS;
		static constexpr uint32 NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP = MAX_COMMAND_BUFFER_SUBMISSION_COUNT / MAX_SUBMISSION_GROUPS;

		static constexpr uint32 HALF_FENCE_SUBMISSION_COUNT				= NUM_FENCE_SUBMISSION_PER_GROUP / uint32{ 2 };
		static constexpr uint32 HALF_SEMAPHORE_SUBMISSION_COUNT			= NUM_SEMAPHORE_SUBMISSION_PER_GROUP / uint32{ 2 };

		using FenceBuffer			= std::array<std::pair<rhi::Fence const*, uint64>, MAX_FENCE_SUBMISSION_COUNT>;
		using SemaphoreBuffer		= std::array<rhi::Semaphore const*, MAX_SEMAPHORE_SUBMISSION_COUNT>;
		using CommandBufferBuffer	= std::array<rhi::CommandBuffer*, MAX_COMMAND_BUFFER_SUBMISSION_COUNT>;

		rhi::DeviceQueueType type;

		FenceBuffer submittedFences;
		SemaphoreBuffer submittedSemaphores;
		CommandBufferBuffer submittedCommandBuffers;
		
		std::array<Data, MAX_SUBMISSION_GROUPS> submissionGroups;
		uint32 numSubmissionGroups;
	};

	rhi::Device& m_device;
	lib::unique_ptr<Queue> m_mainQueueSubmissions;
	lib::unique_ptr<Queue> m_transferQueueSubmissions;

	auto get_queue(rhi::DeviceQueueType type) -> Queue&;
	auto send_to_gpu(Queue& queue) -> void;
	auto clear(Queue& queue) -> void;
	auto setup_queue(Queue& queue) -> void;
};
}

#endif // !SANDBOX_SUBMISSION_QUEUE
