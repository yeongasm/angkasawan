#pragma once
#ifndef RENDERER_RHI_VULKAN_VK_DEVICE_H
#define RENDERER_RHI_VULKAN_VK_DEVICE_H

#include "fmt/format.h"
#include "lib/map.h"
#include "lib/paged_array.h"
#include "vk.h"
#include "swapchain.h"
#include "pipeline.h"
#include "command_pool.h"
#include "shader.h"
#include "sampler.h"

namespace rhi
{

static constexpr uint32 INVALID_QUEUE_FAMILY_INDEX = std::numeric_limits<uint32>::max();

namespace vulkan
{

enum class Resource
{
	Surface,
	Swapchain,
	Shader,
	Buffer,
	Image,
	Pipeline,
	Sampler,
	CommandPool,
	CommandBuffer,
	Semaphore
};

struct Queue
{
	VkQueue	queue;
	VkQueueFamilyProperties properties;
	uint32 familyIndex = INVALID_QUEUE_FAMILY_INDEX;
};

struct Image
{
	VkImage	handle;
	VkImageView	imageView;
	VmaAllocation allocation;
	bool isSwapchainImage = false;
};

struct Surface
{
	VkSurfaceKHR handle;
	lib::array<VkSurfaceFormatKHR> availableColorFormats;
	VkSurfaceCapabilitiesKHR capabilities;
};

struct Swapchain
{
	vulkan::Surface* pSurface;
	VkSwapchainKHR handle;
	VkSurfaceFormatKHR surfaceColorFormat;
	lib::array<VkImage> images;
	lib::array<VkImageView> imageViews;
};

struct Shader
{
	VkShaderModule handle;
	VkShaderStageFlagBits stage;
};

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
struct Buffer
{
	VkBuffer handle;
	VkDeviceAddress address;
	VmaAllocation allocation;
};

struct Sampler
{
	VkSampler handle;
	uint32 references;
};

struct Pipeline
{
	VkPipeline handle;
	VkPipelineLayout layout;
};

struct Zombie
{
	std::uintptr_t address; // Only used for surfaces & command buffers.
	uint32 location;
	Resource type;
};

struct CommandPool
{
	VkCommandPool handle;
};

struct CommandBuffer
{
	static constexpr size_t MAX_COMMAND_BUFFER_BARRIER_COUNT = 16;

	VkCommandBuffer handle;
	std::array<VkMemoryBarrier2, MAX_COMMAND_BUFFER_BARRIER_COUNT> memoryBarriers;
	std::array<VkBufferMemoryBarrier2, MAX_COMMAND_BUFFER_BARRIER_COUNT> bufferBarriers;
	std::array<VkImageMemoryBarrier2, MAX_COMMAND_BUFFER_BARRIER_COUNT> imageBarriers;
	size_t numMemoryBarrier;
	size_t numBufferBarrier;
	size_t numImageBarrier;
};

struct CommandBufferPool
{
	std::array<CommandBuffer, MAX_COMMAND_BUFFER_PER_POOL> commandBuffers;
	std::array<size_t, MAX_COMMAND_BUFFER_PER_POOL> freeSlots;
	uint32 commandBufferCount;
	uint32 freeSlotCount;
	uint32 currentFreeSlot;
};

struct Semaphore
{
	VkSemaphore handle;
	VkSemaphoreType type;
	uint64 value;
};

}

struct ResourcePool
{
	lib::paged_array<vulkan::CommandPool, 16> commandPools;
	lib::map<std::uintptr_t, vulkan::CommandBufferPool> commandBufferPools;
	lib::map<std::uintptr_t, vulkan::Surface> surfaces;
	lib::paged_array<vulkan::Swapchain, 8> swapchains;
	lib::paged_array<vulkan::Shader, 16> shaders;
	lib::paged_array<vulkan::Buffer, 16> buffers;
	lib::paged_array<vulkan::Image, 16> images;
	lib::paged_array<vulkan::Sampler, 16> samplers;
	lib::map<uint64, uint32> samplerCache;
	lib::paged_array<vulkan::Pipeline, 16> pipelines;
	lib::paged_array<vulkan::Semaphore, 16> semaphores;
	lib::array<vulkan::Zombie> zombies;

	auto location_of(vulkan::Swapchain& swapchain) -> uint32;
	auto location_of(vulkan::Shader& shader) -> uint32;
	auto location_of(vulkan::Buffer& buffer) -> uint32;
	auto location_of(vulkan::Image& image) -> uint32;
	auto location_of(vulkan::Sampler& sampler) -> uint32;
	auto location_of(vulkan::Pipeline& pipeline) -> uint32;
	auto location_of(vulkan::CommandPool& commandPool) -> uint32;
	auto location_of(vulkan::Semaphore& semaphore) -> uint32;
};

struct DescriptorCache
{
	// Descriptors.
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet	descriptorSet;
	
	// Buffer device address.
	VkBuffer bdaBuffer;
	VmaAllocation bdaAllocation;
	std::uintptr_t* bdaHostAddress;
	lib::array<VkDeviceAddress> bufferAddresses; // This dynamic array stores a list of buffer device addresses that gets memcopied into the bda buffer.

	// Pipeline layouts.
	lib::map<uint32, VkPipelineLayout> pipelineLayouts;
};

struct APIContext
{
	VkInstance instance = {};
	VkPhysicalDevice gpu = {};
	VkDevice device	= {};
	VkPhysicalDeviceProperties properties = {};
	VkPhysicalDeviceFeatures features = {};
	VmaAllocator allocator = {};
	VkDebugUtilsMessengerEXT debugger = {};
	vulkan::Queue mainQueue = {};
	vulkan::Queue transferQueue = {};
	vulkan::Queue computeQueue = {};
	DeviceInitInfo init	= {};
	DeviceConfig config = {};
	ResourcePool gpuResourcePool = {};
	DescriptorCache descriptorCache = {};
	lib::array<VkSemaphore> wait_semaphores = {};
	lib::array<VkSemaphore> signal_semaphores = {};
	lib::array<VkPipelineStageFlags> wait_dst_stage_masks = {};
	lib::array<uint64> wait_timeline_semaphore_values = {};
	lib::array<uint64> signal_timeline_semaphore_values = {};
	lib::array<VkCommandBuffer> submit_command_buffers = {};
	lib::array<VkSwapchainKHR> present_swapchains = {};
	lib::array<uint32> present_image_indices = {};
	bool validation = {};

	auto initialize(DeviceInitInfo const&) -> bool;
	auto terminate() -> void;
	auto create_swapchain(SwapchainInfo&& info, Swapchain* oldSwapchain = nullptr) -> Swapchain;
	auto destroy_swapchain(Swapchain& swapchain, bool destroySurface = false) -> void;
	auto create_shader(CompiledShaderInfo&& info) -> Shader;
	auto destroy_shader(Shader& shader) -> void;
	auto allocate_buffer(BufferInfo&& info) -> Buffer;
	auto release_buffer(Buffer& buffer) -> void;
	auto create_image(ImageInfo&& info) -> Image;
	auto destroy_image(Image& image) -> void;
	auto create_sampler(SamplerInfo&& info) -> Sampler;
	auto destroy_sampler(Sampler& sampler) -> void;
	auto create_pipeline(RasterPipelineInfo&& info, PipelineShaderInfo const& pipelineShaders) -> RasterPipeline;
	auto destroy_pipeline(RasterPipeline& pipeline) -> void;
	auto clear_destroyed_resources() -> void;
	auto wait_idle() -> void;
	auto create_command_pool(CommandPoolInfo&& info) -> CommandPool;
	auto destroy_command_pool(CommandPool& commandPool) -> void;
	auto create_binary_semaphore(SemaphoreInfo&& info) -> Semaphore;
	auto destroy_binary_semaphore(Semaphore& semaphore) -> void;
	auto create_timeline_semaphore(FenceInfo&& info) -> Fence;
	auto destroy_timeline_semaphore(Fence& fence) -> void;
	auto submit(SubmitInfo const& info) -> bool;
	auto present(PresentInfo const& info) -> bool;
	auto setup_debug_name(Swapchain const& swapchain) -> void;
	auto setup_debug_name(Shader const& shader) -> void;
	auto setup_debug_name(Buffer const& buffer) -> void;
	auto setup_debug_name(Image const& image) -> void;
	auto setup_debug_name(Sampler const& sampler) -> void;
	auto setup_debug_name(RasterPipeline const& pipeline) -> void;
	auto setup_debug_name(CommandPool const& commandPool) -> void;
	auto setup_debug_name(CommandBuffer const& commandBuffer) -> void;
	auto setup_debug_name(Semaphore const& semaphore) -> void;
	auto setup_debug_name(Fence const& fence) -> void;
	//auto update_descriptor_set_buffer(VkBuffer buffer, size_t offset, size_t size, uint32 index) -> void;
	//auto update_descriptor_set_image(VkImageView imageView, ImageUsage usage, uint32 index) -> void;
	//auto update_descriptor_set_sampler(VkSampler sampler, uint32 index) -> void;
private:
	auto create_vulkan_instance(DeviceInitInfo const& info) -> bool;
	auto create_debug_messenger(DeviceInitInfo const& info) -> bool;
	auto choose_physical_device() -> bool;
	auto get_device_queue_family_indices() -> void;
	auto create_logical_device() -> bool;
	auto get_queue_handles() -> void;
	auto create_device_allocator() -> bool;
	auto clear_zombies() -> void;
	auto initialize_resource_pool() -> void;
	auto cleanup_resource_pool() -> void;
	auto destroy_surface_zombie(std::uintptr_t address) -> void;
	auto destroy_swapchain_zombie(uint32 location) -> void;
	auto destroy_shader_zombie(uint32 location) -> void;
	auto destroy_buffer_zombie(uint32 location) -> void;
	auto destroy_image_zombie(uint32 location) -> void;
	auto destroy_sampler_zombie(uint32 location) -> void;
	auto destroy_command_pool_zombie(uint32 location) -> void;
	auto destroy_command_buffer_zombie(std::uintptr_t address, size_t location) -> void;
	auto destroy_semaphore_zombie(uint32 location) -> void;
	auto create_surface(SurfaceInfo const&) -> vulkan::Surface*;
	auto create_descriptor_pool() -> bool;
	auto create_descriptor_set_layout() -> bool;
	auto allocate_descriptor_set() -> bool;
	auto create_pipeline_layouts() -> bool;
	auto get_appropriate_pipeline_layout(uint32 pushConstantSize, uint32 const max) -> VkPipelineLayout;
	auto initialize_descriptor_cache() -> bool;
	auto clear_descriptor_cache() -> void;
	auto destroy_pipeline_internal(Pipeline& pipeline) -> void;
	auto flush_submit_info_buffers() -> void;
};

}

#endif // !RENDERER_RHI_VULKAN_VK_DEVICE_H

