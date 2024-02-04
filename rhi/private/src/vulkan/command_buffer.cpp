#include "command_buffer.h"
#include "vulkan/vk_device.h"

/**
* Vulkan Command Buffers Specification
* https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap6.html
*/

namespace rhi
{

extern VkAttachmentLoadOp	translate_attachment_load_op	(AttachmentLoadOp loadOp);
extern VkAttachmentStoreOp	translate_attachment_store_op	(AttachmentStoreOp storeOp);
extern VkFilter				translate_texel_filter			(TexelFilter filter);

auto translate_image_aspect_flags(ImageAspect flags) -> VkImageAspectFlags
{
	uint32 const mask = (uint32)flags;

	constexpr VkImageAspectFlagBits bits[] = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_ASPECT_STENCIL_BIT,
		VK_IMAGE_ASPECT_METADATA_BIT,
		VK_IMAGE_ASPECT_PLANE_0_BIT,
		VK_IMAGE_ASPECT_PLANE_1_BIT,
		VK_IMAGE_ASPECT_PLANE_2_BIT
	};

	uint32 constexpr numBits = (uint32)std::size(bits);

	VkImageUsageFlags result = 0;

	for (uint32 i = 0; i < numBits; ++i)
	{
		uint32 const exist = (mask & (1 << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_image_layout(ImageLayout layout) -> VkImageLayout
{
	switch (layout)
	{
	case ImageLayout::General:
		return VK_IMAGE_LAYOUT_GENERAL;
	case ImageLayout::Color_Attachment:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Stencil_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	case ImageLayout::Shader_Read_Only:
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case ImageLayout::Transfer_Src:
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case ImageLayout::Transfer_Dst:
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case ImageLayout::Preinitialized:
		return VK_IMAGE_LAYOUT_PREINITIALIZED;
	case ImageLayout::Depth_Read_Only_Stencil_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Attachment_Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	case ImageLayout::Depth_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
	case ImageLayout::Stencil_Attachment:
		return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
	case ImageLayout::Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
	case ImageLayout::Read_Only:
		return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
	case ImageLayout::Attachment:
		return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	case ImageLayout::Present_Src:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	case ImageLayout::Shared_Present:
		return VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
	case ImageLayout::Fragment_Density_Map:
		return VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	case ImageLayout::Fragment_Shading_Rate_Attachment:
		return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	case ImageLayout::Undefined:
	default:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

auto translate_pipeline_stage_flags(PipelineStage stages) -> VkPipelineStageFlags2
{
	uint64 const mask = (uint64)stages;

	constexpr VkPipelineStageFlagBits2 bits[] = {
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
		VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
		VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
		VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
		VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
		VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_HOST_BIT,
		VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_2_COPY_BIT,
		VK_PIPELINE_STAGE_2_RESOLVE_BIT,
		VK_PIPELINE_STAGE_2_BLIT_BIT,
		VK_PIPELINE_STAGE_2_CLEAR_BIT,
		VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
		VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
		VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
		VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV,
		VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV,
		VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR
	};

	uint64 constexpr numBits = (uint64)std::size(bits);

	VkPipelineStageFlags2 result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;

		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_memory_access_flags(MemoryAccessType accesses) -> VkAccessFlags2
{
	uint64 mask = (uint64)accesses;

	constexpr VkAccessFlagBits2 bits[] = {
		VK_ACCESS_2_HOST_READ_BIT,
		VK_ACCESS_2_HOST_WRITE_BIT,
		VK_ACCESS_2_MEMORY_READ_BIT,
		VK_ACCESS_2_MEMORY_WRITE_BIT
	};

	uint64 constexpr numBits = (uint64)std::size(bits);

	VkAccessFlags2 result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_shader_stage_flags(ShaderStage shaderStage) -> VkShaderStageFlags
{
	uint64 mask = static_cast<uint64>(shaderStage);

	constexpr VkShaderStageFlagBits bits[] = {
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_SHADER_STAGE_ALL_GRAPHICS,
		VK_SHADER_STAGE_ALL
	};

	uint64 constexpr numBits = static_cast<uint64>(std::size(bits));

	VkShaderStageFlags result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

CommandBuffer::CommandBuffer(
	CommandBufferInfo&& info,
	APIContext* context,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) },
	m_completion_timeline{},
	m_recording_timeline{},
	m_state{ State::Initial }
{
	m_context->setup_debug_name(*this);
	m_completion_timeline = m_context->create_timeline_semaphore({ .name = m_info.name, .initialValue = 0 });
}

CommandBuffer::CommandBuffer(CommandBuffer&& rhs) noexcept
{
	*this = std::move(rhs);
}

auto CommandBuffer::operator=(CommandBuffer&& rhs) noexcept -> CommandBuffer&
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		m_completion_timeline = std::move(rhs.m_completion_timeline);
		m_recording_timeline = std::move(rhs.m_recording_timeline);
		m_state = rhs.m_state;
		Resource::operator=(std::move(rhs));
		new (&rhs) CommandBuffer{};
	}
	return *this;
}

auto CommandBuffer::info() const -> CommandBufferInfo const&
{
	return m_info;
}

auto CommandBuffer::is_initial() const -> bool
{
	return m_state == State::Initial;
}

auto CommandBuffer::is_recording() const -> bool
{
	return m_state == State::Recording;
}

auto CommandBuffer::is_executable() const -> bool
{
	return m_state == State::Executable;
}

auto CommandBuffer::is_pending_complete() const -> bool
{
	return m_state == State::Pending;
}

auto CommandBuffer::is_completed() -> bool
{
	uint64 const val = m_completion_timeline.value();

	if (m_recording_timeline && val >= m_recording_timeline)
	{
		m_state = State::Executable;
	}

	return m_recording_timeline && val >= m_recording_timeline;
}

auto CommandBuffer::is_invalid() const -> bool
{
	return m_state == State::Invalid;
}

auto CommandBuffer::current_state() const -> CommandBuffer::State
{
	return m_state;
}

auto CommandBuffer::reset() -> void
{
	if (valid() &&
		is_completed() &&
		m_state == State::Executable)
	{
		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vkResetCommandBuffer(cmdBuffer.handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		m_state = State::Initial;
	}
}

auto CommandBuffer::begin() -> bool
{
	if (valid() &&
		m_state == State::Initial)
	{
		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		if (vkBeginCommandBuffer(cmdBuffer.handle, &beginInfo) == VK_SUCCESS)
		{
			m_state = State::Recording;
			++m_recording_timeline;
		}
	}
	return m_state == State::Recording;
}

auto CommandBuffer::end() -> void
{
	if (valid() && 
		m_state == State::Recording)
	{
		flush_barriers();
		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vkEndCommandBuffer(cmdBuffer.handle);
		m_state = State::Executable;
	}
}

auto CommandBuffer::clear(Image const& image) -> void
{
	return clear(image, { .dstImageLayout = ImageLayout::General, .clearValue = image.info().clearValue });
}

auto CommandBuffer::clear(Image const& image, ImageClearInfo const& info) -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		ClearValue imgClearValue = image.info().clearValue;
		vulkan::Image& img = image.as<vulkan::Image>();

		VkImageSubresourceRange subResourceRange{
			.aspectMask = translate_image_aspect_flags(info.subresource.aspectFlags),
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
				cmdBuffer.handle, 
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
			lib::memcopy(&colorValue.uint32, &imgClearValue.color.u32, sizeof(imgClearValue.color));
			vkCmdClearColorImage(
				cmdBuffer.handle, 
				img.handle, 
				translate_image_layout(info.dstImageLayout), 
				&colorValue, 
				1, 
				&subResourceRange
			);
		}
	}
}

auto CommandBuffer::clear(Buffer const& buffer) -> void
{
	BufferInfo const& info = buffer.info();
	return clear(buffer, { .offset = 0, .size = info.size, .data = 0u });
}

auto CommandBuffer::clear(Buffer const& buffer, BufferClearInfo const& info) -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vulkan::Buffer& buf = buffer.as<vulkan::Buffer>();
		vkCmdFillBuffer(
			cmdBuffer.handle, 
			buf.handle, 
			static_cast<VkDeviceSize>(info.offset), 
			static_cast<VkDeviceSize>(info.size), 
			info.data
		);
	}
}

auto CommandBuffer::draw(DrawInfo const& info) const -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vkCmdDraw(cmdBuffer.handle, info.vertexCount, info.instanceCount, info.firstVertex, info.firstInstance);
	}
}

auto CommandBuffer::draw_indexed(DrawIndexedInfo const& info) const -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vkCmdDrawIndexed(cmdBuffer.handle, info.indexCount, info.instanceCount, info.firstIndex, info.vertexOffset, info.firstInstance);
	}
}

auto CommandBuffer::draw_indirect(Buffer const& drawInfoBuffer, DrawIndirectInfo const& info) const -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	vulkan::Buffer& buff = drawInfoBuffer.as<vulkan::Buffer>();

	if (info.indexed)
	{
		vkCmdDrawIndexedIndirect(
			cmdBuffer.handle,
			buff.handle,
			static_cast<VkDeviceSize>(info.offset),
			info.drawCount,
			info.stride
		);
	}
	else
	{
		vkCmdDrawIndirect(
			cmdBuffer.handle,
			buff.handle,
			static_cast<VkDeviceSize>(info.offset),
			info.drawCount,
			info.stride
		);
	}
}

auto CommandBuffer::draw_indirect_count(Buffer const& drawInfoBuffer, Buffer const& drawCountBuffer, DrawIndirectCountInfo const& info) const -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	vulkan::Buffer& drawInfoBuf = drawInfoBuffer.as<vulkan::Buffer>();
	vulkan::Buffer& countBuf = drawCountBuffer.as<vulkan::Buffer>();

	if (info.indexed)
	{
		vkCmdDrawIndexedIndirectCount(
			cmdBuffer.handle,
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
			cmdBuffer.handle,
			drawInfoBuf.handle,
			static_cast<VkDeviceSize>(info.offset),
			countBuf.handle,
			static_cast<VkDeviceSize>(info.countBufferOffset),
			info.maxDrawCount,
			info.stride
		);
	}
}

auto CommandBuffer::bind_vertex_buffer(Buffer const& buffer, BindVertexBufferInfo const& info) -> void
{
	if (valid() &&
		buffer.valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		using buffer_usage_t = std::underlying_type_t<BufferUsage>;
		buffer_usage_t const usage = static_cast<buffer_usage_t>(buffer.info().bufferUsage);
		if (usage & static_cast<buffer_usage_t>(BufferUsage::Vertex))
		{
			vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
			vulkan::Buffer& buf = buffer.as<vulkan::Buffer>();
			vkCmdBindVertexBuffers(cmdBuffer.handle, info.firstBinding, 1, &buf.handle, &info.offset);
		}
	}
}

auto CommandBuffer::bind_index_buffer(Buffer const& buffer, BindIndexBufferInfo const& info) -> void
{
	if (valid() &&
		buffer.valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		using buffer_usage_t = std::underlying_type_t<BufferUsage>;
		buffer_usage_t const usage = static_cast<buffer_usage_t>(buffer.info().bufferUsage);
		if (usage & static_cast<buffer_usage_t>(BufferUsage::Index))
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
			vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
			vulkan::Buffer& buf = buffer.as<vulkan::Buffer>();
			vkCmdBindIndexBuffer(cmdBuffer.handle, buf.handle, info.offset, type);
		}
	}
}

auto CommandBuffer::bind_push_constant(BindPushConstantInfo const& info) -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		uint32 sz = static_cast<uint32>(info.size);
		uint32 off = static_cast<uint32>(info.offset);
		uint32 const MAX_PUSH_CONSTANT_SIZE = m_context->config.pushConstantMaxSize;

		ASSERTION(sz <= MAX_PUSH_CONSTANT_SIZE && "Constant supplied exceeded maximum size allowed by the driver.");
		ASSERTION(off <= MAX_PUSH_CONSTANT_SIZE && "Offset supplied exceeded maximum push constant size allowed by the driver.");
		ASSERTION((sz % 4 == 0) && "Size must be a multiple of 4.");
		ASSERTION((off % 4 == 0) && "Offset must be a multiple of 4.");

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		VkPipelineLayout layout = m_context->descriptorCache.pipelineLayouts[(sz + 3u) & (~0x03u)];
		VkShaderStageFlags shaderStageFlags = translate_shader_stage_flags(info.shaderStage);
		vkCmdPushConstants(cmdBuffer.handle, layout, shaderStageFlags, off, sz, info.data);
	}
}

auto CommandBuffer::bind_pipeline(RasterPipeline const& pipeline) -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vulkan::Pipeline& pipelineResource = pipeline.as<vulkan::Pipeline>();

		vkCmdBindDescriptorSets(
			cmdBuffer.handle, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			pipelineResource.layout, 
			0, 
			1, 
			&m_context->descriptorCache.descriptorSet, 
			0, 
			nullptr
		);
		vkCmdBindPipeline(cmdBuffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineResource.handle);
	}
}

auto CommandBuffer::pipeline_barrier(MemoryBarrierInfo const& barrier) -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}
	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	if (cmdBuffer.numMemoryBarrier >= vulkan::CommandBuffer::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}
	cmdBuffer.memoryBarriers[cmdBuffer.numMemoryBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = translate_memory_access_flags(barrier.dstAccess.type),
	};
}

auto CommandBuffer::pipeline_barrier(Buffer const& buffer, BufferBarrierInfo const& barrier) -> void
{
	if (!valid() ||
		!buffer.valid() ||
		m_state != State::Recording)
	{
		return;
	}

	vulkan::Buffer& bufferResource = buffer.as<vulkan::Buffer>();
	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();

	if (cmdBuffer.numBufferBarrier >= vulkan::CommandBuffer::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueueType::Main:
		srcQueueIndex = m_context->mainQueue.familyIndex;
		break;
	case DeviceQueueType::Transfer:
		srcQueueIndex = m_context->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		srcQueueIndex = m_context->computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueueType::Main:
		dstQueueIndex = m_context->mainQueue.familyIndex;
		break;
	case DeviceQueueType::Transfer:
		dstQueueIndex = m_context->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		dstQueueIndex = m_context->computeQueue.familyIndex;
		break;
	default:
		break;
	}

	cmdBuffer.bufferBarriers[cmdBuffer.numBufferBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.srcStageMask = translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = translate_memory_access_flags(barrier.dstAccess.type),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.buffer	= bufferResource.handle,
		.offset	= barrier.offset,
		.size = (barrier.size == std::numeric_limits<size_t>::max()) ? buffer.info().size : barrier.size,
	};
}

//auto CommandBuffer::pipeline_barrier(BufferView const& bufferView, BufferViewBarrierInfo const& barrier) -> void
//{
//	if (!valid() ||
//		!bufferView.valid() ||
//		m_state != State::Recording)
//	{
//		return;
//	}
//
//	vulkan::Buffer& bufferResource = bufferView.buffer().as<vulkan::Buffer>();
//	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
//
//	if (cmdBuffer.numBufferBarrier >= vulkan::CommandBuffer::MAX_COMMAND_BUFFER_BARRIER_COUNT)
//	{
//		flush_barriers();
//	}
//
//	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
//	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;
//
//	switch (barrier.srcQueue)
//	{
//	case DeviceQueueType::Main:
//		srcQueueIndex = m_context->mainQueue.familyIndex;
//		break;
//	case DeviceQueueType::Transfer:
//		srcQueueIndex = m_context->transferQueue.familyIndex;
//		break;
//	case DeviceQueueType::Compute:
//		srcQueueIndex = m_context->computeQueue.familyIndex;
//		break;
//	default:
//		break;
//	}
//
//	switch (barrier.dstQueue)
//	{
//	case DeviceQueueType::Main:
//		dstQueueIndex = m_context->mainQueue.familyIndex;
//		break;
//	case DeviceQueueType::Transfer:
//		dstQueueIndex = m_context->transferQueue.familyIndex;
//		break;
//	case DeviceQueueType::Compute:
//		dstQueueIndex = m_context->computeQueue.familyIndex;
//		break;
//	default:
//		break;
//	}
//
//	cmdBuffer.bufferBarriers[cmdBuffer.numBufferBarrier++] = {
//		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
//		.srcStageMask = translate_pipeline_stage_flags(barrier.srcAccess.stages),
//		.srcAccessMask = translate_memory_access_flags(barrier.srcAccess.type),
//		.dstStageMask = translate_pipeline_stage_flags(barrier.dstAccess.stages),
//		.dstAccessMask = translate_memory_access_flags(barrier.dstAccess.type),
//		.srcQueueFamilyIndex = srcQueueIndex,
//		.dstQueueFamilyIndex = dstQueueIndex,
//		.buffer = bufferResource.handle,
//		.offset = bufferView.offset_from_buffer(),
//		.size = bufferView.size()
//	};
//
//	BufferView& alias = *const_cast<BufferView*>(&bufferView);
//
//	if (barrier.srcQueue == barrier.dstQueue ||
//		m_execution_queue == barrier.dstQueue)
//	{
//		alias.m_owning_queue = barrier.dstQueue;
//	}
//	else if (alias.m_owning_queue == rhi::DeviceQueueType::None)
//	{
//		alias.m_owning_queue = barrier.srcQueue;
//	}
//}

auto CommandBuffer::pipeline_barrier(Image const& image, ImageBarrierInfo const& barrier) -> void
{
	if (!valid() ||
		!image.valid() ||
		m_state != State::Recording)
	{
		return;
	}

	vulkan::Image& imageResource = image.as<vulkan::Image>();
	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();

	if (cmdBuffer.numImageBarrier >= vulkan::CommandBuffer::MAX_COMMAND_BUFFER_BARRIER_COUNT)
	{
		flush_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueueType::Main:
		srcQueueIndex = m_context->mainQueue.familyIndex;
		break;
	case DeviceQueueType::Transfer:
		srcQueueIndex = m_context->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		srcQueueIndex = m_context->computeQueue.familyIndex;
		break;
	default:
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueueType::Main:
		dstQueueIndex = m_context->mainQueue.familyIndex;
		break;
	case DeviceQueueType::Transfer:
		dstQueueIndex = m_context->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		dstQueueIndex = m_context->computeQueue.familyIndex;
		break;
	default:
		break;
	}

	cmdBuffer.imageBarriers[cmdBuffer.numImageBarrier++] = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask = translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask = translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask = translate_memory_access_flags(barrier.dstAccess.type),
		.oldLayout = translate_image_layout(barrier.oldLayout),
		.newLayout = translate_image_layout(barrier.newLayout),
		.srcQueueFamilyIndex = srcQueueIndex,
		.dstQueueFamilyIndex = dstQueueIndex,
		.image = imageResource.handle,
		.subresourceRange = {
			.aspectMask = translate_image_aspect_flags(barrier.subresource.aspectFlags),
			.baseMipLevel = barrier.subresource.mipLevel,
			.levelCount	= barrier.subresource.levelCount,
			.baseArrayLayer = barrier.subresource.baseArrayLayer,
			.layerCount	= barrier.subresource.layerCount
		}
	};
}

auto CommandBuffer::flush_barriers() -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	if (cmdBuffer.numMemoryBarrier > 0 ||
		cmdBuffer.numBufferBarrier > 0 ||
		cmdBuffer.numImageBarrier > 0)
	{
		VkDependencyInfo dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.memoryBarrierCount = static_cast<uint32>(cmdBuffer.numMemoryBarrier),
			.bufferMemoryBarrierCount = static_cast<uint32>(cmdBuffer.numBufferBarrier),
			.imageMemoryBarrierCount = static_cast<uint32>(cmdBuffer.numImageBarrier)
		};

		if (cmdBuffer.numMemoryBarrier)
		{
			dependencyInfo.pMemoryBarriers = cmdBuffer.memoryBarriers.data();
		}

		if (cmdBuffer.numBufferBarrier)
		{
			dependencyInfo.pBufferMemoryBarriers = cmdBuffer.bufferBarriers.data();
		}

		if (cmdBuffer.numImageBarrier)
		{
			dependencyInfo.pImageMemoryBarriers = cmdBuffer.imageBarriers.data();
		}

		vkCmdPipelineBarrier2(cmdBuffer.handle, &dependencyInfo);
		
		cmdBuffer.numMemoryBarrier = 0;
		cmdBuffer.numBufferBarrier = 0;
		cmdBuffer.numImageBarrier = 0;
	}
}

auto CommandBuffer::begin_rendering(RenderingInfo const& info) -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	flush_barriers();

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();

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
		.layerCount	= 1u,
		.colorAttachmentCount = static_cast<uint32>(info.colorAttachments.size()),
	};
	std::array<VkRenderingAttachmentInfo, MAX_COMMAND_BUFFER_ATTACHMENT> colorAttachments;
	if (info.colorAttachments.size())
	{
		for (size_t i = 0; RenderAttachment const& attachment : info.colorAttachments)
		{
			if (attachment.pImage)
			{
				vulkan::Image& colorAttachment = attachment.pImage->as<vulkan::Image>();
				Color<float32> clearColor = attachment.pImage->info().clearValue.color.f32;
				colorAttachments[i] = {
					.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
					.imageView = colorAttachment.imageView,
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

	if (info.depthAttachment &&
		info.depthAttachment->pImage)
	{
		vulkan::Image& depthAttachment = info.depthAttachment->pImage->as<vulkan::Image>();
		depthAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depthAttachment.imageView,
			.imageLayout = translate_image_layout(info.depthAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = translate_attachment_load_op(info.depthAttachment->loadOp),
			.storeOp = translate_attachment_store_op(info.depthAttachment->storeOp),
			.clearValue = { .depthStencil = { .depth = info.depthAttachment->pImage->info().clearValue.depthStencil.depth }}
		};
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;
	}

	if (info.stencilAttachment &&
		info.stencilAttachment->pImage)
	{
		vulkan::Image& stencilAttachment = info.stencilAttachment->pImage->as<vulkan::Image>();
		stencilAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = stencilAttachment.imageView,
			.imageLayout = translate_image_layout(info.stencilAttachment->imageLayout),
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = translate_attachment_load_op(info.stencilAttachment->loadOp),
			.storeOp = translate_attachment_store_op(info.stencilAttachment->storeOp),
			.clearValue = { .depthStencil = { .stencil = info.depthAttachment->pImage->info().clearValue.depthStencil.stencil }}
		};
		renderingInfo.pStencilAttachment = &stencilAttachmentInfo;
	}
	vkCmdBeginRendering(cmdBuffer.handle, &renderingInfo);
}

auto CommandBuffer::end_rendering() -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vkCmdEndRendering(cmdBuffer.handle);
	}
}

auto CommandBuffer::copy_buffer_to_buffer(Buffer const& src, Buffer const& dst, BufferCopyInfo const& info) -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vulkan::Buffer& srcBuffer = src.as<vulkan::Buffer>();
		vulkan::Buffer& dstBuffer = dst.as<vulkan::Buffer>();
		VkBufferCopy bufferCopy{
			.srcOffset = info.srcOffset,
			.dstOffset = info.dstOffset,
			.size = info.size 
		};
		ASSERTION(info.size);
		vkCmdCopyBuffer(cmdBuffer.handle, srcBuffer.handle, dstBuffer.handle, 1, &bufferCopy);
	}
}

auto CommandBuffer::copy_buffer_to_image(Buffer const& src, Image const& dst, BufferImageCopyInfo const& info) -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	flush_barriers();

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	vulkan::Buffer& buf = src.as<vulkan::Buffer>();
	vulkan::Image& img = dst.as<vulkan::Image>();

	VkImageLayout layout = translate_image_layout(info.dstImageLayout);
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
	vkCmdCopyBufferToImage(cmdBuffer.handle, buf.handle, img.handle, layout, 1, &imageCopy);
}

auto CommandBuffer::copy_image_to_buffer(Image const& src, Buffer const& dst, ImageBufferCopyInfo const& info) -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	flush_barriers();

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	vulkan::Image& img = src.as<vulkan::Image>();
	vulkan::Buffer& buf = dst.as<vulkan::Buffer>();

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
	vkCmdCopyImageToBuffer(cmdBuffer.handle, img.handle, layout, buf.handle, 1, &imageCopy);
}

auto CommandBuffer::copy_image_to_image(Image const& src, Image const& dst, ImageCopyInfo const& info) -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	flush_barriers();

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	vulkan::Image& srcImage = src.as<vulkan::Image>();
	vulkan::Image& dstImage = dst.as<vulkan::Image>();

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
	vkCmdCopyImage(cmdBuffer.handle, srcImage.handle, srcLayout, dstImage.handle, dstLayout, 1, &imageCopy);
}

auto CommandBuffer::blit_image(Image const& src, Image const& dst, ImageBlitInfo const& info) -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	flush_barriers();

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	vulkan::Image& srcImage = src.as<vulkan::Image>();
	vulkan::Image& dstImage = dst.as<vulkan::Image>();

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask	= translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount = info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{.x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{.x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask		= translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel		= info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount		= info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{.x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{.x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = translate_image_layout(info.dstImageLayout);
	VkFilter const filter = translate_texel_filter(info.filter);

	vkCmdBlitImage(cmdBuffer.handle, srcImage.handle, srcImgLayout, dstImage.handle, dstImgLayout, 1, &region, filter);
}

auto CommandBuffer::blit_image(Image const& src, Swapchain const& dst, ImageBlitInfo const& info) -> void
{
	if (!valid() ||
		m_state != State::Recording)
	{
		return;
	}

	flush_barriers();

	vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
	vulkan::Image& srcImage = src.as<vulkan::Image>();
	vulkan::Image const& dstImage = dst.current_image().as<vulkan::Image>();

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask	= translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel = info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount	= info.srcSubresource.layerCount
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
			{.x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{.x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = translate_image_layout(info.dstImageLayout);
	VkFilter const filter = translate_texel_filter(info.filter);

	vkCmdBlitImage(cmdBuffer.handle, srcImage.handle, srcImgLayout, dstImage.handle, dstImgLayout, 1, &region, filter);
}

auto CommandBuffer::set_viewport(rhi::Viewport const& viewport) -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();

		VkViewport vp{
			.x = viewport.x,
			.y = viewport.y,
			.width = viewport.width,
			.height	= viewport.height,
			.minDepth = viewport.minDepth,
			.maxDepth = viewport.maxDepth,
		};
		vkCmdSetViewport(cmdBuffer.handle, 0, 1, &vp);
	}
}

auto CommandBuffer::set_scissor(rhi::Rect2D const& rect) -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		flush_barriers();

		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();

		VkRect2D scissor{
			.offset = { 
				.x = rect.offset.x, 
				.y = rect.offset.y 
			},
			.extent = { 
				.width	= rect.extent.width, 
				.height = rect.extent.height 
			}
		};
		vkCmdSetScissor(cmdBuffer.handle, 0, 1, &scissor);
	}
}

auto CommandBuffer::begin_debug_label(DebugLabelInfo const& info) const -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
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
		vkCmdBeginDebugUtilsLabelEXT(cmdBuffer.handle, &debugLabel);
	}
}

auto CommandBuffer::end_debug_label() const -> void
{
	if (valid() &&
		m_state == State::Recording)
	{
		vulkan::CommandBuffer& cmdBuffer = as<vulkan::CommandBuffer>();
		vkCmdEndDebugUtilsLabelEXT(cmdBuffer.handle);
	}
}
}