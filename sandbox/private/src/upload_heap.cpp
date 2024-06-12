#include <numeric>
#include "upload_heap.h"

namespace sandbox
{
auto HeapBlock::remaining_capacity() const -> size_t
{
	return buffer->size() - byteOffset;
}

auto HeapBlock::write(void const* data, size_t size, size_t offset) -> void
{
	size_t const writeOffset = std::clamp(offset, 0ull, byteOffset);

	buffer->write(data, size, writeOffset);

	byteOffset = writeOffset + size;
}

auto HeapBlock::data() const -> void*
{
	std::byte* ptr = static_cast<std::byte*>(buffer->data());

	ptr += byteOffset;

	return ptr;
}

auto UploadHeap::HeapPool::current_heap() const -> HeapBlock&
{
	return heaps[current];
}

auto UploadHeap::HeapPool::num_available_heaps() const -> size_t
{
	size_t count = 0;
	for (size_t i = current; i < heaps.size(); ++i)
	{
		if (!std::cmp_equal(heaps[i].remaining_capacity(), 0))
		{
			++count;
		}
	}
	return count;
}

auto UploadHeap::HeapPool::num_heaps_allocated() const -> size_t
{
	return heaps.size();
}

auto UploadHeap::HeapPool::remaining_heaps_allocatable() const -> size_t
{
	return UploadHeap::MAX_UPLOAD_HEAP_PER_POOL - heaps.size();
}

auto UploadHeap::HeapPool::remaining_capacity() const -> size_t
{
	if (!heaps.size())
	{
		return 0;
	}

	size_t remainingCapacity = 0;

	remainingCapacity = std::accumulate(heaps.begin(), heaps.end(), remainingCapacity, [](size_t capacity, HeapBlock const& heap) -> size_t {  return capacity + heap.remaining_capacity(); });

	size_t const unallocatedHeapCount = MAX_UPLOAD_HEAP_PER_POOL - heaps.size();

	return remainingCapacity + (unallocatedHeapCount * HEAP_BLOCK_SIZE);
}

UploadHeap::UploadHeap(gpu::Device& device, CommandQueue& commandQueue) :
	m_heapPoolQueue{},
	m_imageUploadInfo{},
	m_bufferUploadInfo{},
	m_gpuUploadWaitTimelineValue{},
	m_cpuUploadTimeline{},
	m_device{ device },
	m_commandQueue{ commandQueue },
	m_gpuUploadTimeline{ gpu::Fence::from(m_device, { .name = "upload heap gpu timeline" }) },
	m_nextPool{}
{}

auto UploadHeap::device() const -> gpu::Device&
{
	return m_device;
}

auto UploadHeap::request_heaps(size_t size) -> std::span<HeapBlock>
{
	HeapPool& heapPool = next_heap_pool();

	if (!heapPool.heaps.capacity())
	{
		heapPool.heaps.reserve(MAX_UPLOAD_HEAP_PER_POOL);
	}

	if (std::cmp_equal(heapPool.current, MAX_UPLOAD_HEAP_PER_POOL - 1u) &&
		size > heapPool.remaining_capacity())
	{
		return std::span<HeapBlock>{};
	}

	auto numHeapsRequired = (size + HEAP_BLOCK_SIZE - 1) / HEAP_BLOCK_SIZE;

	// Allocate more heaps if we require more but only when we haven't reached the allowed maximum number of heaps per pool.
	// At this point, we won't know if the current heap has sufficient space for the data size that it's being requested for.
	if (numHeapsRequired > heapPool.num_available_heaps() &&
		std::cmp_less(heapPool.heaps.size(), MAX_UPLOAD_HEAP_PER_POOL))
	{
		auto heapsToAllocate = numHeapsRequired - heapPool.num_available_heaps();

		heapsToAllocate = std::min(heapsToAllocate, heapPool.remaining_heaps_allocatable());

		allocate_heap(heapsToAllocate);
	}

	HeapBlock* currentHeap = &heapPool.current_heap();

	// We do not need to check if the pool's current index exceeds MAX_UPLOAD_HEAP_PER_POOL here.
	// The check in the beggining guarantees that the current and remaining pool will have enough storage space for the specified size.
	while (std::cmp_equal(currentHeap->remaining_capacity(), 0))
	{
		++heapPool.current;

		currentHeap = &heapPool.current_heap();
	}

	auto const heapSpanRange = std::min(numHeapsRequired, static_cast<size_t>(MAX_UPLOAD_HEAP_PER_POOL) - heapPool.current);

	return std::span{ currentHeap, heapSpanRange };
}

auto UploadHeap::send_to_gpu(bool waitIdle) -> FenceInfo
{
	if (do_upload())
	{
		upload_to_gpu(waitIdle);
	}
	increment_counter();

	return FenceInfo{ m_gpuUploadTimeline, m_cpuUploadTimeline };
}

auto UploadHeap::upload_data_to_image(ImageDataUploadInfo&& info) -> upload_id
{
	// For images, support to break uploads into smaller sized chunks is non-existent
	// mainly because images with optimal layout can't be treated as a linear sequence of bytes.
	auto const& imageInfo = info.image->info();

	if (info.mipLevel > imageInfo.mipLevel)
	{
		return upload_id{};
	}

	auto& imgUploadPool = next_image_upload_info_pool();
	
	auto heapBlock = next_available_heap(info.size);

	if (heapBlock == nullptr ||
		std::cmp_greater_equal(imgUploadPool.uploads.size(), MAX_UPLOADS_PER_POOL))
	{
		upload_to_gpu(true);

		reset_next_pool();

		heapBlock = next_available_heap(info.size);
	}

	auto const writtenByteOffset = heapBlock->byteOffset;

	heapBlock->buffer->write(info.data, info.size, writtenByteOffset);

	heapBlock->byteOffset += info.size;

	ImageUploadInfo uploadInfo{
		.copyInfo = {
			.src = *heapBlock->buffer,
			.dst = *info.image,
			.bufferOffset = writtenByteOffset,
			.imageSubresource = {
				.aspectFlags = info.aspectMask,
				.mipLevel = info.mipLevel,
			},
			.imageOffset = {},
			.imageExtent = {
				.width = imageInfo.dimension.width >> info.mipLevel,
				.height = imageInfo.dimension.height >> info.mipLevel,
				.depth = 1u
			}
		},
		.owningQueue = info.srcQueue,
		.dstQueue = info.dstQueue
	};

	imgUploadPool.uploads.push_back(std::move(uploadInfo));

	return upload_id{ m_cpuUploadTimeline + 1 };
}

auto UploadHeap::upload_data_to_buffer(BufferDataUploadInfo&& info) -> upload_id
{
	auto& bufferUploadPool = next_buffer_upload_info_pool();

	std::byte const* data = static_cast<std::byte*>(info.data);
	size_t remainingSizeToUpload = info.size;

	while (!std::cmp_equal(remainingSizeToUpload, 0))
	{
		auto heapBlocks = request_heaps(remainingSizeToUpload);

		if (heapBlocks.empty() ||
			std::cmp_greater_equal(bufferUploadPool.uploads.size(), MAX_UPLOADS_PER_POOL))
		{
			upload_to_gpu(true);

			reset_next_pool();

			heapBlocks = request_heaps(remainingSizeToUpload);
		}

		for (size_t i = 0; i < heapBlocks.size(); ++i)
		{
			auto& heapBlock = heapBlocks[i];

			size_t writeSize = HEAP_BLOCK_SIZE;
		
			if (i + 1 == heapBlocks.size())
			{
				writeSize = remainingSizeToUpload;
			}

			if (heapBlock.remaining_capacity() < info.size)
			{
				writeSize = heapBlock.remaining_capacity();
			}

			auto const writtenByteOffset = heapBlock.byteOffset;

			heapBlock.buffer->write(data, writeSize, writtenByteOffset);

			heapBlock.byteOffset += writeSize;

			BufferUploadInfo uploadInfo{
				.copyInfo = {
					.src = *heapBlock.buffer,
					.dst = *info.dst,
					.srcOffset = writtenByteOffset,
					.dstOffset = info.dstOffset + (i * HEAP_BLOCK_SIZE),
					.size = writeSize,
				},
				.owningQueue = info.srcQueue,
				.dstQueue = info.dstQueue
			};

			bufferUploadPool.uploads.push_back(std::move(uploadInfo));

			data += writeSize;
			remainingSizeToUpload -= writeSize;
		}
	}

	return upload_id{ m_cpuUploadTimeline + 1 };
}

auto UploadHeap::upload_heap_to_buffer(BufferHeapBlockUploadInfo&& info) -> upload_id
{
	auto& bufferUploadPool = next_buffer_upload_info_pool();

	if (std::cmp_greater_equal(bufferUploadPool.uploads.size(), MAX_UPLOADS_PER_POOL))
	{
		upload_to_gpu(true);

		reset_next_pool();
	}

	BufferUploadInfo uploadInfo{
		.copyInfo = {
			.src = *info.heapBlock.buffer,
			.dst = *info.dst,
			.srcOffset = info.heapWriteOffset,
			.dstOffset = info.dstOffset,
			.size = info.heapWriteSize,
		},
		.owningQueue = info.srcQueue,
		.dstQueue = info.dstQueue
	};

	bufferUploadPool.uploads.push_back(std::move(uploadInfo));

	return upload_id{ m_cpuUploadTimeline + 1 };
}

auto UploadHeap::upload_completed(upload_id id) -> bool
{
	return m_gpuUploadTimeline->value() >= id.get();
}

auto UploadHeap::current_upload_id() const -> upload_id
{
	return upload_id{ m_cpuUploadTimeline + 1 };
}

auto UploadHeap::allocate_heap(size_t count) -> void
{
	auto& heapPool = next_heap_pool();
	auto const maxAllocatableCount = MAX_UPLOAD_HEAP_PER_POOL - heapPool.heaps.size();
	
	for (size_t i = 0; i < maxAllocatableCount && i < count; ++i)
	{
		gpu::BufferInfo heapBlockInfo{
			.name = lib::format("upload heap {}:{}", m_nextPool, heapPool.heaps.size()),
			.size = HEAP_BLOCK_SIZE,
			.bufferUsage = gpu::BufferUsage::Transfer_Src,
			.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Host_Writable,
			.sharingMode = gpu::SharingMode::Exclusive
		};
		heapPool.heaps.emplace_back(gpu::Buffer::from(m_device, std::move(heapBlockInfo)));
	}
}

auto UploadHeap::upload_to_gpu(bool waitIdle) -> void
{
	++m_cpuUploadTimeline;

	auto submitGroup = m_commandQueue.new_submission_group(gpu::DeviceQueue::Transfer);
	auto cmd = m_commandQueue.next_free_command_buffer({ .queue = gpu::DeviceQueue::Transfer });

	ASSERTION(cmd.valid() && "Ran out of command buffers for recording!");

	InfoPool<ImageUploadInfo>& imageUploadInfos		= next_image_upload_info_pool();
	InfoPool<BufferUploadInfo>& bufferUploadInfos	= next_buffer_upload_info_pool();

	std::span imageUploads  = std::span{ imageUploadInfos.uploads.data(), imageUploadInfos.uploads.size() };
	std::span bufferUploads = std::span{ bufferUploadInfos.uploads.data(), bufferUploadInfos.uploads.size() };

	cmd->reset();
	cmd->begin();

	acquire_buffer_resources(*cmd, bufferUploads);

	cmd->pipeline_barrier({
		.srcAccess = gpu::access::TOP_OF_PIPE_NONE,
		.dstAccess = gpu::access::TRANSFER_WRITE
	});

	acquire_image_resources(*cmd, imageUploads);

	copy_to_buffers(*cmd, bufferUploads);
	copy_to_images(*cmd, imageUploads);

	release_image_resources(*cmd, imageUploads);
	release_buffer_resources(*cmd, bufferUploads);

	cmd->end();

	submitGroup.submit(cmd);

	submitGroup.wait(m_gpuUploadTimeline, m_gpuUploadWaitTimelineValue[m_nextPool]);
	submitGroup.signal(m_gpuUploadTimeline, m_cpuUploadTimeline);

	m_commandQueue.send_to_gpu(gpu::DeviceQueue::Transfer);

	m_gpuUploadWaitTimelineValue[m_nextPool] = m_cpuUploadTimeline;

	if (waitIdle)
	{
		m_device.wait_idle();
	}
}

auto UploadHeap::increment_counter() -> void
{
	m_nextPool = (m_nextPool + 1) % MAX_POOL_IN_QUEUE;

	reset_next_pool();
}

auto UploadHeap::reset_next_pool() -> void
{
	auto& heapPool = next_heap_pool();

	auto& imageUploadPool	= next_image_upload_info_pool();
	auto& bufferUploadPool	= next_buffer_upload_info_pool();

	for (auto& heap : heapPool.heaps)
	{
		heap.byteOffset = 0;
	}

	heapPool.current = 0;

	imageUploadPool.uploads.clear();
	bufferUploadPool.uploads.clear();
}

auto UploadHeap::copy_to_images(gpu::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void
{
	for (ImageUploadInfo& uploadInfo : imageUploads)
	{
		cmd.copy_buffer_to_image(uploadInfo.copyInfo);
	}
}

auto UploadHeap::copy_to_buffers(gpu::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& uploadInfo : bufferUploads)
	{
		cmd.copy_buffer_to_buffer(uploadInfo.copyInfo);
	}
}

auto UploadHeap::acquire_image_resources(gpu::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void
{
	for (ImageUploadInfo& info : imageUploads)
	{
		if (info.copyInfo.dst.info().sharingMode == gpu::SharingMode::Concurrent)
		{
			continue;
		}

		if (info.owningQueue != gpu::DeviceQueue::Transfer &&
			info.owningQueue != gpu::DeviceQueue::None)
		{
			cmd.pipeline_image_barrier({
				.image = info.copyInfo.dst,
				.dstAccess = gpu::access::TRANSFER_WRITE,
				.newLayout = gpu::ImageLayout::Transfer_Dst,
				.subresource = info.copyInfo.imageSubresource,
				.srcQueue = info.owningQueue,
				.dstQueue = gpu::DeviceQueue::Transfer
			});
		}
		else
		{
			cmd.pipeline_image_barrier({
				.image = info.copyInfo.dst,
				.dstAccess = gpu::access::TRANSFER_WRITE,
				.oldLayout = gpu::ImageLayout::Undefined,
				.newLayout = gpu::ImageLayout::Transfer_Dst,
				.subresource = info.copyInfo.imageSubresource,
				.srcQueue = gpu::DeviceQueue::None,
				.dstQueue = gpu::DeviceQueue::None
			});
		}
	}
}

auto UploadHeap::acquire_buffer_resources(gpu::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& info : bufferUploads)
	{
		if (info.copyInfo.dst.info().sharingMode == gpu::SharingMode::Concurrent)
		{
			continue;
		}

		if (info.owningQueue != gpu::DeviceQueue::Transfer &&
			info.owningQueue != gpu::DeviceQueue::None)
		{
			cmd.pipeline_buffer_barrier({
				.buffer = info.copyInfo.dst,
				.size = info.copyInfo.size,
				.offset = info.copyInfo.dstOffset,
				.dstAccess = gpu::access::TOP_OF_PIPE_NONE,
				.srcQueue = info.owningQueue,
				.dstQueue = gpu::DeviceQueue::Transfer
			});
		}
	}
}

auto UploadHeap::release_image_resources(gpu::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void
{
	for (ImageUploadInfo& info : imageUploads)
	{
		if (info.copyInfo.dst.info().sharingMode == gpu::SharingMode::Concurrent)
		{
			continue;
		}

		cmd.pipeline_image_barrier({
			.image = info.copyInfo.dst,
			.srcAccess = gpu::access::TRANSFER_WRITE,
			.oldLayout = gpu::ImageLayout::Transfer_Dst,
			.newLayout = gpu::ImageLayout::Transfer_Dst,
			.subresource = info.copyInfo.imageSubresource,
			.srcQueue = gpu::DeviceQueue::Transfer,
			.dstQueue = info.dstQueue
		});
	}
}

auto UploadHeap::release_buffer_resources(gpu::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& info : bufferUploads)
	{
		if (info.copyInfo.dst.info().sharingMode == gpu::SharingMode::Concurrent)
		{
			continue;
		}

		cmd.pipeline_buffer_barrier({
			.buffer = info.copyInfo.dst,
			.size = info.copyInfo.size,
			.offset = info.copyInfo.dstOffset,
			.srcAccess = gpu::access::TRANSFER_WRITE,
			.srcQueue = gpu::DeviceQueue::Transfer,
			.dstQueue = info.dstQueue
		});
	}
}

auto UploadHeap::do_upload() const -> bool
{
	auto const& imageUploadInfoPool  = next_image_upload_info_pool();
	auto const& bufferUploadInfoPool = next_buffer_upload_info_pool();

	return !imageUploadInfoPool.uploads.empty() || !bufferUploadInfoPool.uploads.empty();
}

auto UploadHeap::next_available_heap(size_t size) -> HeapBlock*
{
	auto& heapPool = next_heap_pool();

	if (!heapPool.heaps.capacity())
	{
		heapPool.heaps.reserve(MAX_UPLOAD_HEAP_PER_POOL);
	}

	HeapBlock* pHeap = nullptr;

	// Get the first heap that can fit.
	size_t const numAvailableHeaps = heapPool.heaps.size();
	// HeapPool.current is only incremented when the current heap it's referencing has no more capacity to hold data.
	// Hence the reason why we start indexing from HeapPool.current.
	for (size_t i = heapPool.current; i < numAvailableHeaps; ++i)
	{
		HeapBlock& heapBlock = heapPool.heaps[i];
		if (heapBlock.remaining_capacity() >= size)
		{
			pHeap = &heapBlock;
			break;
		}
	}

	if (!pHeap && 
		std::cmp_less(heapPool.heaps.size(), MAX_UPLOAD_HEAP_PER_POOL))
	{
		allocate_heap(1);

		pHeap = &heapPool.heaps.back();

		// Don't increment HeapPool.current here because there could still be storage space in the previous block.
	}

	return pHeap;
}

auto UploadHeap::next_heap_pool() const -> HeapPool&
{
	return m_heapPoolQueue[m_nextPool];
}

auto UploadHeap::next_image_upload_info_pool() const -> InfoPool<ImageUploadInfo>&
{
	return m_imageUploadInfo[m_nextPool];
}

auto UploadHeap::next_buffer_upload_info_pool() const -> InfoPool<BufferUploadInfo>&
{
	return m_bufferUploadInfo[m_nextPool];
}

}