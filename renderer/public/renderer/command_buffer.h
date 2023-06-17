#pragma once
#ifndef RENDERER_RHI_VULKAN_COMMAND_BUFFER
#define RENDERER_RHI_VULKAN_COMMAND_BUFFER

#include "containers/array.h"
#include "rhi.h"

namespace rhi
{

struct DeviceContext;

namespace command
{

struct ImageBlitInfo
{
	ImageLayout				srcImageLayout;
	std::array<Offset3D, 2> srcOffset;
	ImageSubresource		srcSubresource;
	ImageLayout				dstImageLayout;
	std::array<Offset3D, 2> dstOffset;
	ImageSubresource		dstSubresource;
	TexelFilter				filter;
};

struct RenderAttachment
{
	Image*				image;
	ImageLayout			layout;
	AttachmentLoadOp	loadOp;
	AttachmentStoreOp	storeOp;
	ClearValue			clearValue;
};

struct RenderMetadata
{
	RHIObjPtr						data;	// Stores the structure's actual API representation. We don't need to create this every frame.
	ftl::Array<RenderAttachment>	colorAttachments;
	std::optional<RenderAttachment> depthAttachment;
	std::optional<RenderAttachment> stencilAttachment;
	Rect2D							renderArea;
};

/**
* Pipeline barrier access information.
*/
struct Access
{
	PipelineStage		stages	= PipelineStage::Top_Of_Pipe;
	MemoryAccessType	type	= MemoryAccessType::Memory_Read;
};

/**
* From lpotrick:
* 1. A buffer pipeline barrier is just a memory barrier + ownership transfer to other queues.
* 2. An image pipeline barrier is just a memory barrier + layout transition + ownership transfer.
* 
* Ignore all accesses except:
* a) read
* b) write
* c) host read
* d) host write
* 
* Others are just old API bloat. very rarely you can employ finer access.
*/
struct PipelineMemoryBarrier
{
	//RHIObjPtr	data;	// Stores the structure's actual API representation. We don't need to create this every frame.
	Access		srcAccess = {};
	Access		dstAccess = {};
};

struct BufferMemoryBarrier
{
	uint32				size		= std::numeric_limits<uint32>::max();
	uint32				offset		= 0;
	Access				srcAccess	= {};
	Access				dstAccess	= {};
	DeviceQueueType		srcQueue	= DeviceQueueType::Main;
	DeviceQueueType		dstQueue	= DeviceQueueType::Main;
};

struct ImageMemoryBarrier
{
	Access				srcAccess	= {};
	Access				dstAccess	= {};
	ImageLayout			oldLayout	= ImageLayout::Undefined;
	ImageLayout			newLayout	= ImageLayout::Undefined;
	ImageSubresource	subresource = {};
	DeviceQueueType		srcQueue	= DeviceQueueType::Main;
	DeviceQueueType		dstQueue	= DeviceQueueType::Main;
};

/**
* A thin abstraction wrapper around Vulkan command buffers and DX12's command list.
* We use Vulkan terminologies because we're Vulkan-first.
*/
struct CommandBuffer
{
	enum class State : uint8
	{
		Initial,
		Recording,
		Executable,
		Pending,
		Invalid
	};

	ftl::Ref<struct CommandPool>		owner;
	ftl::Ref<struct rhi::DeviceContext>	deviceContext;
	RHIObjPtr							data;
	uint32								memoryBarrierCount;
	uint32								imageBarrierCount;
	uint32								bufferBarrierCount;
	State								state = State::Invalid;

	bool is_recording				();
	bool valid						();
	void reset						();
	bool begin						();
	void end						();
	void flush_pipeline_barriers	();

	//void draw() const;
	//void draw_indirect() const;
	//void draw_indexed_indirect() const;
	//void bind_push_constant() const;
	//void bind_vertex_buffer	() const;
	//void bind_index_buffer	() const;
	void bind_pipeline				(RasterPipeline const& pipeline);
	void pipeline_barrier			(PipelineMemoryBarrier const& barrier);
	void pipeline_barrier			(Buffer const& buffer, BufferMemoryBarrier const& barrier);
	void pipeline_barrier			(Image const& image, ImageMemoryBarrier const& barrier);
	void begin_rendering			(RenderMetadata const& metadata);
	void end_rendering				();
	void blit_image					(Image const& src, Image const& dst, ImageBlitInfo const& info);
	void blit_image					(Image const& src, Swapchain const& dst, ImageBlitInfo const& info);
	void set_viewport				(rhi::Viewport const& viewport);
	void set_scissor				(rhi::Rect2D const& rect);
	//void submit						(uint32 frame);
	//void copy_buffer() const;
	//void copy_buffer_to_image() const;
	//void copy_image_to_buffer() const;
	//void bind_descriptor_set() const;
};

/**
* A thin abstraction wrapper around Vulkan command pools and DX12's command allocator.
* We use Vulkan terminologies because we're Vulkan-first.
*/
struct CommandPool
{
	ftl::Array<CommandBuffer*>		allocatedCommandBuffers;
	ftl::Ref<rhi::DeviceContext>	deviceContext;
	RHIObjPtr						data;

	bool	valid					() const;
	bool	allocate_command_buffer	(CommandBuffer& commandBuffer);
	void	release_command_buffers	();
	void	reset					() const;
};

}

}

#endif // !RENDERER_RHI_VULKAN_COMMAND_BUFFER
