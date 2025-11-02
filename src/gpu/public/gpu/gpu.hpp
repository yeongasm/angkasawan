#pragma once
#ifndef GPU_H
#define GPU_H

#include <atomic>
#include <expected>
#include <array>

#include "common.hpp"
#include "shared_resource.hpp"

namespace gpu
{
class MemoryBlock final : public shared<MemoryBlock>
{
public:
	using shared<MemoryBlock>::shared;

	auto info() const -> MemoryBlockInfo const&;
	auto valid() const -> bool;
	auto aliased() const -> bool;
	auto size() const -> size_t;

	static auto from(Device& device, MemoryBlockAllocateInfo&& info) -> MemoryBlock;
private:
	friend shared<MemoryBlock>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Buffer final : public shared<Buffer>
{
public:
	using shared<Buffer>::shared;

	auto info() const -> BufferInfo const&;
	auto valid() const -> bool;
	auto data() const -> void*;
	auto size() const -> size_t;
	auto write(void const* data, size_t size, size_t offset) const -> void;
	auto clear() const -> void;
	auto is_host_visible() const -> bool;
	auto is_transient() const -> bool;
	auto gpu_address() const -> device_address_t;
	auto id() const -> resource_id_t;

	template <typename T>
	auto write(T const& data, size_t offset) const -> void
	{
		write(&data, sizeof(T), offset);
	}

	template <typename T>
	auto write(std::span<T> data, size_t offset) const -> void
	{
		write(data.data(), data.size_bytes(), offset);
	}

	static auto memory_requirement(Device& device, BufferInfo const& info) -> MemoryRequirementInfo;

	static auto from(Device& device, BufferInfo&& info, MemoryBlock memoryBlock = {}) -> Buffer;
private:
	friend shared<Buffer>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Image final : public shared<Image>
{
public:
	using shared<Image>::shared;

	auto info() const -> ImageInfo const&;
	auto valid() const -> bool;
	auto is_swapchain_image() const -> bool;
	auto is_transient() const -> bool;
	auto id() const -> resource_id_t;

	static auto memory_requirement(Device& device, ImageInfo const& info) -> MemoryRequirementInfo;

	static auto from(Device& device, ImageInfo&& info, MemoryBlock memoryBlock = {}) -> Image;
private:
	friend shared<Image>;
	friend class Swapchain;

	static auto from(Device& device, struct SwapchainImpl& swapchain) -> lib::array<Image>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Sampler final : public shared<Sampler>
{
public:
	using shared<Sampler>::shared;

	auto info() const -> SamplerInfo const&;
	auto valid() const -> bool;
	auto id() const -> resource_id_t;

	static auto from(Device& device, SamplerInfo&& info) -> Sampler;
private:
	friend shared<Sampler>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Semaphore final : public shared<Semaphore>
{
public:
	using shared<Semaphore>::shared;

	auto info() const -> SemaphoreInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, SemaphoreInfo&& info) -> Semaphore;
private:
	friend shared<Semaphore>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Fence final : public shared<Fence>
{
public:
	using shared<Fence>::shared;

	auto info() const -> FenceInfo const&;
	auto valid() const -> bool;
	auto value() const -> uint64;
	auto wait_for_value(uint64 value, uint64 timeout = std::numeric_limits<uint64>::max()) const -> bool;
	auto signal(uint64 value) const -> void;
	auto signal() const -> void;

	static auto from(Device& device, FenceInfo&& info) -> Fence;
private:
	friend shared<Fence>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Event final : public shared<Event>
{
public:
	using shared<Event>::shared;

	auto info() const -> EventInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, EventInfo&& info) -> Event;
private:
	friend shared<Event>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Swapchain final : public shared<Swapchain>
{
public:
	using shared<Swapchain>::shared;

	auto info() const -> SwapchainInfo const&;
	auto valid() const -> bool;
	auto num_images() const -> uint32;
	/**
	* Returns the current free image in the swapchain *after* acquire_next_image() is called.
	*/
	auto current_image() const -> Image;
	/**
	* Returns the index of the current free image in the swapchain *after* acquire_next_image() is called.
	*/
	auto current_image_index() const -> uint32;
	/**
	* Has to be called at the start of the frame.
	*/
	auto acquire_next_image() -> Image;
	auto current_acquire_semaphore() const -> Semaphore;
	auto current_present_semaphore() const -> Semaphore;
	auto cpu_frame_count() const -> uint64;
	auto gpu_fence() const -> Fence;
	auto image_format() const -> Format;

	auto resize(Extent2D dim) -> bool;

	static auto from(Device& device, SwapchainInfo&& info, Swapchain previousSwapchain = {}) -> Swapchain;
private:
	friend shared<Swapchain>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

class Shader final : public shared<Shader>
{
public:
	using shared<Shader>::shared;

	auto info() const -> ShaderInfo const&;
	auto valid() const -> bool;

	explicit operator bool() const;

	static auto from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> Shader;
private:
	friend shared<Shader>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

struct RasterPipelineShaderInfo
{
	Shader vertexShader;
	Shader pixelShader;
	std::span<ShaderAttribute> vertexInputAttrib;
};

struct ComputePipelineShaderInfo
{
	Shader computeShader;
};

class Pipeline : public shared<Pipeline>
{
public:
	using shared<Pipeline>::shared;

	auto type() const -> PipelineType;
	auto valid() const -> bool;

	static auto from(Device& device, RasterPipelineShaderInfo const& pipelineShaderInfo, RasterPipelineInfo&& info) -> Pipeline;
	static auto from(Device& device, ComputePipelineShaderInfo const& pipelineShaderInfo, ComputePipelineInfo&& info) -> Pipeline;
private:
	friend shared<Pipeline>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

struct RenderAttachment
{
	Image image;
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
	Image src;
	Image dst;
	ImageLayout srcImageLayout;
	std::array<Offset3D, 2> srcOffset;
	ImageSubresource srcSubresource;
	ImageLayout	dstImageLayout;
	std::array<Offset3D, 2> dstOffset;
	ImageSubresource dstSubresource;
	TexelFilter	filter;
};

struct ImageSwapchainBlitInfo
{
	Image src;
	Swapchain dst;
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
	Image image;
	ImageLayout dstImageLayout;
	ClearValue clearValue;
	ImageSubresource subresource;
};

struct BufferClearInfo
{
	Buffer buffer;
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
	Buffer buffer;
	size_t size = std::numeric_limits<size_t>::max();
	size_t offset = 0;
	Access srcAccess = {};
	Access dstAccess = {};
	DeviceQueue	srcQueue = DeviceQueue::Main;
	DeviceQueue	dstQueue = DeviceQueue::Main;
};

struct ImageBarrierInfo
{
	Image image;
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
	Buffer drawInfoBuffer;
	size_t offset;
	uint32 drawCount;
	uint32 stride;
	bool indexed;
};

struct DrawIndirectCountInfo
{
	Buffer drawInfoBuffer;
	Buffer drawCountBuffer;
	size_t offset; // offset into the buffer that contains the packed draw parameters.
	size_t countBufferOffset; // offset into a buffer that contains the packed unsigned 32-bit integer that signifies draw count.
	uint32 maxDrawCount;
	uint32 stride;
	bool indexed;
};

struct DispatchInfo
{
	uint32 x = 1;
	uint32 y = 1;
	uint32 z = 1;
};

struct DispatchIndirectInfo
{
	Buffer dispatchInfoBuffer;
	size_t offset = 0;
};

struct BindVertexBufferInfo
{
	Buffer buffer;
	uint32 firstBinding;
	size_t offset;
};

struct BindIndexBufferInfo
{
	Buffer buffer;
	size_t offset;
	IndexType indexType = IndexType::Uint_32;
};

struct BufferCopyInfo
{
	Buffer src;
	Buffer dst;
	size_t srcOffset;
	size_t dstOffset;
	size_t size;
};

struct BufferImageCopyInfo
{
	Buffer src;
	Image dst;
	size_t bufferOffset;
	uint32 bufferRowLength;
	uint32 bufferImageHeight;
	ImageLayout dstImageLayout = ImageLayout::Transfer_Dst;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

struct ImageBufferCopyInfo
{
	Image src;
	Buffer dst;
	size_t bufferOffset;
	ImageLayout srcImageLayout = ImageLayout::Transfer_Src;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

struct ImageCopyInfo
{
	Image src;
	Image dst;
	ImageLayout srcImageLayout = ImageLayout::Transfer_Src;
	ImageSubresource srcSubresource = {};
	Offset3D srcOffset = {};
	ImageLayout dstImageLayout = ImageLayout::Transfer_Dst;
	ImageSubresource dstSubresource = {};
	Offset3D dstOffset;
	Extent3D extent;
};

struct EventSignalInfo
{
	Event event;
	std::span<MemoryBarrierInfo> memoryBarrierInfo;
	std::span<BufferBarrierInfo> bufferBarrierInfo;
	std::span<ImageBarrierInfo> imageBarrierInfo;
};

using EventWaitInfo = EventSignalInfo;

struct EventResetInfo
{
	Event event;
	PipelineStage stage;
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

struct PresentInfo
{
	std::span<Swapchain> swapchains;
};

class CommandPool : public shared<CommandPool>
{
public:
	using shared<CommandPool>::shared;

	auto info() const -> CommandPoolInfo const&;
	auto valid() const -> bool;
	auto reset() const -> void;

	static auto from(Device& device, CommandPoolInfo&& info) -> CommandPool;
private:
	friend shared<CommandPool>;

	static auto zombify(Device&, ref_counted_base&) -> void;
};

constexpr auto workgroup_size(uint32 elementSize, uint32 blockSize) -> uint32
{
	return (elementSize + 1 - blockSize) / blockSize;
}

/*
* Should not have persistent lifetimes.
* If persistent lifetime is required, then have a CommandQueue for each frame in flight
*/
class CommandRecorder : lib::non_copyable
{
public:
	CommandRecorder() = default;

	auto valid() const -> bool;
	auto recording_timeline() const -> uint64;
	auto current_timeline_fence() const -> Fence;

	auto begin() -> bool;
	auto end() -> void;

	auto clear(Image& image) -> void;
	auto clear(ImageClearInfo const& info) -> void;
	auto clear(Buffer& buffer) -> void;
	auto clear(BufferClearInfo const& info) -> void;

	auto draw(DrawInfo const& info) const -> void;
	auto draw_indexed(DrawIndexedInfo const& info) const -> void;
	auto draw_indirect(DrawIndirectInfo const& info) const -> void;
	auto draw_indirect_count(DrawIndirectCountInfo const& info) const -> void;

	auto bind_vertex_buffer(BindVertexBufferInfo const& info) -> void;
	auto bind_index_buffer(BindIndexBufferInfo const& info) -> void;
	auto bind_push_constant(BindPushConstantInfo const& info) -> void;

	auto bind_pipeline(Pipeline& pipeline) -> void;

	auto dispatch(DispatchInfo const& info) -> void;
	auto dispatch_indirect(DispatchIndirectInfo const& info) -> void;

	auto signal_event(EventSignalInfo const& info) -> void;
	auto wait_events(std::span<EventWaitInfo const> const& infos) -> void;
	auto wait_event(EventWaitInfo const& info) -> void;
	auto reset_event(EventResetInfo const& info) -> void;

	auto pipeline_barrier(MemoryBarrierInfo const& barrier) -> void;
	auto pipeline_buffer_barrier(BufferBarrierInfo const& barrier) -> void;
	auto pipeline_image_barrier(ImageBarrierInfo const& barrier) -> void;
	auto flush_barriers() -> void;

	auto begin_rendering(RenderingInfo const& info) -> void;
	auto end_rendering() -> void;

	auto copy_buffer_to_buffer(BufferCopyInfo const& info) -> void;
	auto copy_buffer_to_image(BufferImageCopyInfo const& info) -> void;
	auto copy_image_to_buffer(ImageBufferCopyInfo const& info) -> void;
	auto copy_image_to_image(ImageCopyInfo const& info) -> void;
	auto blit_image(ImageBlitInfo const& info) -> void;
	auto blit_image_swapchain(ImageSwapchainBlitInfo const& info) -> void;

	auto set_viewport(Viewport const& viewport) -> void;
	auto set_scissor(Rect2D const& rect) -> void;

	auto begin_debug_label(DebugLabelInfo const& info) const -> void;
	auto end_debug_label() const -> void;

	/**
	* Requests a CommandBuffer from the CommandPool.
	*/
	static auto from(CommandPool& commandPool) -> CommandRecorder;
private:
	friend class Device;

	CommandPool m_cmdPool;
	uint64 m_index;

	CommandRecorder(CommandPool const& pool, uint64 commandBufferIndex);
};

using device_address = uint64;

struct SubmitInfo
{
	DeviceQueue queue;
	std::span<CommandRecorder> commandRecorders;
	std::span<Semaphore> waitSemaphores;
	std::span<Semaphore> signalSemaphores;
	std::span<std::pair<Fence, uint64>> waitFences;
	std::span<std::pair<Fence, uint64>> signalFences;
};

class Device : public lib::non_copyable_non_movable
{
public:
	using cpu_timeline_t = std::atomic_uint64_t::value_type;

	Device() = default;
	~Device() = default;

	[[nodiscard]] auto info() const -> DeviceInfo const&;
	[[nodiscard]] auto config() const -> DeviceConfig const&;
	auto wait_idle() const -> void;
	[[nodiscard]] auto cpu_timeline() const -> uint64;
	[[nodiscard]] auto gpu_timeline() const -> uint64;

	auto submit(SubmitInfo const& info) -> bool;
	auto present(PresentInfo const& info) -> bool;

	/**
	* Should be called every frame.
	*/
	auto clear_garbage() -> void;

	static auto from(DeviceInitInfo const& info) -> std::expected<std::unique_ptr<Device>, std::string_view>;
	static auto destroy(std::unique_ptr<Device>& device) -> void;
protected:
	DeviceInfo m_info;
	DeviceInitInfo m_initInfo;
	DeviceConfig m_config;
	std::atomic_uint64_t m_cpuTimeline;
	Fence m_gpuTimeline;
};
}

#endif // !GPU_H
