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
//#define MAX_UNIFORM_BUFFER_PAGE_SIZE	KILOBYTES(4)

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
		uint32				NumOutputs;
		bool				HasDepthStencil;
	};

	struct HwCmdBufferUnrecordInfo
	{
		Handle<HFramePass>	FramePassHandle;
		bool				UnbindRenderpass;
		bool				Unrecord;
	};

	struct HwBindVboEboInfo
	{
		Handle<HFramePass> FramePassHandle;
		Handle<HBuffer> Vbo;
		Handle<HBuffer> Ebo;
	};

	struct HwDrawInfo
	{
		Handle<HFramePass> FramePassHandle;
		uint32 VertexOffset;
		uint32 IndexOffset;
		uint32 VertexCount;
		uint32 IndexCount;
	};

	struct HwBlitInfo
	{
		Handle<HFramePass>	FramePassHandle;
		Handle<HImage>		Source;
		Handle<HImage>		Destination;
	};

	struct HwBufferCreateInfo
	{
		Handle<HBuffer>*	Handle;
		void*				Data;
		size_t				Count;
		size_t				Size;
		EBufferType			Type;
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
		ETextureUsage		Usage;
		ETextureType		Type;
		ImageUsageBits		UsageFlagBits;
		Handle<HImage>*		Handle;
	};

	struct HwFramebufferCreateInfo
	{
		HwAttachmentInfo		Outputs[MAX_FRAMEBUFFER_OUTPUTS];
		Handle<HImage>			DefaultOutputs[Default_Output_Max];
		Handle<HFramePass>		Handle;
		ESampleCount			Samples;
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
		BitSet<uint32>		Order;
		ESampleCount		Samples;
		uint32				NumColorOutputs;
		bool				HasDepthStencilAttachment;
	};

	struct HwPipelineCreateInfo
	{

		struct ShaderInfo
		{
			Handle<HShader> Handle;
			EShaderType		Type;
			ShaderAttrib*	Attributes;
			size_t			AttributeCount;
		};

		using ShaderArr = StaticArray<ShaderInfo, 4>;
		using DescLayoutArr = StaticArray<Handle<HDescLayout>, 16>;

		ShaderArr			Shaders;
		DescLayoutArr		DescriptorLayouts;
		ETopologyType		Topology;
		EPolygonMode		PolyMode;
		EFrontFaceDir		FrontFace;
		ECullingMode		CullMode;
		Handle<HFramePass>	FramePassHandle;
		Handle<HPipeline>*	Handle;
		uint32				VertexStride;
		uint32				NumColorAttachments;
		bool				HasDepth;
		bool				HasStencil;

		/**
		* TODO(Ygsm):
		* 1. Figure out color blending.
		* 2. Figure out a way to dynamically create the pipeline's layout.
		*/
	};

	struct HwDescriptorSetLayoutCreateInfo
	{
		uint32					Binding;
		EDescriptorType			Type;
		BitSet<uint32>			ShaderStages;
		Handle<HDescLayout>*	Handle;
	};

	struct HwDescriptorPoolCreateInfo
	{
		uint32				Type;
		uint32				NumDescriptorsOfType;
		Handle<HDescPool>*	Handle;
	};

	struct HwDescriptorSetAllocateInfo
	{
		Handle<HDescPool>	Pool;
		Handle<HDescLayout> Layout;
		Handle<HDescSet>*	Handle;
		uint32				Size;
		uint32				Count;
	};

	struct HwDescriptorSetMapInfo
	{
		Handle<HDescSet>	DescriptorSetHandle;
		Handle<HBuffer>		BufferHandle;
		uint32				Binding;
		uint32				Offset;
		uint32				Range;
		EDescriptorType		DescriptorType;
	};

	struct HwDescriptorSetBindInfo
	{
		Handle<HFramePass>	FramePassHandle;
		Handle<HPipeline>	PipelineHandle;
		Handle<HDescSet>	DescriptorSetHandle;
		EDescriptorType		DescriptorType;
		uint32				Offset;
	};

	struct HwDescriptorPoolFlushInfo
	{
		Handle<HDescPool>	Handle;
		uint32				Frame;
	};

	struct HwImageCreateStruct : public HwAttachmentInfo
	{
		uint32			Width;
		uint32			Height;
		uint32			Depth;
		uint32			MipLevels;
		ESampleCount	Samples;
	};

	struct HwDataToDescriptorSetMapInfo
	{
		Handle<HDescSet> DescriptorSetHandle;
		Handle<HBuffer> BufferHandle;
		void*	Data;
		uint32	Size;
		uint32	Offset;
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
	* Buffer page for uniform buffers.
	*/
	//class BufferPage
	//{
	//private:

	//	VkBuffer		Buffer;
	//	VmaAllocation	Allocation;
	//	size_t			Offset;
	//	BufferPage*		Next;

	//	static size_t	_BufferPageSize;

	//public:

	//	BufferPage();
	//	~BufferPage();

	//	DELETE_COPY_AND_MOVE(BufferPage)

	//	void InitializePage();
	//	void MapMemory(size_t Offset, void* Data);
	//	void AllocateOntoBuffer(uint32 Size);
	//};

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

		VkPhysicalDeviceProperties GPUProperties;

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

		bool CreateDescriptorPool	(HwDescriptorPoolCreateInfo& CreateInfo);
		void DestroyDescriptorPool	(Handle<HDescPool>& Hnd);

		bool CreateDescriptorSetLayout	(HwDescriptorSetLayoutCreateInfo& CreateInfo);
		void DestroyDescriptorSetLayout	(Handle<HDescLayout>& Hnd);

		bool CreatePipeline			(HwPipelineCreateInfo& CreateInfo);
		void DestroyPipeline		(Handle<HPipeline>& Hnd);
		void ReleasePipeline		(Handle<HPipeline>& Hnd);

		bool CreateFramebuffer		(HwFramebufferCreateInfo& CreateInfo);
		void DestroyFramebuffer		(Handle<HFramePass>& Hnd);

		bool CreateRenderPass		(HwRenderpassCreateInfo& CreateInfo);
		void DestroyRenderPass		(Handle<HFramePass>& Hnd);

		void ReleaseFramePass		(Handle<HFramePass>& Hnd);

		/**
		* Only adds a command buffer entry into the driver.
		* Does not allocate it from the command pool yet.
		*/
		//bool AddCommandBufferEntry	(HwCmdBufferAllocInfo& AllocateInfo);
		void RecordCommandBuffer	(HwCmdBufferRecordInfo& RecordInfo);
		void UnrecordCommandBuffer	(HwCmdBufferUnrecordInfo& UnrecordInfo);
		void BindVboAndEbo			(HwBindVboEboInfo& BindInfo);
		void Draw					(HwDrawInfo& DrawInfo);
		void BlitImage				(HwBlitInfo& BlitInfo);
		void BlitImageToSwapchain	(HwBlitInfo& BlitInfo);
		//void FreeCommandBuffer		(Handle<HCmdBuffer>& Hnd);

		//void PresentImageOnScreen	(Handle<HImage>& Hnd);

		void PushCmdBufferForSubmit	(Handle<HFramePass>& Hnd);

		bool AllocateDescriptorSet	(HwDescriptorSetAllocateInfo& AllocInfo);
		void MapDescSetToBuffer		(HwDescriptorSetMapInfo& MapInfo);
		void BindDescriptorSets		(HwDescriptorSetBindInfo& BindInfo);

		void MapDataToDescriptorSet	(HwDataToDescriptorSetMapInfo& MapInfo);

		//void FreeDescriptorSetBuffer(Handle<HDescSet>& Hnd);
		//void FlushDescriptorPool	(HwDescriptorPoolFlushInfo& FlushInfo);

		size_t PadDataSizeForUniform(size_t Size);
	};

}

#endif // !LEARNVK_RENDERER_API_VK_VULKAN_LOADER