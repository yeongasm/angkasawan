#pragma once
#ifndef SANDBOX_UPLOAD_HEAP_H
#define SANDBOX_UPLOAD_HEAP_H

#include "lib/paged_array.h"
#include "command_queue.h"
#include "buffer_view_registry.h"

namespace sandbox
{
struct ImageDataUploadInfo
{
	image_handle image;
	void* data;
	size_t size;
	uint32 mipLevel = 0;
	rhi::DeviceQueueType dstQueue = rhi::DeviceQueueType::Main;
	rhi::ImageAspect aspectMask = rhi::ImageAspect::Color;
};

struct BufferDataUploadInfo
{
	buffer_handle buffer;
	size_t offset;
	void* data;
	size_t size;
	rhi::DeviceQueueType dstQueue = rhi::DeviceQueueType::Main;
};

using upload_id = lib::named_type<uint64, struct _GPU_UPLOAD_ID_>;

struct FenceInfo
{
	lib::ref<rhi::Fence> fence;
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
		rhi::DeviceQueueType dstQueue;
	};

	struct BufferUploadInfo
	{
		rhi::BufferCopyInfo copyInfo;
		lib::ref<rhi::Buffer> src;
		lib::ref<BufferView> dst;
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
	using Fences = std::array<Resource<rhi::Fence>, MAX_POOL_IN_QUEUE>;
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
	auto upload_to_gpu() -> void;
	auto increment_counter() -> void;
	auto reset_pools() -> void;
	auto copy_to_images(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto copy_to_buffers(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto acquire_image_resources(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto acquire_buffer_resources(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto release_image_resources(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto release_buffer_resources(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto do_upload() const -> bool;
};
}

#endif // !SANDBOX_UPLOAD_HEAP_H
