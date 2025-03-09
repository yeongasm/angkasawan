#pragma once
#ifndef VK_GPU_H
#define VK_GPU_H

#include <deque>
#include <mutex>
#include <array>

#include "lib/array.hpp"
#include "lib/map.hpp"

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
class DeviceImpl;

enum class ResourceType : uint32
{
	Memory_Block,
	Semaphore,
	Fence,
	Event,
	Buffer,
	Image,
	Sampler,
	Swapchain,
	Shader,
	Pipeline,
	Command_Pool
};

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
	MemoryBlockImpl(DeviceImpl& device, bool aliased);

	VmaAllocation handle = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo = {};
};

class SemaphoreImpl : public Semaphore
{
public:
	SemaphoreImpl() = default;
	SemaphoreImpl(DeviceImpl& device);

	VkSemaphore handle = VK_NULL_HANDLE;
};

class FenceImpl : public Fence
{
public:
	FenceImpl() = default;
	FenceImpl(DeviceImpl& device);

	VkSemaphore handle = VK_NULL_HANDLE;
};

class EventImpl : public Event
{
public:
	EventImpl() = default;
	EventImpl(DeviceImpl& device);

	VkEvent handle = VK_NULL_HANDLE;
};

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
class BufferImpl : public Buffer
{
public:
	BufferImpl() = default;
	BufferImpl(DeviceImpl& device);

	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceAddress address = {};
	Resource<MemoryBlock> allocationBlock = {};
};

class ImageImpl : public Image
{
public:
	ImageImpl() = default;
	ImageImpl(DeviceImpl& device);

	VkImage	handle = VK_NULL_HANDLE;
	VkImageView	imageView = VK_NULL_HANDLE;
	Resource<MemoryBlock> allocationBlock = {};
};

class SamplerImpl : public Sampler
{
public:
	SamplerImpl() = default;
	SamplerImpl(DeviceImpl& device);

	VkSampler handle = VK_NULL_HANDLE;
	std::atomic_uint32_t refCount = {};
};

struct Surface
{
	VkSurfaceKHR handle = VK_NULL_HANDLE;
	lib::array<VkSurfaceFormatKHR> availableColorFormats = {};
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::atomic_uint32_t refCount = {};
};

class SwapchainImpl : public Swapchain
{
public:
	SwapchainImpl() = default;
	SwapchainImpl(DeviceImpl& device);

	Surface* pSurface = nullptr;
	VkSwapchainKHR handle = VK_NULL_HANDLE;
	VkSurfaceFormatKHR surfaceColorFormat = {};
};

class ShaderImpl : public Shader
{
public:
	ShaderImpl() = default;
	ShaderImpl(DeviceImpl& device);

	VkShaderModule handle = VK_NULL_HANDLE;
	VkShaderStageFlagBits stage = {};
};

class PipelineImpl : public Pipeline
{
public:
	PipelineImpl() = default;
	PipelineImpl(DeviceImpl& device);

	VkPipeline handle = VK_NULL_HANDLE;
	VkPipelineLayout layout = {};
};

class CommandBufferImpl : public CommandBuffer
{
public:
	CommandBufferImpl() = default;
	CommandBufferImpl(DeviceImpl& device);

	auto get_memory_barrier_info(MemoryBarrierInfo const& info) const -> VkMemoryBarrier2;
	auto get_buffer_barrier_info(BufferBarrierInfo const& info) const -> VkBufferMemoryBarrier2;
	auto get_image_barrier_info(ImageBarrierInfo const& info) const -> VkImageMemoryBarrier2;

	static constexpr size_t MAX_COMMAND_BUFFER_BARRIER_COUNT = 16;

	template <typename T>
	using BarrierArray = std::array<T, MAX_COMMAND_BUFFER_BARRIER_COUNT>;

	VkCommandBuffer handle = VK_NULL_HANDLE;
	BarrierArray<VkMemoryBarrier2> memoryBarriers = {};
	BarrierArray<VkBufferMemoryBarrier2> bufferBarriers = {};
	BarrierArray<VkImageMemoryBarrier2> imageBarriers = {};
	size_t numMemoryBarrier = {};
	size_t numBufferBarrier = {};
	size_t numImageBarrier = {};
}; 

struct CommandBufferPool
{
	std::array<CommandBufferImpl, MAX_COMMAND_BUFFER_PER_POOL> commandBuffers;
	std::array<size_t, MAX_COMMAND_BUFFER_PER_POOL> freeSlots;
	uint32 commandBufferCount;
	uint32 freeSlotCount;
	uint32 currentFreeSlot;
};

class CommandPoolImpl : public CommandPool
{
public:
	CommandPoolImpl() = default;
	CommandPoolImpl(DeviceImpl& device);

	VkCommandPool handle = VK_NULL_HANDLE;
	CommandBufferPool commandBufferPool = {};
};

struct Zombie
{
	using device_timeline_t = Device::cpu_timeline_t;
	
	device_timeline_t timeline;
	void const* resource;
	ResourceType resourceType;
};

struct ResourcePool
{
	lib::hive<SemaphoreImpl> binarySemaphore;
	lib::hive<FenceImpl> timelineSemaphore;
	lib::hive<EventImpl> events;
	lib::hive<BufferImpl> buffers;
	lib::hive<ImageImpl> images;
	lib::hive<SamplerImpl> samplers;
	lib::map<uint64, SamplerImpl*> samplerCache;
	lib::hive<Surface> surfaces;
	lib::hive<SwapchainImpl> swapchains;
	lib::hive<ShaderImpl> shaders;
	lib::hive<PipelineImpl> pipelines;
	lib::hive<MemoryBlockImpl> memoryBlocks;
	lib::hive<CommandPoolImpl> commandPools;

	std::deque<Zombie> zombies;
	std::mutex zombieMutex;
};

struct DescriptorCache
{
	// Pipeline layouts.
	lib::map<uint32, VkPipelineLayout> pipelineLayouts;

	// Descriptors.
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet	descriptorSet;
	
	// Buffer device address.
	VkBuffer bdaBuffer;
	VmaAllocation bdaAllocation;
	VkDeviceAddress* bdaHostAddress;
};

class DeviceImpl : public Device
{
public:
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

	auto destroy_memory_block(void const* resource) -> void;
	auto destroy_binary_semaphore(void const* resource) -> void;
	auto destroy_timeline_semaphore(void const* resource) -> void;
	auto destroy_event(void const* resource) -> void;
	auto destroy_buffer(void const* resource) -> void;
	auto destroy_image(void const* resource) -> void;
	auto destroy_sampler(void const* resource) -> void;
	auto destroy_swapchain(void const* resource) -> void;
	auto destroy_shader(void const* resource) -> void;
	auto destroy_pipeline(void const* resource) -> void;
	auto destroy_command_pool(void const* resource) -> void;

	auto clear_descriptor_cache() -> void;
	auto cleanup_resource_pool() -> void;
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
auto to_impl(CommandBuffer const& cmdBuffer) -> vk::CommandBufferImpl const&;
auto to_impl(CommandPool const& cmdPool) -> vk::CommandPoolImpl const&;
}

#endif // !VK_GPU_H

