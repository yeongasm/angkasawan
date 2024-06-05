#include "vulkan/vkgpu.h"

/**
* Vulkan Command Buffers Specification
* https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap6.html
*/

namespace gpu
{
CommandBuffer::CommandBuffer(Device& device) :
	DeviceResource{ device },
	m_info{},
	m_commandPool{},
	m_recordingTimeline{}//,
	//m_completionTimeline{},
	//m_recordingTimeline{},
	//m_state{ CommandBufferState::Initial }
{}

auto CommandBuffer::info() const -> CommandBufferInfo const&
{
	return m_info;
}

auto CommandBuffer::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

//auto CommandBuffer::is_initial() const -> bool
//{
//	return m_state == CommandBufferState::Initial;
//}
//
//auto CommandBuffer::is_recording() const -> bool
//{
//	return m_state == CommandBufferState::Recording;
//}
//
//auto CommandBuffer::is_executable() const -> bool
//{
//	return m_state == CommandBufferState::Executable;
//}
//
//auto CommandBuffer::is_pending_complete() const -> bool
//{
//	return m_state == CommandBufferState::Pending;
//}
//
//auto CommandBuffer::is_completed() -> bool
//{
//	uint64 const completionTimeline = m_completionTimeline->value();
//
//	if (completionTimeline >= m_recordingTimeline)
//	{
//		m_state = CommandBufferState::Executable;
//	}
//
//	return completionTimeline >= m_recordingTimeline;
//}
//
//auto CommandBuffer::is_invalid() const -> bool
//{
//	return m_state == CommandBufferState::Invalid;
//}
//
//auto CommandBuffer::current_state() const -> CommandBufferState
//{
//	return m_state;
//}
//
auto CommandBuffer::recording_timeline() const -> uint64
{
	return m_recordingTimeline.load(std::memory_order_acquire);
}
//
//auto CommandBuffer::completion_timeline() const -> uint64
//{
//	return m_completionTimeline->value();
//}

auto CommandBuffer::reset() -> void
{
	if (valid() && 
		recording_timeline() < m_device->gpu_timeline())
	{
		auto&& self = to_impl(*this);

		vkResetCommandBuffer(self.handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}
}

auto CommandBuffer::begin() -> bool
{
	if (!valid()) [[unlikely]]
	{
		return false;
	}

	auto&& self = to_impl(*this);

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	if (vkBeginCommandBuffer(self.handle, &beginInfo) == VK_SUCCESS)
	{
		m_recordingTimeline.fetch_add(1, std::memory_order_relaxed);

		return true;
	}

	return false;
}

auto CommandBuffer::end() -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto&& self = to_impl(*this);

	vkEndCommandBuffer(self.handle);
}

auto CommandBuffer::clear(Image& image) -> void
{
	return clear(image, { .dstImageLayout = ImageLayout::General, .clearValue = image.info().clearValue });
}

auto CommandBuffer::clear(Image& image, ImageClearInfo const& info) -> void
{
	if (!valid() || !image.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	ClearValue imgClearValue = image.info().clearValue;

	auto const& self = to_impl(*this);
	auto const& img = to_impl(image);

	VkImageSubresourceRange subResourceRange{
		.aspectMask = vk::translate_image_aspect_flags(info.subresource.aspectFlags),
		.baseMipLevel = info.subresource.mipLevel,
		.levelCount = info.subresource.levelCount,
		.baseArrayLayer = info.subresource.baseArrayLayer,
		.layerCount = info.subresource.layerCount
	};

	if (!is_color_format(image.info().format))
	{
		VkClearDepthStencilValue depthStencilValue{
			.depth = imgClearValue.depthStencil.depth,
			.stencil = imgClearValue.depthStencil.stencil
		};
		vkCmdClearDepthStencilImage(
			self.handle,
			img.handle,
			vk::translate_image_layout(info.dstImageLayout),
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
			self.handle,
			img.handle,
			vk::translate_image_layout(info.dstImageLayout),
			&colorValue,
			1,
			&subResourceRange
		);
	}
}

auto CommandBuffer::clear(Buffer& buffer) -> void
{
	size_t const size = buffer.valid() ? buffer.info().size : 0;
	clear(buffer, { .offset = 0, .size = size, .data = 0u });
}

auto CommandBuffer::clear(Buffer& buffer, BufferClearInfo const& info) -> void
{
	if (!valid() || !buffer.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto&& self = to_impl(*this);
	auto const& buf = to_impl(buffer);

	vkCmdFillBuffer(
		self.handle,
		buf.handle,
		static_cast<VkDeviceSize>(info.offset),
		static_cast<VkDeviceSize>(info.size),
		info.data
	);
}

auto CommandBuffer::draw(DrawInfo const& info) const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto&& self = to_impl(*this);

	vkCmdDraw(self.handle, info.vertexCount, info.instanceCount, info.firstVertex, info.firstInstance);
}

auto CommandBuffer::draw_indexed(DrawIndexedInfo const& info) const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto&& self = to_impl(*this);

	vkCmdDrawIndexed(self.handle, info.indexCount, info.instanceCount, info.firstIndex, info.vertexOffset, info.firstInstance);
}

auto CommandBuffer::draw_indirect(Buffer& drawInfoBuffer, DrawIndirectInfo const& info) const -> void
{
	if (!valid() || !drawInfoBuffer.valid()) [[unlikely]]
	{
		return;
	}

	auto&& self = to_impl(*this);
	auto const& buf = to_impl(drawInfoBuffer);

	if (info.indexed)
	{
		vkCmdDrawIndexedIndirect(
			self.handle,
			buf.handle,
			static_cast<VkDeviceSize>(info.offset),
			info.drawCount,
			info.stride
		);
	}
	else
	{
		vkCmdDrawIndirect(
			self.handle,
			buf.handle,
			static_cast<VkDeviceSize>(info.offset),
			info.drawCount,
			info.stride
		);
	}
}

auto CommandBuffer::draw_indirect_count(Buffer& drawInfoBuffer, Buffer& drawCountBuffer, DrawIndirectCountInfo const& info) const -> void
{
	if (!valid() || !drawInfoBuffer.valid() || !drawCountBuffer.valid()) [[unlikely]]
	{
		return;
	}

	auto&& self = to_impl(*this);
	auto const& drawInfoBuf = to_impl(drawInfoBuffer);
	auto const& countBuf = to_impl(drawCountBuffer);

	if (info.indexed)
	{
		vkCmdDrawIndexedIndirectCount(
			self.handle,
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
			self.handle,
			drawInfoBuf.handle,
			static_cast<VkDeviceSize>(info.offset),
			countBuf.handle,
			static_cast<VkDeviceSize>(info.countBufferOffset),
			info.maxDrawCount,
			info.stride
		);
	}
}

auto CommandBuffer::bind_vertex_buffer(Buffer& buffer, BindVertexBufferInfo const& info) -> void
{
	if (!valid() || !buffer.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& buf = to_impl(buffer);

	using buffer_usage_t = std::underlying_type_t<BufferUsage>;

	buffer_usage_t const usage = static_cast<buffer_usage_t>(buffer.info().bufferUsage);
	buffer_usage_t constexpr VERTEX_BUFFER_USAGE = static_cast<buffer_usage_t>(BufferUsage::Vertex);

	if (usage & VERTEX_BUFFER_USAGE)
	{
		vkCmdBindVertexBuffers(self.handle, info.firstBinding, 1, &buf.handle, &info.offset);
	}
}

auto CommandBuffer::bind_index_buffer(Buffer& buffer, BindIndexBufferInfo const& info) -> void
{
	if (!valid() || !buffer.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& buf = to_impl(buffer);

	using buffer_usage_t = std::underlying_type_t<BufferUsage>;

	buffer_usage_t const usage = static_cast<buffer_usage_t>(buffer.info().bufferUsage);
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
		vkCmdBindIndexBuffer(self.handle, buf.handle, info.offset, type);
	}
}

auto CommandBuffer::bind_push_constant(BindPushConstantInfo const& info) -> void
{
	if (!valid())
	{
		return;
	}

	flush_barriers();

	auto&& vkdevice = to_device(m_device);
	auto const& self = to_impl(*this);

	uint32 sz = static_cast<uint32>(info.size);
	uint32 off = static_cast<uint32>(info.offset);

	uint32 const MAX_PUSH_CONSTANT_SIZE = m_device->config().pushConstantMaxSize;

	ASSERTION(sz <= MAX_PUSH_CONSTANT_SIZE && "Constant supplied exceeded maximum size allowed by the driver.");
	ASSERTION(off <= MAX_PUSH_CONSTANT_SIZE && "Offset supplied exceeded maximum push constant size allowed by the driver.");
	ASSERTION((sz % 4 == 0) && "Size must be a multiple of 4.");
	ASSERTION((off % 4 == 0) && "Offset must be a multiple of 4.");

	VkPipelineLayout layout = vkdevice.descriptorCache.pipelineLayouts[(sz + 3u) & (~0x03u)];
	VkShaderStageFlags shaderStageFlags = vk::translate_shader_stage_flags(info.shaderStage);

	vkCmdPushConstants(self.handle, layout, shaderStageFlags, off, sz, info.data);
}

auto CommandBuffer::bind_pipeline(Pipeline& pipeline) -> void
{
	if (!valid() || !pipeline.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto&& vkdevice = to_device(m_device);
	auto&& self = to_impl(*this);
	auto const& pipelineResource = to_impl(pipeline);

	PipelineType const pipelineType = pipeline.type();

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
		self.handle,
		pipelineBindPoint,
		pipelineResource.layout,
		0,
		1,
		&vkdevice.descriptorCache.descriptorSet,
		0,
		nullptr
	);

	vkCmdBindPipeline(self.handle, pipelineBindPoint, pipelineResource.handle);
}

auto CommandBuffer::pipeline_barrier(MemoryBarrierInfo const& barrier) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto&& self = to_impl(*this);

	if (self.numMemoryBarrier >= vk::CommandBufferImpl::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	self.memoryBarriers[self.numMemoryBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = vk::translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = vk::translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = vk::translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = vk::translate_memory_access_flags(barrier.dstAccess.type),
	};
}

auto CommandBuffer::pipeline_barrier(Buffer& buffer, BufferBarrierInfo const& barrier) -> void
{
	if (!valid() || !buffer.valid()) [[unlikely]]
	{
		return;
	}

	auto&& vkdevice = to_device(m_device);
	auto&& self = to_impl(*this);
	auto const& buf = to_impl(buffer);

	if (self.numBufferBarrier >= vk::CommandBufferImpl::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueue::Main:
		srcQueueIndex = vkdevice.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		srcQueueIndex = vkdevice.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		srcQueueIndex = vkdevice.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueue::Main:
		dstQueueIndex = vkdevice.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		dstQueueIndex = vkdevice.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		dstQueueIndex = vkdevice.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	self.bufferBarriers[self.numBufferBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.srcStageMask = vk::translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = vk::translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = vk::translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = vk::translate_memory_access_flags(barrier.dstAccess.type),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.buffer = buf.handle,
		.offset = barrier.offset,
		.size = (barrier.size == std::numeric_limits<size_t>::max()) ? buffer.info().size : barrier.size,
	};
}

auto CommandBuffer::pipeline_barrier(Image& image, ImageBarrierInfo const& barrier) -> void
{
	if (!valid() || !image.valid()) [[unlikely]]
	{
		return;
	}

	auto&& vkdevice = to_device(m_device);
	auto&& self = to_impl(*this);
	auto const& img = to_impl(image);

	if (self.numImageBarrier >= vk::CommandBufferImpl::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueue::Main:
		srcQueueIndex = vkdevice.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		srcQueueIndex = vkdevice.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		srcQueueIndex = vkdevice.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueue::Main:
		dstQueueIndex = vkdevice.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		dstQueueIndex = vkdevice.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		dstQueueIndex = vkdevice.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	self.imageBarriers[self.numImageBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = vk::translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = vk::translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = vk::translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = vk::translate_memory_access_flags(barrier.dstAccess.type),
		.oldLayout = vk::translate_image_layout(barrier.oldLayout),
		.newLayout = vk::translate_image_layout(barrier.newLayout),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.image = img.handle,
		.subresourceRange = {
			.aspectMask = vk::translate_image_aspect_flags(barrier.subresource.aspectFlags),
			.baseMipLevel = barrier.subresource.mipLevel,
			.levelCount = barrier.subresource.levelCount,
			.baseArrayLayer = barrier.subresource.baseArrayLayer,
			.layerCount = barrier.subresource.layerCount
		}
	};
}

auto CommandBuffer::flush_barriers() -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto&& self = to_impl(*this);

	if (self.numMemoryBarrier > 0 ||
		self.numBufferBarrier > 0 ||
		self.numImageBarrier > 0)
	{
		VkDependencyInfo dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.memoryBarrierCount = static_cast<uint32>(self.numMemoryBarrier),
			.bufferMemoryBarrierCount = static_cast<uint32>(self.numBufferBarrier),
			.imageMemoryBarrierCount = static_cast<uint32>(self.numImageBarrier)
		};

		if (self.numMemoryBarrier)
		{
			dependencyInfo.pMemoryBarriers = self.memoryBarriers.data();
		}

		if (self.numBufferBarrier)
		{
			dependencyInfo.pBufferMemoryBarriers = self.bufferBarriers.data();
		}

		if (self.numImageBarrier)
		{
			dependencyInfo.pImageMemoryBarriers = self.imageBarriers.data();
		}

		vkCmdPipelineBarrier2(self.handle, &dependencyInfo);

		self.numMemoryBarrier = 0;
		self.numBufferBarrier = 0;
		self.numImageBarrier = 0;
	}
}

auto CommandBuffer::begin_rendering(RenderingInfo const& info) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto&& self = to_impl(*this);

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

	if (!info.colorAttachments.empty())
	{
		for (size_t i = 0; RenderAttachment const& attachment : info.colorAttachments)
		{
			if (attachment.image.valid())
			{
				auto&& image = to_impl(*attachment.image);

				Color<float32> clearColor = attachment.image->info().clearValue.color.f32;

				colorAttachments[i] = {
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = image.imageView,
					.imageLayout = vk::translate_image_layout(attachment.imageLayout),
					.resolveMode = VK_RESOLVE_MODE_NONE,
					.resolveImageView = VK_NULL_HANDLE,
					.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.loadOp = vk::translate_attachment_load_op(attachment.loadOp),
					.storeOp = vk::translate_attachment_store_op(attachment.storeOp),
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
		auto const& depthImage = to_impl(*info.depthAttachment->image);

		depthAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depthImage.imageView,
			.imageLayout = vk::translate_image_layout(info.depthAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = vk::translate_attachment_load_op(info.depthAttachment->loadOp),
			.storeOp = vk::translate_attachment_store_op(info.depthAttachment->storeOp),
			.clearValue = {
				.depthStencil = {
					.depth = depthImage.info().clearValue.depthStencil.depth
				}
			}
		};
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	}

	if (info.stencilAttachment && info.stencilAttachment->image.valid())
	{
		Resource<Image> const& stencilAttachment = info.stencilAttachment->image;
		auto&& stencilImage = to_impl(*stencilAttachment);

		stencilAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = stencilImage.imageView,
			.imageLayout = vk::translate_image_layout(info.stencilAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = vk::translate_attachment_load_op(info.stencilAttachment->loadOp),
			.storeOp = vk::translate_attachment_store_op(info.stencilAttachment->storeOp),
			.clearValue = {
				.depthStencil = {
					.stencil = stencilAttachment->info().clearValue.depthStencil.stencil
				}
			}
		};
		renderingInfo.pStencilAttachment = &stencilAttachmentInfo;
	}

	vkCmdBeginRendering(self.handle, &renderingInfo);
}

auto CommandBuffer::end_rendering() -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto&& self = to_impl(*this);

	vkCmdEndRendering(self.handle);
}

auto CommandBuffer::copy_buffer_to_buffer(Buffer& src, Buffer& dst, BufferCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid())
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& srcBuf = to_impl(src);
	auto const& dstBuf = to_impl(dst);

	VkBufferCopy bufferCopy{
		.srcOffset = info.srcOffset,
		.dstOffset = info.dstOffset,
		.size = info.size
	};

	ASSERTION(info.size);

	vkCmdCopyBuffer(self.handle, srcBuf.handle, dstBuf.handle, 1, &bufferCopy);
}

auto CommandBuffer::copy_buffer_to_image(Buffer& src, Image& dst, BufferImageCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& srcBuf = to_impl(src);
	auto const& dstImg = to_impl(dst);

	VkImageLayout layout = vk::translate_image_layout(info.dstImageLayout);
	VkImageAspectFlags imageAspect = vk::translate_image_aspect_flags(info.imageSubresource.aspectFlags);

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

	vkCmdCopyBufferToImage(self.handle, srcBuf.handle, dstImg.handle, layout, 1, &imageCopy);
}

auto CommandBuffer::copy_image_to_buffer(Image& src, Buffer& dst, ImageBufferCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& srcImg = to_impl(src);
	auto const& dstBuf = to_impl(dst);

	VkImageLayout layout = vk::translate_image_layout(info.srcImageLayout);
	VkImageAspectFlags imageAspect = vk::translate_image_aspect_flags(info.imageSubresource.aspectFlags);

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

	vkCmdCopyImageToBuffer(self.handle, srcImg.handle, layout, dstBuf.handle, 1, &imageCopy);
}

auto CommandBuffer::copy_image_to_image(Image& src, Image& dst, ImageCopyInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& srcImg = to_impl(src);
	auto const& dstImg = to_impl(dst);

	VkImageLayout srcLayout = vk::translate_image_layout(info.srcImageLayout);
	VkImageLayout dstLayout = vk::translate_image_layout(info.dstImageLayout);

	VkImageAspectFlags srcAspect = vk::translate_image_aspect_flags(info.srcSubresource.aspectFlags);
	VkImageAspectFlags dstAspect = vk::translate_image_aspect_flags(info.dstSubresource.aspectFlags);

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

	vkCmdCopyImage(self.handle, srcImg.handle, srcLayout, dstImg.handle, dstLayout, 1, &imageCopy);
}

auto CommandBuffer::blit_image(Image& src, Image& dst, ImageBlitInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& srcImg = to_impl(src);
	auto const& dstImg = to_impl(dst);

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask = vk::translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{.x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{.x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask = vk::translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel = info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount = info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{.x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{.x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = vk::translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = vk::translate_image_layout(info.dstImageLayout);
	VkFilter const filter = vk::translate_texel_filter(info.filter);

	vkCmdBlitImage(self.handle, srcImg.handle, srcImgLayout, dstImg.handle, dstImgLayout, 1, &region, filter);
}

auto CommandBuffer::blit_image(Image& src, Swapchain& dst, ImageBlitInfo const& info) -> void
{
	if (!valid() || !src.valid() || !dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);
	auto const& srcImg = to_impl(src);
	auto const& dstImg = to_impl(*dst.current_image());

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask = vk::translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{.x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{.x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask = vk::translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel = info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount = info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{.x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{.x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = vk::translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = vk::translate_image_layout(info.dstImageLayout);
	VkFilter const filter = vk::translate_texel_filter(info.filter);

	vkCmdBlitImage(self.handle, srcImg.handle, srcImgLayout, dstImg.handle, dstImgLayout, 1, &region, filter);
}

auto CommandBuffer::set_viewport(Viewport const& viewport) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);

	VkViewport vp{
		.x = viewport.x,
		.y = viewport.y,
		.width = viewport.width,
		.height = viewport.height,
		.minDepth = viewport.minDepth,
		.maxDepth = viewport.maxDepth,
	};
	vkCmdSetViewport(self.handle, 0, 1, &vp);
}

auto CommandBuffer::set_scissor(Rect2D const& rect) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& self = to_impl(*this);

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

	vkCmdSetScissor(self.handle, 0, 1, &scissor);
}

auto CommandBuffer::begin_debug_label(DebugLabelInfo const& info) const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& self = to_impl(*this);

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

	vkCmdBeginDebugUtilsLabelEXT(self.handle, &debugLabel);
}

auto CommandBuffer::end_debug_label() const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& self = to_impl(*this);

	vkCmdEndDebugUtilsLabelEXT(self.handle);
}

auto CommandBuffer::from(Resource<CommandPool>& commandPool) -> Resource<CommandBuffer>
{
	auto&& vkdevice = to_device(commandPool->device());
	auto&& cmdPool = to_impl(*commandPool);

	vk::CommandBufferPool& cmdBufferPool = cmdPool.commandBufferPool;

	if (cmdBufferPool.freeSlotCount)
	{
		uint32 iterations = 0;

		while (iterations < cmdBufferPool.freeSlotCount)
		{
			 cmdBufferPool.currentFreeSlot = (cmdBufferPool.currentFreeSlot + 1) % cmdBufferPool.freeSlotCount;

			 size_t const index = cmdBufferPool.freeSlots[cmdBufferPool.currentFreeSlot];
			 vk::CommandBufferImpl& allocatedCmdBuffer = cmdBufferPool.commandBuffers[index];

			 if (allocatedCmdBuffer.valid() && 
				 allocatedCmdBuffer.recording_timeline() < vkdevice.gpu_timeline())
			 {
				 --cmdBufferPool.freeSlotCount;

				 return Resource<CommandBuffer>{ static_cast<uint64>(index), allocatedCmdBuffer };
			 }

			 ++iterations;
		}
	}

	if (cmdBufferPool.commandBufferCount + 1 > MAX_COMMAND_BUFFER_PER_POOL)
	{
		return null_resource;
	}

	size_t const index = cmdBufferPool.commandBufferCount++;

	vk::CommandBufferImpl& vkcmdbuffer = cmdBufferPool.commandBuffers[index];

	new (&vkcmdbuffer) vk::CommandBufferImpl{ vkdevice };

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = cmdPool.handle,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer handle = VK_NULL_HANDLE;

	VkResult result = vkAllocateCommandBuffers(vkdevice.device, &allocateInfo, &handle);

	if (result != VK_SUCCESS)
	{
		return null_resource;
	}

	CommandBufferInfo info{
		.name = lib::format("<command_buffer:{}>:{}", index, commandPool->info().name.c_str())
	};

	vkcmdbuffer.handle = handle;
	vkcmdbuffer.m_info = std::move(info);
	vkcmdbuffer.m_commandPool = commandPool;

	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		vkdevice.setup_debug_name(vkcmdbuffer);
	}

	return Resource<CommandBuffer>{ static_cast<uint64>(index), vkcmdbuffer };
}

auto CommandBuffer::destroy(CommandBuffer& resource, uint64 id) -> void
{
	auto&& cmdBufferImpl = to_impl(resource);
	auto&& cmdPoolImpl = to_impl(*resource.m_commandPool);

	auto& cmdBufferPool = cmdPoolImpl.commandBufferPool;

	if (&cmdBufferImpl >= cmdBufferPool.commandBuffers.data() &&
		&cmdBufferImpl < cmdBufferPool.commandBuffers.data() + MAX_COMMAND_BUFFER_PER_POOL)
	{
		cmdBufferPool.freeSlots[cmdBufferPool.currentFreeSlot] = static_cast<size_t>(id);
		++cmdBufferPool.freeSlotCount;
		cmdBufferPool.currentFreeSlot = (cmdBufferPool.currentFreeSlot + 1) % MAX_COMMAND_BUFFER_PER_POOL;
	}
}

namespace vk
{
CommandBufferImpl::CommandBufferImpl(DeviceImpl& device) :
	CommandBuffer{ device }
{}
}
}