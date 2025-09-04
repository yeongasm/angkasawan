#pragma once
#ifndef VK_GPU_H
#define VK_GPU_H

#include <deque>
#include <mutex>
#include <array>

#include <ankerl/unordered_dense.h>

#include "lib/array.hpp"
#include "lib/function.hpp"

#include "vk.h"
#include "gpu.hpp"

#define CHECK_OP(op)						\
	if (auto res = op; res != VK_SUCCESS)	\
	{										\
		return {};							\
	}

namespace gpu
{
namespace vk
{
namespace detail
{
consteval auto fnv1a_32(const char* str) -> uint32
{
    constexpr uint32 FNV_OFFSET_BASIS = 0x811C9DC5u; // 2166136261
    constexpr uint32 FNV_PRIME = 0x01000193u;        // 16777619

    uint32 hash = FNV_OFFSET_BASIS;
    while (*str) 
	{
        hash ^= static_cast<uint8_t>(*str++);
        hash *= FNV_PRIME;
    }
    return hash;
}

template <typename T>
struct type_id
{
	static constexpr uint32 value = fnv1a_32(std::source_location::current().function_name());
};

template <typename T>
static constexpr uint32 type_id_v = type_id<T>::value;
}

struct DeviceImpl;

static constexpr uint32 INVALID_QUEUE_FAMILY_INDEX = std::numeric_limits<uint32>::max();

struct Queue
{
	VkQueue	queue = VK_NULL_HANDLE;
	VkQueueFamilyProperties properties = {};
	uint32 familyIndex = INVALID_QUEUE_FAMILY_INDEX;
};

class MemoryBlockImpl : public MemoryBlock
{
public:
	MemoryBlockImpl() = default;
	MemoryBlockImpl(bool aliased);

	VmaAllocation handle = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo = {};
};

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
class BufferImpl : public Buffer
{
public:
	BufferImpl() = default;

	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceAddress address = {};
	MemoryBlock::handle_type allocationBlock = {};
};

class ImageImpl : public Image
{
public:
	ImageImpl() = default;

	VkImage	handle = VK_NULL_HANDLE;
	VkImageView	imageView = VK_NULL_HANDLE;
	MemoryBlock::handle_type allocationBlock = {};
};

class SamplerImpl : public Sampler
{
public:
	SamplerImpl() = default;

	VkSampler handle = VK_NULL_HANDLE;
};

struct Surface
{
	VkSurfaceKHR handle = VK_NULL_HANDLE;
	lib::array<VkSurfaceFormatKHR> availableColorFormats = {};
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::atomic_uint32_t refCount = {};
};

class SemaphoreImpl : public Semaphore
{
public:
	SemaphoreImpl() = default;

	VkSemaphore handle = VK_NULL_HANDLE;
};

class FenceImpl : public Fence
{
public:
	FenceImpl() = default;
	FenceImpl(DeviceImpl& device);

	DeviceImpl* vkdevice = nullptr;
	VkSemaphore handle = VK_NULL_HANDLE;
};

class EventImpl : public Event
{
public:
	EventImpl() = default;

	VkEvent handle = VK_NULL_HANDLE;
};

class SwapchainImpl : public Swapchain
{
public:
	SwapchainImpl() = default;
	SwapchainImpl(DeviceImpl& device);

	using surface_iterator = typename plf::colony<Surface>::iterator;
	
	DeviceImpl* vkdevice = {};
	surface_iterator surface = {};
	VkSwapchainKHR handle = VK_NULL_HANDLE;
	VkSurfaceFormatKHR surfaceColorFormat = {};
};

class ShaderImpl : public Shader
{
public:
	ShaderImpl() = default;

	VkShaderModule handle = VK_NULL_HANDLE;
	VkShaderStageFlagBits stage = {};
};

class PipelineImpl : public Pipeline
{
public:
	PipelineImpl() = default;

	VkPipeline handle = VK_NULL_HANDLE;
	VkPipelineLayout layout = {};
};

struct CommandBufferImpl
{
	CommandBufferImpl() = default;
	CommandBufferImpl(CommandBufferImpl&& rhs);

	static constexpr size_t MAX_COMMAND_BUFFER_BARRIER_COUNT = 8;

	template <typename T>
	using BarrierArray = std::array<T, MAX_COMMAND_BUFFER_BARRIER_COUNT>;

	VkCommandBuffer handle = VK_NULL_HANDLE;
	BarrierArray<VkMemoryBarrier2> memoryBarriers = {};
	BarrierArray<VkBufferMemoryBarrier2> bufferBarriers = {};
	BarrierArray<VkImageMemoryBarrier2> imageBarriers = {};
	size_t numMemoryBarrier = {};
	size_t numBufferBarrier = {};
	size_t numImageBarrier = {};
	std::atomic_uint64_t recordingTimeline = {};
	Fence::handle_type gpuTimeline = {};

	auto get_memory_barrier_info(MemoryBarrierInfo const& info) const -> VkMemoryBarrier2;
	auto get_buffer_barrier_info(DeviceImpl const& device, BufferBarrierInfo const& info) const -> VkBufferMemoryBarrier2;
	auto get_image_barrier_info(DeviceImpl const& device, ImageBarrierInfo const& info) const -> VkImageMemoryBarrier2;
}; 

struct CommandBufferPool
{
	lib::array<CommandBufferImpl> commandBuffers;
	uint32 current;
};

class CommandPoolImpl : public CommandPool
{
public:
	CommandPoolImpl() = default;
	CommandPoolImpl(DeviceImpl& device);

	DeviceImpl* vkdevice = {};
	VkCommandPool handle = VK_NULL_HANDLE;
	CommandBufferPool commandBufferPool = {};

	auto allocate_new_command_buffer() -> uint64;
};

struct Zombie
{
	using device_timeline_t = Device::cpu_timeline_t;
	using destroy_fn 		= lib::function<void(DeviceImpl&), { .capacity = sizeof(uintptr_t) * 5 }>;
	
	device_timeline_t timeline;
	destroy_fn destroyFn;
};

struct _ResourceMeta
{
	uint32 type;
	uint32 id;
};

struct ResourcePool
{
	template <typename T>
	using Cache = ankerl::unordered_dense::map<uint64, typename plf::colony<T>::iterator>;

	struct
	{
		plf::colony<SemaphoreImpl> binarySemaphore{ plf::limits{ 8, 64 } };
		plf::colony<FenceImpl> timelineSemaphore{ plf::limits{ 8, 64 } };
		plf::colony<EventImpl> events{ plf::limits{ 8, 64 } };
		plf::colony<BufferImpl> buffers{ plf::limits{ 8, 64 } };
		plf::colony<ImageImpl> images{ plf::limits{ 8, 64 } };
		plf::colony<SamplerImpl> samplers{ plf::limits{ 8, 64 } };
		plf::colony<Surface> surfaces{ plf::limits{ 4, 8 } };
		plf::colony<SwapchainImpl> swapchains{ plf::limits{ 4, 8 } };
		plf::colony<ShaderImpl> shaders{ plf::limits{ 8, 64 } };
		plf::colony<PipelineImpl> pipelines{ plf::limits{ 8, 64 } };
		plf::colony<MemoryBlockImpl> memoryBlocks{ plf::limits{ 8, 64 } };
		plf::colony<CommandPoolImpl> commandPools{ plf::limits{ 4, 16 } };
	} stores;
	
	struct
	{
		Cache<SemaphoreImpl> binarySemaphore;
		Cache<FenceImpl> timelineSemaphore;
		Cache<EventImpl> event;
		Cache<BufferImpl> buffer;
		Cache<ImageImpl> image;
		Cache<SamplerImpl> sampler;
		Cache<SwapchainImpl> swapchain;
		Cache<ShaderImpl> shader;
		Cache<PipelineImpl> pipeline;
		Cache<MemoryBlockImpl> memoryBlock;
		Cache<CommandPoolImpl> commandPool;
	} caches;

	struct
	{
		uint32 images;
		uint32 buffers;
		uint32 sampler;
		uint32 others;
	} idCounter;

	/*
	* NOTE(afiq):
	* Don't need separate reference counters for each resource at the moment.
	* Might consider it in the future.
	*/
	using RefCountCache = ankerl::unordered_dense::map<uint64_t, plf::colony<std::atomic_uint64_t>::iterator>;
	/* 
	* This needed to be done because std::atomic is not copy assignable.
	* Hence we need to store it in some container that doesn't move contents around when growing.
	*/
	plf::colony<std::atomic_uint64_t> referenceCounts;
	RefCountCache referenceCountCache;

	std::deque<Zombie> zombies;
	std::mutex zombieMutex;
};

struct DescriptorCache
{
	// Pipeline layouts.
	ankerl::unordered_dense::map<uint32, VkPipelineLayout> pipelineLayouts;

	// Descriptors.
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet	descriptorSet;
	
	// Buffer device address.
	VkBuffer bdaBuffer;
	VmaAllocation bdaAllocation;
	VkDeviceAddress* bdaHostAddress;
};

struct DeviceImpl : public Device
{
	VkInstance instance = {};
	VkPhysicalDevice gpu = {};
	VkDevice device	= {};
	VkPhysicalDeviceProperties properties = {};
	VkPhysicalDeviceVulkan11Properties vulkan11Properties = {};
	VkPhysicalDeviceVulkan12Properties vulkan12Properties = {};
	VkPhysicalDeviceVulkan13Properties vulkan13Properties = {};
	VkPhysicalDeviceFeatures features = {};
	VkPhysicalDeviceSubgroupProperties subgroupProperties = {};
	VmaAllocator allocator = {};
	VkDebugUtilsMessengerEXT debugger = {};
	Queue mainQueue = {};
	Queue transferQueue = {};
	Queue computeQueue = {};
	ResourcePool gpuResourcePool = {};
	DescriptorCache descriptorCache = {};
	lib::array<VkSemaphore> waitSemaphores = {};
	lib::array<VkSemaphore> signalSemaphores = {};
	lib::array<VkPipelineStageFlags> waitDstStageMasks = {};
	lib::array<uint64> waitTimelineSemaphoreValues = {};
	lib::array<uint64> signalTimelineSemaphoreValues = {};
	lib::array<VkCommandBuffer> submitCommandBuffers = {};
	lib::array<VkSwapchainKHR> presentSwapchains = {};
	lib::array<uint32> presentImageIndices = {};

	auto initialize(DeviceInitInfo const&) -> bool;
	auto terminate() -> void;

	auto push_constant_pipeline_layout(uint32 const pushConstantSize, uint32 const max) -> VkPipelineLayout;

	auto setup_debug_name(SwapchainImpl const& swapchain) -> void;
	auto setup_debug_name(ShaderImpl const& shader) -> void;
	auto setup_debug_name(BufferImpl const& buffer) -> void;
	auto setup_debug_name(ImageImpl const& image) -> void;
	auto setup_debug_name(SamplerImpl const& sampler) -> void;
	auto setup_debug_name(PipelineImpl const& pipeline) -> void;
	auto setup_debug_name(CommandPoolImpl const& commandPool) -> void;
	auto setup_debug_name(CommandBufferImpl const& commandBuffer) -> void;
	auto setup_debug_name(SemaphoreImpl const& semaphore) -> void;
	auto setup_debug_name(FenceImpl const& fence) -> void;
	auto setup_debug_name(EventImpl const& event) -> void;
	auto setup_debug_name(MemoryBlockImpl const& event) -> void;

	auto create_vulkan_instance() -> bool;
	auto create_debug_messenger() -> bool;
	auto choose_physical_device() -> bool;
	auto get_physical_device_subgroup_properties() -> void;
	auto get_device_queue_family_indices() -> void;
	auto create_logical_device() -> bool;
	auto get_queue_handles() -> void;
	auto create_device_allocator() -> bool;

	auto create_descriptor_pool() -> bool;
	auto create_descriptor_set_layout() -> bool;
	auto allocate_descriptor_set() -> bool;
	auto create_pipeline_layouts() -> bool;
	auto initialize_descriptor_cache() -> bool;

	auto flush_submit_info_buffers() -> void;
	
	auto clear_descriptor_cache() -> void;
	auto cleanup_resource_pool() -> void;

	auto bind(ImageImpl const& image, uint32 at) -> void;
	auto bind(BufferImpl const& buffer, uint32 at) -> void;
	auto bind(SamplerImpl const& sampler, uint32 at) -> void;

	auto begin_referencing(uint64 id) -> void;
};

auto translate_memory_usage(MemoryUsage const usage) -> VmaAllocationCreateFlags;
auto translate_buffer_usage_flags(BufferUsage const flags) -> VkBufferUsageFlags;
auto translate_format(Format const format) -> VkFormat;
auto translate_image_usage_flags(ImageUsage const usage) -> VkImageUsageFlags;
auto translate_image_type(ImageType const type) -> VkImageType;
auto translate_sample_count(SampleCount const samples) -> VkSampleCountFlagBits;
auto translate_image_tiling(ImageTiling const tiling) -> VkImageTiling;
auto translate_image_view_type(ImageType const type) -> VkImageViewType;
auto translate_swapchain_presentation_mode(SwapchainPresentMode const mode) -> VkPresentModeKHR;
auto translate_color_space(ColorSpace const colorSpace) -> VkColorSpaceKHR;
auto translate_shader_stage(ShaderType const type) -> VkShaderStageFlagBits;
auto translate_texel_filter(TexelFilter const filter) -> VkFilter;
auto translate_mipmap_mode(MipmapMode const mode) -> VkSamplerMipmapMode;
auto translate_sampler_address_mode(SamplerAddress const address) -> VkSamplerAddressMode;
auto translate_compare_op(CompareOp const op) -> VkCompareOp;
auto translate_border_color(BorderColor const color) -> VkBorderColor;
auto translate_attachment_load_op(AttachmentLoadOp const loadOp) -> VkAttachmentLoadOp;
auto translate_attachment_store_op(AttachmentStoreOp const storeOp) -> VkAttachmentStoreOp;
auto translate_shader_attrib_format(Format const format) -> VkFormat;
auto translate_topology(TopologyType const topology) -> VkPrimitiveTopology;
auto translate_polygon_mode(PolygonMode const mode) -> VkPolygonMode;
auto translate_cull_mode(CullingMode const mode) -> VkCullModeFlags;
auto translate_front_face_dir(FrontFace const face) -> VkFrontFace;
auto translate_blend_factor(BlendFactor const factor) -> VkBlendFactor;
auto translate_blend_op(BlendOp const op) -> VkBlendOp;
auto sampler_info_packed_uint64(SamplerInfo const& info) -> uint64;
auto translate_image_aspect_flags(ImageAspect flags) -> VkImageAspectFlags;
auto translate_image_layout(ImageLayout layout) -> VkImageLayout;
auto translate_pipeline_stage_flags(PipelineStage stages) -> VkPipelineStageFlags2;
auto translate_memory_access_flags(MemoryAccessType accesses) -> VkAccessFlags2;
auto translate_shader_stage_flags(ShaderStage shaderStage) -> VkShaderStageFlags;
auto translate_sharing_mode(SharingMode sharingMode) -> VkSharingMode;
auto stride_for_shader_attrib_format(Format const format) -> uint32;
auto vk_to_rhi_color_space(VkColorSpaceKHR colorSpace) -> ColorSpace;

auto get_image_create_info(ImageInfo const& info) -> VkImageCreateInfo;
auto get_buffer_create_info(BufferInfo const& info) -> VkBufferCreateInfo;

auto to_error_message(VkResult result) -> std::string_view;
}

auto to_device(Device const* device) -> vk::DeviceImpl const&;
auto to_device(Device const& device) -> vk::DeviceImpl const&;
auto to_device(Device* device) -> vk::DeviceImpl&;
auto to_device(Device& device) -> vk::DeviceImpl&;
auto to_impl(MemoryBlock& memoryBlock) -> vk::MemoryBlockImpl&;
auto to_impl(Semaphore& semaphore) -> vk::SemaphoreImpl&;
auto to_impl(Fence& fence) -> vk::FenceImpl&;
auto to_impl(Event& event) -> vk::EventImpl&;
auto to_impl(Image& image) -> vk::ImageImpl&;
auto to_impl(Buffer& buffer) -> vk::BufferImpl&;
auto to_impl(Sampler& sampler) -> vk::SamplerImpl&;
auto to_impl(Shader& shader) -> vk::ShaderImpl&;
auto to_impl(Pipeline& pipeline) -> vk::PipelineImpl&;
auto to_impl(Swapchain& swapchain) -> vk::SwapchainImpl&;
auto to_impl(CommandBuffer& cmdBuffer) -> vk::CommandBufferImpl&;
auto to_impl(CommandPool& cmdPool) -> vk::CommandPoolImpl&;
auto to_impl(MemoryBlock const& memoryBlock) -> vk::MemoryBlockImpl const&;
auto to_impl(Semaphore const& semaphore) -> vk::SemaphoreImpl const&;
auto to_impl(Fence const& fence) -> vk::FenceImpl const&;
auto to_impl(Event const& event) -> vk::EventImpl const&;
auto to_impl(Image const& image) -> vk::ImageImpl const&;
auto to_impl(Buffer const& buffer) -> vk::BufferImpl const&;
auto to_impl(Sampler const& image) -> vk::SamplerImpl const&;
auto to_impl(Shader const& shader) -> vk::ShaderImpl const&;
auto to_impl(Pipeline const& pipeline) -> vk::PipelineImpl const&;
auto to_impl(Swapchain const& swapchain) -> vk::SwapchainImpl const&;
auto to_impl(CommandPool const& cmdPool) -> vk::CommandPoolImpl const&;
}

#endif // !VK_GPU_H

