#pragma once
#ifndef RENDERER_RHI_DEVICE_H
#define RENDERER_RHI_DEVICE_H

#include "containers/array.h"
#include "command_buffer.h"

namespace rhi
{

struct DeviceContext;

/**
* A thin abstraction wrapper around Vulkan's device queues and DX12's command queue.
* We use Vulkan terminologies because we're Vulkan-first.
*/
/*class DeviceQueue
{
public:
	struct Properties
	{
		struct Capabilities
		{
			bool graphics			: 1;
			bool compute			: 1;
			bool transfer			: 1;
			bool sparseBinding		: 1;
			bool protectedMemory	: 1;
		} capabilities;

		uint32		queueCount;
		uint32		timestampValidBits;
		Extent3D	minImageTransferGranularity;
	};
private:
	friend class RenderDevice;
	friend struct command::CommandBuffer;

	Properties							properties;
	DeviceContext*						pContext;
	DeviceQueueType						type;
	RHIObjPtr							data;
	ftl::Array<command::CommandPool*>	commandPools;	// We track every command pool that is created with this queue.
public:

	DeviceQueue()	= default;
	~DeviceQueue()	= default;

	DeviceQueueType	get_type			() const;
	bool			create_command_pool	(command::CommandPool& commandPool);
	void			destroy_command_pool(command::CommandPool& commandPool);
	void			wait_idle			() const;
	bool			submit				() const;
};*/

class RenderDevice
{
private:
	/*struct BinarySemaphorePool
	{
		static constexpr size_t POOL_SIZE = 16;

		struct SemaphoreReference
		{
			size_t page;
			size_t offset;
			size_t count;
		};
		using SemaphoreReferences = ftl::Array<SemaphoreReference>;

		struct SemaphorePool
		{
			std::array<Semaphore, POOL_SIZE> pool;
			size_t current;
		};
		using SemaphorePages = ftl::Array<SemaphorePool>;

		SemaphorePages		pages;
		SemaphoreReferences	references;
		ftl::Array<size_t>	freeIndices;
	};*/

	DeviceContext*		m_context;
	/*DeviceQueue			m_mainQueue;*/
	/*BinarySemaphorePool	m_binarySemaphorePool;*/
	API const			m_api;
	ShaderLang const	m_shadingLanguage;

	/*bool				create_semaphore				(Semaphore& sempahore);
	void				destroy_semaphore				(Semaphore& sempahore);*/
public:
	RenderDevice	();
	RenderDevice	(API api, ShaderLang shadingLanguage);
	~RenderDevice	() = default;

	bool				initialize						(DeviceInitInfo const& info);
	void				terminate						();
	uint32				stride_for_shader_attrib_format	(ShaderAttribute::Format format) const;
	bool				is_img_format_color_format		(ImageFormat format) const;
	DeviceType			get_device_type					() const;
	DeviceInfo			get_device_info					() const;
	DeviceConfig const&	get_device_config				() const;
	API					get_api							() const;
	/*DeviceQueue&		get_main_queue					();*/
	bool				create_surface_win32			(Surface& surface);
	void				destroy_surface					(Surface& surface);
	bool				create_swapchain				(Swapchain& swapchain, Swapchain* const pOldSwapchain);
	void				destroy_swapchain				(Swapchain& swapchain);
	bool				update_next_swapchain_image		(Swapchain& swapchain, Semaphore& semaphore);
	bool				create_shader					(Shader& shader);
	void				destroy_shader					(Shader& shader);
	bool				create_raster_pipeline			(RasterPipeline& pipeline);
	void				destroy_raster_pipeline			(RasterPipeline& pipeline);
	bool				allocate_buffer					(Buffer& buffer);
	void				release_buffer					(Buffer& buffer);
	bool				create_image					(Image& image);
	void				destroy_image					(Image& image);
	void				cache_render_metadata			(command::RenderMetadata& metadata);
	void				uncache_render_metadata			(command::RenderMetadata& metadata);
	bool				create_command_pool				(command::CommandPool& commandPool, rhi::DeviceQueueType type = rhi::DeviceQueueType::Main);
	void				destroy_command_pool			(command::CommandPool& commandPool);
	void				clear_resources					();
	ShaderLang			get_shading_language			() const;
	/*uint32				request_semaphores				(uint32 count);
	void				return_semaphores				(uint32 index);
	void				release_semaphores				();*/
	bool				compile_shader					(ShaderCompileInfo const& compileInfo, ShaderInfo& shaderInfo, std::string* error = nullptr);
	void				wait_idle						() const;
};

DeviceConfig	device_default_configuration	();
bool			create_device					(RenderDevice& device, DeviceInitInfo const& info);
void			destroy_device					(RenderDevice& device);

}

#endif // !RENDERER_RHI_DEVICE_H

