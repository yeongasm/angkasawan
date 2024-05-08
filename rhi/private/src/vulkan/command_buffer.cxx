module;

#include <mutex>
#include "vulkan/vk.h"

#include "lib/string.h"

module forge;

namespace frg
{
auto CommandBuffer::info() const -> CommandBufferInfo const&
{
	return m_info;
}

auto CommandBuffer::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto CommandBuffer::is_initial() const -> bool
{
	return m_state == CommandBufferState::Initial;
}

auto CommandBuffer::is_recording() const -> bool
{
	return m_state == CommandBufferState::Recording;
}

auto CommandBuffer::is_executable() const -> bool
{
	return m_state == CommandBufferState::Executable;
}

auto CommandBuffer::is_pending_complete() const -> bool
{
	return m_state == CommandBufferState::Pending;
}

auto CommandBuffer::is_completed() -> bool
{
	uint64 const val = m_completionTimeline->value();

	if (m_recordingTimeline && val >= m_recordingTimeline)
	{
		m_state = CommandBufferState::Executable;
	}

	return m_recordingTimeline && val >= m_recordingTimeline;
}

auto CommandBuffer::is_invalid() const -> bool
{
	return m_state == CommandBufferState::Invalid;
}

auto CommandBuffer::current_state() const -> CommandBufferState
{
	return m_state;
}

auto CommandBuffer::reset() -> void
{
	if (valid() && is_completed() && is_executable())
	{
		vkResetCommandBuffer(m_impl.handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		m_state = CommandBufferState::Initial;
	}
}

auto CommandBuffer::begin() -> bool
{
	if (!valid() || !is_initial()) [[unlikely]]
	{
		return false;
	}

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	if (vkBeginCommandBuffer(m_impl.handle, &beginInfo) == VK_SUCCESS)
	{
		m_state = CommandBufferState::Recording;
		++m_recordingTimeline;
	}

	return m_state == CommandBufferState::Recording;
}

auto CommandBuffer::end() -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	vkEndCommandBuffer(m_impl.handle);

	m_state = CommandBufferState::Executable;
}

auto CommandBuffer::clear(Resource<Image> image) -> void
{
	return clear(image, { .dstImageLayout = ImageLayout::General, .clearValue = image->info().clearValue });
}

auto CommandBuffer::clear(Resource<Image> image, ImageClearInfo const& info) -> void
{
	if (!valid() || !image.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	ClearValue imgClearValue = image->info().clearValue;

	api::Image const& img = image->m_impl;

	VkImageSubresourceRange subResourceRange{
		.aspectMask = api::translate_image_aspect_flags(info.subresource.aspectFlags),
		.baseMipLevel = info.subresource.mipLevel,
		.levelCount = info.subresource.levelCount,
		.baseArrayLayer = info.subresource.baseArrayLayer,
		.layerCount = info.subresource.layerCount
	};

	if (!is_color_format(image->info().format))
	{
		VkClearDepthStencilValue depthStencilValue{
			.depth = imgClearValue.depthStencil.depth,
			.stencil = imgClearValue.depthStencil.stencil
		};
		vkCmdClearDepthStencilImage(
			m_impl.handle,
			img.handle,
			api::translate_image_layout(info.dstImageLayout),
			&depthStencilValue,
			1,
			&subResourceRange
		);
	}
	else
	{
		VkClearColorValue colorValue{};
		lib::memcopy(&colorValue.uint32, &imgClearValue.color.u32, sizeof(imgClearValue.color));
		vkCmdClearColorImage(
			m_impl.handle,
			img.handle,
			api::translate_image_layout(info.dstImageLayout),
			&colorValue,
			1,
			&subResourceRange
		);
	}
}

auto CommandBuffer::clear(Resource<Buffer> buffer) -> void
{
	size_t const size = buffer->valid() ? buffer->info().size : 0;
	clear(buffer, { .offset = 0, .size = size, .data = 0u });
}

auto CommandBuffer::clear(Resource<Buffer> buffer, BufferClearInfo const& info) -> void
{
	if (!valid() || !buffer.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}
		
	flush_barriers();

	vkCmdFillBuffer(
		m_impl.handle,
		buffer->m_impl.handle,
		static_cast<VkDeviceSize>(info.offset),
		static_cast<VkDeviceSize>(info.size),
		info.data
	);
}

auto CommandBuffer::draw(DrawInfo const& info) const -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	vkCmdDraw(m_impl.handle, info.vertexCount, info.instanceCount, info.firstVertex, info.firstInstance);
}

auto CommandBuffer::draw_indexed(DrawIndexedInfo const& info) const -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	vkCmdDrawIndexed(m_impl.handle, info.indexCount, info.instanceCount, info.firstIndex, info.vertexOffset, info.firstInstance);
}

auto CommandBuffer::draw_indirect(Resource<Buffer> drawInfoBuffer, DrawIndirectInfo const& info) const -> void
{
	if (!valid() || !drawInfoBuffer.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	api::Buffer const& buff = drawInfoBuffer->m_impl;

	if (info.indexed)
	{
		vkCmdDrawIndexedIndirect(
			m_impl.handle,
			buff.handle,
			static_cast<VkDeviceSize>(info.offset),
			info.drawCount,
			info.stride
		);
	}
	else
	{
		vkCmdDrawIndirect(
			m_impl.handle,
			buff.handle,
			static_cast<VkDeviceSize>(info.offset),
			info.drawCount,
			info.stride
		);
	}
}

auto CommandBuffer::draw_indirect_count(Resource<Buffer> drawInfoBuffer, Resource<Buffer> drawCountBuffer, DrawIndirectCountInfo const& info) const -> void
{
	if (!valid() || !drawInfoBuffer.valid() || !drawCountBuffer.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	api::Buffer const& drawInfoBuf = drawInfoBuffer->m_impl;
	api::Buffer const& countBuf = drawCountBuffer->m_impl;

	if (info.indexed)
	{
		vkCmdDrawIndexedIndirectCount(
			m_impl.handle,
			drawInfoBuf.handle,
			static_cast<VkDeviceSize>(info.offset),
			countBuf.handle,
			static_cast<VkDeviceSize>(info.countBufferOffset),
			info.maxDrawCount,
			info.stride
		);
	}
	else
	{
		vkCmdDrawIndirectCount(
			m_impl.handle,
			drawInfoBuf.handle,
			static_cast<VkDeviceSize>(info.offset),
			countBuf.handle,
			static_cast<VkDeviceSize>(info.countBufferOffset),
			info.maxDrawCount,
			info.stride
		);
	}
}

auto CommandBuffer::bind_vertex_buffer(Resource<Buffer> buffer, BindVertexBufferInfo const& info) -> void
{
	if (!valid() || !buffer.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	using buffer_usage_t = std::underlying_type_t<BufferUsage>;

	buffer_usage_t const usage = static_cast<buffer_usage_t>(buffer->info().bufferUsage);
	buffer_usage_t constexpr VERTEX_BUFFER_USAGE = static_cast<buffer_usage_t>(BufferUsage::Vertex);

	if (usage & VERTEX_BUFFER_USAGE)
	{
		vkCmdBindVertexBuffers(m_impl.handle, info.firstBinding, 1, &buffer->m_impl.handle, &info.offset);
	}
}

auto CommandBuffer::bind_index_buffer(Resource<Buffer> buffer, BindIndexBufferInfo const& info) -> void
{
	if (!valid() || !buffer.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	using buffer_usage_t = std::underlying_type_t<BufferUsage>;

	buffer_usage_t const usage = static_cast<buffer_usage_t>(buffer->info().bufferUsage);
	buffer_usage_t constexpr INDEX_BUFFER_USAGE = static_cast<buffer_usage_t>(BufferUsage::Index);

	if (usage & INDEX_BUFFER_USAGE)
	{
		VkIndexType type = VK_INDEX_TYPE_UINT32;

		if (info.indexType == IndexType::Uint_16)
		{
			type = VK_INDEX_TYPE_UINT16;
		}
		else if (info.indexType == IndexType::Uint_8)
		{
			type = VK_INDEX_TYPE_UINT8_EXT;
		}
		vkCmdBindIndexBuffer(m_impl.handle, buffer->m_impl.handle, info.offset, type);
	}
}

auto CommandBuffer::bind_push_constant(BindPushConstantInfo const& info) -> void
{
	if (!valid() || !is_recording())
	{
		return;
	}

	flush_barriers();

	uint32 sz = static_cast<uint32>(info.size);
	uint32 off = static_cast<uint32>(info.offset);

	uint32 const MAX_PUSH_CONSTANT_SIZE = m_device->m_config.pushConstantMaxSize;

	ASSERTION(sz <= MAX_PUSH_CONSTANT_SIZE && "Constant supplied exceeded maximum size allowed by the driver.");
	ASSERTION(off <= MAX_PUSH_CONSTANT_SIZE && "Offset supplied exceeded maximum push constant size allowed by the driver.");
	ASSERTION((sz % 4 == 0) && "Size must be a multiple of 4.");
	ASSERTION((off % 4 == 0) && "Offset must be a multiple of 4.");

	VkPipelineLayout layout = m_device->m_apiContext.descriptorCache.pipelineLayouts[(sz + 3u) & (~0x03u)];
	VkShaderStageFlags shaderStageFlags = api::translate_shader_stage_flags(info.shaderStage);

	vkCmdPushConstants(m_impl.handle, layout, shaderStageFlags, off, sz, info.data);
}

auto CommandBuffer::bind_pipeline(Resource<Pipeline> pipeline) -> void
{
	if (!valid() || !pipeline.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	api::Pipeline const& pipelineResource = pipeline->m_impl;
	PipelineType const pipelineType = pipeline->type();

	VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	
	if (pipelineType == PipelineType::Compute)
	{
		pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	}
	else if (pipelineType == PipelineType::Ray_Tracing)
	{
		pipelineBindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
	}

	vkCmdBindDescriptorSets(
		m_impl.handle,
		pipelineBindPoint,
		pipelineResource.layout,
		0,
		1,
		&m_device->m_apiContext.descriptorCache.descriptorSet,
		0,
		nullptr
	);

	vkCmdBindPipeline(m_impl.handle, pipelineBindPoint, pipelineResource.handle);
}

auto CommandBuffer::pipeline_barrier(MemoryBarrierInfo const& barrier) -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	if (m_impl.numMemoryBarrier >= api::CommandBuffer::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	m_impl.memoryBarriers[m_impl.numMemoryBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = api::translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = api::translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = api::translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = api::translate_memory_access_flags(barrier.dstAccess.type),
	};
}

auto CommandBuffer::pipeline_barrier(Resource<Buffer> buffer, BufferBarrierInfo const& barrier) -> void
{
	if (!valid() || !buffer.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	if (m_impl.numBufferBarrier >= api::CommandBuffer::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueue::Main:
		srcQueueIndex = m_device->m_apiContext.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		srcQueueIndex = m_device->m_apiContext.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		srcQueueIndex = m_device->m_apiContext.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueue::Main:
		dstQueueIndex = m_device->m_apiContext.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		dstQueueIndex = m_device->m_apiContext.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		dstQueueIndex = m_device->m_apiContext.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	m_impl.bufferBarriers[m_impl.numBufferBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.srcStageMask = api::translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = api::translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = api::translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = api::translate_memory_access_flags(barrier.dstAccess.type),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.buffer = buffer->m_impl.handle,
		.offset = barrier.offset,
		.size = (barrier.size == std::numeric_limits<size_t>::max()) ? buffer->info().size : barrier.size,
	};
}

auto CommandBuffer::pipeline_barrier(Resource<Image> image, ImageBarrierInfo const& barrier) -> void
{
	if (!valid() || !image.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	if (m_impl.numImageBarrier >= api::CommandBuffer::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueue::Main:
		srcQueueIndex = m_device->m_apiContext.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		srcQueueIndex = m_device->m_apiContext.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		srcQueueIndex = m_device->m_apiContext.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueue::Main:
		dstQueueIndex = m_device->m_apiContext.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		dstQueueIndex = m_device->m_apiContext.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		dstQueueIndex = m_device->m_apiContext.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	m_impl.imageBarriers[m_impl.numImageBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = api::translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = api::translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = api::translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = api::translate_memory_access_flags(barrier.dstAccess.type),
		.oldLayout = api::translate_image_layout(barrier.oldLayout),
		.newLayout = api::translate_image_layout(barrier.newLayout),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.image = image->m_impl.handle,
		.subresourceRange = {
			.aspectMask = api::translate_image_aspect_flags(barrier.subresource.aspectFlags),
			.baseMipLevel = barrier.subresource.mipLevel,
			.levelCount = barrier.subresource.levelCount,
			.baseArrayLayer = barrier.subresource.baseArrayLayer,
			.layerCount = barrier.subresource.layerCount
		}
	};
}

auto CommandBuffer::flush_barriers() -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	if (m_impl.numMemoryBarrier > 0 ||
		m_impl.numBufferBarrier > 0 ||
		m_impl.numImageBarrier > 0)
	{
		VkDependencyInfo dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.memoryBarrierCount = static_cast<uint32>(m_impl.numMemoryBarrier),
			.bufferMemoryBarrierCount = static_cast<uint32>(m_impl.numBufferBarrier),
			.imageMemoryBarrierCount = static_cast<uint32>(m_impl.numImageBarrier)
		};

		if (m_impl.numMemoryBarrier)
		{
			dependencyInfo.pMemoryBarriers = m_impl.memoryBarriers.data();
		}

		if (m_impl.numBufferBarrier)
		{
			dependencyInfo.pBufferMemoryBarriers = m_impl.bufferBarriers.data();
		}

		if (m_impl.numImageBarrier)
		{
			dependencyInfo.pImageMemoryBarriers = m_impl.imageBarriers.data();
		}

		vkCmdPipelineBarrier2(m_impl.handle, &dependencyInfo);

		m_impl.numMemoryBarrier = 0;
		m_impl.numBufferBarrier = 0;
		m_impl.numImageBarrier = 0;
	}
}

auto CommandBuffer::begin_rendering(RenderingInfo const& info) -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	VkRenderingAttachmentInfo depthAttachmentInfo = {};
	VkRenderingAttachmentInfo stencilAttachmentInfo = {};

	VkRenderingInfo renderingInfo{
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
		.renderArea = {
			.offset = {
				.x = info.renderArea.offset.x,
				.y = info.renderArea.offset.y
			},
			.extent = {
				.width = info.renderArea.extent.width,
				.height = info.renderArea.extent.height
			}
		},
		.layerCount = 1u,
		.colorAttachmentCount = static_cast<uint32>(info.colorAttachments.size()),
	};

	std::array<VkRenderingAttachmentInfo, MAX_COMMAND_BUFFER_ATTACHMENT> colorAttachments;

	if (info.colorAttachments.size())
	{
		for (size_t i = 0; RenderAttachment const& attachment : info.colorAttachments)
		{
			if (attachment.image.valid())
			{
				Color<float32> clearColor = attachment.image->info().clearValue.color.f32;

				colorAttachments[i] = {
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = attachment.image->m_impl.imageView,
					.imageLayout = api::translate_image_layout(attachment.imageLayout),
					.resolveMode = VK_RESOLVE_MODE_NONE,
					.resolveImageView = VK_NULL_HANDLE,
					.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.loadOp = api::translate_attachment_load_op(attachment.loadOp),
					.storeOp = api::translate_attachment_store_op(attachment.storeOp),
					.clearValue = {
						.color = {
							.float32 = {
								clearColor.r,
								clearColor.g,
								clearColor.b,
								clearColor.a
							}
						}
					}
				};
			}
			++i;
		}
		renderingInfo.pColorAttachments = colorAttachments.data();
	}

	if (info.depthAttachment && info.depthAttachment->image.valid())
	{
		Resource<Image> const& depthAttachment = info.depthAttachment->image;

		depthAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depthAttachment->m_impl.imageView,
			.imageLayout = api::translate_image_layout(info.depthAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = api::translate_attachment_load_op(info.depthAttachment->loadOp),
			.storeOp = api::translate_attachment_store_op(info.depthAttachment->storeOp),
			.clearValue = {
				.depthStencil = {
					.depth = depthAttachment->info().clearValue.depthStencil.depth 
				}
			}
		};
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	}

	if (info.stencilAttachment && info.stencilAttachment->image.valid())
	{
		Resource<Image> const& stencilAttachment = info.stencilAttachment->image;

		stencilAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = stencilAttachment->m_impl.imageView,
			.imageLayout = api::translate_image_layout(info.stencilAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = api::translate_attachment_load_op(info.stencilAttachment->loadOp),
			.storeOp = api::translate_attachment_store_op(info.stencilAttachment->storeOp),
			.clearValue = {
				.depthStencil = {
					.stencil = stencilAttachment->info().clearValue.depthStencil.stencil 
				}
			}
		};
		renderingInfo.pStencilAttachment = &stencilAttachmentInfo;
	}

	vkCmdBeginRendering(m_impl.handle, &renderingInfo);
}

auto CommandBuffer::end_rendering() -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	vkCmdEndRendering(m_impl.handle);
}

auto CommandBuffer::copy_buffer_to_buffer(Resource<Buffer> src, Resource<Buffer> dst, BufferCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid() || !is_recording())
	{
		return;
	}

	flush_barriers();

	VkBufferCopy bufferCopy{
		.srcOffset = info.srcOffset,
		.dstOffset = info.dstOffset,
		.size = info.size
	};

	ASSERTION(info.size);

	vkCmdCopyBuffer(m_impl.handle, src->m_impl.handle, dst->m_impl.handle, 1, &bufferCopy);
}

auto CommandBuffer::copy_buffer_to_image(Resource<Buffer> src, Resource<Image> dst, BufferImageCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	VkImageLayout layout = api::translate_image_layout(info.dstImageLayout);
	VkImageAspectFlags imageAspect = api::translate_image_aspect_flags(info.imageSubresource.aspectFlags);

	VkBufferImageCopy imageCopy{
		.bufferOffset = info.bufferOffset,
		.bufferRowLength = 0u,
		.bufferImageHeight = 0u,
		.imageSubresource = {
			.aspectMask = imageAspect,
			.mipLevel = info.imageSubresource.mipLevel,
			.baseArrayLayer = info.imageSubresource.baseArrayLayer,
			.layerCount = info.imageSubresource.layerCount,
		},
		.imageOffset = {
			.x = info.imageOffset.x,
			.y = info.imageOffset.y,
			.z = info.imageOffset.z,
		},
		.imageExtent = {
			.width = info.imageExtent.width,
			.height = info.imageExtent.height,
			.depth = info.imageExtent.depth
		}
	};

	vkCmdCopyBufferToImage(m_impl.handle, src->m_impl.handle, dst->m_impl.handle, layout, 1, &imageCopy);
}

auto CommandBuffer::copy_image_to_buffer(Resource<Image> src, Resource<Buffer> dst, ImageBufferCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	VkImageLayout layout = api::translate_image_layout(info.srcImageLayout);
	VkImageAspectFlags imageAspect = api::translate_image_aspect_flags(info.imageSubresource.aspectFlags);

	VkBufferImageCopy imageCopy{
		.bufferOffset = info.bufferOffset,
		.bufferRowLength = 0u,
		.bufferImageHeight = 0u,
		.imageSubresource = {
			.aspectMask = imageAspect,
			.mipLevel = info.imageSubresource.mipLevel,
			.baseArrayLayer = info.imageSubresource.baseArrayLayer,
			.layerCount = info.imageSubresource.layerCount,
		},
		.imageOffset = {
			.x = info.imageOffset.x,
			.y = info.imageOffset.y,
			.z = info.imageOffset.z,
		},
		.imageExtent = {
			.width = info.imageExtent.width,
			.height = info.imageExtent.height,
			.depth = info.imageExtent.depth
		}
	};

	vkCmdCopyImageToBuffer(m_impl.handle, src->m_impl.handle, layout, dst->m_impl.handle, 1, &imageCopy);
}

auto CommandBuffer::copy_image_to_image(Resource<Image> src, Resource<Image> dst, ImageCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	VkImageLayout srcLayout = api::translate_image_layout(info.srcImageLayout);
	VkImageLayout dstLayout = api::translate_image_layout(info.dstImageLayout);

	VkImageAspectFlags srcAspect = api::translate_image_aspect_flags(info.srcSubresource.aspectFlags);
	VkImageAspectFlags dstAspect = api::translate_image_aspect_flags(info.dstSubresource.aspectFlags);

	VkImageCopy imageCopy{
		.srcSubresource = {
			.aspectMask = srcAspect,
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount,
		},
		.srcOffset = {
			.x = info.srcOffset.x,
			.y = info.srcOffset.y,
			.z = info.srcOffset.z,
		},
		.dstSubresource = {
			.aspectMask = dstAspect,
			.mipLevel = info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount = info.dstSubresource.layerCount,
		},
		.dstOffset = {
			.x = info.dstOffset.x,
			.y = info.dstOffset.y,
			.z = info.dstOffset.z,
		},
		.extent = {
			.width = info.extent.width,
			.height = info.extent.height,
			.depth = info.extent.depth
		},
	};

	vkCmdCopyImage(m_impl.handle, src->m_impl.handle, srcLayout, dst->m_impl.handle, dstLayout, 1, &imageCopy);
}

auto CommandBuffer::blit_image(Resource<Image> src, Resource<Image> dst, ImageBlitInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask = api::translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{ .x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{ .x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask = api::translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel = info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount = info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{ .x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{ .x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = api::translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = api::translate_image_layout(info.dstImageLayout);
	VkFilter const filter = api::translate_texel_filter(info.filter);

	vkCmdBlitImage(m_impl.handle, src->m_impl.handle, srcImgLayout, dst->m_impl.handle, dstImgLayout, 1, &region, filter);
}

auto CommandBuffer::blit_image(Resource<Image> src, Resource<Swapchain> dst, ImageBlitInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	Resource<Image> dstImage = dst->current_image();

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask = api::translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{ .x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{ .x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask = api::translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel = info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount = info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{ .x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{ .x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = api::translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = api::translate_image_layout(info.dstImageLayout);
	VkFilter const filter = api::translate_texel_filter(info.filter);

	vkCmdBlitImage(m_impl.handle, src->m_impl.handle, srcImgLayout, dstImage->m_impl.handle, dstImgLayout, 1, &region, filter);
}

auto CommandBuffer::set_viewport(Viewport const& viewport) -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	VkViewport vp{
		.x = viewport.x,
		.y = viewport.y,
		.width = viewport.width,
		.height = viewport.height,
		.minDepth = viewport.minDepth,
		.maxDepth = viewport.maxDepth,
	};
	vkCmdSetViewport(m_impl.handle, 0, 1, &vp);
}

auto CommandBuffer::set_scissor(Rect2D const& rect) -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	VkRect2D scissor{
		.offset = {
			.x = rect.offset.x,
			.y = rect.offset.y
		},
		.extent = {
			.width = rect.extent.width,
			.height = rect.extent.height
		}
	};

	vkCmdSetScissor(m_impl.handle, 0, 1, &scissor);
}

auto CommandBuffer::begin_debug_label(DebugLabelInfo const& info) const -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	VkDebugUtilsLabelEXT debugLabel{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pLabelName = info.name.data(),
		.color = {
			info.color[0],
			info.color[1],
			info.color[2],
			info.color[3]
		}
	};

	vkCmdBeginDebugUtilsLabelEXT(m_impl.handle, &debugLabel);
}

auto CommandBuffer::end_debug_label() const -> void
{
	if (!valid() || !is_recording()) [[unlikely]]
	{
		return;
	}

	vkCmdEndDebugUtilsLabelEXT(m_impl.handle);
}

auto CommandBuffer::from(Resource<CommandPool> commandPool, CommandBufferInfo&& info) -> Resource<CommandBuffer>
{
	auto& gpuResourcePool = commandPool->device().m_gpuResourcePool;
	command_pool_id id = commandPool.id();

	if (!gpuResourcePool.commandBufferPools.contains(id))
	{
		gpuResourcePool.commandBufferPools.emplace(CommandBufferPool{});
	}

	CommandBufferPool& cmdBufferPool = gpuResourcePool.commandBufferPools[id];

	if (cmdBufferPool.commandBufferCount + 1 > MAX_COMMAND_BUFFER_PER_POOL)
	{
		return null_resource;
	}

	size_t index = cmdBufferPool.commandBufferCount;

	if (cmdBufferPool.freeSlotCount)
	{
		index = cmdBufferPool.freeSlots[cmdBufferPool.currentFreeSlot];
		cmdBufferPool.currentFreeSlot = (cmdBufferPool.currentFreeSlot - 1) % MAX_COMMAND_BUFFER_PER_POOL;

		--cmdBufferPool.freeSlotCount;
	}
	else
	{
		++cmdBufferPool.commandBufferCount;
	}

	CommandBuffer& cmdBuffer = cmdBufferPool.commandBuffers[index];

	new (&cmdBuffer) CommandBuffer{ commandPool->device() };

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool->m_impl.handle,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	
	VkResult result = vkAllocateCommandBuffers(cmdBuffer.m_device->m_apiContext.device, &allocateInfo, &cmdBuffer.m_impl.handle);

	if (result != VK_SUCCESS)
	{
		destroy(cmdBuffer, index);

		return null_resource;
	}

	if (info.name.size())
	{
		info.name.format("<command_buffer>:{}", info.name.c_str());
	}

	auto completionTimelineName = lib::format("<fence>:{}:completion_timeline", info.name.c_str());

	cmdBuffer.m_info = std::move(info);
	cmdBuffer.m_completionTimeline = Fence::from(commandPool->device(), { .name = std::move(completionTimelineName), .initialValue = 0});
	cmdBuffer.m_commandPool = commandPool;

	return Resource{ index, cmdBuffer };
}

auto CommandBuffer::destroy(CommandBuffer const& resource, id_type id) -> void
{
	Device& device = resource.device();

	command_pool_id cmdPoolId = resource.m_commandPool.id();

	auto& cmdBufferPool =  device.m_gpuResourcePool.commandBufferPools[cmdPoolId];

	if (&resource >= cmdBufferPool.commandBuffers.data() &&
		&resource < cmdBufferPool.commandBuffers.data() + MAX_COMMAND_BUFFER_PER_POOL)
	{
		uint64 const cpuTimelineValue = device.cpu_timeline();

		std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

		device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = static_cast<size_t>(id), .type = resource_type_id_v<CommandBuffer> });
	}
}

CommandBuffer::CommandBuffer(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{},
	m_completionTimeline{},
	m_recordingTimeline{},
	m_state{ CommandBufferState::Initial }
{}
}