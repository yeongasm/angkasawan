module;

#include <variant>
#include <atomic>
#include <mutex>
#include <deque>

#include "lib/named_type.h"
#include "lib/string.h"
#include "lib/paged_array.h"
#include "lib/map.h"

#include "rhi_api.h"

export module forge;

export import forge.common;
import forge.api;

namespace frg
{
export class Semaphore;
export class Fence;
export class Buffer;
export class Image;
export class Sampler;
export class Swapchain;
export class Shader;
export class Pipeline;
export class CommandPool;
export class CommandBuffer;
export class Device;

template <typename T>
using Pool = lib::paged_array<T, 16>;

export using semaphore_id		= Pool<Semaphore>::index;
export using fence_id			= Pool<Fence>::index;
export using buffer_id			= Pool<Buffer>::index;
export using image_id			= Pool<Image>::index;
export using sampler_id			= Pool<Sampler>::index;
export using swapchain_id		= Pool<Swapchain>::index;
export using shader_id			= Pool<Shader>::index;
export using pipeline_id		= Pool<Pipeline>::index;
export using command_pool_id	= Pool<CommandPool>::index;
export using command_buffer_id	= size_t;

export struct PipelineShaderInfo;

struct ResourcePool;

export class RefCountedResource
{
public:
	RHI_API RefCountedResource() = default;
	RHI_API ~RefCountedResource() = default;

	RefCountedResource(RefCountedResource const&) = delete;
	RefCountedResource& operator=(RefCountedResource const&) = delete;

	RefCountedResource(RefCountedResource&&) = delete;
	RefCountedResource& operator=(RefCountedResource&&) = delete;

	RHI_API auto reference() -> void;
	RHI_API auto dereference() -> void;
	RHI_API auto ref_count() const -> uint64;
protected:
	std::atomic_uint64_t m_refCount;
};

export class RefCountedDeviceResource : public RefCountedResource
{
public:
	RefCountedDeviceResource() = default;
	RefCountedDeviceResource(Device& device);

	~RefCountedDeviceResource() = default;

	RHI_API auto device() const -> Device&;
protected:
	Device* m_device;
};

export class NullResource {};
export inline constexpr NullResource null_resource = {};

export template <typename T>
class Resource
{
public:
	using resource_type = T;
	using pointer = resource_type*;
	using const_pointer = resource_type const*;
	using reference = resource_type&;
	using const_reference = resource_type const&;

	using id_type = typename resource_type::id_type;

	RHI_API Resource() = default;

	RHI_API Resource(id_type id, resource_type& resource) :
		m_id{ id },
		m_resource{ &resource }
	{}

	RHI_API ~Resource() { destroy(); }

	RHI_API Resource(NullResource) :
		m_id{},
		m_resource{}
	{}

	RHI_API Resource(Resource const& rhs) :
		m_id{ rhs.m_id },
		m_resource{ &rhs.m_resource }
	{
		m_resource->reference();
	}

	RHI_API Resource& operator=(Resource const& rhs)
	{
		if (this != &rhs)
		{
			m_id = rhs.m_id;
			m_resource = rhs.m_resource;

			m_resource->reference();
		}
		return *this;
	}

	RHI_API Resource(Resource&& rhs) noexcept :
		m_id{ std::move(rhs.m_id) },
		m_resource{ std::move(rhs.m_resource) }
	{
		new (&rhs) Resource{};
	}

	RHI_API Resource& operator=(Resource&& rhs) noexcept
	{
		if (this != &rhs)
		{
			m_id = std::move(rhs.m_id);
			m_resource	= std::move(rhs.m_resource);

			new (&rhs) Resource{};
		}
		return *this;
	}

	RHI_API auto operator->() const -> pointer { return m_resource; }
	RHI_API auto operator*() const -> reference { return *m_resource; }

	RHI_API auto valid() const -> bool { return m_resource != nullptr && m_resource->valid(); }

	RHI_API auto destroy() -> void
	{
		if (m_resource && m_resource->ref_count())
		{
			m_resource->dereference();

			if (!m_resource->ref_count())
			{
				resource_type::destroy(*m_resource, m_id);
				m_resource	= nullptr;
				m_id		= id_type::from(std::numeric_limits<uint64>::max());
			}
		}
	}

	RHI_API auto id() const -> id_type { return m_id; }
private:
	id_type			m_id		= id_type::from(std::numeric_limits<uint64>::max());
	resource_type*	m_resource	= nullptr;
};

class Semaphore : public RefCountedDeviceResource
{
public:
	using id_type = semaphore_id;

	Semaphore() = default;
	~Semaphore() = default;

	RHI_API auto info() const -> SemaphoreInfo const&;
	RHI_API auto valid() const -> bool;

	RHI_API static auto from(Device& device, SemaphoreInfo&& info) -> Resource<Semaphore>;
private:
	friend class Device;
	friend class Swapchain;
	friend class Resource<Semaphore>;

	RHI_API static auto destroy(Semaphore const& resource, id_type id) -> void;

	Semaphore(Device& device);

	SemaphoreInfo	m_info;
	api::Semaphore	m_impl;
};

class Fence : public RefCountedDeviceResource
{
public:
	using id_type = fence_id;

	Fence() = default;
	~Fence() = default;

	RHI_API auto info() const -> FenceInfo const&;
	RHI_API auto valid() const -> bool;
	RHI_API auto value() const -> uint64;
	RHI_API auto signal(uint64 const value) const -> void;
	RHI_API auto signal() const -> void;
	RHI_API auto wait_for_value(uint64 value, uint64 timeout = std::numeric_limits<uint64>::max()) const -> bool;

	RHI_API static auto from(Device& device, FenceInfo&& info) -> Resource<Fence>;
private:
	friend class Device;
	friend class Resource<Fence>;

	RHI_API static auto destroy(Fence const& resource, id_type id) -> void;

	Fence(Device& device);

	FenceInfo		m_info;
	api::Semaphore	m_impl;
};

class Buffer : public RefCountedDeviceResource
{
public:
	using id_type = buffer_id;

	Buffer() = default;
	~Buffer() = default;

	RHI_API auto info() const -> BufferInfo const&;
	RHI_API auto valid() const -> bool;
	RHI_API auto data() const -> void*;
	RHI_API auto size() const -> size_t;
	RHI_API auto write(void const* data, size_t size, size_t offset) const -> void;
	RHI_API auto clear() const -> void;
	RHI_API auto is_host_visible() const -> bool;
	RHI_API auto gpu_address() const -> uint64;

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

	RHI_API static auto from(Device& device, BufferInfo&& info) -> Resource<Buffer>;
private:
	friend class CommandBuffer;
	friend class Resource<Buffer>;

	RHI_API static auto destroy(Buffer const& resource, id_type id) -> void;

	Buffer(Device& device);

	BufferInfo	m_info;
	api::Buffer m_impl;
};

class Image : public RefCountedDeviceResource
{
public:
	using id_type = image_id;

	Image() = default;
	~Image() = default;

	RHI_API auto info() const -> ImageInfo const&;
	RHI_API auto valid() const -> bool;
	RHI_API auto is_swapchain_image() const -> bool;

	RHI_API static auto from(Device& device, ImageInfo&& info) -> Resource<Image>;
private:
	friend class CommandBuffer;
	friend class Swapchain;
	friend class Resource<Image>;

	RHI_API static auto from(Swapchain& swapchain) -> lib::array<Resource<Image>>;
	RHI_API static auto destroy(Image const& resource, id_type id) -> void;

	Image(Device& device);

	ImageInfo	m_info;
	api::Image	m_impl;
};

class Sampler : public RefCountedDeviceResource
{
public:
	using id_type = sampler_id;

	Sampler()	= default;
	~Sampler()	= default;

	RHI_API auto info() const->SamplerInfo const&;
	RHI_API auto valid() const -> bool;
	RHI_API auto info_packed() const->uint64;

	RHI_API static auto from(Device& device, SamplerInfo&& info) -> Resource<Sampler>;
private:
	friend class Resource<Sampler>;

	RHI_API static auto destroy(Sampler const& resource, id_type id) -> void;

	Sampler(Device& device);

	SamplerInfo		m_info;
	api::Sampler	m_impl;
	uint64			m_packedInfoBits;
};

class Swapchain : public RefCountedDeviceResource
{
public:
	using id_type = swapchain_id;

	Swapchain() = default;
	~Swapchain() = default;

	RHI_API auto info() const -> SwapchainInfo const&;
	RHI_API auto valid() const -> bool;
	RHI_API auto state() const -> SwapchainState;
	RHI_API auto num_images() const -> uint32;
	RHI_API auto current_image() const -> Resource<Image>;
	RHI_API auto acquire_next_image() -> Resource<Image>;
	RHI_API auto current_acquire_semaphore() const -> Resource<Semaphore>;
	RHI_API auto current_present_semaphore() const -> Resource<Semaphore>;
	RHI_API auto cpu_frame_count() const -> uint64;
	RHI_API auto gpu_frame_count() const -> uint64;
	RHI_API auto get_gpu_fence() const -> Resource<Fence>;
	RHI_API auto image_format() const -> Format;
	RHI_API auto color_space() const -> ColorSpace;

	RHI_API static auto from(Device& device, SwapchainInfo&& info, Resource<Swapchain> previousSwapchain = null_resource) -> Resource<Swapchain>;
private:
	friend class Image;
	friend class Resource<Swapchain>;

	RHI_API static auto destroy(Swapchain const& resource, id_type id) -> void;

	Swapchain(Device& device);

	using ImageArray		= lib::array<Resource<Image>>;
	using SemaphoreArray	= lib::array<Resource<Semaphore>>;

	SwapchainInfo	m_info;
	api::Swapchain	m_impl;
	SwapchainState	m_state = SwapchainState::Error;
	ImageArray		m_images;
	Resource<Fence> m_gpuElapsedFrames;
	SemaphoreArray	m_acquireSemaphore;
	SemaphoreArray	m_presentSemaphore;
	ColorSpace		m_colorSpace = ColorSpace::Srgb_Non_Linear;
	uint64			m_cpuElapsedFrames;
	uint32			m_currentFrameIndex;
	uint32			m_nextImageIndex;
};

class Shader : public RefCountedDeviceResource
{
public:
	using id_type = shader_id;

	Shader()	= default;
	~Shader()	= default;

	RHI_API auto info() const -> ShaderInfo const&;
	RHI_API auto valid() const -> bool;

	RHI_API static auto from(Device& device, CompiledShaderInfo const& compiledShaderInfo) -> Resource<Shader>;
private:
	friend class Pipeline;
	friend class Resource<Shader>;

	RHI_API static auto destroy(Shader const& resource, id_type id) -> void;

	Shader(Device& device);

	ShaderInfo	m_info;
	api::Shader m_impl;
};

struct PipelineShaderInfo
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

	RHI_API auto info() const -> RasterPipelineInfo const&;
private:
	friend class Pipeline;

	RasterPipelineInfo	m_info;
};

template <typename T>
concept is_pipeline_type = std::same_as<T, RasterPipeline>;

class Pipeline : public RefCountedDeviceResource
{
public:
	using id_type = pipeline_id;

	Pipeline() = default;
	~Pipeline() = default;

	RHI_API auto type() const -> PipelineType;
	RHI_API auto valid() const -> bool;

	template <is_pipeline_type T>
	RHI_API auto as() const -> T const*
	{
		return std::get_if<T>(m_pipelineVariant);
	}

	RHI_API static auto from(Device& device, PipelineShaderInfo const& pipelineShaderInfo, RasterPipelineInfo&& info) -> Resource<Pipeline>;
private:
	friend class CommandBuffer;
	friend class Resource<Pipeline>;

	RHI_API static auto destroy(Pipeline const& resource, id_type id) -> void;

	using PipelineVariant = std::variant<RasterPipeline>;

	Pipeline(Device& device);

	PipelineVariant m_pipelineVariant;
	api::Pipeline	m_impl;
	PipelineType	m_type;
};

export struct RenderAttachment
{
	Resource<Image> image;
	ImageLayout imageLayout;
	AttachmentLoadOp loadOp;
	AttachmentStoreOp storeOp;
};

export struct RenderingInfo
{
	std::span<RenderAttachment>	colorAttachments;
	RenderAttachment* depthAttachment;
	RenderAttachment* stencilAttachment;
	Rect2D renderArea;
};

export struct ImageBlitInfo
{
	ImageLayout srcImageLayout;
	std::array<Offset3D, 2> srcOffset;
	ImageSubresource srcSubresource;
	ImageLayout	dstImageLayout;
	std::array<Offset3D, 2> dstOffset;
	ImageSubresource dstSubresource;
	TexelFilter	filter;
};

export struct ImageClearInfo
{
	ImageLayout dstImageLayout;
	ClearValue clearValue;
	ImageSubresource subresource;
};

export struct BufferClearInfo
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
export struct MemoryBarrierInfo
{
	Access srcAccess = {};
	Access dstAccess = {};
};

export struct BufferBarrierInfo
{
	size_t size = std::numeric_limits<size_t>::max();
	size_t offset = 0;
	Access srcAccess = {};
	Access dstAccess = {};
	DeviceQueue	srcQueue = DeviceQueue::Main;
	DeviceQueue	dstQueue = DeviceQueue::Main;
};

export struct ImageBarrierInfo
{
	Access srcAccess = {};
	Access dstAccess = {};
	ImageLayout	oldLayout = ImageLayout::Undefined;
	ImageLayout	newLayout = ImageLayout::Undefined;
	ImageSubresource subresource = {};
	DeviceQueue	srcQueue = DeviceQueue::Main;
	DeviceQueue	dstQueue = DeviceQueue::Main;
};

export struct DrawInfo
{
	uint32 vertexCount;
	uint32 instanceCount = 1;
	uint32 firstVertex;
	uint32 firstInstance;
};

export struct DrawIndexedInfo
{
	uint32 indexCount;
	uint32 instanceCount = 1;
	uint32 firstIndex;
	uint32 vertexOffset;
	uint32 firstInstance;
};

export struct DrawIndirectInfo
{
	size_t offset;
	uint32 drawCount;
	uint32 stride;
	bool indexed;
};

export struct DrawIndirectCountInfo
{
	size_t offset; // offset into the buffer that contains the packed draw parameters.
	size_t countBufferOffset; // offset into a buffer that contains the packed unsigned 32-bit integer that signifies draw count.
	uint32 maxDrawCount;
	uint32 stride;
	bool indexed;
};

export struct BindVertexBufferInfo
{
	uint32 firstBinding;
	size_t offset;
};

export struct BindIndexBufferInfo
{
	size_t offset;
	IndexType indexType = IndexType::Uint_32;
};

export struct BufferCopyInfo
{
	size_t srcOffset;
	size_t dstOffset;
	size_t size;
};

export struct BufferImageCopyInfo
{
	size_t bufferOffset;
	ImageLayout dstImageLayout = ImageLayout::Transfer_Dst;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

export struct ImageBufferCopyInfo
{
	size_t bufferOffset;
	ImageLayout srcImageLayout = ImageLayout::Transfer_Src;
	ImageSubresource imageSubresource = {};
	Offset3D imageOffset = {};
	Extent3D imageExtent;
};

export struct ImageCopyInfo
{
	ImageLayout srcImageLayout = ImageLayout::Transfer_Src;
	ImageSubresource srcSubresource = {};
	Offset3D srcOffset = {};
	ImageLayout dstImageLayout = ImageLayout::Transfer_Dst;
	ImageSubresource dstSubresource = {};
	Offset3D dstOffset;
	Extent3D extent;
};

export struct DebugLabelInfo
{
	std::string_view name;
	float32 color[4] = { 1.f, 1.f, 1.f, 1.f };
};

export struct BindPushConstantInfo
{
	void const* data;
	size_t offset = 0;
	size_t size;
	ShaderStage shaderStage = ShaderStage::All;
};

export struct SubmitInfo
{
	DeviceQueue queue;
	std::span<Resource<CommandBuffer>> commandBuffers;
	std::span<Resource<Semaphore>> waitSemaphores;
	std::span<Resource<Semaphore>> signalSemaphores;
	std::span<std::pair<Resource<Fence>, uint64>> waitFences;
	std::span<std::pair<Resource<Fence>, uint64>> signalFences;
};

export struct PresentInfo
{
	std::span<Resource<Swapchain>> swapchains;
};

class CommandBuffer : public RefCountedDeviceResource
{
public:
	using id_type = command_buffer_id;

	CommandBuffer() = default;
	~CommandBuffer() = default;

	RHI_API auto info() const -> CommandBufferInfo const&;
	RHI_API auto valid() const -> bool;
	RHI_API auto is_initial() const -> bool;
	RHI_API auto is_recording() const -> bool;
	RHI_API auto is_executable() const -> bool;
	RHI_API auto is_pending_complete() const -> bool;
	RHI_API auto is_completed() -> bool;
	RHI_API auto is_invalid() const -> bool;
	RHI_API auto current_state() const-> CommandBufferState;

	RHI_API auto reset() -> void;

	RHI_API auto begin() -> bool;
	RHI_API auto end() -> void;

	RHI_API auto clear(Resource<Image> image) -> void;
	RHI_API auto clear(Resource<Image> image, ImageClearInfo const& info) -> void;
	RHI_API auto clear(Resource<Buffer> buffer) -> void;
	RHI_API auto clear(Resource<Buffer> buffer, BufferClearInfo const& info) -> void;

	RHI_API auto draw(DrawInfo const& info) const -> void;
	RHI_API auto draw_indexed(DrawIndexedInfo const& info) const -> void;
	RHI_API auto draw_indirect(Resource<Buffer> drawInfoBuffer, DrawIndirectInfo const& info) const -> void;
	RHI_API auto draw_indirect_count(Resource<Buffer> drawInfoBuffer, Resource<Buffer> drawCountBuffer, DrawIndirectCountInfo const& info) const -> void;

	RHI_API auto bind_vertex_buffer(Resource<Buffer> buffer, BindVertexBufferInfo const& info) -> void;
	RHI_API auto bind_index_buffer(Resource<Buffer> buffer, BindIndexBufferInfo const& info) -> void;
	RHI_API auto bind_push_constant(BindPushConstantInfo const& info) -> void;

	RHI_API auto bind_pipeline(Resource<Pipeline> pipeline) -> void;

	RHI_API auto pipeline_barrier(MemoryBarrierInfo const& barrier) -> void;
	RHI_API auto pipeline_barrier(Resource<Buffer> buffer, BufferBarrierInfo const& barrier) -> void;
	RHI_API auto pipeline_barrier(Resource<Image> image, ImageBarrierInfo const& barrier) -> void;
	RHI_API auto flush_barriers() -> void;

	RHI_API auto begin_rendering(RenderingInfo const& info) -> void;
	RHI_API auto end_rendering() -> void;

	RHI_API auto copy_buffer_to_buffer(Resource<Buffer> src, Resource<Buffer> dst, BufferCopyInfo const& info) -> void;
	RHI_API auto copy_buffer_to_image(Resource<Buffer> src, Resource<Image> dst, BufferImageCopyInfo const& info) -> void;
	RHI_API auto copy_image_to_buffer(Resource<Image> src, Resource<Buffer> dst, ImageBufferCopyInfo const& info) -> void;
	RHI_API auto copy_image_to_image(Resource<Image> src, Resource<Image> dst, ImageCopyInfo const& info) -> void;
	RHI_API auto blit_image(Resource<Image> src, Resource<Image> dst, ImageBlitInfo const& info) -> void;
	RHI_API auto blit_image(Resource<Image> src, Resource<Swapchain> dst, ImageBlitInfo const& info) -> void;

	RHI_API auto set_viewport(Viewport const& viewport) -> void;
	RHI_API auto set_scissor(Rect2D const& rect) -> void;

	RHI_API auto begin_debug_label(DebugLabelInfo const& info) const -> void;
	RHI_API auto end_debug_label() const -> void;

	RHI_API static auto from(Resource<CommandPool> commandPool, CommandBufferInfo&& info) -> Resource<CommandBuffer>;
private:
	friend class Device;
	friend class Resource<CommandBuffer>;

	/**
	 * Destroying a command buffer doesn't actually releases it from the command pool.
	 * Rather, it is returned to the command pool for re-use.
	 */
	RHI_API static auto destroy(CommandBuffer const& resource, id_type id) -> void;

	CommandBuffer(Device& device);

	CommandBufferInfo		m_info;
	api::CommandBuffer		m_impl;
	Resource<CommandPool>	m_commandPool;
	Resource<Fence>			m_completionTimeline;
	uint64					m_recordingTimeline;
	CommandBufferState		m_state;
};

class CommandPool : public RefCountedDeviceResource
{
public:
	using id_type = command_pool_id;

	CommandPool()	= default;
	~CommandPool()	= default;

	RHI_API auto info() const -> CommandPoolInfo const&;
	RHI_API auto valid() const -> bool;
	RHI_API auto reset() const -> void;
	/*RHI_API auto allocate_command_buffer(CommandBufferInfo&& info) -> Resource<CommandBuffer>;*/

	RHI_API static auto from(Device& device, CommandPoolInfo&& info) -> Resource<CommandPool>;
private:
	friend class CommandBuffer;
	friend class Resource<CommandPool>;

	RHI_API static auto destroy(CommandPool const& resource, id_type id) -> void;

	CommandPool(Device& device);

	CommandPoolInfo		m_info;
	api::CommandPool	m_impl;
};

struct CommandBufferPool
{
	std::array<CommandBuffer, MAX_COMMAND_BUFFER_PER_POOL> commandBuffers;
	std::array<size_t, MAX_COMMAND_BUFFER_PER_POOL> freeSlots;
	uint32 commandBufferCount;
	uint32 freeSlotCount;
	uint32 currentFreeSlot;
};

using resource_type_id = lib::named_type<uint32, struct _Resource_Type_>;

consteval auto next_resource_type_id() -> resource_type_id;

template <typename T>
consteval resource_type_id resource_type_id_v{ next_resource_type_id() };

using device_timeline_t = uint64;
using resource_id_t = uint64;

struct Zombie
{
	device_timeline_t	timeline;
	resource_id_t		id;
	resource_type_id	type;
};

struct ResourcePool
{
	using semaphore_pool_type	= Pool<Semaphore>;
	using fence_pool_type		= Pool<Fence>;
	using buffer_pool_type		= Pool<Buffer>;
	using image_pool_type		= Pool<Image>;
	using swapchain_pool_type	= Pool<Swapchain>;
	using shader_pool_type		= Pool<Shader>;
	using sampler_pool_type		= Pool<Sampler>;
	using pipeline_pool_type	= Pool<Pipeline>;
	
	using command_pool_pool_type = Pool<CommandPool>;

	lib::map<command_pool_id, CommandBufferPool> commandBufferPools;
	lib::map<uint64, sampler_id> samplerPermutationCache;

	semaphore_pool_type semaphores;
	fence_pool_type		fences;
	buffer_pool_type	buffers;
	image_pool_type		images;
	swapchain_pool_type swapchains;
	shader_pool_type	shaders;
	sampler_pool_type	samplers;
	pipeline_pool_type	pipelines;
	
	command_pool_pool_type commandPools;

	std::deque<Zombie>	zombies;

	std::mutex			zombieMutex;
};

class Device
{
public:
	Device() = default;
	~Device() = default;

	Device(Device const&)					= delete;
	Device& operator=(Device const&)		= delete;

	Device(Device&&) noexcept				= delete;
	Device& operator=(Device&&) noexcept	= delete;

	RHI_API [[nodiscard]] auto info() const -> DeviceInfo const&;
	RHI_API [[nodiscard]] auto config() const -> DeviceConfig const&;
	RHI_API auto wait_idle() const -> void;
	RHI_API [[nodiscard]] auto cpu_timeline() const -> uint64;
	RHI_API [[nodiscard]] auto gpu_timeline() const -> uint64;

	RHI_API auto submit(SubmitInfo const& info) -> bool;
	RHI_API auto present(PresentInfo const& info) -> bool;

	/**
	 * Should be called every frame.
	 */
	RHI_API auto clear_resources() -> void;
	/**
	 * NOTE(afiq):
	 * Return a std::expected instead of a std::optional when we use C++23.
	 */
	RHI_API auto from(DeviceInitInfo const& info) -> std::optional<std::unique_ptr<Device>>;
	RHI_API auto destroy(Device& device) -> void;
private:
	friend class Semaphore;
	friend class Fence;
	friend class Buffer;
	friend class Image;
	friend class Sampler;
	friend class Swapchain;
	friend class Shader;
	friend class Pipeline;
	friend class CommandBuffer;
	friend class CommandPool;

	auto initialize(DeviceInitInfo const& info) -> bool;
	auto terminate() -> void;

	DeviceInfo m_info;
	DeviceInitInfo m_initInfo;
	DeviceConfig m_config;
	ResourcePool m_gpuResourcePool;
	api::Context m_apiContext;
	std::atomic_uint64_t m_cpuTimeline;
	Resource<Fence> m_gpuTimeline = NullResource{};
};

//class _Instance_
//{
//public:
//    _Instance_() = default;
//    ~_Instance_() = default;
//
//    RHI_API auto create_device(DeviceInitInfo const& info) -> std::optional<lib::ref<Device>>;
//    RHI_API auto destroy_device(Device& device) -> void;
//private:
//    friend RHI_API auto create_instance() -> Instance;
//    friend RHI_API auto destroy_instance() -> void;
//
//    static std::unique_ptr<_Instance_> _instance;
//    lib::map<lib::hash_string_view, std::unique_ptr<Device>> m_devices = {};
//
//    _Instance_(_Instance_ const&)               = delete;
//    _Instance_(_Instance_&&)                    = delete;
//    _Instance_& operator=(_Instance_ const&)    = delete;
//    _Instance_& operator=(_Instance_&&)         = delete;
//};
//
//export RHI_API auto create_instance() -> Instance;
//export RHI_API auto destroy_instance() -> void;
}

template <>
struct std::hash<frg::command_pool_id>
{
	size_t operator()(frg::command_pool_id const& index) const noexcept
	{
		return (std::hash<uint64>{})(index.to_uint64());
	}
};