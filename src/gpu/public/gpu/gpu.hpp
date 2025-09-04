#pragma once
#include "lib/common.hpp"
#include <type_traits>
#ifndef GPU_H
#define GPU_H

#include <variant>
#include <atomic>
#include <expected>
#include <array>

#include "lib/type.hpp"

#include "common.hpp"

namespace gpu
{
class Semaphore;
class Fence;
class Event;
class MemoryBlock;
class Buffer;
class Image;
class Sampler;
class Swapchain;
class Shader;
class Pipeline;
class CommandPool;
class CommandBuffer;
class Device;

namespace detail
{
template <typename DeviceType>
struct handle_base
{
	DeviceType* m_device = {};

	handle_base() = default;

	handle_base(Device& device) : 
		m_device{ &device }
	{}

	handle_base(handle_base const& rhs) : 
		m_device{ rhs.m_device }
	{}

	handle_base(handle_base&& rhs) : 
		m_device{ std::exchange(rhs.m_device, nullptr) }
	{}

	auto inc_ref(uint64 id) -> void { m_device->inc_ref(id); }
	auto dec_ref(uint64 id) -> uint64 { return m_device->dec_ref(id); }

	template <typename T>
	auto resource_for(lib::type<T> type, uint64 id) const -> T&
	{
		ASSERTION(id != std::numeric_limits<uint64>::max() && "Unable to access resource with invalid id");
		return m_device->get_resource(type, id); 
	}
};
};

template <typename T>
concept is_gpu_resource = std::same_as<T, Semaphore> 
	|| std::same_as<T, Fence>
	|| std::same_as<T, Event>
	|| std::same_as<T, MemoryBlock>
	|| std::same_as<T, Buffer>
	|| std::same_as<T, Image>
	|| std::same_as<T, Sampler>
	|| std::same_as<T, Swapchain>
	|| std::same_as<T, Shader>
	|| std::same_as<T, Pipeline>
	|| std::same_as<T, CommandPool>
	|| std::same_as<T, CommandBuffer>
	|| std::same_as<T, Device>;

template <is_gpu_resource T, typename Deleter>
requires is_gpu_resource<T> && std::invocable<Deleter, Device&, uint64>
class handle : protected detail::handle_base<Device>
{
private:
	using super = detail::handle_base<Device>;
public:
	using resource_type = T;
	using pointer = resource_type*;
	using const_pointer = resource_type const*;
	using reference = resource_type&;
	using const_reference = resource_type const&;
	using deleter_type = Deleter;

	handle() = default;

	handle(Device& device, uint64 id) :   
		handle_base{ device },
		m_id{ id }
	{
		this->inc_ref(m_id);
	}

	~handle() { destroy(); }

	handle(handle const& rhs) :
		handle_base{ rhs },
		m_id{ rhs.m_id }
	{
		this->inc_ref(m_id);
	}

	handle& operator=(handle const& rhs)
	{
		if (this != &rhs)
		{
			// If the current resource object already holds something, destroy it first before a reassignment.
			destroy();

			m_device = rhs.m_device;
			m_id = rhs.m_id;

			super::inc_ref(m_id);
		}
		return *this;
	}

	handle(handle&& rhs) noexcept :
		handle_base{ std::move(rhs) },
		m_id{ std::exchange(rhs.m_id, std::numeric_limits<uint64>::max()) }
	{}

	handle& operator=(handle&& rhs) noexcept
	{
		if (this != &rhs)
		{
			// If the current resource object already holds something, destroy it first before a reassignment.
			destroy();

			m_device = std::exchange(rhs.m_device, nullptr);
			m_id = std::exchange(rhs.m_id, std::numeric_limits<uint64>::max());
		}
		return *this;
	}

	auto operator*() const -> reference { return super::resource_for<resource_type>(lib::type<resource_type>{}, m_id); }
	auto id() const -> uint64 { return m_id; }
	template <typename Self>
	auto device(this Self&& self) -> auto&& { return std::forward_like<Self>(*self.m_device); }
	// auto device() -> Device& { return *m_device; }
	// auto device() const -> Device const& { return *m_device; }
	auto valid() const -> bool { return m_device != nullptr && m_id != std::numeric_limits<uint64>::max(); }

	explicit operator bool() const { return valid(); }

	auto destroy() -> void
	{
		if (m_device && 
			m_id != std::numeric_limits<uint64>::max() &&
			super::dec_ref(m_id) == 0)
		{
			deleter_type{}(*m_device, m_id);
		} 
		m_device = nullptr;
		m_id = std::numeric_limits<uint64>::max();
	}
private:
	uint64 m_id = std::numeric_limits<uint64>::max();
};

using DeviceAddress = uint64;

class MemoryBlock
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<MemoryBlock, Deleter>;

	MemoryBlock() = default;
	~MemoryBlock() = default;

	auto info() const -> MemoryBlockInfo const&;
	auto valid() const -> bool;
	auto aliased() const -> bool;
	auto size() const -> size_t;

	static auto from(Device& device, MemoryBlockAllocateInfo&& info) -> handle_type;
protected:
	MemoryBlock(bool aliased);

	MemoryBlockInfo m_info;
	bool const m_aliased = false;
};

class Semaphore
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Semaphore, Deleter>;

	Semaphore() = default;
	~Semaphore() = default;

	auto info() const -> SemaphoreInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, SemaphoreInfo&& info) -> handle_type;
protected:
	SemaphoreInfo m_info;
};

class Fence
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Fence, Deleter>;

	Fence() = default;
	~Fence() = default;

	auto info() const -> FenceInfo const&;
	auto valid() const -> bool;
	auto value() const -> uint64;
	auto wait_for_value(uint64 value, uint64 timeout = std::numeric_limits<uint64>::max()) const -> bool;
	auto signal(uint64 value) const -> void;
	auto signal() const -> void;

	static auto from(Device& device, FenceInfo&& info) -> handle_type;
protected:
	FenceInfo m_info;
};

class Event
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Event, Deleter>;

	Event() = default;
	~Event() = default;

	auto info() const -> EventInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, EventInfo&& info) -> handle_type;
protected:
	EventInfo m_info;
};

class Buffer
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Buffer, Deleter>;

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

	static auto from(Device& device, BufferInfo&& info, MemoryBlock::handle_type memoryBlock = {}) -> handle_type;
protected:
	BufferInfo m_info;
};

class Sampler
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Sampler, Deleter>;

	Sampler() = default;
	~Sampler() = default;

	auto info() const -> SamplerInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, SamplerInfo&& info) -> handle_type;
protected:
	SamplerInfo m_info;
	uint64 m_packedInfoBits;
};

struct ImageBindInfo
{
	Sampler::handle_type sampler;
	uint32 index;
};

class Image
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Image, Deleter>;

	Image() = default;
	~Image() = default;

	auto info() const -> ImageInfo const&;
	auto valid() const -> bool;
	auto is_swapchain_image() const -> bool;
	auto is_transient() const -> bool;

	static auto memory_requirement(Device& device, ImageInfo const& info) -> MemoryRequirementInfo;

	static auto from(Device& device, ImageInfo&& info, MemoryBlock::handle_type memoryBlock = {}) -> handle_type;
	static auto from(Swapchain& swapchain) -> lib::array<handle_type>;
protected:
	friend class Swapchain;

	ImageInfo m_info;
};

class Swapchain
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Swapchain, Deleter>;

	Swapchain() = default;
	~Swapchain() = default;

	auto info() const -> SwapchainInfo const&;
	auto valid() const -> bool;
	auto state() const -> SwapchainState;
	auto num_images() const -> uint32;
	/**
	* Returns the current free image in the swapchain *after* acquire_next_image() is called.
	*/
	auto current_image() const -> Image::handle_type;
	/**
	* Returns the index of the current free image in the swapchain *after* acquire_next_image() is called.
	*/
	auto current_image_index() const -> uint32;
	/**
	* Has to be called at the start of the frame.
	*/
	auto acquire_next_image() -> Image::handle_type;
	auto current_acquire_semaphore() const -> Semaphore::handle_type;
	auto current_present_semaphore() const -> Semaphore::handle_type;
	auto cpu_frame_count() const -> uint64;
	auto gpu_fence() const -> Fence::handle_type;
	auto image_format() const -> Format;

	auto resize(Extent2D dim) -> bool;

	static auto from(Device& device, SwapchainInfo&& info, handle_type previousSwapchain = {}) -> handle_type;
protected:
	using ImageArray		= lib::array<Image::handle_type>;
	using SemaphoreArray	= lib::array<Semaphore::handle_type>;

	SwapchainInfo m_info;
	SwapchainState m_state = SwapchainState::Error;
	ImageArray m_images;
	Fence::handle_type m_gpuElapsedFrames;
	SemaphoreArray m_acquireSemaphore;
	SemaphoreArray m_presentSemaphore;
	std::atomic_uint64_t m_cpuElapsedFrames;
	uint32 m_acquireSemaphoreIndex;
	uint32 m_currentImageIndex;
};

class Shader
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Shader, Deleter>;

	Shader() = default;
	~Shader() = default;

	auto info() const->ShaderInfo const&;
	auto valid() const -> bool;

	static auto from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> handle_type;
protected:
	ShaderInfo	m_info;
};

struct RasterPipelineShaderInfo
{
	Shader::handle_type vertexShader;
	Shader::handle_type pixelShader;
	std::span<ShaderAttribute> vertexInputAttrib;
};

struct ComputePipelineShaderInfo
{
	Shader::handle_type computeShader;
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

class Pipeline
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<Pipeline, Deleter>;

	Pipeline() = default;
	~Pipeline() = default;

	auto type() const -> PipelineType;
	auto valid() const -> bool;

	template <is_pipeline_type T>
	auto as() const -> T const* { return std::get_if<T>(&m_pipelineVariant); }

	static auto from(Device& device, RasterPipelineShaderInfo const& pipelineShaderInfo, RasterPipelineInfo&& info) -> handle_type;
	static auto from(Device& device, ComputePipelineShaderInfo const& pipelineShaderInfo, ComputePipelineInfo&& info) -> handle_type;
protected:
	using PipelineVariant = std::variant<RasterPipeline, ComputePipeline>;

	PipelineVariant m_pipelineVariant;
	PipelineType m_type;
};

struct RenderAttachment
{
	Image::handle_type image;
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
	uint32 bufferRowLength;
	uint32 bufferImageHeight;
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

struct PresentInfo
{
	std::span<Swapchain::handle_type> swapchains;
};

class CommandPool
{
public:
	struct Deleter
	{
		auto operator()(Device& device, uint64 id) const -> void;
	};

	using handle_type = handle<CommandPool, Deleter>;

	CommandPool() = default;
	~CommandPool() = default;

	auto info() const -> CommandPoolInfo const&;
	auto valid() const -> bool;
	auto reset() const -> void;

	static auto from(Device& device, CommandPoolInfo&& info) -> handle_type;
protected:
	CommandPoolInfo	m_info;
};

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
	auto current_timeline_fence() const -> Fence::handle_type;

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
	static auto from(CommandPool::handle_type& commandPool) -> CommandRecorder;
private:
	friend class Device;

	CommandPool::handle_type m_cmdPool;
	uint64 m_index;

	CommandRecorder(CommandPool::handle_type const& pool, uint64 commandBufferIndex);
};

static_assert(std::is_move_constructible_v<CommandRecorder>, "CommandRecorder needs to be move assignable.");
static_assert(std::is_move_assignable_v<CommandRecorder>, "CommandRecorder needs to be move constructible.");

template <is_gpu_resource T>
using resource = T::handle_type;

using device_address = DeviceAddress;

struct SubmitInfo
{
	DeviceQueue queue;
	std::span<CommandRecorder> commandRecorders;
	std::span<Semaphore::handle_type> waitSemaphores;
	std::span<Semaphore::handle_type> signalSemaphores;
	std::span<std::pair<Fence::handle_type, uint64>> waitFences;
	std::span<std::pair<Fence::handle_type, uint64>> signalFences;
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
	/**
	 * NOTE(afiq):
	 * Return a std::expected instead of a std::optional when we use C++23.
	 */
	static auto from(DeviceInitInfo const& info) -> std::expected<std::unique_ptr<Device>, std::string_view>;
	static auto destroy(std::unique_ptr<Device>& device) -> void;
protected:
	friend struct detail::handle_base<Device>;

	DeviceInfo m_info;
	DeviceInitInfo m_initInfo;
	DeviceConfig m_config;
	std::atomic_uint64_t m_cpuTimeline;
	Fence::handle_type m_gpuTimeline;

	auto inc_ref(uint64 id) -> void;
	[[nodiscard]] auto dec_ref(uint64 id) -> uint64;

	auto get_resource(lib::type<Semaphore> type, uint64 id) -> Semaphore&;
	auto get_resource(lib::type<Fence> type, uint64 id) -> Fence&;
	auto get_resource(lib::type<Event> type, uint64 id) -> Event&;
	auto get_resource(lib::type<MemoryBlock> type, uint64 id) -> MemoryBlock&;
	auto get_resource(lib::type<Buffer> type, uint64 id) -> Buffer&;
	auto get_resource(lib::type<Image> type, uint64 id) -> Image&;
	auto get_resource(lib::type<Sampler> type, uint64 id) -> Sampler&;
	auto get_resource(lib::type<Swapchain> type, uint64 id) -> Swapchain&;
	auto get_resource(lib::type<Shader> type, uint64 id) -> Shader&;
	auto get_resource(lib::type<Pipeline> type, uint64 id) -> Pipeline&;
	auto get_resource(lib::type<CommandPool> type, uint64 id) -> CommandPool&;
};
}

#endif // !GPU_H
