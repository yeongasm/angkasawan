#pragma once
#ifndef GPU_H
#define GPU_H

#include <variant>
#include <expected>
#include <atomic>

#include "lib/handle.hpp"
#include "lib/paged_array.hpp"

#include "common.hpp"

#define NOCOPYANDMOVE(Object)					\
	Object(Object const&) = delete;				\
	Object& operator=(Object const&) = delete;	\
	Object(Object&&) = delete;					\
	Object& operator=(Object&&) = delete;

#define NOCOPY(Object)							\
	Object(Object const&) = delete;				\
	Object& operator=(Object const&) = delete;

namespace gpu
{
class Semaphore;
class Fence;
class Buffer;
class Image;
class Sampler;
class Swapchain;
class Shader;
class Pipeline;
class CommandPool;
class CommandBuffer;
class Device;

template <typename T>
class Resource
{
public:
	using resource_type = T;
	using pointer = resource_type*;
	using const_pointer = resource_type const*;
	using reference = resource_type&;
	using const_reference = resource_type const&;

	Resource() = default;

	Resource(uint64 id, resource_type& resource) :
		m_id{ id },
		m_resource{ &resource }
	{
		m_resource->reference();
	}

	~Resource() { destroy(); }

	Resource(Resource const& rhs) :
		m_id{ rhs.m_id },
		m_resource{ rhs.m_resource }
	{
		m_resource->reference();
	}

	Resource& operator=(Resource const& rhs)
	{
		if (this != &rhs)
		{
			// If the current resource object already holds something, destroy it first before a reassignment.
			destroy();

			m_id = rhs.m_id;
			m_resource = rhs.m_resource;

			m_resource->reference();
		}
		return *this;
	}

	Resource(Resource&& rhs) noexcept :
		m_id{ std::move(rhs.m_id) },
		m_resource{ std::move(rhs.m_resource) }
	{
		new (&rhs) Resource{};
	}

	Resource& operator=(Resource&& rhs) noexcept
	{
		if (this != &rhs)
		{
			// If the current resource object already holds something, destroy it first before a reassignment.
			destroy();

			m_id = std::move(rhs.m_id);
			m_resource = std::move(rhs.m_resource);

			new (&rhs) Resource{};
		}
		return *this;
	}

	auto operator->() const -> pointer { return m_resource; }
	auto operator*() const -> reference { return *m_resource; }

	auto valid() const -> bool { return m_resource != nullptr && m_resource->valid(); }

	explicit operator bool() const { return valid(); }

	auto destroy() -> void
	{
		if (m_resource && 
			std::cmp_equal(m_resource->dereference(), 0))
		{
			resource_type::destroy(*m_resource, m_id);
		} 
		m_resource = nullptr;
		m_id = std::numeric_limits<uint64>::max();
	}

	auto id() const -> uint64 { return m_id; }
private:
	uint64				m_id		= std::numeric_limits<uint64>::max();
	resource_type*		m_resource	= nullptr;
};

using DeviceAddress = uint64;

struct BufferBindInfo
{
	size_t offset = 0;
	size_t size = std::numeric_limits<size_t>::max();
	uint32 index;
};

struct ImageBindInfo
{
	Resource<Sampler> sampler;
	uint32 index;
};

struct SamplerBindInfo
{
	uint32 index;
};

class RefCountedResource
{
public:
	RefCountedResource() = default;
	~RefCountedResource() = default;

	NOCOPYANDMOVE(RefCountedResource)

	auto reference([[maybe_unused]] bool chain = false) -> void
	{
		m_refCount.fetch_add(1, std::memory_order_relaxed);
	}

	auto dereference([[maybe_unused]] bool chain = false) -> uint64
	{
		return m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
	}

	auto ref_count() const -> uint64
	{
		return m_refCount.load(std::memory_order_acquire);
	}

private:
	std::atomic_uint64_t m_refCount = {};
};

class DeviceResource : public RefCountedResource
{
public:
	DeviceResource() = default;

	DeviceResource(Device& device) :
		m_device{ &device }
	{}

	~DeviceResource() = default;

	auto device() const -> Device& 
	{ 
		ASSERTION(m_device && "Invalid resource! No device attached to this resource.");
		return *m_device;
	}
protected:
	Device* m_device = nullptr;
};

struct MemoryBlockAllocateInfo
{
	lib::string name;
	MemoryRequirementInfo memoryRequirement;
};

class MemoryBlock : public DeviceResource
{
public:
	MemoryBlock() = default;
	~MemoryBlock() = default;

	auto info() const -> MemoryBlockInfo const&;
	auto valid() const -> bool;
	auto aliased() const -> bool;
	auto size() const -> size_t;

	static auto from(Device& device, MemoryBlockAllocateInfo&& info) -> Resource<MemoryBlock>;
protected:
	friend class Resource<MemoryBlock>;

	static auto destroy(MemoryBlock& resource, uint64 id) -> void;

	MemoryBlock(Device& device, bool aliased);

	MemoryBlockInfo m_info;
	bool const m_aliased = false;
};

class Semaphore : public DeviceResource
{
public:
	Semaphore() = default;
	~Semaphore() = default;

	auto info() const -> SemaphoreInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, SemaphoreInfo&& info) -> Resource<Semaphore>;
protected:
	friend class Resource<Semaphore>;

	static auto destroy(Semaphore& resource, uint64 id) -> void;

	Semaphore(Device& device);

	SemaphoreInfo m_info;
};

class Fence : public DeviceResource
{
public:
	using fence_value_t = uint64;

	Fence() = default;
	~Fence() = default;

	auto info() const -> FenceInfo const&;
	auto valid() const -> bool;
	auto value() const -> fence_value_t;
	auto signal(fence_value_t const value) const -> void;
	auto signal() const -> void;
	auto wait_for_value(fence_value_t value, uint64 timeout = std::numeric_limits<uint64>::max()) const -> bool;

	static auto from(Device& device, FenceInfo&& info) -> Resource<Fence>;
protected:
	friend class Resource<Fence>;

	static auto destroy(Fence const& resource, uint64 id) -> void;

	Fence(Device& device);

	FenceInfo m_info;
};

class Event : public DeviceResource
{
public:
	Event() = default;
	~Event() = default;

	auto info() const -> EventInfo const&;
	auto valid() const -> bool;
	auto state() const -> EventState;
	auto signal() const -> void;
	auto reset() const -> void;

	static auto from(Device& device, EventInfo&& info) -> Resource<Event>;

protected:
	friend class Resource<Event>;

	static auto destroy(Event const& resource, uint64 id) -> void;

	Event(Device& device);

	EventInfo m_info;
};

class Buffer : public DeviceResource
{
public:
	Buffer() = default;
	~Buffer() = default;

	auto info() const -> BufferInfo const&;
	auto valid() const -> bool;
	auto data() const -> void*;
	auto size() const -> size_t;
	auto write(void const* data, size_t size, size_t offset) const -> void;
	auto clear() const -> void;
	auto is_host_visible() const -> bool;
	auto is_transient() const -> bool;
	auto gpu_address() const -> DeviceAddress;
	auto bind(BufferBindInfo const& info) const -> BufferBindInfo;

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

	static auto from(Device& device, BufferInfo&& info, Resource<MemoryBlock> memoryBlock = {}) -> Resource<Buffer>;
protected:
	friend class Resource<Buffer>;

	static auto destroy(Buffer& resource, uint64 id) -> void;

	Buffer(Device& device);

	BufferInfo m_info;
};

class Image : public DeviceResource
{
public:
	Image() = default;
	~Image() = default;

	auto info() const -> ImageInfo const&;
	auto valid() const -> bool;
	auto is_swapchain_image() const -> bool;
	auto is_transient() const -> bool;
	auto bind(ImageBindInfo const& info) const -> ImageBindInfo;

	static auto memory_requirement(Device& device, ImageInfo const& info) -> MemoryRequirementInfo;

	static auto from(Device& device, ImageInfo&& info, Resource<MemoryBlock> memoryBlock = {}) -> Resource<Image>;
	static auto from(Swapchain& swapchain) -> lib::array<Resource<Image>>;
protected:
	friend class Resource<Image>;
	friend class Swapchain;

	static auto destroy(Image& resource, uint64 id) -> void;

	Image(Device& device);

	ImageInfo m_info;
};

class Sampler : public DeviceResource
{
public:
	Sampler() = default;
	~Sampler() = default;

	auto info() const -> SamplerInfo const&;
	auto valid() const -> bool;
	auto info_packed() const -> uint64;
	auto bind(SamplerBindInfo const& info) const -> SamplerBindInfo;

	static auto from(Device& device, SamplerInfo&& info) -> Resource<Sampler>;
protected:
	friend class Resource<Sampler>;

	static auto destroy(Sampler& resource, uint64 id) -> void;

	Sampler(Device& device);

	SamplerInfo m_info;
	uint64 m_packedInfoBits;
};

class Swapchain : public DeviceResource
{
public:
	Swapchain() = default;
	~Swapchain() = default;

	auto info() const -> SwapchainInfo const&;
	auto valid() const -> bool;
	auto state() const -> SwapchainState;
	auto num_images() const -> uint32;
	/**
	* Returns the current free image in the swapchain *after* acquire_next_image() is called.
	*/
	auto current_image() const -> Resource<Image>;
	/**
	* Returns the index of the current free image in the swapchain *after* acquire_next_image() is called.
	*/
	auto current_image_index() const -> uint32;
	/**
	* Has to be called at the start of the frame.
	*/
	auto acquire_next_image() -> Resource<Image>;
	auto current_acquire_semaphore() const -> Resource<Semaphore>;
	auto current_present_semaphore() const -> Resource<Semaphore>;
	auto cpu_frame_count() const -> uint64;
	auto gpu_frame_count() const -> uint64;
	auto get_gpu_fence() const -> Resource<Fence>;
	auto image_format() const -> Format;
	auto color_space() const -> ColorSpace;

	auto resize(Extent2D dim) -> bool;

	static auto from(Device& device, SwapchainInfo&& info, Resource<Swapchain> previousSwapchain = {}) -> Resource<Swapchain>;
protected:
	friend class Resource<Swapchain>;

	static auto destroy(Swapchain& resource, uint64 id) -> void;

	Swapchain(Device& device);

	using ImageArray		= lib::array<Resource<Image>>;
	using SemaphoreArray	= lib::array<Resource<Semaphore>>;

	SwapchainInfo m_info;
	SwapchainState m_state = SwapchainState::Error;
	ImageArray m_images;
	Resource<Fence> m_gpuElapsedFrames;
	SemaphoreArray m_acquireSemaphore;
	SemaphoreArray m_presentSemaphore;
	ColorSpace m_colorSpace = ColorSpace::Srgb_Non_Linear;
	std::atomic_uint64_t m_cpuElapsedFrames;
	uint32 m_currentFrameIndex;
	uint32 m_previousFrameIndex;
	uint32 m_nextImageIndex;
};

class Shader : public DeviceResource
{
public:
	Shader() = default;
	~Shader() = default;

	auto info() const->ShaderInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> Resource<Shader>;
protected:
	friend class Resource<Shader>;

	static auto destroy(Shader& resource, uint64 id) -> void;

	Shader(Device& device);

	ShaderInfo	m_info;
};

struct RasterPipelineShaderInfo
{
	Resource<Shader> vertexShader;
	Resource<Shader> pixelShader;
	std::span<ShaderAttribute> vertexInputAttrib;
};

class RasterPipeline
{
public:
	RasterPipeline() = default;
	~RasterPipeline() = default;

	auto info() const -> RasterPipelineInfo const&;
private:
	friend class Pipeline;

	RasterPipelineInfo m_info;
};

class ComputePipeline
{
public:
	ComputePipeline() = default;
	~ComputePipeline() = default;

	auto info() const -> ComputePipelineInfo const&;
private:
	friend class Pipeline;

	ComputePipelineInfo m_info;
};

template <typename T>
concept is_pipeline_type = std::same_as<T, RasterPipeline> || std::same_as<T, ComputePipeline>;

class Pipeline : public DeviceResource
{
public:
	Pipeline() = default;
	~Pipeline() = default;

	auto type() const -> PipelineType;
	auto valid() const -> bool;

	template <is_pipeline_type T>
	auto as() const -> T const* { return std::get_if<T>(&m_pipelineVariant); }

	static auto from(Device& device, RasterPipelineShaderInfo const& pipelineShaderInfo, RasterPipelineInfo&& info) -> Resource<Pipeline>;
	static auto from(Device& device, Resource<Shader>& computeShader, ComputePipelineInfo&& info) -> Resource<Pipeline>;
protected:
	friend class Resource<Pipeline>;

	static auto destroy(Pipeline& resource, uint64 id) -> void;

	using PipelineVariant = std::variant<RasterPipeline, ComputePipeline>;

	Pipeline(Device& device);

	PipelineVariant m_pipelineVariant;
	PipelineType m_type;
};

struct RenderAttachment
{
	Resource<Image> image;
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
	Image& src;
	Image& dst;
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
	Image& src;
	Swapchain& dst;
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
	Image& image;
	ImageLayout dstImageLayout;
	ClearValue clearValue;
	ImageSubresource subresource;
};

struct BufferClearInfo
{
	Buffer& buffer;
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
	Buffer& buffer;
	size_t size = std::numeric_limits<size_t>::max();
	size_t offset = 0;
	Access srcAccess = {};
	Access dstAccess = {};
	DeviceQueue	srcQueue = DeviceQueue::Main;
	DeviceQueue	dstQueue = DeviceQueue::Main;
};

struct ImageBarrierInfo
{
	Image& image;
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
	Buffer& drawInfoBuffer;
	size_t offset;
	uint32 drawCount;
	uint32 stride;
	bool indexed;
};

struct DrawIndirectCountInfo
{
	Buffer& drawInfoBuffer;
	Buffer& drawCountBuffer;
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
	Buffer& dispatchInfoBuffer;
	size_t offset = 0;
};

struct BindVertexBufferInfo
{
	Buffer& buffer;
	uint32 firstBinding;
	size_t offset;
};

struct BindIndexBufferInfo
{
	Buffer& buffer;
	size_t offset;
	IndexType indexType = IndexType::Uint_32;
};

struct BufferCopyInfo
{
	Buffer& src;
	Buffer& dst;
	size_t srcOffset;
	size_t dstOffset;
	size_t size;
};

struct BufferImageCopyInfo
{
	Buffer& src;
	Image& dst;
	size_t bufferOffset;
	ImageLayout dstImageLayout = ImageLayout::Transfer_Dst;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

struct ImageBufferCopyInfo
{
	Image& src;
	Buffer& dst;
	size_t bufferOffset;
	ImageLayout srcImageLayout = ImageLayout::Transfer_Src;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

struct ImageCopyInfo
{
	Image& src;
	Image& dst;
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
	Event& event;
	std::span<MemoryBarrierInfo> memoryBarrierInfo;
	std::span<BufferBarrierInfo> bufferBarrierInfo;
	std::span<ImageBarrierInfo> imageBarrierInfo;
};

using EventWaitInfo = EventSignalInfo;

struct EventResetInfo
{
	Event& event;
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

struct SubmitInfo
{
	DeviceQueue queue;
	std::span<Resource<CommandBuffer>> commandBuffers;
	std::span<Resource<Semaphore>> waitSemaphores;
	std::span<Resource<Semaphore>> signalSemaphores;
	std::span<std::pair<Resource<Fence>, uint64>> waitFences;
	std::span<std::pair<Resource<Fence>, uint64>> signalFences;
};

struct PresentInfo
{
	std::span<Resource<Swapchain>> swapchains;
};

class CommandBuffer : public DeviceResource
{
public:
	CommandBuffer() = default;
	~CommandBuffer() = default;

	auto info() const -> CommandBufferInfo const&;
	auto valid() const -> bool;
	auto recording_timeline() const -> uint64;

	auto reset() -> void;

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
	* Total CommandBuffer that is allowed to be requested from a CommandPool is defined by the constant MAX_COMMAND_BUFFER_PER_POOL.
	* When the total number of CommandBuffers have been requested, a null_resource is returned instead.
	*/
	static auto from(Resource<CommandPool>& commandPool) -> Resource<CommandBuffer>;
protected:
	friend class Resource<CommandBuffer>;

	/**
	 * Destroying a command buffer doesn't actually releases it from the command pool.
	 * Rather, it is returned to the command pool for re-use.
	 */
	static auto destroy(CommandBuffer& resource, uint64 id) -> void;

	CommandBuffer(Device& device);

	CommandBufferInfo m_info;
	Resource<CommandPool> m_commandPool;
	std::atomic_uint64_t m_recordingTimeline;
};

class CommandPool : public DeviceResource
{
public:
	CommandPool() = default;
	~CommandPool() = default;

	auto info() const -> CommandPoolInfo const&;
	auto valid() const -> bool;
	auto reset() const -> void;

	static auto from(Device& device, CommandPoolInfo&& info) -> Resource<CommandPool>;
protected:
	friend class Resource<CommandPool>;

	static auto destroy(CommandPool& resource, uint64 id) -> void;

	CommandPool(Device& device);

	CommandPoolInfo	m_info;
};

using semaphore		= Resource<Semaphore>;
using fence			= Resource<Fence>;
using event			= Resource<Event>;
using buffer		= Resource<Buffer>;
using image			= Resource<Image>;
using sampler		= Resource<Sampler>;
using swapchain		= Resource<Swapchain>;
using shader		= Resource<Shader>;
using pipeline		= Resource<Pipeline>;
using command_pool	= Resource<CommandPool>;
using command_buffer = Resource<CommandBuffer>;

using device_address = DeviceAddress;

class Device
{
public:
	using cpu_timeline_t = std::atomic_uint64_t::value_type;

	Device() = default;
	~Device() = default;

	NOCOPYANDMOVE(Device)

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
	/**
	 * NOTE(afiq):
	 * Return a std::expected instead of a std::optional when we use C++23.
	 */
	static auto from(DeviceInitInfo const& info) -> std::expected<std::unique_ptr<Device>, std::string_view>;
	static auto destroy(std::unique_ptr<Device>& device) -> void;
protected:
	DeviceInfo m_info;
	DeviceInitInfo m_initInfo;
	DeviceConfig m_config;
	std::atomic_uint64_t m_cpuTimeline;
	Resource<Fence> m_gpuTimeline;
};
}

#endif // !GPU_H
