#pragma once
#ifndef RENDER_UPLOAD_HEAP_HPP
#define RENDER_UPLOAD_HEAP_HPP

#include "lib/handle.hpp"
#include "lib/paged_array.hpp"
#include "command_queue.hpp"

namespace render
{
struct HeapBlock
{
	gpu::buffer buffer = {};
	size_t byteOffset = {};

	auto remaining_capacity() const -> size_t;
	auto write(void const* data, size_t size, size_t offset = std::numeric_limits<size_t>::max()) -> void;

	auto data() const -> void*;

	template <typename T>
	auto data_as() const -> T* { return static_cast<T*>(data()); }
};

struct BufferDataUploadInfo
{
	gpu::buffer dst;
	size_t dstOffset;
	void* data;
	size_t size;
	gpu::DeviceQueue srcQueue = gpu::DeviceQueue::None;
	gpu::DeviceQueue dstQueue = gpu::DeviceQueue::Main;
};

struct BufferHeapBlockUploadInfo
{
	HeapBlock& heapBlock;
	size_t heapWriteOffset;
	size_t heapWriteSize;
	gpu::buffer dst;
	size_t dstOffset;
	gpu::DeviceQueue srcQueue = gpu::DeviceQueue::None;
	gpu::DeviceQueue dstQueue = gpu::DeviceQueue::Main;
};

struct ImageDataUploadInfo
{
	gpu::image image;
	void* data;
	size_t size;
	uint32 mipLevel = 0;
	gpu::DeviceQueue srcQueue = gpu::DeviceQueue::None;
	gpu::DeviceQueue dstQueue = gpu::DeviceQueue::Main;
	gpu::ImageAspect aspectMask = gpu::ImageAspect::Color;
};

using upload_id = lib::handle<struct UPLOAD_HEAP_ID, uint64, std::numeric_limits<uint64>::max()>;

struct FenceInfo
{
	gpu::fence fence;
	uint64 value;
};

struct HeapBlockRequestResult
{
	std::span<HeapBlock> heapBlocks;
	bool overload;
};

/**
* Uploads data to the specified device local buffer and images on the GPU via a pool of upload heaps.
* 
* TODO(afiq):
* 1. Take ReBAR into account. Buffers that are from the ReBAR shouldn't need to go through the staging process.
* 2. Release heap blocks that are NMRU after a while (duration TBD).
*/
class UploadHeap
{
public:
	static constexpr size_t HEAP_BLOCK_SIZE = 8_MiB;
	static constexpr uint32 MAX_UPLOAD_HEAP_PER_POOL = 8;
	static constexpr size_t HEAP_POOL_MAX_SIZE = 64_MiB;

	UploadHeap(gpu::Device& device, CommandQueue& commandQueue);
	~UploadHeap() = default;

	[[nodiscard]] auto device() const -> gpu::Device&;
	/**
	* @brief Retrieves HeapBlocks with spaces from the current active heap pool.
	* 
	* If <size> is larger than 64 MiBs, an empty span is returned instead. Consider breaking the data into smaller chunks and upload in a separate frame.
	* 
	* @param size Size of data that will be added onto the heap block(s).
	* 
	* @return std::span<HeapBlock>
	*/
	[[nodiscard]] auto request_heaps(size_t size) -> std::span<HeapBlock>;
	auto upload_data_to_image(ImageDataUploadInfo&& info) -> upload_id;
	auto upload_data_to_buffer(BufferDataUploadInfo&& info) -> upload_id;
	auto upload_heap_to_buffer(BufferHeapBlockUploadInfo&& info) -> upload_id;
	auto send_to_gpu(bool waitIdle = false) -> FenceInfo;
	auto upload_completed(upload_id id) -> bool;
	auto current_upload_id() const -> upload_id;
private:
	/**
	* Default size of a motherboard's BAR (Base Address Register) is 256 MiBs.
	* Each pool is allowed to have a maximum of 64 MiBs (256 / 4) with each staging buffer having a maximum of 8 MiBs (64 / 8).
	* 
	* With ReBAR, this is not necessary.
	*/
	static constexpr uint32 MAX_POOL_IN_QUEUE = 4;
	
	static constexpr uint32 MAX_UPLOADS_PER_POOL = 64;

	struct HeapPool
	{
		lib::array<HeapBlock> heaps = {};
		size_t current = {};
		uint64 cpuTimelineValue = {};

		auto current_heap() const -> HeapBlock&;
		auto num_available_heaps() const -> size_t;
		auto num_heaps_allocated() const -> size_t;
		auto remaining_heaps_allocatable() const -> size_t;
		auto remaining_capacity() const -> size_t;
	};
	using HeapPoolQueue = std::array<HeapPool, MAX_POOL_IN_QUEUE>;

	struct ImageUploadInfo
	{
		gpu::BufferImageCopyInfo copyInfo;
		gpu::DeviceQueue owningQueue;
		gpu::DeviceQueue dstQueue;
	};

	struct BufferUploadInfo
	{
		gpu::BufferCopyInfo copyInfo;
		gpu::DeviceQueue owningQueue;
		gpu::DeviceQueue dstQueue;
	};

	template <typename T>
	struct InfoPool
	{
		lib::array<T> uploads;
	};

	using ImageUploadInfoQueue	= std::array<InfoPool<ImageUploadInfo>, MAX_POOL_IN_QUEUE>;
	using BufferUploadInfoQueue = std::array<InfoPool<BufferUploadInfo>, MAX_POOL_IN_QUEUE>;
	
	mutable HeapPoolQueue m_heapPoolQueue;
	mutable ImageUploadInfoQueue m_imageUploadInfo;
	mutable BufferUploadInfoQueue m_bufferUploadInfo;
	std::array<uint64, MAX_POOL_IN_QUEUE> m_gpuUploadWaitTimelineValue;
	uint64 m_cpuUploadTimeline;
	gpu::Device& m_device;
	CommandQueue& m_commandQueue;
	gpu::fence m_gpuUploadTimeline;
	uint32 m_nextPool;

	auto allocate_heap(size_t count) -> void;
	auto upload_to_gpu(bool waitIdle = true) -> void;
	auto increment_counter() -> void;
	auto reset_next_pool() -> void;
	auto copy_to_images(gpu::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto copy_to_buffers(gpu::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto acquire_image_resources(gpu::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto acquire_buffer_resources(gpu::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto release_image_resources(gpu::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void;
	auto release_buffer_resources(gpu::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void;
	auto do_upload() const -> bool;
	/*
	* Differs from request_heap() by returning a HeapBlock that will fit the requested size.
	*/
	auto next_available_heap(size_t size) -> HeapBlock*;
	auto next_heap_pool() const -> HeapPool&;
	auto next_image_upload_info_pool() const -> InfoPool<ImageUploadInfo>&;
	auto next_buffer_upload_info_pool() const -> InfoPool<BufferUploadInfo>&;
};
}

#endif // !RENDER_UPLOAD_HEAP_HPP
