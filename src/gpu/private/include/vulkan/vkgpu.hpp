#pragma once
#include <type_traits>
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
struct DeviceImpl;

static constexpr uint32 INVALID_QUEUE_FAMILY_INDEX = std::numeric_limits<uint32>::max();

struct Queue
{
	VkQueue	queue = VK_NULL_HANDLE;
	VkQueueFamilyProperties properties = {};
	uint32 familyIndex = INVALID_QUEUE_FAMILY_INDEX;
};

struct MemoryBlockImpl : ref_counted_base
{
	VmaAllocation handle;
	VmaAllocationInfo allocationInfo;
	MemoryBlockInfo info;
	bool aliased;
};

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
struct BufferImpl : ref_counted_base
{
	VkBuffer handle;
	VkDeviceAddress address;
	MemoryBlock memoryBlock;
	BufferInfo info;
	uint64 id;
};

struct ImageImpl : ref_counted_base
{
	VkImage	handle;
	VkImageView	imageView;
	MemoryBlock memoryBlock;
	ImageInfo info;
	uint64 id;
};

struct SamplerImpl : ref_counted_base
{
	VkSampler handle;
	SamplerInfo info;
	uint64 packedInfoBits;
	uint64 id;
};

struct Surface
{
	VkSurfaceKHR handle = VK_NULL_HANDLE;
	lib::array<VkSurfaceFormatKHR> availableColorFormats = {};
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::atomic_uint32_t refCount = {};
};

struct SemaphoreImpl : ref_counted_base
{
	VkSemaphore handle;
	SemaphoreInfo info;
};

struct FenceImpl : ref_counted_base
{
	VkSemaphore handle;
	FenceInfo info;
};

struct EventImpl : ref_counted_base
{
	VkEvent handle;
	EventInfo info;
};

struct SwapchainImpl : ref_counted_base
{
	using surface_iterator = typename plf::colony<Surface>::iterator;
	
	VkSwapchainKHR handle = VK_NULL_HANDLE;
	surface_iterator surface = {};
	lib::array<Image> images;
	Fence gpuTimeline;
	std::atomic_uint64_t cpuTimeline;
	lib::array<Semaphore> acquireSemaphore;
	lib::array<Semaphore> presentSemaphore;
	uint32 acquireSemaphoreIndex;
	uint32 currentImageIndex;
	VkSurfaceFormatKHR surfaceColorFormat = {};
	SwapchainInfo info;
};

struct ShaderImpl : ref_counted_base
{
	VkShaderModule handle = VK_NULL_HANDLE;
	VkShaderStageFlagBits stage = {};
	ShaderInfo info;
};

struct PipelineImpl : ref_counted_base
{
	VkPipeline handle = VK_NULL_HANDLE;
	VkPipelineLayout layout = {};
	std::variant<RasterPipelineInfo, ComputePipelineInfo> info;
	PipelineType type;
};

struct CommandBufferImpl
{
	CommandBufferImpl() = default;
	CommandBufferImpl(CommandBufferImpl&& rhs);

	static constexpr size_t MAX_COMMAND_BUFFER_BARRIER_COUNT = 8;

	template <typename T>
	using BarrierArray = std::array<T, MAX_COMMAND_BUFFER_BARRIER_COUNT>;

	VkCommandBuffer handle;
	BarrierArray<VkMemoryBarrier2> memoryBarriers;
	BarrierArray<VkBufferMemoryBarrier2> bufferBarriers;
	BarrierArray<VkImageMemoryBarrier2> imageBarriers;
	size_t numMemoryBarrier;
	size_t numBufferBarrier;
	size_t numImageBarrier;
	std::atomic_uint64_t recordingTimeline;
	Fence gpuTimeline;

	auto get_memory_barrier_info(MemoryBarrierInfo const& info) const -> VkMemoryBarrier2;
	auto get_buffer_barrier_info(DeviceImpl const& device, BufferBarrierInfo const& info) const -> VkBufferMemoryBarrier2;
	auto get_image_barrier_info(DeviceImpl const& device, ImageBarrierInfo const& info) const -> VkImageMemoryBarrier2;
}; 

struct CommandBufferPool
{
	lib::array<CommandBufferImpl> commandBuffers;
	uint32 current;
};

struct CommandPoolImpl : ref_counted_base
{
	VkCommandPool handle;
	CommandBufferPool commandBufferPool;
	CommandPoolInfo info;
};

template <> 
struct implementation<MemoryBlock> 	
{
	using type = MemoryBlockImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Buffer> 		
{
	using type = BufferImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Image> 		
{
	using type = ImageImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Sampler> 		
{
	using type = SamplerImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Semaphore> 	
{
	using type = SemaphoreImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Fence> 		
{
	using type = FenceImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Event> 		
{
	using type = EventImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Swapchain> 	
{
	using type = SwapchainImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Shader> 		
{
	using type = ShaderImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<to_type>(resource));
	}
};

template <> 
struct implementation<Pipeline> 	
{
	using type = PipelineImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using cast_to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<cast_to_type>(resource));
	}
};

template <> 
struct implementation<CommandPool> 	
{
	using type = CommandPoolImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, ref_counted_base> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using cast_to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<cast_to_type>(resource));
	}
};


struct Zombie
{
	using device_timeline_t = Device::cpu_timeline_t;
	using destroy_fn 		= lib::function<void(DeviceImpl&), { .capacity = sizeof(uintptr_t) * 2 }>;
	
	device_timeline_t timeline;
	destroy_fn destroyFn;
};

struct ResourcePool
{
	struct
	{
		plf::colony<BufferImpl> buffers{ plf::limits{ 8, 64 } };
		plf::colony<ImageImpl> images{ plf::limits{ 8, 64 } };
		plf::colony<SamplerImpl> samplers{ plf::limits{ 8, 64 } };
		plf::colony<Surface> surfaces{ plf::limits{ 4, 8 } };
		plf::colony<MemoryBlockImpl> memoryBlocks{ plf::limits{ 8, 64 } };
		plf::colony<SemaphoreImpl> semaphores{ plf::limits{ 8, 64 } };
		plf::colony<FenceImpl> fences{ plf::limits{ 8, 64 } };
		plf::colony<EventImpl> events{ plf::limits{ 8, 64 } };
		plf::colony<SwapchainImpl> swapchains{ plf::limits{ 8, 64 } };
		plf::colony<ShaderImpl> shaders{ plf::limits{ 8, 64 } };
		plf::colony<PipelineImpl> pipelines{ plf::limits{ 8, 64 } };
		plf::colony<CommandPoolImpl> commandPools{ plf::limits{ 4, 16 } };
	} stores;
	
	struct
	{
		uint32 images;
		uint32 buffers;
		uint32 sampler;
	} idCounter;

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

struct DeviceImpl final : public Device
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
};

template <> 
struct implementation<Device> 	
{
	using type = DeviceImpl;

	template <typename T>
	requires (std::same_as<std::decay_t<T>, Device> && std::is_reference_v<T>)
	static auto of(T&& resource) -> decltype(auto)
	{
		using reference = type&;
		using const_reference = type const&;
		using cast_to_type = std::conditional_t<std::is_const_v<T>, const_reference, reference>;

		return std::forward_like<T>(static_cast<cast_to_type>(resource));
	}
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

#endif // !VK_GPU_H

