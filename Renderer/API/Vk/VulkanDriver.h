#pragma once
#ifndef LEARNVK_RENDERER_API_VK_VULKAN_LOADER
#define LEARNVK_RENDERER_API_VK_VULKAN_LOADER

#include "Vk.h"
#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"
#include "Library/Containers/Map.h"
#include "Library/Containers/Deque.h"
#include "Library/Containers/Path.h"
#include "Assets/GPUHandles.h"
#include "API/RendererFlagBits.h"

namespace vk
{
#define MAX_FRAMES_IN_FLIGHT 2

	using BinaryBuffer = Array<uint8>;
	using DWordBuffer  = Array<uint32>;

	enum DriverQueueTypes : uint32
	{
		Queue_Type_Graphics = 0,
		Queue_Type_Present  = 1,
		Queue_Max = 2
	};


	enum DriverSemaphoreTypes : uint32
	{
		Semaphore_Type_ImageAvailable = 0,
		Semaphore_Type_RenderComplete = 1,
		Semaphore_Max = 2
	};

	struct HwCmdBufferAllocInfo
	{
		Handle<HCmdBuffer>* Handle;
		//CommandBufferLevel	Level;
	};

	struct HwCmdBufferRecordInfo
	{
		Handle<HCmdBuffer>*		CommandBufferHandle;
		Handle<HPipeline>*		PipelineHandle;
		Handle<HFramebuffer>	FramebufferHandle;
		float32					ClearColor[Color_Channel_Max];
		int32					SurfaceOffset[Surface_Offset_Max];
		uint32					SurfaceExtent[Surface_Extent_Max];
	};

	struct HwShaderCreateInfo
	{
		DWordBuffer*		DWordBuf;
		Handle<HShader>*	Handle;
	};


	struct HwAttachmentInfo
	{
		AttachmentType		Type;
		AttachmentUsage		Usage;
		AttachmentDimension Dimension;
		Handle<HImage>*		Handle;
	};

	struct HwFramebufferCreateInfo
	{
		float32					Width;
		float32					Height;
		float32					Depth;
		SampleCount				Samples;
		Array<HwAttachmentInfo> Attachments;
		uint32					NumColorAttachments;
		Handle<HFramebuffer>*	Handle;
	};

	struct HwPipelineCreateInfo
	{
		struct ShaderInfo
		{
			Handle<HShader> Handle;
			ShaderType		Type;
		};

		Array<ShaderInfo>		Shaders;
		TopologyType			Topology;
		PolygonMode				PolyMode;
		FrontFaceDir			FrontFace;
		CullingMode				CullMode;
		Handle<HFramebuffer>	FramebufferHandle;
		Handle<HPipeline>*		Handle;

		/**
		* TODO(Ygsm):
		* 1. Figure out color blending.
		* 2. Figure out a way to dynamically create the pipeline's layout.
		*/
	};

	/**
	* Queue parameters.
	*/
	struct QueueParams
	{
		VkQueue Handle;
		uint32	FamilyIndex;
	};

	/**
	* Swapchain parameters.
	*/
	struct SwapChainParams
	{
		VkSwapchainKHR		Handle;
		VkSurfaceFormatKHR	SurfaceFormat;
		Array<VkImage>		Images;
		Array<VkImageView>	ImageViews;
		VkExtent2D			Extent;
		uint32				NumImages;
	};

	/**
	* Default framebuffer.
	*/
	struct DefaultFramebuffer
	{
		VkFramebuffer	Framebuffers[MAX_FRAMES_IN_FLIGHT];
		VkCommandBuffer	CmdBuffers[MAX_FRAMES_IN_FLIGHT];
		VkRenderPass	Renderpass;
	};
	
	struct HwImageCreateStruct
	{
		uint32					Width;
		uint32					Height;
		uint32					Depth;
		uint32					MipLevels;
		VkFormat				Format;
		VkImageType				Dimension;
		VkSampleCountFlagBits	Samples;
		VkImageLayout			InitialLayout;
		VkImageUsageFlags		UsageFlags;
		Handle<HImage>*			Handle;
	};

	/**
	* Vulkan initialisation.
	*/
	struct VulkanCommon
	{
		VkInstance			Instance;
		VkPhysicalDevice	GPU;
		VkDevice			Device;
		VkSurfaceKHR		Surface;

		SwapChainParams		SwapChain;
		QueueParams			Queues[Queue_Max];
		DefaultFramebuffer	DefFramebuffer;

		VkSemaphore			Semaphores[MAX_FRAMES_IN_FLIGHT][Semaphore_Max];
		VkFence				Fence[MAX_FRAMES_IN_FLIGHT];
		Array<VkFence>		ImageFences;

		bool CreateVulkanInstance();
		bool ChoosePhysicalDevice();
		bool CreateLogicalDevice();
		bool CreatePresentationSurface();
		bool GetDeviceQueue();
		bool CreateSyncObjects();
		bool CreateSwapchain();

		bool CreateDefaultRenderPass();
		bool CreateDefaultFramebuffer();
	};

	/**
	* NOTE(Ygsm):
	* Currently implementing vma into the driver.
	* Creating image attachments for renderpasses.
	*/
	struct VulkanDriver : public VulkanCommon
	{
		VmaAllocator			Allocator;
		VkCommandPool			CommandPool;
		uint32					NextImageIndex;
		uint32					CurrentFrame;
		bool					RenderFrame;

		bool CreateVmAllocator();

		bool InitializeDriver();
		void TerminateDriver();

		/**
		* Temporary command buffer generation until we can create something that is more dynamic
		*/
		//void TempRecordCommandBuffers();

		/**
		* Releases the command buffers and command pool every frame.
		*/
		void Flush();

		/**
		* Called at the start of every frame.
		*/
		void Clear();

		/**
		* Recreates swap chain and command pool when window resizes.
		*/
		void OnWindowResize();

		/**
		* Submits the command buffer(s) into the queue and sends them to the presentation engine.
		*/
		bool SwapBuffers();

		uint32 FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties);

		/**
		* Creates an image and it's image view.
		*/
		bool CreateImage(HwImageCreateStruct& CreateInfo);

		/**
		* Releases the image and it's image view.
		*/
		void DestroyImage(Handle<HImage>& Hnd);

		bool CreateCommandPool();
		bool AllocateCommandBuffers();

		bool CreateShader			(HwShaderCreateInfo& CreateInfo);
		void DestroyShader			(Handle<HShader>& Hnd);

		bool CreatePipeline			(HwPipelineCreateInfo& CreateInfo);
		void DestroyPipeline		(Handle<HPipeline>& Hnd);

		/**
		* Only adds a command buffer entry into the driver.
		* Does not allocate it from the command pool yet.
		*/
		bool AddCommandBufferEntry	(HwCmdBufferAllocInfo& AllocateInfo);
		void RecordCommandBuffer	(HwCmdBufferRecordInfo& RecordInfo);
		void FreeCommandBuffer		(Handle<HCmdBuffer>& Hnd);

		/**
		* Does nothing yet ...
		*/
		bool CreateFramebuffer		(HwFramebufferCreateInfo& CreateInfo);
		/**
		* Does nothing yet ...
		*/
		void DestroyFramebuffer		(Handle<HFramebuffer>& Hnd);
	};
}

#endif // !LEARNVK_RENDERER_API_VK_VULKAN_LOADER