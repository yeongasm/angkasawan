#pragma once
#ifndef LEARNVK_RENDERER_API_VK_VULKAN_LOADER
#define LEARNVK_RENDERER_API_VK_VULKAN_LOADER

#include "Vk.h"
#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"
#include "Library/Containers/Map.h"
#include "Library/Containers/Deque.h"
#include "Library/Containers/Path.h"
#include "Library/Containers/Bitset.h"
#include "Assets/GPUHandles.h"
#include "API/RendererFlagBits.h"
#include "API/ShaderAttribute.h"

namespace vk
{

#define MAX_SWAPCHAIN_IMAGE_ALLOWED		3
#define MAX_FRAMES_IN_FLIGHT			2
#define MAX_FRAMEBUFFER_OUTPUTS			16
#define MAX_BUFFER_VERTEX_DATA			1 << 20
#define MAX_BUFFER_INSTANCE_DATA		1 << 24

	using BinaryBuffer = Array<uint8>;
	using DWordBuffer  = Array<uint32>;
	using ImageUsageBits = uint32;

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

	enum DefaultOutputs : uint32
	{
		Default_Output_Color		= 0,
		Default_Output_DepthStencil = 1,
		Default_Output_Max = 2
	};

	struct HwCmdBufferRecordInfo
	{
		Handle<HPipeline>	PipelineHandle;
		Handle<HFramePass>	FramePassHandle;
		float32				ClearColor[Color_Channel_Max];
		int32				SurfaceOffset[Surface_Offset_Max];
		uint32				SurfaceExtent[Surface_Extent_Max];
	};

	struct HwDrawInfo
	{
		Handle<HFramePass>	FramePassHandle;
		Handle<HBuffer>		Vbo;
		Handle<HBuffer>		Ebo;
		uint32				VertexOffset;
		uint32				IndexOffset;
	};

	struct HwBufferCreateInfo
	{
		Handle<HBuffer>*	Handle;
		void*				Data;
		size_t				Count;
		size_t				Size;
		BufferType			Type;
	};

	struct HwShaderCreateInfo
	{
		DWordBuffer*		DWordBuf;
		Handle<HShader>*	Handle;
	};

	/**
	* NOTE(Ygsm):
	* We do not need to include input attachments in the framebuffer because it will not be included.
	* Texture attachments will be included into the pass via samplers.
	*/
	struct HwAttachmentInfo
	{
		TextureUsage		Usage;
		TextureType			Type;
		ImageUsageBits		UsageFlagBits;
		Handle<HImage>*		Handle;
	};

	struct HwFramebufferCreateInfo
	{
		HwAttachmentInfo		Outputs[MAX_FRAMEBUFFER_OUTPUTS];
		Handle<HImage>			DefaultOutputs[Default_Output_Max];
		Handle<HFramePass>		Handle;
		SampleCount				Samples;
		float32					Width;
		float32					Height;
		float32					Depth;
		uint32					NumOutputs;
		uint32					NumColorOutputs;
	};

	struct HwRenderpassCreateInfo
	{
		Handle<HFramePass>* Handle;
		BitSet<uint32>		DefaultOutputs;
		RenderPassOrder		Order;
		SampleCount			Samples;
		uint32				NumColorOutputs;
		bool				HasDepthStencilAttachment;
	};

	struct HwPipelineCreateInfo
	{
		struct ShaderInfo
		{
			Handle<HShader> Handle;
			ShaderType		Type;
			ShaderAttrib*	Attributes;
			size_t			AttributeCount;
		};

		Array<ShaderInfo>		Shaders;
		TopologyType			Topology;
		PolygonMode				PolyMode;
		FrontFaceDir			FrontFace;
		CullingMode				CullMode;
		Handle<HFramePass>		FramePassHandle;
		Handle<HPipeline>*		Handle;
		uint32					VertexStride;
		bool					HasDepthStencil;

		/**
		* TODO(Ygsm):
		* 1. Figure out color blending.
		* 2. Figure out a way to dynamically create the pipeline's layout.
		*/
	};

	struct HwImageCreateStruct : public HwAttachmentInfo
	{
		uint32			Width;
		uint32			Height;
		uint32			Depth;
		uint32			MipLevels;
		SampleCount		Samples;
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

	struct FramePassParams
	{
		VkFramebuffer	Framebuffers[MAX_SWAPCHAIN_IMAGE_ALLOWED];
		VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT];
		VkRenderPass	Renderpass;
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
		Handle<HFramePass>	DefaultFramebuffer;

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
		Array<VkCommandBuffer>	CommandBuffersForSubmit;
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

		bool CreateBuffer			(HwBufferCreateInfo& CreateInfo);
		void DestroyBuffer			(Handle<HBuffer>& Hnd);

		bool CreateShader			(HwShaderCreateInfo& CreateInfo);
		void DestroyShader			(Handle<HShader>& Hnd);

		bool CreatePipeline			(HwPipelineCreateInfo& CreateInfo);
		void DestroyPipeline		(Handle<HPipeline>& Hnd);

		/**
		* Only adds a command buffer entry into the driver.
		* Does not allocate it from the command pool yet.
		*/
		//bool AddCommandBufferEntry	(HwCmdBufferAllocInfo& AllocateInfo);
		void RecordCommandBuffer	(HwCmdBufferRecordInfo& RecordInfo);
		void UnrecordCommandBuffer	(HwCmdBufferRecordInfo& RecordInfo);
		void Draw					(HwDrawInfo& DrawInfo);
		//void FreeCommandBuffer		(Handle<HCmdBuffer>& Hnd);

		//void PresentImageOnScreen	(Handle<HImage>& Hnd);

		bool CreateFramebuffer		(HwFramebufferCreateInfo& CreateInfo);
		void DestroyFramebuffer		(Handle<HFramePass>& Hnd);

		bool CreateRenderPass		(HwRenderpassCreateInfo& CreateInfo);
		void DestroyRenderPass		(Handle<HFramePass>& Hnd);

		void PushCmdBufferForSubmit	(Handle<HFramePass>& Hnd);
	};

}

#endif // !LEARNVK_RENDERER_API_VK_VULKAN_LOADER