#include "upload_heap.h"

namespace sandbox
{
UploadHeap::UploadHeap(
	CommandQueue& transferQueue,
	ResourceCache& resourceCache
) :
	m_transfer_command_queue{ transferQueue },
	m_resource_cache{ resourceCache },
	m_staging_buffer_pool_queue{},
	m_image_upload_queue{},
	m_buffer_upload_queue{},
	m_fences{},
	m_fence_values{},
	m_upload_counter{}
{}

auto UploadHeap::initialize() -> void
{
	for (size_t i = 0; i < m_fences.size(); ++i)
	{
		m_fences[i] = m_transfer_command_queue.submission_queue().device().create_fence({ .name = lib::format("upload_heap_timeline_{}", i) });
		m_fence_values[i] = 0ull;
	}
}

auto UploadHeap::terminate() -> void
{
	for (size_t i = 0; i < m_fences.size(); ++i)
	{
		m_transfer_command_queue.submission_queue().device().destroy_fence(m_fences[i]);
		m_fence_values[i] = 0ul;
	}

	for (auto&& stagingBufferPool : m_staging_buffer_pool_queue)
	{
		std::span stagingBuffers = std::span{ stagingBufferPool.buffers.data(), stagingBufferPool.numBuffers };
		for (auto&& buffer : stagingBuffers)
		{
			m_resource_cache.destroy_buffer(buffer.id());
		}
	}
}

auto UploadHeap::send_to_gpu() -> FenceInfo
{
	uint32 const poolIndex = next_pool_index();

	rhi::Fence& fence = m_fences[poolIndex];

	if (do_upload(poolIndex))
	{
		upload_to_gpu(poolIndex);
	}
	increment_counter(poolIndex);

	return FenceInfo{ fence, m_fence_values[poolIndex] };
}

auto UploadHeap::upload_data_to_image(ImageDataUploadInfo&& info) -> upload_id
{
	rhi::ImageInfo const& imageInfo = info.image.info();

	if (info.mipLevel > imageInfo.mipLevel)
	{
		return upload_id{};
	}

	uint32 const poolIndex = next_pool_index();
	InfoPool<ImageUploadInfo>& uploadPool = m_image_upload_queue[poolIndex];
	Resource<rhi::Buffer> stagingBuffer = next_available_buffer(info.size);

	if (stagingBuffer.is_null() ||
		uploadPool.count >= MAX_UPLOADS_PER_POOL)
	{
		upload_to_gpu(poolIndex);
		reset_pools(poolIndex);
		stagingBuffer = next_available_buffer(info.size);
	}

	rhi::BufferWriteInfo bufferWriteInfo = stagingBuffer->write(info.data, info.size);

	uploadPool.uploads[uploadPool.count++] = {
		.copyInfo = {
			.bufferOffset = bufferWriteInfo.offset,
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
		.src = stagingBuffer,
		.dst = info.image,
		.owningQueue = info.srcQueue,
		.dstQueue = info.dstQueue
	};

	return upload_id{ m_upload_counter };
}

auto UploadHeap::upload_data_to_buffer(BufferDataUploadInfo&& info) -> upload_id
{
	uint32 const poolIndex = next_pool_index();
	InfoPool<BufferUploadInfo>& uploadPool = m_buffer_upload_queue[poolIndex];
	Resource<rhi::Buffer> stagingBuffer = next_available_buffer(info.size);

	if (stagingBuffer.is_null() ||
		uploadPool.count >= MAX_UPLOADS_PER_POOL)
	{
		upload_to_gpu(poolIndex);
		reset_pools(poolIndex);
		stagingBuffer = next_available_buffer(info.size);
	}

	rhi::BufferWriteInfo bufferWriteInfo = stagingBuffer->write(info.data, info.size);

	uploadPool.uploads[uploadPool.count++] = {
		.copyInfo = {
			.srcOffset = bufferWriteInfo.offset,
			.dstOffset = info.offset,
			.size = bufferWriteInfo.size,
		},
		.src = stagingBuffer,
		.dst = info.buffer,
		.owningQueue = info.srcQueue,
		.dstQueue = info.dstQueue
	};

	return upload_id{ m_upload_counter };
}

auto UploadHeap::upload_completed(upload_id id) -> bool
{
	return id.get() <= m_upload_counter;
}

auto UploadHeap::current_upload_id() const -> upload_id
{
	return upload_id{ m_upload_counter };
}

auto UploadHeap::next_available_buffer(size_t size) -> Resource<rhi::Buffer>
{
	uint32 const poolIndex = next_pool_index();
	BufferPool& pool = m_staging_buffer_pool_queue[poolIndex];

	Resource<rhi::Buffer> stagingBuffer = {};

	if (!pool.numBuffers)
	{
		create_buffer(poolIndex);
	}
	else
	{
		Resource<rhi::Buffer> const& current = pool.buffers[pool.index];

		if (current->offset() + size >= current->size())
		{
			++pool.index;
			if (pool.index >= pool.numBuffers &&
				pool.numBuffers < MAX_UPLOAD_HEAP_BUFFERS_PER_POOL)
			{
				create_buffer(poolIndex);
			}
		}
	}

	if (pool.index < pool.numBuffers)
	{
		stagingBuffer = pool.buffers[pool.index];
	}

	return stagingBuffer;
}

auto UploadHeap::next_pool_index() const -> uint32
{
	return m_upload_counter % MAX_POOL_IN_QUEUE;
}

auto UploadHeap::create_buffer(uint32 const poolIndex) -> void
{
	BufferPool& pool = m_staging_buffer_pool_queue[poolIndex];
	pool.buffers[pool.index] = m_resource_cache.create_buffer({
		.name = lib::format("upload_heap_{}_{}", poolIndex, pool.index),
		.size = 64_MiB,
		.bufferUsage = rhi::BufferUsage::Transfer_Src,
		.memoryUsage = rhi::MemoryUsage::Can_Alias | rhi::MemoryUsage::Host_Writable
		});
	++pool.numBuffers;
}

auto UploadHeap::upload_to_gpu(uint32 const poolIndex) -> void
{
	auto submitGroup = m_transfer_command_queue.new_submission_group();
	auto cmd = m_transfer_command_queue.next_free_command_buffer(std::this_thread::get_id());

	ASSERTION(!cmd.is_null() && "Ran out of command buffers for recording!");

	InfoPool<ImageUploadInfo>& imageUploadInfos		= m_image_upload_queue[poolIndex];
	InfoPool<BufferUploadInfo>& bufferUploadInfos	= m_buffer_upload_queue[poolIndex];

	std::span imageUploads  = std::span{ imageUploadInfos.uploads.data(), imageUploadInfos.count };
	std::span bufferUploads = std::span{ bufferUploadInfos.uploads.data(), bufferUploadInfos.count };

	cmd->reset();
	cmd->begin();

	acquire_buffer_resources(*cmd, bufferUploads);

	cmd->pipeline_barrier({
		.srcAccess = rhi::access::TOP_OF_PIPE_NONE,
		.dstAccess = rhi::access::TRANSFER_WRITE
	});

	acquire_image_resources(*cmd, imageUploads);

	copy_to_buffers(*cmd, bufferUploads);
	copy_to_images(*cmd, imageUploads);

	release_image_resources(*cmd, imageUploads);
	release_buffer_resources(*cmd, bufferUploads);

	cmd->end();

	submitGroup.submit_command_buffer(*cmd);

	rhi::Fence& fence = m_fences[poolIndex];

	submitGroup.wait_on_fence(fence, m_fence_values[poolIndex]);
	submitGroup.signal_fence(fence, ++m_fence_values[poolIndex]);

	m_transfer_command_queue.submission_queue().send_to_gpu_transfer_submissions();
}

auto UploadHeap::increment_counter(uint32 const poolIndex) -> void
{
	++m_upload_counter;
	reset_pools(poolIndex);
}

auto UploadHeap::reset_pools(uint32 const poolIndex) -> void
{
	BufferPool& pool = m_staging_buffer_pool_queue[poolIndex];
	auto& imageUploadPool = m_image_upload_queue[poolIndex];
	auto& bufferUploadPool = m_buffer_upload_queue[poolIndex];

	for (uint32 i = 0; i < pool.numBuffers; ++i)
	{
		pool.buffers[i]->flush();
	}

	pool.index = 0;
	imageUploadPool.count = 0;
	bufferUploadPool.count = 0;
}

auto UploadHeap::copy_to_images(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void
{
	for (ImageUploadInfo& uploadInfo : imageUploads)
	{
		rhi::Buffer const& src = *uploadInfo.src;
		rhi::Image const& dst = *uploadInfo.dst;

		cmd.copy_buffer_to_image(src, dst, uploadInfo.copyInfo);
	}
}

auto UploadHeap::copy_to_buffers(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& uploadInfo : bufferUploads)
	{
		rhi::Buffer const& src = *uploadInfo.src;
		rhi::Buffer const& dst = *uploadInfo.dst;

		cmd.copy_buffer_to_buffer(src, dst, uploadInfo.copyInfo);
	}
}

auto UploadHeap::acquire_image_resources(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void
{
	for (ImageUploadInfo& info : imageUploads)
	{
		rhi::Image const& image = *info.dst;

		if (info.owningQueue != rhi::DeviceQueueType::Transfer &&
			info.owningQueue != rhi::DeviceQueueType::None)
		{
			cmd.pipeline_barrier(
				image,
				{
					.dstAccess = rhi::access::TRANSFER_WRITE,
					.newLayout = rhi::ImageLayout::Transfer_Dst,
					.subresource = info.copyInfo.imageSubresource,
					.srcQueue = info.owningQueue,
					.dstQueue = rhi::DeviceQueueType::Transfer
				}
			);
		}
		else
		{
			cmd.pipeline_barrier(
				image,
				{
					.dstAccess = rhi::access::TRANSFER_WRITE,
					.oldLayout = rhi::ImageLayout::Undefined,
					.newLayout = rhi::ImageLayout::Transfer_Dst,
					.subresource = info.copyInfo.imageSubresource,
					.srcQueue = rhi::DeviceQueueType::None,
					.dstQueue = rhi::DeviceQueueType::None
				}
			);
		}
	}
}

auto UploadHeap::acquire_buffer_resources(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& info : bufferUploads)
	{
		rhi::Buffer const& buffer = *info.dst;

		if (info.owningQueue != rhi::DeviceQueueType::Transfer &&
			info.owningQueue != rhi::DeviceQueueType::None)
		{
			cmd.pipeline_barrier(
				buffer,
				{
					.size = info.copyInfo.size,
					.offset = info.copyInfo.dstOffset,
					.dstAccess = rhi::access::TOP_OF_PIPE_NONE,
					.srcQueue = info.owningQueue,
					.dstQueue = rhi::DeviceQueueType::Transfer
				}
			);
		}
	}
}

auto UploadHeap::release_image_resources(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void
{
	for (ImageUploadInfo& info : imageUploads)
	{
		cmd.pipeline_barrier(
			*info.dst,
			{
				.srcAccess = rhi::access::TRANSFER_WRITE,
				.oldLayout = rhi::ImageLayout::Transfer_Dst,
				.newLayout = rhi::ImageLayout::Transfer_Dst,
				.subresource = info.copyInfo.imageSubresource,
				.srcQueue = rhi::DeviceQueueType::Transfer,
				.dstQueue = info.dstQueue
			}
		);
	}
}

auto UploadHeap::release_buffer_resources(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& info : bufferUploads)
	{
		cmd.pipeline_barrier(
			*info.dst,
			{
				.size = info.copyInfo.size,
				.offset = info.copyInfo.dstOffset,
				.srcAccess = rhi::access::TRANSFER_WRITE,
				.srcQueue = rhi::DeviceQueueType::Transfer,
				.dstQueue = info.dstQueue
			}
		);
	}
}

auto UploadHeap::do_upload(uint32 const poolIndex) const -> bool
{
	auto const& imageUploadInfoPool  = m_image_upload_queue[poolIndex];
	auto const& bufferUploadInfoPool = m_buffer_upload_queue[poolIndex];

	return imageUploadInfoPool.count || bufferUploadInfoPool.count;
}
}