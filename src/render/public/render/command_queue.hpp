#pragma once
#ifndef RENDER_COMMAND_QUEUE_HPP
#define RENDER_COMMAND_QUEUE_HPP

#include <thread>
#include "lib/map.hpp"
#include "gpu/gpu.hpp"

namespace render
{
struct Queue
{
	static constexpr uint32 MAX_FENCE_SUBMISSION_COUNT = 128;
	static constexpr uint32 MAX_SEMAPHORE_SUBMISSION_COUNT = 128;
	static constexpr uint32 MAX_COMMAND_BUFFER_SUBMISSION_COUNT = 128;
	static constexpr uint32 MAX_SUBMISSION_GROUPS = 8;

	static constexpr uint32 NUM_FENCE_SUBMISSION_PER_GROUP = MAX_FENCE_SUBMISSION_COUNT / MAX_SUBMISSION_GROUPS;
	static constexpr uint32 NUM_SEMAPHORE_SUBMISSION_PER_GROUP = MAX_SEMAPHORE_SUBMISSION_COUNT / MAX_SUBMISSION_GROUPS;
	static constexpr uint32 NUM_COMMAND_BUFFER_SUBMISSION_PER_GROUP = MAX_COMMAND_BUFFER_SUBMISSION_COUNT / MAX_SUBMISSION_GROUPS;

	static constexpr uint32 HALF_FENCE_SUBMISSION_COUNT = NUM_FENCE_SUBMISSION_PER_GROUP / uint32{ 2 };
	static constexpr uint32 HALF_SEMAPHORE_SUBMISSION_COUNT = NUM_SEMAPHORE_SUBMISSION_PER_GROUP / uint32{ 2 };

	struct SubmissionData
	{
		uint32 id;
		uint32 numSignalFences;
		uint32 numWaitFences;
		uint32 numSignalSemaphores;
		uint32 numWaitSemaphores;
		uint32 numCommandBuffers;
	};

	using FenceBuffer = std::array<std::pair<gpu::fence, uint64>, MAX_FENCE_SUBMISSION_COUNT>;
	using SemaphoreBuffer = std::array<gpu::semaphore, MAX_SEMAPHORE_SUBMISSION_COUNT>;
	using CommandBufferBuffer = std::array<gpu::command_buffer, MAX_COMMAND_BUFFER_SUBMISSION_COUNT>;
	using SubmissionDataBuffer = std::array<SubmissionData, MAX_SUBMISSION_GROUPS>;

	FenceBuffer	submittedFences;
	SemaphoreBuffer	submittedSemaphores;
	CommandBufferBuffer submittedCommandBuffers;
	SubmissionDataBuffer submissionGroupData;
	lib::map<std::thread::id, gpu::command_pool> commandPoolStore;
	uint32 numSubmissionGroups;
};

class SubmissionGroup
{
public:
	SubmissionGroup(Queue& queue, Queue::SubmissionData& data);
	~SubmissionGroup() = default;

	auto submit(gpu::command_buffer const& commandBuffer) -> void;
	auto signal(gpu::fence const& fence, uint64 value) -> void;
	auto signal(gpu::semaphore const& semaphore) -> void;
	auto wait(gpu::fence const& fence, uint64 value) -> void;
	auto wait(gpu::semaphore const& semaphore) -> void;
private:
	Queue& m_queue;
	Queue::SubmissionData& m_data;
};

struct RequestCommandBufferInfo
{
	std::thread::id tid = std::this_thread::get_id();
	gpu::DeviceQueue queue = gpu::DeviceQueue::Main;
};

class CommandQueue
{
public:
	CommandQueue(gpu::Device& device);
	~CommandQueue() = default;

	auto new_submission_group(gpu::DeviceQueue queue = gpu::DeviceQueue::Main) -> SubmissionGroup;
	auto next_free_command_buffer(RequestCommandBufferInfo&& info = {}) -> gpu::command_buffer;
	auto send_to_gpu(gpu::DeviceQueue queue = gpu::DeviceQueue::Main) -> void;
	auto clear(gpu::DeviceQueue queue = gpu::DeviceQueue::Main) -> void;
private:
	std::unique_ptr<Queue> m_mainQueue;
	std::unique_ptr<Queue> m_transferQueue;
	gpu::Device& m_device;

	auto get_queue(gpu::DeviceQueue type) -> Queue&;
	auto send_to_gpu(Queue& queue, gpu::DeviceQueue type) -> void;
	auto clear(Queue& queue) -> void;
};
}

#endif // !RENDER_COMMAND_QUEUE_HPP
