#include <utility>
#include "vulkan/vkgpu.hpp"

/**
* Vulkan Command Buffers Specification
* https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap6.html
*/

namespace gpu
{
auto CommandRecorder::valid() const -> bool
{
	if (!m_cmdPool.valid())
	{
		return false;
	}
	auto const& pool = shared_base::impl_of(m_cmdPool);
	return m_index < pool.commandBufferPool.commandBuffers.size() && pool.commandBufferPool.commandBuffers[m_index].handle != VK_NULL_HANDLE;
}

auto CommandRecorder::recording_timeline() const -> uint64
{
	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	return self.recordingTimeline.load(std::memory_order_acquire);
}

auto CommandRecorder::current_timeline_fence() const -> Fence
{
	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	return self.gpuTimeline;
}

auto CommandRecorder::begin() -> bool
{
	if (!valid()) [[unlikely]]
	{
		return false;
	}

	auto&& pool = shared_base::impl_of(m_cmdPool);
	auto&& self = pool.commandBufferPool.commandBuffers[m_index];

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	if (vkBeginCommandBuffer(self.handle, &beginInfo) == VK_SUCCESS)
	{
		self.recordingTimeline.fetch_add(1, std::memory_order_relaxed);

		return true;
	}

	return false;
}

auto CommandRecorder::end() -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	vkEndCommandBuffer(self.handle);
}

auto CommandRecorder::clear(Image& image) -> void
{
	return clear({ .image = image, .dstImageLayout = ImageLayout::General, .clearValue = image.info().clearValue });
}

auto CommandRecorder::clear(ImageClearInfo const& info) -> void
{
	if (!valid() || !info.image.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	ClearValue imgClearValue = info.image.info().clearValue;

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	auto const& img = shared_base::impl_of(info.image);

	VkImageSubresourceRange subResourceRange{
		.aspectMask = translate_image_aspect_flags(info.subresource.aspectFlags),
		.baseMipLevel = info.subresource.mipLevel,
		.levelCount = info.subresource.levelCount,
		.baseArrayLayer = info.subresource.baseArrayLayer,
		.layerCount = info.subresource.layerCount
	};

	if (!is_color_format(info.image.info().format))
	{
		VkClearDepthStencilValue depthStencilValue{
			.depth = imgClearValue.depthStencil.depth,
			.stencil = imgClearValue.depthStencil.stencil
		};
		vkCmdClearDepthStencilImage(
			self.handle,
			img.handle,
			translate_image_layout(info.dstImageLayout),
			&depthStencilValue,
			1,
			&subResourceRange
		);
	}
	else
	{
		VkClearColorValue colorValue{};
		std::memcpy(&colorValue.uint32, &imgClearValue.color.u32, sizeof(imgClearValue.color));
		vkCmdClearColorImage(
			self.handle,
			img.handle,
			translate_image_layout(info.dstImageLayout),
			&colorValue,
			1,
			&subResourceRange
		);
	}
}

auto CommandRecorder::clear(Buffer& buffer) -> void
{
	size_t const size = buffer.valid() ? buffer.info().size : 0;
	clear({ .buffer = buffer, .offset = 0, .size = size, .data = 0u });
}

auto CommandRecorder::clear(BufferClearInfo const& info) -> void
{
	if (!valid() || !info.buffer.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	auto const& buf = shared_base::impl_of(info.buffer);

	vkCmdFillBuffer(
		self.handle,
		buf.handle,
		static_cast<VkDeviceSize>(info.offset),
		static_cast<VkDeviceSize>(info.size),
		info.data
	);
}

auto CommandRecorder::draw(DrawInfo const& info) const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	vkCmdDraw(self.handle, info.vertexCount, info.instanceCount, info.firstVertex, info.firstInstance);
}

auto CommandRecorder::draw_indexed(DrawIndexedInfo const& info) const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	vkCmdDrawIndexed(self.handle, info.indexCount, info.instanceCount, info.firstIndex, info.vertexOffset, info.firstInstance);
}

auto CommandRecorder::draw_indirect(DrawIndirectInfo const& info) const -> void
{
	if (!valid() || !info.drawInfoBuffer.valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	auto const& buf = shared_base::impl_of(info.drawInfoBuffer);

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

auto CommandRecorder::draw_indirect_count(DrawIndirectCountInfo const& info) const -> void
{
	if (!valid() || !info.drawInfoBuffer.valid() || !info.drawCountBuffer.valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	
	auto const& drawInfoBuf = shared_base::impl_of(info.drawInfoBuffer);
	auto const& countBuf = shared_base::impl_of(info.drawCountBuffer);

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

auto CommandRecorder::bind_vertex_buffer(BindVertexBufferInfo const& info) -> void
{
	if (!valid() || !info.buffer.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];	
	auto const& buf = shared_base::impl_of(info.buffer);

	auto const usage = std::to_underlying(info.buffer.info().bufferUsage);
	auto constexpr VERTEX_BUFFER_USAGE = std::to_underlying(BufferUsage::Vertex);

	if (usage & VERTEX_BUFFER_USAGE)
	{
		vkCmdBindVertexBuffers(self.handle, info.firstBinding, 1, &buf.handle, &info.offset);
	}
}

auto CommandRecorder::bind_index_buffer(BindIndexBufferInfo const& info) -> void
{
	if (!valid() || !info.buffer.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	auto const& buf = shared_base::impl_of(info.buffer);

	auto const usage = std::to_underlying(info.buffer.info().bufferUsage);
	auto constexpr INDEX_BUFFER_USAGE = std::to_underlying(BufferUsage::Index);

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

auto CommandRecorder::bind_push_constant(BindPushConstantInfo const& info) -> void
{
	if (!valid())
	{
		return;
	}

	flush_barriers();

	auto&& pool = shared_base::impl_of(m_cmdPool);
	auto&& vkdevice = static_cast<DeviceImpl&>(m_cmdPool.device());
	auto&& self = pool.commandBufferPool.commandBuffers[m_index];

	uint32 const sz 	= static_cast<uint32>(info.size);
	uint32 const off 	= static_cast<uint32>(info.offset);

	[[maybe_unused]] uint32 const MAX_PUSH_CONSTANT_SIZE = vkdevice.config().pushConstantMaxSize;

	ASSERTION(sz <= MAX_PUSH_CONSTANT_SIZE && "Constant supplied exceeded maximum size allowed by the driver.");
	ASSERTION(off <= MAX_PUSH_CONSTANT_SIZE && "Offset supplied exceeded maximum push constant size allowed by the driver.");
	ASSERTION((sz % 4 == 0) && "Size must be a multiple of 4.");
	ASSERTION((off % 4 == 0) && "Offset must be a multiple of 4.");

	VkPipelineLayout layout = vkdevice.descriptorCache.pipelineLayouts[(sz + 3u) & (~0x03u)];
	VkShaderStageFlags shaderStageFlags = translate_shader_stage_flags(info.shaderStage);

	vkCmdPushConstants(self.handle, layout, shaderStageFlags, off, sz, info.data);
}

auto CommandRecorder::bind_pipeline(Pipeline& pipeline) -> void
{
	if (!valid() || !pipeline.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& vkdevice = static_cast<DeviceImpl const&>(m_cmdPool.device());
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	
	auto const& pipelineResource = shared_base::impl_of(pipeline);

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

auto CommandRecorder::dispatch(DispatchInfo const& info) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	vkCmdDispatch(self.handle, info.x, info.y, info.z);
}

auto CommandRecorder::dispatch_indirect(DispatchIndirectInfo const& info) -> void
{
	if (!valid() || !info.dispatchInfoBuffer.valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	
	auto&& vkbuffer = shared_base::impl_of(info.dispatchInfoBuffer);

	vkCmdDispatchIndirect(self.handle, vkbuffer.handle, info.offset);
}

struct SplitBarrierDependencyInfoBuffer
{
	lib::array<VkMemoryBarrier2> vkMemoryBarriers = {};
	lib::array<VkBufferMemoryBarrier2> vkBufferBarriers = {};
	lib::array<VkImageMemoryBarrier2> vkImageBarriers = {};
};

/**
* TODO(afiq):
* Implement a more robust solution.
* Shamelessly stolen from Daxa, lpotrick will forgive me. Too lazy to think of an appropriate solution.
*/
static thread_local lib::array<SplitBarrierDependencyInfoBuffer> splitBarrierInfoBuffer = {};
static thread_local lib::array<VkDependencyInfo> dependencyInfoBuffer = {};
static thread_local lib::array<VkEvent> splitBarrierEventBuffer = {};

auto CommandRecorder::signal_event(EventSignalInfo const& info) -> void
{
	if (!valid() || !info.event.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& vkdevice = static_cast<DeviceImpl const&>(m_cmdPool.device());
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	auto&& vkevent = shared_base::impl_of(info.event);

	auto& splitBarrierInfo = splitBarrierInfoBuffer.emplace_back();

	for (auto& barrier : info.memoryBarrierInfo)
	{
		splitBarrierInfo.vkMemoryBarriers.push_back(self.get_memory_barrier_info(barrier));
	}

	for (auto& barrier : info.bufferBarrierInfo)
	{
		splitBarrierInfo.vkBufferBarriers.push_back(self.get_buffer_barrier_info(vkdevice, barrier));
	}

	for (auto& barrier : info.imageBarrierInfo)
	{
		splitBarrierInfo.vkImageBarriers.push_back(self.get_image_barrier_info(vkdevice, barrier));
	}

	VkDependencyInfo dependencyInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = static_cast<uint32>(splitBarrierInfo.vkMemoryBarriers.size()),
		.pMemoryBarriers = splitBarrierInfo.vkMemoryBarriers.data(),
		.bufferMemoryBarrierCount = static_cast<uint32>(splitBarrierInfo.vkBufferBarriers.size()),
		.pBufferMemoryBarriers = splitBarrierInfo.vkBufferBarriers.data(),
		.imageMemoryBarrierCount = static_cast<uint32>(splitBarrierInfo.vkImageBarriers.size()),
		.pImageMemoryBarriers = splitBarrierInfo.vkImageBarriers.data()
	};

	vkCmdSetEvent2(self.handle, vkevent.handle, &dependencyInfo);

	splitBarrierInfoBuffer.clear();
}

auto CommandRecorder::wait_events(std::span<EventWaitInfo const> const& infos) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& vkdevice = static_cast<DeviceImpl const&>(m_cmdPool.device());
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	flush_barriers();

	for (auto const& info : infos)
	{
		if (!info.event.valid()) [[unlikely]]
		{
			continue;
		}

		auto const& vkevent = shared_base::impl_of(info.event);
		auto& splitBarrierInfo = splitBarrierInfoBuffer.emplace_back();

		for (auto& barrier : info.memoryBarrierInfo)
		{
			splitBarrierInfo.vkMemoryBarriers.push_back(self.get_memory_barrier_info(barrier));
		}

		for (auto& barrier : info.bufferBarrierInfo)
		{
			splitBarrierInfo.vkBufferBarriers.push_back(self.get_buffer_barrier_info(vkdevice, barrier));
		}

		for (auto& barrier : info.imageBarrierInfo)
		{
			splitBarrierInfo.vkImageBarriers.push_back(self.get_image_barrier_info(vkdevice, barrier));
		}

		dependencyInfoBuffer.emplace_back(
			VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			nullptr,
			0u,
			static_cast<uint32>(splitBarrierInfo.vkMemoryBarriers.size()),
			splitBarrierInfo.vkMemoryBarriers.data(),
			static_cast<uint32>(splitBarrierInfo.vkBufferBarriers.size()),
			splitBarrierInfo.vkBufferBarriers.data(),
			static_cast<uint32>(splitBarrierInfo.vkImageBarriers.size()),
			splitBarrierInfo.vkImageBarriers.data()
		);
		splitBarrierEventBuffer.push_back(vkevent.handle);
	}

	vkCmdWaitEvents2(self.handle, static_cast<uint32>(splitBarrierEventBuffer.size()), splitBarrierEventBuffer.data(), dependencyInfoBuffer.data());

	splitBarrierInfoBuffer.clear();
	splitBarrierEventBuffer.clear();
	dependencyInfoBuffer.clear();
}

auto CommandRecorder::wait_event(EventWaitInfo const& info) -> void
{
	wait_events(std::span{ &info, 1 });
}

auto CommandRecorder::reset_event(EventResetInfo const& info) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	auto&& vkevent = shared_base::impl_of(info.event);

	vkCmdResetEvent2(self.handle, vkevent.handle, translate_pipeline_stage_flags(info.stage));
}

auto CommandRecorder::pipeline_barrier(MemoryBarrierInfo const& barrier) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto& pool = shared_base::impl_of(m_cmdPool);
	auto& self = pool.commandBufferPool.commandBuffers[m_index];

	if (self.numMemoryBarrier >= CommandBufferImpl::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	self.memoryBarriers[self.numMemoryBarrier++] = self.get_memory_barrier_info(barrier);
}

auto CommandRecorder::pipeline_buffer_barrier(BufferBarrierInfo const& barrier) -> void
{
	if (!valid() || !barrier.buffer.valid()) [[unlikely]]
	{
		return;
	}

	auto& pool = shared_base::impl_of(m_cmdPool);
	auto const& vkdevice = static_cast<DeviceImpl const&>(m_cmdPool.device());
	auto& self = pool.commandBufferPool.commandBuffers[m_index];

	if (self.numBufferBarrier >= CommandBufferImpl::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	self.bufferBarriers[self.numBufferBarrier++] = self.get_buffer_barrier_info(vkdevice, barrier);
}

auto CommandRecorder::pipeline_image_barrier(ImageBarrierInfo const& barrier) -> void
{
	if (!valid() || !barrier.image.valid()) [[unlikely]]
	{
		return;
	}

	auto& pool = shared_base::impl_of(m_cmdPool);
	auto const& vkdevice = static_cast<DeviceImpl const&>(m_cmdPool.device());
	auto& self = pool.commandBufferPool.commandBuffers[m_index];

	if (self.numImageBarrier >= CommandBufferImpl::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	self.imageBarriers[self.numImageBarrier++] = self.get_image_barrier_info(vkdevice, barrier);
}

auto CommandRecorder::flush_barriers() -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto& pool = shared_base::impl_of(m_cmdPool);
	auto& self = pool.commandBufferPool.commandBuffers[m_index];

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

auto CommandRecorder::begin_rendering(RenderingInfo const& info) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];	

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
				auto&& image = shared_base::impl_of(attachment.image);

				Color<float32> clearColor = attachment.image.info().clearValue.color.f32;

				colorAttachments[i] = {
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = image.imageView,
					.imageLayout = translate_image_layout(attachment.imageLayout),
					.resolveMode = VK_RESOLVE_MODE_NONE,
					.resolveImageView = VK_NULL_HANDLE,
					.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.loadOp = translate_attachment_load_op(attachment.loadOp),
					.storeOp = translate_attachment_store_op(attachment.storeOp),
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
		auto const& depthImage = shared_base::impl_of(info.depthAttachment->image);

		depthAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depthImage.imageView,
			.imageLayout = translate_image_layout(info.depthAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = translate_attachment_load_op(info.depthAttachment->loadOp),
			.storeOp = translate_attachment_store_op(info.depthAttachment->storeOp),
			.clearValue = {
				.depthStencil = {
					.depth = depthImage.info.clearValue.depthStencil.depth
				}
			}
		};
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	}

	if (info.stencilAttachment && info.stencilAttachment->image.valid())
	{
		auto const& attachment = shared_base::impl_of(info.stencilAttachment->image);

		stencilAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = attachment.imageView,
			.imageLayout = translate_image_layout(info.stencilAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = translate_attachment_load_op(info.stencilAttachment->loadOp),
			.storeOp = translate_attachment_store_op(info.stencilAttachment->storeOp),
			.clearValue = {
				.depthStencil = {
					.stencil = attachment.info.clearValue.depthStencil.stencil
				}
			}
		};
		renderingInfo.pStencilAttachment = &stencilAttachmentInfo;
	}

	vkCmdBeginRendering(self.handle, &renderingInfo);
}

auto CommandRecorder::end_rendering() -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	vkCmdEndRendering(self.handle);
}

auto CommandRecorder::copy_buffer_to_buffer(BufferCopyInfo const& info) -> void
{
	if (!valid() || !info.src.valid() || !info.dst.valid())
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	auto const& srcBuf = shared_base::impl_of(info.src);
	auto const& dstBuf = shared_base::impl_of(info.dst);

	VkBufferCopy bufferCopy{
		.srcOffset = info.srcOffset,
		.dstOffset = info.dstOffset,
		.size = info.size
	};

	ASSERTION(info.size);

	vkCmdCopyBuffer(self.handle, srcBuf.handle, dstBuf.handle, 1, &bufferCopy);
}

auto CommandRecorder::copy_buffer_to_image(BufferImageCopyInfo const& info) -> void
{
	if (!valid() || !info.src.valid() || !info.dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	auto const& srcBuf = shared_base::impl_of(info.src);
	auto const& dstImg = shared_base::impl_of(info.dst);

	VkImageLayout layout = translate_image_layout(info.dstImageLayout);
	VkImageAspectFlags imageAspect = translate_image_aspect_flags(info.imageSubresource.aspectFlags);

	VkBufferImageCopy imageCopy{
		.bufferOffset = info.bufferOffset,
		.bufferRowLength = info.bufferRowLength,
		.bufferImageHeight = info.bufferImageHeight,
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

auto CommandRecorder::copy_image_to_buffer(ImageBufferCopyInfo const& info) -> void
{
	if (!valid() || !info.src.valid() || !info.dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	auto const& srcImg = shared_base::impl_of(info.src);
	auto const& dstBuf = shared_base::impl_of(info.dst);

	VkImageLayout layout = translate_image_layout(info.srcImageLayout);
	VkImageAspectFlags imageAspect = translate_image_aspect_flags(info.imageSubresource.aspectFlags);

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

auto CommandRecorder::copy_image_to_image(ImageCopyInfo const& info) -> void
{
	if (!valid() || !info.src.valid() || !info.dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];
	
	auto const& srcImg = shared_base::impl_of(info.src);
	auto const& dstImg = shared_base::impl_of(info.dst);

	VkImageLayout srcLayout = translate_image_layout(info.srcImageLayout);
	VkImageLayout dstLayout = translate_image_layout(info.dstImageLayout);

	VkImageAspectFlags srcAspect = translate_image_aspect_flags(info.srcSubresource.aspectFlags);
	VkImageAspectFlags dstAspect = translate_image_aspect_flags(info.dstSubresource.aspectFlags);

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

auto CommandRecorder::blit_image(ImageBlitInfo const& info) -> void
{
	if (!valid() || !info.src.valid() || !info.dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	auto const& srcImg = shared_base::impl_of(info.src);
	auto const& dstImg = shared_base::impl_of(info.dst);

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask = translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{ .x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{ .x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask = translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel = info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount = info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{ .x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{ .x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = translate_image_layout(info.dstImageLayout);
	VkFilter const filter = translate_texel_filter(info.filter);

	vkCmdBlitImage(self.handle, srcImg.handle, srcImgLayout, dstImg.handle, dstImgLayout, 1, &region, filter);
}

auto CommandRecorder::blit_image_swapchain(ImageSwapchainBlitInfo const& info) -> void
{
	if (!valid() || !info.src.valid() || !info.dst.valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	auto const& srcImg = shared_base::impl_of(info.src);
	auto const& dstImg = shared_base::impl_of(info.dst.current_image());

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask = translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{ .x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{ .x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask = translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel = info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount = info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{ .x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{ .x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = translate_image_layout(info.dstImageLayout);
	VkFilter const filter = translate_texel_filter(info.filter);

	vkCmdBlitImage(self.handle, srcImg.handle, srcImgLayout, dstImg.handle, dstImgLayout, 1, &region, filter);
}

auto CommandRecorder::set_viewport(Viewport const& viewport) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

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

auto CommandRecorder::set_scissor(Rect2D const& rect) -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	flush_barriers();

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

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

auto CommandRecorder::begin_debug_label(DebugLabelInfo const& info) const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

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

auto CommandRecorder::end_debug_label() const -> void
{
	if (!valid()) [[unlikely]]
	{
		return;
	}

	auto const& pool = shared_base::impl_of(m_cmdPool);
	auto const& self = pool.commandBufferPool.commandBuffers[m_index];

	vkCmdEndDebugUtilsLabelEXT(self.handle);
}

auto CommandRecorder::from(CommandPool& commandPool) -> CommandRecorder
{
	auto&& pool = shared_base::impl_of(commandPool);
	auto& vkdevice = static_cast<DeviceImpl&>(commandPool.device());

	CommandBufferPool& cmdBufferPool = pool.commandBufferPool;

	size_t const size = cmdBufferPool.commandBuffers.size();
	size_t iterations = 0;

	while (iterations < size)
	{
		auto const current = static_cast<uint64>(cmdBufferPool.current);

		auto&& cmdBuffer = cmdBufferPool.commandBuffers[current];

		auto const& gpuTimelineSemaphore = shared_base::impl_of(cmdBuffer.gpuTimeline);

		/*
		* recording_timeline < current_timleine, never possible unless something goes extremely wrong. Some say it's black magic.
		* recording_timeline > current_timeline, command buffer work begins, should not be used.
		* recording_timeline == current_timeline, timeline semaphore is signalled once gpu queue finishes recording, able to be re-used.
		*/
		auto const cpuTimeline = cmdBuffer.recordingTimeline.load(std::memory_order_acquire);
		uint64 gpuTimeline = 0;
		vkGetSemaphoreCounterValue(vkdevice.device, gpuTimelineSemaphore.handle, &gpuTimeline);

		if (cpuTimeline == gpuTimeline)
		{
			vkResetCommandBuffer(cmdBuffer.handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			return { commandPool, current };
		}

		cmdBufferPool.current = (cmdBufferPool.current + 1) % size;
	}

	auto index = cmdBufferPool.commandBuffers.size();
	auto&& vkcmdbuffer = cmdBufferPool.commandBuffers.emplace_back();

	VkCommandBufferAllocateInfo allocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = pool.handle,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer handle = VK_NULL_HANDLE;

	CHECK_OP(vkAllocateCommandBuffers(vkdevice.device, &allocateInfo, &handle))

	vkcmdbuffer.handle = handle;
	vkcmdbuffer.gpuTimeline = Fence::from(vkdevice, {});

	return { commandPool, index };
}

CommandRecorder::CommandRecorder(CommandPool const& pool, uint64 commandBufferIndex) :
	m_cmdPool{ pool },
	m_index{ commandBufferIndex }
{}

CommandBufferImpl::CommandBufferImpl(CommandBufferImpl&& rhs) :
	handle{ std::exchange(rhs.handle, {}) },
	numMemoryBarrier{ std::exchange(rhs.numMemoryBarrier, {}) },
	numBufferBarrier{ std::exchange(rhs.numBufferBarrier, {}) },
	numImageBarrier{ std::exchange(rhs.numImageBarrier, {}) },
	gpuTimeline{ std::exchange(rhs.gpuTimeline, {}) }
{
	std::memcpy(memoryBarriers.data(), rhs.memoryBarriers.data(), MAX_COMMAND_BUFFER_BARRIER_COUNT * sizeof(VkMemoryBarrier2));
	std::memcpy(bufferBarriers.data(), rhs.bufferBarriers.data(), MAX_COMMAND_BUFFER_BARRIER_COUNT * sizeof(VkBufferMemoryBarrier2));
	std::memcpy(imageBarriers.data(), rhs.imageBarriers.data(), MAX_COMMAND_BUFFER_BARRIER_COUNT * sizeof(VkImageMemoryBarrier2));

	recordingTimeline.store(rhs.recordingTimeline.load(std::memory_order_acquire), std::memory_order_relaxed);
}

auto CommandBufferImpl::get_memory_barrier_info(MemoryBarrierInfo const& info) const -> VkMemoryBarrier2
{
	return VkMemoryBarrier2{
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = translate_pipeline_stage_flags(info.srcAccess.stages),
		.srcAccessMask = translate_memory_access_flags(info.srcAccess.type),
		.dstStageMask = translate_pipeline_stage_flags(info.dstAccess.stages),
		.dstAccessMask = translate_memory_access_flags(info.dstAccess.type),
	};
}

auto CommandBufferImpl::get_buffer_barrier_info(DeviceImpl const& device, BufferBarrierInfo const& info) const -> VkBufferMemoryBarrier2
{
	auto const& vkbuffer = shared_base::impl_of(info.buffer);

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (info.srcQueue)
	{
	case DeviceQueue::Main:
		srcQueueIndex = device.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		srcQueueIndex = device.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		srcQueueIndex = device.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (info.dstQueue)
	{
	case DeviceQueue::Main:
		dstQueueIndex = device.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		dstQueueIndex = device.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		dstQueueIndex = device.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	return VkBufferMemoryBarrier2{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.srcStageMask = translate_pipeline_stage_flags(info.srcAccess.stages),
		.srcAccessMask = translate_memory_access_flags(info.srcAccess.type),
		.dstStageMask = translate_pipeline_stage_flags(info.dstAccess.stages),
		.dstAccessMask = translate_memory_access_flags(info.dstAccess.type),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.buffer = vkbuffer.handle,
		.offset = info.offset,
		.size = (info.size == std::numeric_limits<size_t>::max()) ? info.buffer.info().size : info.size,
	};
}

auto CommandBufferImpl::get_image_barrier_info(DeviceImpl const& device, ImageBarrierInfo const& info) const -> VkImageMemoryBarrier2
{
	auto const& vkimage = shared_base::impl_of(info.image);

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (info.srcQueue)
	{
	case DeviceQueue::Main:
		srcQueueIndex = device.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		srcQueueIndex = device.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		srcQueueIndex = device.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (info.dstQueue)
	{
	case DeviceQueue::Main:
		dstQueueIndex = device.mainQueue.familyIndex;
		break;
	case DeviceQueue::Transfer:
		dstQueueIndex = device.transferQueue.familyIndex;
		break;
	case DeviceQueue::Compute:
		dstQueueIndex = device.computeQueue.familyIndex;
		break;
	default:
		break;
	}

	return VkImageMemoryBarrier2{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = translate_pipeline_stage_flags(info.srcAccess.stages),
		.srcAccessMask = translate_memory_access_flags(info.srcAccess.type),
		.dstStageMask = translate_pipeline_stage_flags(info.dstAccess.stages),
		.dstAccessMask = translate_memory_access_flags(info.dstAccess.type),
		.oldLayout = translate_image_layout(info.oldLayout),
		.newLayout = translate_image_layout(info.newLayout),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.image = vkimage.handle,
		.subresourceRange = {
			.aspectMask = translate_image_aspect_flags(info.subresource.aspectFlags),
			.baseMipLevel = info.subresource.mipLevel,
			.levelCount = info.subresource.levelCount,
			.baseArrayLayer = info.subresource.baseArrayLayer,
			.layerCount = info.subresource.layerCount
		}
	};
}
}