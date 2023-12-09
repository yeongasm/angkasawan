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
		m_fences[i] = m_resource_cache.create_fence({ .name = lib::format("upload_heap_timeline_{}", i) });
		m_fence_values[i] = 0ull;
	}
}

auto UploadHeap::terminate() -> void
{
	for (size_t i = 0; i < m_fences.size(); ++i)
	{
		m_resource_cache.destroy_fence(m_fences[i].id());
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

	Resource<rhi::Fence>& fenceResource = m_fences[poolIndex];

	if (do_upload())
	{
		upload_to_gpu();
	}
	increment_counter();

	return FenceInfo{ fenceResource, m_fence_values[poolIndex] };
}

auto UploadHeap::upload_data_to_image(ImageDataUploadInfo&& info) -> upload_id
{
	lib::ref<rhi::Image> dstImage = m_resource_cache.get_image(info.image);
	if (dstImage.is_null())
	{
		return upload_id{};
	}

	rhi::ImageInfo const& imageInfo = dstImage->info();

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
		upload_to_gpu();
		reset_pools();
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
		.dst = dstImage,
		.dstQueue = info.dstQueue
	};

	return upload_id{ m_upload_counter };
}

auto UploadHeap::upload_data_to_buffer(BufferDataUploadInfo&& info) -> upload_id
{
	lib::ref<rhi::Buffer> dstBuffer = m_resource_cache.get_buffer(info.buffer);
	if (dstBuffer.is_null())
	{
		return upload_id{};
	}

	rhi::BufferInfo const& bufferInfo = dstBuffer->info();

	uint32 const poolIndex = next_pool_index();
	InfoPool<BufferUploadInfo>& uploadPool = m_buffer_upload_queue[poolIndex];
	Resource<rhi::Buffer> stagingBuffer = next_available_buffer(info.size);

	if (stagingBuffer.is_null() ||
		uploadPool.count >= MAX_UPLOADS_PER_POOL)
	{
		upload_to_gpu();
		reset_pools();
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
		.dst = dstBuffer,
		.dstQueue = info.dstQueue
	};

	return upload_id{ m_upload_counter };
}

auto UploadHeap::upload_completed(upload_id id) -> bool
{
	return id.get() <= m_upload_counter;
}

auto UploadHeap::next_available_buffer(size_t size) -> Resource<rhi::Buffer>
{
	uint32 const poolIndex = next_pool_index();
	BufferPool& pool = m_staging_buffer_pool_queue[poolIndex];

	Resource<rhi::Buffer> stagingBuffer;

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

auto UploadHeap::upload_to_gpu() -> void
{
	auto submitGroup = m_transfer_command_queue.new_submission_group();
	auto cmd = m_transfer_command_queue.next_free_command_buffer(std::this_thread::get_id());

	ASSERTION(cmd.is_null() && "Ran out of command buffers for recording!");

	uint32 const poolIndex = next_pool_index();

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

	Resource<rhi::Fence>& fenceResource = m_fences[poolIndex];
	lib::ref<rhi::Fence>& fence = fenceResource;

	submitGroup.wait_on_fence(*fence, m_fence_values[poolIndex]);
	submitGroup.signal_fence(*fence, ++m_fence_values[poolIndex]);

	m_transfer_command_queue.submission_queue().send_to_gpu_transfer_submissions();
}

auto UploadHeap::increment_counter() -> void
{
	++m_upload_counter;
	reset_pools();
}

auto UploadHeap::reset_pools() -> void
{
	auto poolIndex = next_pool_index();
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
		rhi::Buffer& src = *uploadInfo.src;
		rhi::Image& dst = *uploadInfo.dst;

		cmd.copy_buffer_to_image(src, dst, uploadInfo.copyInfo);
	}
}

auto UploadHeap::copy_to_buffers(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& uploadInfo : bufferUploads)
	{
		rhi::Buffer& src = *uploadInfo.src;
		rhi::Buffer& dst = *uploadInfo.dst;

		cmd.copy_buffer_to_buffer(src, dst, uploadInfo.copyInfo);
	}
}

auto UploadHeap::acquire_image_resources(rhi::CommandBuffer& cmd, std::span<ImageUploadInfo>& imageUploads) -> void
{
	for (ImageUploadInfo& info : imageUploads)
	{
		cmd.pipeline_barrier(
			*info.dst,
			{
				.dstAccess = rhi::access::TRANSFER_WRITE,
				.newLayout = rhi::ImageLayout::Transfer_Dst,
				.subresource = info.copyInfo.imageSubresource,
				.srcQueue = info.dst->owner(),
				.dstQueue = rhi::DeviceQueueType::Transfer
			}
		);
	}
}

auto UploadHeap::acquire_buffer_resources(rhi::CommandBuffer& cmd, std::span<BufferUploadInfo>& bufferUploads) -> void
{
	for (BufferUploadInfo& info : bufferUploads)
	{
		rhi::Buffer& buffer = *info.dst;

		if (buffer.owner() != rhi::DeviceQueueType::Transfer &&
			buffer.owner() != rhi::DeviceQueueType::None)
		{
			cmd.pipeline_barrier(
				*info.dst,
				{
					.dstAccess = rhi::access::TOP_OF_PIPE_NONE,
					.srcQueue = info.dst->owner(),
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
				.newLayout = rhi::ImageLayout::Undefined,
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
				.srcAccess = rhi::access::TRANSFER_WRITE,
				.srcQueue = rhi::DeviceQueueType::Transfer,
				.dstQueue = info.dstQueue
			}
		);
	}
}

auto UploadHeap::do_upload() const -> bool
{
	uint32 const poolIndex = next_pool_index();

	auto const& imageUploadInfoPool  = m_image_upload_queue[poolIndex];
	auto const& bufferUploadInfoPool = m_buffer_upload_queue[poolIndex];

	return imageUploadInfoPool.count || bufferUploadInfoPool.count;
}
}