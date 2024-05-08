module;

#include "lib/map.h"
#include "lib/paged_array.h"
#include "vk.h"

export module forge.api;

export import forge.common;

namespace frg::api
{
static constexpr uint32 INVALID_QUEUE_FAMILY_INDEX = std::numeric_limits<uint32>::max();

export struct Queue
{
	VkQueue	queue;
	VkQueueFamilyProperties properties;
	uint32 familyIndex = INVALID_QUEUE_FAMILY_INDEX;
};

export struct Image
{
	VkImage	handle;
	VkImageView	imageView;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
};

export struct Surface
{
	VkSurfaceKHR handle;
	lib::array<VkSurfaceFormatKHR> availableColorFormats;
	VkSurfaceCapabilitiesKHR capabilities;
};

export struct Swapchain
{
	Surface* pSurface;
	VkSwapchainKHR handle;
	VkSurfaceFormatKHR surfaceColorFormat;
};

export struct Shader
{
	VkShaderModule handle;
	VkShaderStageFlagBits stage;
};

// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
export struct Buffer
{
	VkBuffer handle;
	VkDeviceAddress address;
	VmaAllocation allocation;
	VmaAllocationInfo allocationInfo;
};

export struct Sampler
{
	VkSampler handle;
	uint32 references;
};

export struct Pipeline
{
	VkPipeline handle;
	VkPipelineLayout layout;
};

export struct CommandPool
{
	VkCommandPool handle;
};

export struct CommandBuffer
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

//export struct CommandBufferPool
//{
//	std::array<CommandBuffer, frg::MAX_COMMAND_BUFFER_PER_POOL> commandBuffers;
//	std::array<size_t, frg::MAX_COMMAND_BUFFER_PER_POOL> freeSlots;
//	uint32 commandBufferCount;
//	uint32 freeSlotCount;
//	uint32 currentFreeSlot;
//};

export struct Semaphore
{
	VkSemaphore handle;
	VkSemaphoreType type;
};

export struct DescriptorCache
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

export struct Context
{
	using SurfaceCache = lib::map<std::uintptr_t, std::unique_ptr<api::Surface>>;

	VkInstance instance;
	VkPhysicalDevice gpu;
	VkDevice device;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VmaAllocator allocator;
	VkDebugUtilsMessengerEXT debugger;
	Queue mainQueue;
	Queue transferQueue;
	Queue computeQueue;
	DescriptorCache descriptorCache;
	SurfaceCache surfaceCache;
	lib::array<VkSemaphore> waitSemaphores;
	lib::array<VkSemaphore> signalSemaphores;
	lib::array<VkPipelineStageFlags> waitDstStageMasks;
	lib::array<uint64> waitTimelineSemaphoreValues;
	lib::array<uint64> signalTimelineSemaphoreValues;
	lib::array<VkCommandBuffer> submitCommandBuffers;
	lib::array<VkSwapchainKHR> presentSwapchains;
	lib::array<uint32> presentImageIndices;
	bool validation;

	auto initialize(frg::DeviceInitInfo const& initInfo, frg::DeviceConfig& config) -> bool;
	auto terminate() -> void;
	auto push_constant_pipeline_layout(uint32 const pushConstantSize, uint32 const max) -> VkPipelineLayout;
	auto flush_submit_info_buffers() -> void;
private:
	auto create_vulkan_instance(frg::DeviceInitInfo const& info) -> bool;
	auto create_debug_messenger(frg::DeviceInitInfo const& info) -> bool;
	auto choose_physical_device(frg::DeviceInitInfo const& info) -> bool;
	auto get_device_queue_family_indices() -> void;
	auto create_logical_device() -> bool;
	auto get_queue_handles() -> void;
	auto create_device_allocator() -> bool;
	auto initialize_descriptor_cache(frg::DeviceConfig const& config) -> bool;

	// descriptorCache
	auto create_descriptor_pool(frg::DeviceConfig const& config) -> bool;
	auto create_descriptor_set_layout(frg::DeviceConfig const& config) -> bool;
	auto allocate_descriptor_set() -> bool;
	auto create_pipeline_layouts(frg::DeviceConfig const& config) -> bool;
};

export auto translate_memory_usage(frg::MemoryUsage const usage) -> VmaAllocationCreateFlags;
export auto translate_buffer_usage_flags(frg::BufferUsage const flags) -> VkBufferUsageFlags;
export auto translate_format(frg::Format const format) -> VkFormat;
export auto translate_image_usage_flags(frg::ImageUsage const usage) -> VkImageUsageFlags;
export auto translate_image_type(frg::ImageType const type) -> VkImageType;
export auto translate_sample_count(frg::SampleCount const samples) -> VkSampleCountFlagBits;
export auto translate_image_tiling(frg::ImageTiling const tiling) -> VkImageTiling;
export auto translate_image_view_type(frg::ImageType const type) -> VkImageViewType;
export auto translate_swapchain_presentation_mode(frg::SwapchainPresentMode const mode) -> VkPresentModeKHR;
export auto translate_color_space(frg::ColorSpace const colorSpace) -> VkColorSpaceKHR;
export auto translate_shader_stage(frg::ShaderType const type) -> VkShaderStageFlagBits;
export auto translate_texel_filter(frg::TexelFilter const filter) -> VkFilter;
export auto translate_mipmap_mode(frg::MipmapMode const mode) -> VkSamplerMipmapMode;
export auto translate_sampler_address_mode(frg::SamplerAddress const address) -> VkSamplerAddressMode;
export auto translate_compare_op(frg::CompareOp const op) -> VkCompareOp;
export auto translate_border_color(frg::BorderColor const color) -> VkBorderColor;
export auto translate_attachment_load_op(frg::AttachmentLoadOp const loadOp) -> VkAttachmentLoadOp;
export auto translate_attachment_store_op(frg::AttachmentStoreOp const storeOp) -> VkAttachmentStoreOp;
export auto translate_shader_attrib_format(frg::Format const format) -> VkFormat;
export auto translate_topology(frg::TopologyType const topology) -> VkPrimitiveTopology;
export auto translate_polygon_mode(frg::PolygonMode const mode) -> VkPolygonMode;
export auto translate_cull_mode(frg::CullingMode const mode) -> VkCullModeFlags;
export auto translate_front_face_dir(frg::FrontFace const face) -> VkFrontFace;
export auto translate_blend_factor(frg::BlendFactor const factor) -> VkBlendFactor;
export auto translate_blend_op(frg::BlendOp const op) -> VkBlendOp;
export auto translate_image_aspect_flags(frg::ImageAspect flags) -> VkImageAspectFlags;
export auto translate_image_layout(frg::ImageLayout layout) -> VkImageLayout;
export auto translate_pipeline_stage_flags(frg::PipelineStage stages) -> VkPipelineStageFlags2;
export auto translate_memory_access_flags(frg::MemoryAccessType accesses) -> VkAccessFlags2;
export auto translate_shader_stage_flags(frg::ShaderStage shaderStage) -> VkShaderStageFlags;
export auto stride_for_shader_attrib_format(frg::Format const format) -> uint32;

export auto sampler_info_packed_uint64(frg::SamplerInfo const& info) -> uint64;

export auto vk_to_rhi_color_space(VkColorSpaceKHR colorSpace) -> frg::ColorSpace;
}