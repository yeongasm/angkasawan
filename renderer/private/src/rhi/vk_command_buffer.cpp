#include "command_buffer.h"
#include "device.h"
#include "rhi/vulkan/vk_device.h"

/**
* Vulkan Command Buffers Specification
* https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap6.html
*/

namespace rhi
{

namespace command
{

bool CommandBuffer::is_recording()
{
	return state == State::Recording;
}

bool CommandBuffer::valid()
{
	return data.valid();
}

void CommandBuffer::reset()
{
	if (valid())
	{
		vkResetCommandBuffer(data.as<VkCommandBuffer>(), VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}
}

bool CommandBuffer::begin()
{
	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	if (vkBeginCommandBuffer(hcommandBuffer, &beginInfo) == VK_SUCCESS)
	{
		state = State::Recording;
	}

	return state == State::Recording;
}

void CommandBuffer::end()
{
	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();
	vkEndCommandBuffer(hcommandBuffer);
	state = State::Executable;
}

void CommandBuffer::flush_pipeline_barriers()
{
	if (memoryBarrierCount > 0 || imageBarrierCount > 0 || bufferBarrierCount > 0)
	{
		uint32 const slot = data.slot();
		VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();

		CommandArena::BarrierInformation& barriers = owner->data.as<CommandArena>().store.pipelineBarrierInfos[slot];

		VkDependencyInfo dependencyInfo{
			.sType						= VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext						= nullptr,
			.memoryBarrierCount			= memoryBarrierCount,
			.pMemoryBarriers			= (memoryBarrierCount) ? barriers.memoryBarriers.data() : nullptr,
			.bufferMemoryBarrierCount	= bufferBarrierCount,
			.pBufferMemoryBarriers		= (bufferBarrierCount) ? barriers.bufferMemoryBarriers.data() : nullptr,
			.imageMemoryBarrierCount	= imageBarrierCount,
			.pImageMemoryBarriers		= (imageBarrierCount) ? barriers.imageMemoryBarriers.data() : nullptr,
		};

		vkCmdPipelineBarrier2(hcommandBuffer, &dependencyInfo);

		memoryBarrierCount = bufferBarrierCount = imageBarrierCount = 0u;
	}
}

void CommandBuffer::bind_pipeline(RasterPipeline const& pipeline)
{
	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();
	VulkanPipeline& vulkanPipeline = pipeline.data.as<VulkanPipeline>();

	vkCmdBindPipeline(hcommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline.pipeline);
}

void CommandBuffer::pipeline_barrier(PipelineMemoryBarrier const& barrier)
{
	if (memoryBarrierCount >= MAX_COMMAND_BUFFER_PIPELINE_BARRIER_BATCH_SIZE)
	{
		flush_pipeline_barriers();
	}

	uint32 const slot = data.slot();
	CommandArena::BarrierInformation& barriers = owner->data.as<CommandArena>().store.pipelineBarrierInfos[slot];

	barriers.emplace_memory_barrier(VkMemoryBarrier2{
		.sType			= VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.pNext			= nullptr,
		.srcStageMask	= deviceContext->translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask	= deviceContext->translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask	= deviceContext->translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask	= deviceContext->translate_memory_access_flags(barrier.dstAccess.type),
	});

	++memoryBarrierCount;
}

void CommandBuffer::pipeline_barrier(Buffer const& buffer, BufferMemoryBarrier const& barrier)
{
	if (bufferBarrierCount >= MAX_COMMAND_BUFFER_PIPELINE_BARRIER_BATCH_SIZE)
	{
		flush_pipeline_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueueType::Transfer:
		srcQueueIndex = deviceContext->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		srcQueueIndex = deviceContext->computeQueue.familyIndex;
		break;
	default:
		srcQueueIndex = deviceContext->mainQueue.familyIndex;
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueueType::Transfer:
		dstQueueIndex = deviceContext->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		dstQueueIndex = deviceContext->computeQueue.familyIndex;
		break;
	default:
		dstQueueIndex = deviceContext->mainQueue.familyIndex;
		break;
	}

	VulkanBuffer& vulkanBuffer = buffer.data.as<VulkanBuffer>();
	uint32 const slot = data.slot();
	CommandArena::BarrierInformation& barriers = owner->data.as<CommandArena>().store.pipelineBarrierInfos[slot];

	barriers.emplace_buffer_memory_barrier(VkBufferMemoryBarrier2{
		.sType					= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext					= nullptr,
		.srcStageMask			= deviceContext->translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask			= deviceContext->translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask			= deviceContext->translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask			= deviceContext->translate_memory_access_flags(barrier.dstAccess.type),
		.srcQueueFamilyIndex	= srcQueueIndex,
		.dstQueueFamilyIndex	= dstQueueIndex,
		.buffer					= vulkanBuffer.buffer,
		.offset					= barrier.offset,
		.size					= (barrier.size == std::numeric_limits<uint32>::max()) ? buffer.info.size : barrier.size,
	});

	++bufferBarrierCount;
}

void CommandBuffer::pipeline_barrier(Image const& image, ImageMemoryBarrier const& barrier)
{
	if (imageBarrierCount >= MAX_COMMAND_BUFFER_PIPELINE_BARRIER_BATCH_SIZE)
	{
		flush_pipeline_barriers();
	}

	uint32 srcQueueIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32 dstQueueIndex = VK_QUEUE_FAMILY_IGNORED;

	switch (barrier.srcQueue)
	{
	case DeviceQueueType::Transfer:
		srcQueueIndex = deviceContext->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		srcQueueIndex = deviceContext->computeQueue.familyIndex;
		break;
	default:
		srcQueueIndex = deviceContext->mainQueue.familyIndex;
		break;
	}

	switch (barrier.dstQueue)
	{
	case DeviceQueueType::Transfer:
		dstQueueIndex = deviceContext->transferQueue.familyIndex;
		break;
	case DeviceQueueType::Compute:
		dstQueueIndex = deviceContext->computeQueue.familyIndex;
		break;
	default:
		dstQueueIndex = deviceContext->mainQueue.familyIndex;
		break;
	}

	VulkanImage& vulkanImage = image.data.as<VulkanImage>();
	uint32 const slot = data.slot();
	CommandArena::BarrierInformation& barriers = owner->data.as<CommandArena>().store.pipelineBarrierInfos[slot];

	barriers.emplace_image_memory_barrier(VkImageMemoryBarrier2{
		.sType					= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext					= nullptr,
		.srcStageMask			= deviceContext->translate_pipeline_stage_flags(barrier.srcAccess.stages),
		.srcAccessMask			= deviceContext->translate_memory_access_flags(barrier.srcAccess.type),
		.dstStageMask			= deviceContext->translate_pipeline_stage_flags(barrier.dstAccess.stages),
		.dstAccessMask			= deviceContext->translate_memory_access_flags(barrier.dstAccess.type),
		.oldLayout				= deviceContext->translate_image_layout(barrier.oldLayout),
		.newLayout				= deviceContext->translate_image_layout(barrier.newLayout),
		.srcQueueFamilyIndex	= srcQueueIndex,
		.dstQueueFamilyIndex	= dstQueueIndex,
		.image					= vulkanImage.image,
		.subresourceRange = {
			.aspectMask		= deviceContext->translate_image_aspect_flags(barrier.subresource.aspectFlags),
			.baseMipLevel	= barrier.subresource.mipLevel,
			.levelCount		= barrier.subresource.levelCount,
			.baseArrayLayer = barrier.subresource.baseArrayLayer,
			.layerCount		= barrier.subresource.layerCount
		}
	});

	++imageBarrierCount;
}

void CommandBuffer::begin_rendering(RenderMetadata const& metadata)
{
	if (!metadata.data.valid())
	{
		return;
	}

	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();
	/**
	* Render attachments are cached so we don't have to create it every frame.
	*/
	VulkanRenderAttachments& attachment = metadata.data.as<VulkanRenderAttachments>();

	VkRenderingAttachmentInfo* depthAttachmentInfo = nullptr;
	VkRenderingAttachmentInfo* stencilAttachmentInfo = nullptr;

	if (metadata.depthAttachment.has_value())
	{
		depthAttachmentInfo = &attachment.depthAttachment;
	}

	if (metadata.stencilAttachment.has_value())
	{
		stencilAttachmentInfo = &attachment.stencilAttachment;
	}

	VkRenderingInfo renderingInfo{
		.sType		= VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
		.renderArea = { 
			.offset = {
				.x = metadata.renderArea.offset.x, 
				.y = metadata.renderArea.offset.y
			},
			.extent = {
				.width = metadata.renderArea.extent.width, 
				.height = metadata.renderArea.extent.height
			}
		},
		.layerCount				= 1u,
		.colorAttachmentCount	= attachment.colorAttachmentCount,
		.pColorAttachments		= attachment.colorAttachments.data(),
		.pDepthAttachment		= depthAttachmentInfo,
		.pStencilAttachment		= stencilAttachmentInfo,
	};

	vkCmdBeginRendering(hcommandBuffer, &renderingInfo);
}

void CommandBuffer::end_rendering()
{
	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();
	vkCmdEndRendering(hcommandBuffer);
}

void CommandBuffer::blit_image(Image const& src, Image const& dst, ImageBlitInfo const& info)
{
	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();

	VulkanImage& source = src.data.as<VulkanImage>();
	VulkanImage& destination = dst.data.as<VulkanImage>();

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask		= deviceContext->translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel		= info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount		= info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{.x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{.x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask		= deviceContext->translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel		= info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount		= info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{.x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{.x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout = deviceContext->translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout = deviceContext->translate_image_layout(info.dstImageLayout);
	VkFilter const filter = deviceContext->translate_texel_filter(info.filter);

	vkCmdBlitImage(hcommandBuffer, source.image, srcImgLayout, destination.image, dstImgLayout, 1, &region, filter);
}

void CommandBuffer::blit_image(Image const& src, Swapchain const& dst, ImageBlitInfo const& info)
{
	VkCommandBuffer hcommandBuffer	= data.as<VkCommandBuffer>();

	VulkanImage& source				= src.data.as<VulkanImage>();
	VulkanSwapchain& destination	= dst.data.as<VulkanSwapchain>();

	VkImage dstImage = destination.images[destination.nextImageIndex];

	VkImageBlit region{
		.srcSubresource = {
			.aspectMask		= deviceContext->translate_image_aspect_flags(info.srcSubresource.aspectFlags),
			.mipLevel		= info.srcSubresource.mipLevel,
			.baseArrayLayer = info.srcSubresource.baseArrayLayer,
			.layerCount		= info.srcSubresource.layerCount
		},
		.srcOffsets = {
			{ .x = info.srcOffset[0].x, .y = info.srcOffset[0].y, .z = info.srcOffset[0].z },
			{ .x = info.srcOffset[1].x, .y = info.srcOffset[1].y, .z = info.srcOffset[1].z },
		},
		.dstSubresource = {
			.aspectMask		= deviceContext->translate_image_aspect_flags(info.dstSubresource.aspectFlags),
			.mipLevel		= info.dstSubresource.mipLevel,
			.baseArrayLayer = info.dstSubresource.baseArrayLayer,
			.layerCount		= info.dstSubresource.layerCount
		},
		.dstOffsets = {
			{.x = info.dstOffset[0].x, .y = info.dstOffset[0].y, .z = info.dstOffset[0].z },
			{.x = info.dstOffset[1].x, .y = info.dstOffset[1].y, .z = info.dstOffset[1].z },
		}
	};

	VkImageLayout const srcImgLayout	= deviceContext->translate_image_layout(info.srcImageLayout);
	VkImageLayout const dstImgLayout	= deviceContext->translate_image_layout(info.dstImageLayout);
	VkFilter const filter				= deviceContext->translate_texel_filter(info.filter);

	vkCmdBlitImage(hcommandBuffer, source.image, srcImgLayout, dstImage, dstImgLayout, 1, &region, filter);
}

void CommandBuffer::set_viewport(rhi::Viewport const& viewport)
{
	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();

	VkViewport vp{
		.x			= viewport.x,
		.y			= viewport.y,
		.width		= viewport.width,
		.height		= viewport.height,
		.minDepth	= viewport.minDepth,
		.maxDepth	= viewport.maxDepth,
	};

	vkCmdSetViewport(hcommandBuffer, 0, 1, &vp);
}

void CommandBuffer::set_scissor(rhi::Rect2D const& rect)
{
	VkCommandBuffer hcommandBuffer = data.as<VkCommandBuffer>();

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

	vkCmdSetScissor(hcommandBuffer, 0, 1, &scissor);
}

//void CommandBuffer::submit(uint32 frame)
//{
//	CommandArena& arena = owner->data.as<CommandArena>();
//}

bool CommandPool::valid() const
{
	return data.valid();
}

bool CommandPool::allocate_command_buffer(CommandBuffer& commandBuffer)
{
	CommandArena& arena = data.as<CommandArena>();
	// Technically, the spec does not indicate that there is a limit to how many command buffers can be allocated on a pool.
	// But we limit it to a specific number, in this case, specified by the constant MAX_COMMAND_BUFFER_PER_POOL.
	if (arena.allocatedCount >= MAX_COMMAND_BUFFER_PER_POOL)
	{
		return false;
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo{
		.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool		= data.as<CommandArena>().commandPool,
		.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1u
	};
	VkCommandBuffer hcommandBuffer;
	VkResult result = vkAllocateCommandBuffers(deviceContext->device, &commandBufferAllocateInfo, &hcommandBuffer);
	ASSERTION(result == VK_SUCCESS);
	
	if (result != VK_SUCCESS)
	{
		return false;
	}

	size_t const index = arena.allocatedCount;
	arena.store.commandBufferHandles[index] = hcommandBuffer;

	new (&commandBuffer.data) RHIObjPtr{ static_cast<uint32>(index), arena.store.commandBufferHandles[index], rhi_obj_type_id_v<VkCommandBuffer> };

	commandBuffer.owner	= this;
	commandBuffer.deviceContext = deviceContext;

	++arena.allocatedCount;

	// Keep track of the allocated command buffer abstraction inside of the pool.
	allocatedCommandBuffers.push(&commandBuffer);

	return true;
}

void CommandPool::release_command_buffers()
{
	CommandArena& arena = data.as<CommandArena>();
	// For every command buffer allocated with this pool, we reset the state.
	for (CommandBuffer* cmd : allocatedCommandBuffers)
	{
		new (&cmd->data) RHIObjPtr{};
	}

	[[maybe_unused]] auto& store = arena.store;
	
	vkFreeCommandBuffers(deviceContext->device, arena.commandPool, static_cast<uint32>(arena.allocatedCount), arena.store.commandBufferHandles.data());

	std::fill(arena.store.commandBufferHandles.begin(), arena.store.commandBufferHandles.end(), VK_NULL_HANDLE);
	std::fill(arena.store.pipelineBarrierInfos.begin(), arena.store.pipelineBarrierInfos.end(), CommandArena::BarrierInformation{});

	arena.allocatedCount = 0;

	// Clear the contents of the command buffer tracker.
	allocatedCommandBuffers.empty();
}

void CommandPool::reset() const
{
	if (data.valid())
	{
		vkResetCommandPool(deviceContext->device, data.as<CommandArena>().commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

		for (CommandBuffer* cmdBuffer : allocatedCommandBuffers)
		{
			cmdBuffer->state = CommandBuffer::State::Initial;
		}
	}
}

}

}