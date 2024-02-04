#pragma once
#ifndef SANDBOX_UPLOAD_HEAP_H
#define SANDBOX_UPLOAD_HEAP_H

#include "lib/paged_array.h"
#include "command_queue.h"
#include "resource_cache.h"

namespace sandbox
{
struct BufferDataUploadInfo
{
	rhi::Buffer& buffer;
	void* data;
	size_t offset;
	size_t size;
	rhi::DeviceQueueType srcQueue = rhi::DeviceQueueType::None;
	rhi::DeviceQueueType dstQueue = rhi::DeviceQueueType::Main;
	//lib::ref<rhi::Fence> waitFence;
	//uint64 waitValue = 0;
};

struct ImageDataUploadInfo
{
	rhi::Image& image;
	void* data;
	size_t size;
	uint32 mipLevel = 0;
	rhi::DeviceQueueType srcQueue = rhi::DeviceQueueType::None;
	rhi::DeviceQueueType dstQueue = rhi::DeviceQueueType::Main;
	rhi::ImageAspect aspectMask = rhi::ImageAspect::Color;
	//lib::ref<rhi::Fence> waitFence;
	//uint64 waitValue = 0;
};

using upload_id = lib::handle<struct UPLOAD_HEAP_ID, uint64, std::numeric_limits<uint64>::max()>;

struct FenceInfo
{
	rhi::Fence& fence;
	uint64 value;
}; 

class UploadHeap
{
public:
	UploadHeap(CommandQueue& transferQueue, ResourceCache& resourceCache);
	~UploadHeap() = default;

	auto initialize() -> void;
	auto terminate() -> void;
	auto send_to_gpu() -> FenceInfo;
	auto upload_data_to_image(ImageDataUploadInfo&& info) -> upload_id;
	auto upload_data_to_buffer(BufferDataUploadInfo&& info) -> upload_id;
	auto upload_completed(upload_id id) -> bool;
	auto current_upload_id() const -> upload_id;
private:
	static constexpr uint32 MAX_UPLOAD_HEAP_BUFFERS_PER_POOL = 8;
	static constexpr uint32 MAX_UPLOADS_PER_POOL = 64;
	static constexpr uint32 MAX_POOL_IN_QUEUE = 3;

	struct BufferPool
	{
		using Pool = std::array<Resource<rhi::Buffer>, MAX_UPLOAD_HEAP_BUFFERS_PER_POOL>;
		
		Pool buffers = {};
		uint32 index = 0;
		uint32 numBuffers = 0;
	};
	using BufferPoolQueue = std::array<BufferPool, MAX_POOL_IN_QUEUE>;

	struct ImageUploadInfo
	{
		rhi::BufferImageCopyInfo copyInfo;
		lib::ref<rhi::Buffer> src;
		lib::ref<rhi::Image> dst;
		rhi::DeviceQueueType owningQueue;
		rhi::DeviceQueueType dstQueue;
	};

	struct BufferUploadInfo
	{
		rhi::BufferCopyInfo copyInfo;
		lib::ref<rhi::Buffer> src;
		lib::ref<rhi::Buffer> dst;
		rhi::DeviceQueueType owningQueue;
		rhi::DeviceQueueType dstQueue;
	};

	template <typename T>
	struct InfoPool
	{
		std::array<T, MAX_UPLOADS_PER_POOL> uploads;
		uint32 count;
	};

	using ImageUploadInfoQueue	= std::array<InfoPool<ImageUploadInfo>, MAX_POOL_IN_QUEUE>;
	using BufferUploadInfoQueue = std::array<InfoPool<BufferUploadInfo>, MAX_POOL_IN_QUEUE>;
	using Fences = std::array<rhi::Fence, MAX_POOL_IN_QUEUE>;
	using FenceValues = std::array<uint64, MAX_POOL_IN_QUEUE>;

	CommandQueue& m_transfer_command_queue;
	ResourceCache& m_resource_cache;
	BufferPoolQueue m_staging_buffer_pool_queue;
	ImageUploadInfoQueue m_image_upload_queue;
	BufferUploadInfoQueue m_buffer_upload_queue;
	Fences m_fences;
	FenceValues m_fence_values;
	uint64 m_upload_counter;

	auto next_available_buffer(size_t size) -> Resource<rhi::Buffer>;
	auto next_pool_index() const -> uint32;
	auto create_buffer(uint32 const poolIndex) -> void;
	auto upload_to_gpu(uint32 const poolIndex) -> void;
	auto increment_counter(uint32 const poolIndex) -> void;
	auto reset_pools(uint32 const poolIndex) -> void;
	auto copy_to_images(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto copy_to_buffers(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto acquire_image_resources(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto acquire_buffer_resources(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto release_image_resources(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto release_buffer_resources(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto do_upload(uint32 const poolIndex) const -> bool;
};
}

#endif // !SANDBOX_UPLOAD_HEAP_H
