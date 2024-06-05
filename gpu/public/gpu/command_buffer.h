#pragma once
#ifndef RHI_COMMAND_BUFFER_H
#define RHI_COMMAND_BUFFER_H

#include <array>
#include "swapchain.h"
#include "pipeline.h"
#include "buffer.h"

namespace rhi
{
struct APIContext;
class CommandPool;

struct RenderAttachment
{
	Image const* pImage;
	ImageLayout imageLayout;
	AttachmentLoadOp loadOp;
	AttachmentStoreOp storeOp;
};

struct RenderingInfo
{
	std::span<RenderAttachment>	colorAttachments;
	RenderAttachment* depthAttachment;
	RenderAttachment* stencilAttachment;
	Rect2D renderArea;
};

struct ImageBlitInfo
{
	ImageLayout srcImageLayout;
	std::array<Offset3D, 2> srcOffset;
	ImageSubresource srcSubresource;
	ImageLayout	dstImageLayout;
	std::array<Offset3D, 2> dstOffset;
	ImageSubresource dstSubresource;
	TexelFilter	filter;
};

struct ImageClearInfo
{
	ImageLayout dstImageLayout;
	ClearValue clearValue;
	ImageSubresource subresource;
};

struct BufferClearInfo
{
	size_t offset;
	size_t size;
	uint32 data;
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
struct MemoryBarrierInfo
{
	Access srcAccess = {};
	Access dstAccess = {};
};

struct BufferBarrierInfo
{
	size_t size = std::numeric_limits<size_t>::max();
	size_t offset = 0;
	Access srcAccess = {};
	Access dstAccess = {};
	DeviceQueue	srcQueue = DeviceQueue::Main;
	DeviceQueue	dstQueue = DeviceQueue::Main;
};

//struct BufferViewBarrierInfo
//{
//	Access srcAccess = {};
//	Access dstAccess = {};
//	DeviceQueue	srcQueue = DeviceQueue::Main;
//	DeviceQueue	dstQueue = DeviceQueue::Main;
//};

struct ImageBarrierInfo
{
	Access srcAccess = {};
	Access dstAccess = {};
	ImageLayout	oldLayout = ImageLayout::Undefined;
	ImageLayout	newLayout = ImageLayout::Undefined;
	ImageSubresource subresource = {};
	DeviceQueue	srcQueue = DeviceQueue::Main;
	DeviceQueue	dstQueue = DeviceQueue::Main;
};

struct DrawInfo
{
	uint32 vertexCount;
	uint32 instanceCount = 1;
	uint32 firstVertex;
	uint32 firstInstance;
};

struct DrawIndexedInfo
{
	uint32 indexCount;
	uint32 instanceCount = 1;
	uint32 firstIndex;
	uint32 vertexOffset;
	uint32 firstInstance;
};

struct DrawIndirectInfo
{
	size_t offset;
	uint32 drawCount;
	uint32 stride;
	bool indexed;
};

struct DrawIndirectCountInfo
{
	size_t offset; // offset into the buffer that contains the packed draw parameters.
	size_t countBufferOffset; // offset into a buffer that contains the packed unsigned 32-bit integer that signifies draw count.
	uint32 maxDrawCount;
	uint32 stride;
	bool indexed;
};

struct BindVertexBufferInfo
{
	uint32 firstBinding;
	size_t offset;
};

struct BindIndexBufferInfo
{
	size_t offset;
	IndexType indexType = IndexType::Uint_32;
};

struct BufferCopyInfo
{
	size_t srcOffset;
	size_t dstOffset;
	size_t size;
};

struct BufferImageCopyInfo
{
	size_t bufferOffset;
	ImageLayout dstImageLayout = ImageLayout::Transfer_Dst;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

struct ImageBufferCopyInfo
{
	size_t bufferOffset;
	ImageLayout srcImageLayout = ImageLayout::Transfer_Src;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

struct ImageCopyInfo
{
	ImageLayout srcImageLayout = ImageLayout::Transfer_Src;
	ImageSubresource srcSubresource = {};
	Offset3D srcOffset = {};
	ImageLayout dstImageLayout = ImageLayout::Transfer_Dst;
	ImageSubresource dstSubresource = {};
	Offset3D dstOffset;
	Extent3D extent;
};

struct DebugLabelInfo
{
	std::string_view name;
	float32 color[4] = { 1.f, 1.f, 1.f, 1.f };
};

struct BindPushConstantInfo
{
	void const* data;
	size_t offset = 0;
	size_t size;
	ShaderStage shaderStage = ShaderStage::All;
};

/**
* A thin abstraction wrapper around Vulkan command buffers and DX12's command list.
* We use Vulkan terminologies because we're Vulkan-first.
*/
class CommandBuffer : public Resource
{
public:
	enum class State : uint8
	{
		Initial,
		Recording,
		Executable,
		Pending,
		Invalid
	};

	RHI_API CommandBuffer() = default;
	RHI_API ~CommandBuffer() = default;

	RHI_API CommandBuffer(CommandBuffer&&) noexcept;
	RHI_API auto operator=(CommandBuffer&&) noexcept -> CommandBuffer&;

	RHI_API auto info() const -> CommandBufferInfo const&;
	RHI_API auto is_initial() const -> bool;
	RHI_API auto is_recording() const -> bool;
	RHI_API auto is_executable() const -> bool;
	RHI_API auto is_pending_complete() const -> bool;
	RHI_API auto is_completed() -> bool;
	RHI_API auto is_invalid() const -> bool;
	RHI_API auto current_state() const -> State;

	RHI_API auto reset() -> void;

	RHI_API auto begin() -> bool;
	RHI_API auto end() -> void;

	RHI_API auto clear(Image const& image) -> void;
	RHI_API auto clear(Image const& image, ImageClearInfo const& info) -> void;
	RHI_API auto clear(Buffer const& buffer) -> void;
	RHI_API auto clear(Buffer const& buffer, BufferClearInfo const& info) -> void;

	RHI_API auto draw(DrawInfo const& info) const -> void;
	RHI_API auto draw_indexed(DrawIndexedInfo const& info) const -> void;
	RHI_API auto draw_indirect(Buffer const& drawInfoBuffer, DrawIndirectInfo const& info) const -> void;
	RHI_API auto draw_indirect_count(Buffer const& drawInfoBuffer, Buffer const& drawCountBuffer, DrawIndirectCountInfo const& info) const -> void;

	RHI_API auto bind_vertex_buffer(Buffer const& buffer, BindVertexBufferInfo const& info) -> void;
	RHI_API auto bind_index_buffer(Buffer const& buffer, BindIndexBufferInfo const& info) -> void;
	RHI_API auto bind_push_constant(BindPushConstantInfo const& info) -> void;

	//template <typename T>
	//auto bind_push_constant(T const& data, size_t offset = 0, ShaderStage shaderStage = ShaderStage::All) -> void
	//{
	//	bind_push_constant(&data, sizeof(T), offset, shaderStage);
	//}

	RHI_API auto bind_pipeline(RasterPipeline const& pipeline) -> void;

	RHI_API auto pipeline_barrier(MemoryBarrierInfo const& barrier) -> void;
	RHI_API auto pipeline_barrier(Buffer const& buffer, BufferBarrierInfo const& barrier) -> void;
	RHI_API auto pipeline_barrier(Image const& image, ImageBarrierInfo const& barrier) -> void;
	RHI_API auto flush_barriers() -> void;

	RHI_API auto begin_rendering(RenderingInfo const& info) -> void;
	RHI_API auto end_rendering() -> void;

	RHI_API auto copy_buffer_to_buffer(Buffer const& src, Buffer const& dst, BufferCopyInfo const& info) -> void;
	RHI_API auto copy_buffer_to_image(Buffer const& src, Image const& dst, BufferImageCopyInfo const& info) -> void;
	RHI_API auto copy_image_to_buffer(Image const& src, Buffer const& dst, ImageBufferCopyInfo const& info) -> void;
	RHI_API auto copy_image_to_image(Image const& src, Image const& dst, ImageCopyInfo const& info) -> void;
	RHI_API auto blit_image(Image const& src, Image const& dst, ImageBlitInfo const& info) -> void;
	RHI_API auto blit_image(Image const& src, Swapchain const& dst, ImageBlitInfo const& info) -> void;

	RHI_API auto set_viewport(rhi::Viewport const& viewport) -> void;
	RHI_API auto set_scissor(rhi::Rect2D const& rect) -> void;

	RHI_API auto begin_debug_label(DebugLabelInfo const& info) const -> void;
	RHI_API auto end_debug_label() const -> void;

private:
	friend struct APIContext;
	friend class CommandPool;

	CommandBufferInfo m_info;
	Fence m_completion_timeline;
	uint64 m_recording_timeline;
	State m_state = State::Invalid;

	CommandBuffer(
		CommandBufferInfo&& info,
		APIContext* context,
		void* data,
		resource_type typeId
	);
};

}

#endif // !RHI_COMMAND_BUFFER_H
