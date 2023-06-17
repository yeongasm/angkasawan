#pragma once
#ifndef RENDERER_RHI_VULKAN_VK_DEVICE_H
#define RENDERER_RHI_VULKAN_VK_DEVICE_H

#include "fmt/format.h"
#include <array>

#include "containers/map.h"
#include "containers/array.h"
#include "vk.h"
#include "rhi.h"

namespace rhi
{

static constexpr uint32 INVALID_QUEUE_FAMILY_INDEX = std::numeric_limits<uint32>::max();

struct VulkanQueue
{
	VkQueue					queue;
	VkQueueFamilyProperties properties;
	uint32					familyIndex = INVALID_QUEUE_FAMILY_INDEX;
};

struct VulkanSurface
{
	VkSurfaceKHR				surface;
	VkSurfaceFormatKHR			format;
	VkSurfaceCapabilitiesKHR	capabilities;
};

struct VulkanSwapchain
{
	VkSwapchainKHR				swapchain;
	VkExtent2D					extent;
	uint32						imageCount;
	uint32						nextImageIndex;
	std::vector<VkImage>		images;
	std::vector<VkImageView>	imageViews;
};

struct VulkanBuffer
{
	VkBuffer		buffer;
	VkDeviceAddress address;
	VmaAllocation	allocation;
};

struct VulkanImage
{
	VkImage			image;
	VkImageView		imageView;
	VmaAllocation	allocation;
};

struct VulkanShader
{
	VkShaderModule			shader;
	VkShaderStageFlagBits	stage;
};

struct VulkanPipeline
{
	VkPipeline			pipeline;
	VkPipelineLayout	layout;
};

struct VulkanSampler
{
	VkSampler sampler;
};

struct VulkanSemaphore
{
	VkSemaphore		semaphore;
	VkSemaphoreType type;
	uint32			value;
};

struct ZombieObject
{
	uint32			location;
	RHIObjTypeID	type;
};

struct CommandArena
{
	struct BarrierInformation
	{
		template <typename T>
		using Batch = std::array<T, MAX_COMMAND_BUFFER_PIPELINE_BARRIER_BATCH_SIZE>;

		Batch<VkMemoryBarrier2>			memoryBarriers;
		Batch<VkBufferMemoryBarrier2>	bufferMemoryBarriers;
		Batch<VkImageMemoryBarrier2>	imageMemoryBarriers;
		uint32							memoryBarrierCount;
		uint32							bufferBarrierCount;
		uint32							imageBarrierCount;

		void emplace_memory_barrier(VkMemoryBarrier2&& barrier)
		{
			memoryBarriers[memoryBarrierCount++] = std::move(barrier);
		}

		void emplace_buffer_memory_barrier(VkBufferMemoryBarrier2&& barrier)
		{
			bufferMemoryBarriers[bufferBarrierCount++] = std::move(barrier);
		}

		void emplace_image_memory_barrier(VkImageMemoryBarrier2&& barrier)
		{
			imageMemoryBarriers[imageBarrierCount++] = std::move(barrier);
		}

		/*void flush_barriers()
		{
			memoryBarrierCount	= 0u;
			bufferBarrierCount	= 0u;
			imageBarrierCount	= 0u;
		}*/
	};

	struct CommandBufferStore
	{
		std::array<VkCommandBuffer, MAX_COMMAND_BUFFER_PER_POOL>	commandBufferHandles;
		std::array<BarrierInformation, MAX_COMMAND_BUFFER_PER_POOL> pipelineBarrierInfos;
	};

	struct CommandBufferContext
	{
		VkCommandBuffer		handle		= VK_NULL_HANDLE;
		BarrierInformation* barrierInfo = nullptr;
	};

	VulkanQueue*		pQueue;			
	VkCommandPool		commandPool;
	CommandBufferStore	store;
	size_t				allocatedCount;

	/*CommandBufferContext get_command_buffer_context_at(size_t index)
	{
		CommandBufferContext ctx{};
		if (index < allocatedCount && index < MAX_COMMAND_BUFFER_PER_POOL)
		{
			ctx = {
				.handle			= store.commandBufferHandles[index],
				.barrierInfo	= &store.pipelineBarrierInfos[index]
			};
		}
		return ctx;
	}*/
};

struct VulkanRenderAttachments
{
	using ColorAttachments = std::array<VkRenderingAttachmentInfo, MAX_PIPELINE_COLOR_ATTACHMENT_COUNT>;

	ColorAttachments			colorAttachments;
	uint32						colorAttachmentCount;
	VkRenderingAttachmentInfo	depthAttachment;
	VkRenderingAttachmentInfo	stencilAttachment;
};

struct DeviceResourceCache
{
	struct Index
	{
		union
		{
			struct
			{
				uint16 page;
				uint16 offset;
			} metadata;
			uint32 id;
		};
		Index() = default;
		Index(uint32 id) :
			id{ id }
		{}
		Index(uint16 page, uint16 offset) :
			metadata{ page, offset }
		{}
	};

	template <typename T, size_t PAGE_SIZE = 64>
	class ResourceStore
	{
	private:
		struct Resource
		{
			T storage;
			bool valid;
		};

		using Pool		= ftl::StaticArray<Resource, PAGE_SIZE>;
		using Pages		= ftl::Array<ftl::UniquePtr<Pool>, ftl::SystemMemory, ftl::PaddedGrowthPolicy<1>>;
		using Indices	= ftl::Array<Index>;

		Pages	m_pages;
		Indices m_indices;
		size_t	m_current = 0ull;
	public:

		ResourceStore() = default;
		~ResourceStore() = default;

		std::pair<uint32, T&> request_resource()
		{
			if (m_current == m_pages.capacity() ||
				m_pages[m_current]->length() == m_pages[m_current]->size())
			{
				m_current = m_pages.emplace(ftl::make_unique<Pool>());
			}

			Resource* resource = nullptr;
			uint32 id = 0;

			if (m_indices.size())
			{
				Index index = m_indices.back();
				m_indices.pop();
				resource = &m_pages[(size_t)index.metadata.page]->at((size_t)index.metadata.offset);
				id = index.id;
			}
			else
			{
				size_t pos = m_pages[m_current]->emplace(Resource{});
				resource = &m_pages[m_current]->at(pos);

				Index index{ (uint16)m_current, (uint16)pos };
				id = index.id;
			}

			resource->valid = true;

			return std::pair<uint32, T&>{ id, resource->storage };
		}

		/**
		* \brief
		* Request for an empty and unused resource from the cache.
		*/
		template <typename... Arguments>
		std::pair<uint32, T&> request_resource(Arguments&&... args)
		{
			auto result = request_resource();
			new (&result.second) T{ std::forward<Arguments>(args)... };
			return result;
		}
		
		/**
		* \brief
		* Retrieves resource at the provided location within the cache.
		*/
		std::pair<bool, T&> fetch_resource(uint32 id)
		{
			const Index index{ id };
			const size_t page = (size_t)index.metadata.page;
			const size_t slot = (size_t)index.metadata.offset;

			Resource& resource = m_pages[page]->at(slot);
			std::pair<bool, T&> result{ resource.valid, resource.storage };

			return result;
		}

		const std::pair<bool, T&> fetch_resource(uint32 id) const
		{
			return fetch_resource(id);
		}

		void return_resource(uint32 id)
		{
			Index index{ id };
			m_pages[(size_t)index.metadata.page]->at((size_t)index.metadata.offset).valid = false;
			m_indices.emplace(id);
		}
	};

	DeviceResourceCache() = default;
	~DeviceResourceCache() = default;

	ResourceStore<VulkanSurface, 4>				surfaces;
	ResourceStore<VulkanSwapchain, 8>			swapchains;
	ResourceStore<VulkanBuffer>					buffers;
	ResourceStore<VulkanImage>					images;
	ResourceStore<VulkanShader>					shaders;
	ResourceStore<VulkanPipeline>				pipelines;
	ResourceStore<VulkanSampler>				samplers;
	ResourceStore<VulkanSemaphore, 8>			semaphores;
	ResourceStore<CommandArena, 32>				commandArena;
	ResourceStore< VulkanRenderAttachments>		renderingAttachments;
	ftl::Array<ZombieObject>					zombies;
	VkDescriptorPool							descriptorPool;
	VkDescriptorSetLayout						descriptorSetLayout;
	VkDescriptorSet								descriptorSet;
	ftl::Map<uint32, VkPipelineLayout>			pipelineLayouts;
};

struct DeviceContext
{
	VkInstance					instance		= {};
	VkPhysicalDevice			gpu				= {};
	VkDevice					device			= {};
	VkPhysicalDeviceProperties	properties		= {};
	VkPhysicalDeviceFeatures	features		= {};
	VmaAllocator				allocator		= {};
	VkDebugUtilsMessengerEXT	debugger		= {};
	VulkanQueue					mainQueue		= {};
	VulkanQueue					transferQueue	= {};
	VulkanQueue					computeQueue	= {};
	DeviceInitInfo				init			= {};
	DeviceConfig				config			= {};
	DeviceResourceCache			cache			= {};
	bool						validation		= {};

	//DeviceContext() = default;
	//~DeviceContext() = default;

	void					clear_zombies							();
	bool					create_descriptor_pool					();
	bool					create_descriptor_set_layout			();
	bool					allocate_descriptor_set					();
	bool					create_pipeline_layouts					();
	VkPipelineLayout		get_appropriate_pipeline_layout			(uint32 pushConstantSize, uint32 const max);
	bool					initialize_resource_cache				();
	void					cleanup_resource_cache					();
	bool					create_vulkan_instance					(DeviceInitInfo const& info);
	bool					create_debug_messenger					(DeviceInitInfo const& info);
	bool					choose_physical_device					();
	void					get_device_queue_family_indices			();
	bool					create_logical_device					();
	void					get_queue_handles						();
	bool					create_device_allocator					();
	void					destroy_surface							(uint32 location);
	void					destroy_swapchain						(uint32 location);
	void					destroy_image							(uint32 location);
	void					destroy_semaphore						(uint32 location);
	void					destroy_pipeline						(uint32 location);
	void					release_buffer							(uint32 location);
	void					destroy_command_arena					(uint32 location);
	VkImageUsageFlags		translate_image_usage_flags				(ImageUsage flags) const;
	VkImageAspectFlags		translate_image_aspect_flags			(ImageAspect flags) const;
	VkImageLayout			translate_image_layout					(ImageLayout layout) const;
	VkImageType				translate_image_type					(ImageType type) const;
	VkImageViewType			translate_image_view_type				(ImageType type) const;
	VkImageTiling			translate_image_tiling					(ImageTiling tiling) const;
	VkFormat				translate_image_format					(ImageFormat format) const;
	VkAttachmentLoadOp		translate_attachment_load_op			(AttachmentLoadOp loadOp) const;
	VkAttachmentStoreOp		translate_attachment_store_op			(AttachmentStoreOp storeOp) const;
	VmaMemoryUsage			translate_memory_usage_flag				(MemoryLocality locality) const;
	VkBufferUsageFlags		translate_buffer_usage_flags			(BufferUsage flags) const;
	VkSampleCountFlagBits	translate_sample_count					(SampleCount samples) const;
	VkPrimitiveTopology		translate_topology						(TopologyType topology) const;
	VkPolygonMode			translate_polygon_mode					(PolygonMode mode) const;
	VkCullModeFlags			translate_cull_mode						(CullingMode mode) const;
	VkFrontFace				translate_front_face_dir				(FrontFace face) const;
	VkBlendFactor			translate_blend_factor					(BlendFactor factor) const;
	VkBlendOp				translate_blend_op						(BlendOp op) const;
	VkFilter				translate_texel_filter					(TexelFilter filter) const;
	VkPresentModeKHR		translate_swapchain_presentation_mode	(SwapchainPresentMode mode) const;
	VkShaderStageFlagBits	translate_shader_stage					(ShaderType stage) const;
	VkFormat				translate_shader_attrib_format			(ShaderAttribute::Format format) const;
	VkCompareOp				translate_compare_op					(CompareOp op) const;
	VkPipelineStageFlags2	translate_pipeline_stage_flags			(PipelineStage stages) const;
	VkAccessFlags2			translate_memory_access_flags			(MemoryAccessType accesses) const;
	uint32					clamp_swapchain_image_count				(const uint32 current, const VkSurfaceCapabilitiesKHR& capability) const;
	VkExtent2D				get_swapchain_extent					(const VkSurfaceCapabilitiesKHR&, uint32, uint32) const;
};

}

#endif // !RENDERER_RHI_VULKAN_VK_DEVICE_H

