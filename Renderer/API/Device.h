#pragma once
#ifndef ANGKASA1_RENDERER_API_DEVICE_H
#define ANGKASA1_RENDERER_API_DEVICE_H

#include "Engine/Interface.h"
#include "Common.h"
#include "Definitions.h"

// TODO:
// Move contents of vulkan driver to this file and purge VulkanDriver.h

class IRenderDevice
{
public:

	enum ESemaphoreType : uint32
	{
		Semaphore_Type_Image_Available = 0,
		Semaphore_Type_Render_Complete = 1,
		Semaphore_Type_Max = 2
	};

	enum class EHandleType : uint32
	{
		Handle_Type_Command_Buffer,
		Handle_Type_Framebuffer,
		Handle_Type_Renderpass,
		Handle_Type_Image,
		Handle_Type_Image_View,
		Handle_Type_Buffer,
		Handle_Type_Descriptor_Pool,
		Handle_Type_Descriptor_Set_Layout,
		Handle_Type_Descriptor_Set
	};

	struct VulkanFramebuffer
	{
		VkFramebuffer Hnd[MAX_SWAPCHAIN_IMAGE_ALLOWED];
	};

	struct VulkanQueue
	{
		VkQueue Hnd;
		uint32 FamilyIndex;
	};

	struct VulkanSwapchain
	{
		VkSwapchainKHR Hnd;
		VkSurfaceFormatKHR Format;
		Array<VkImage> Images;
		Array<VkImageView> ImageViews;
		uint32 NumOfImages;
		VkExtent2D Extent;
	};

	struct VulkanTransfer
	{
		VkFence Fence;
		VkCommandPool CommandPool;
		VkCommandBuffer TransferCmdBuffer;
		VkCommandBuffer GraphicsCmdBuffer;
		VulkanQueue Queue;
		VkSemaphore Semaphore;
	};

	IRenderDevice();
	~IRenderDevice();

	bool Initialize(const EngineImpl& Engine);
	void Terminate();

	bool CreateSwapchain(uint32 Width, uint32 Height);

	/**
	* Default framebuffer will always take it's width and height from the swapchain.
	*/
	bool CreateDefaultFramebuffer();
	void DestroyDefaultFramebuffer();

	void BeginFrame();
	bool RenderThisFrame() const;
	void EndFrame();

	void DeviceWaitIdle();

	VkCommandPool CreateCommandPool(uint32 QueueFamilyIndex, VkCommandPoolCreateFlags Flags);
	void DestroyCommandPool(VkCommandPool Hnd);
	VkCommandBuffer AllocateCommandBuffer(VkCommandPool PoolHnd, VkCommandBufferLevel Level, uint32 Count);
	void ResetCommandBuffer(VkCommandBuffer Hnd, VkCommandBufferResetFlags Flag);
	VkFence CreateFence(VkFenceCreateFlags Flag = VK_FENCE_CREATE_SIGNALED_BIT);
	const VulkanSwapchain& GetSwapchain() const;
	VkImage GetNextImageInSwapchain() const;
	void WaitFence(VkFence Hnd, uint64 Timeout = UINT64_MAX);
	void ResetFence(VkFence Hnd);
	void DestroyFence(VkFence Hnd);
	VkSemaphore CreateVkSemaphore(VkSemaphoreTypeCreateInfo* pSemaphoreType); // Can't do CreateSemaphore because Windows macro-ed the function signature.
	void DestroyVkSemaphore(VkSemaphore Hnd);

	void BufferBarrier(VkCommandBuffer Cmd, VkBuffer Hnd, size_t Size, size_t Offset, VkAccessFlags SrcAccessMask, VkAccessFlags DstAccessMask, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, uint32 SrcQueue, uint32 DstQueue);
	void ImageBarrier(VkCommandBuffer Cmd, VkImage Hnd, VkImageSubresourceRange* pSubRange, VkImageLayout OldLayout, VkImageLayout NewLayout, VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, uint32 SrcQueue, uint32 DstQueue);

	void WaitTimelineSempahore(VkSemaphore Hnd, uint64 Value, uint64 Timeout = UINT64_MAX);
	void SignalTimelineSemaphore(VkSemaphore Hnd, uint64 Value);
	
	void BeginCommandBuffer(VkCommandBuffer Hnd, VkCommandBufferUsageFlags Flag);
	void EndCommandBuffer(VkCommandBuffer Hnd);

	bool ToSpirV(const String& Code, Array<uint32>& SpirV, uint32 ShaderType);

	/**
	* Returns the current frame's command buffer.
	* Extendable to multi-threaded command buffer recording.
	*/
	const VkCommandBuffer& GetCommandBuffer() const;

	VmaAllocator& GetAllocator();

	VkDevice GetDevice() const;
	const VulkanQueue& GetTransferQueue() const;
	const VulkanQueue& GetGraphicsQueue() const;
	const VulkanQueue& GetPresentationQueue() const;

	VkImageUsageFlags GetImageUsage(uint32 Index) const;
	VkSampleCountFlagBits GetSampleCount(uint32 Index) const;
	VkPipelineBindPoint GetPipelineBindPoint(uint32 Index) const;
	VkImageType GetImageType(uint32 Index) const;
	VkImageViewType GetImageViewType(uint32 Index) const;
	VkDescriptorType GetDescriptorType(uint32 Index) const;
	VkBufferUsageFlagBits GetBufferUsage(uint32 Index) const;
	VkVertexInputRate GetVertexInputRate(uint32 Index) const;
	VkPrimitiveTopology GetPrimitiveTopology(uint32 Index) const;
	VkPolygonMode GetPolygonMode(uint32 Index) const;
	VkFrontFace GetFrontFaceMode(uint32 Index) const;
	VkCullModeFlags GetCullMode(uint32 Index) const;
	VmaMemoryUsage GetMemoryUsage(uint32 Index) const;

	const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const;

	/**
	* Current frame's index.
	*/
	const uint32 GetCurrentFrameIndex() const;
	const uint32 GetNextSwapchainImageIndex() const;

private:

	//IRenderDevice() = delete;
	//~IRenderDevice() = delete;
	DELETE_COPY_AND_MOVE(IRenderDevice)

	EngineImpl* Engine;
	OS::DllHandle Dll;
	VkInstance Instance;
	VkPhysicalDevice Gpu;
	VkDevice Device;
	VkSurfaceKHR Surface;
	VulkanSwapchain Swapchain;
	VulkanQueue GraphicsQueue;
	VulkanQueue PresentQueue;
	VulkanQueue TransferQueue;
	//VulkanTransfer TransferOp;
	VulkanFramebuffer DefaultFramebuffer;
	VkRenderPass DefaultRenderPass;
	VkSemaphore Semaphores[MAX_FRAMES_IN_FLIGHT][Semaphore_Type_Max];
	VkFence Fences[MAX_FRAMES_IN_FLIGHT];
	VkFence ImageFences[MAX_SWAPCHAIN_IMAGE_ALLOWED];
	VkPhysicalDeviceProperties Properties;
	VmaAllocator Allocator;
	VkCommandPool CommandPool;
	VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT];
	VkDebugUtilsMessengerEXT DebugMessenger;
	uint32 NextSwapchainImageIndex;
	uint32 CurrentFrameIndex;
	bool RenderFrame;

	bool LoadVulkanLibrary();
	bool LoadVulkanModuleAndGlobalFunc();
	bool CreateVulkanInstance();
	bool LoadVulkanFunctions();
	bool CreateDebugMessenger();
	bool ChoosePhysicalDevice();
	bool CreateLogicalDevice();
	bool LoadVulkanDeviceFunction();
	bool CreatePresentationSurface(WndHandle Hwnd);
	void GetDeviceQueues();
	bool CreateSyncObjects();
	bool CreateDefaultRenderpass();
	bool CreateAllocator();
	//bool CreateTransferOperation();
	bool CreateCommandPool();
	bool AllocateCommandBuffers();
	void FreeVulkanLibrary();

};

struct IAllocator;
struct SImage;
struct SRenderPass;
struct SPipeline;
struct SMemoryBuffer;
struct SDrawCommand;
struct SShader;
struct SDescriptorPool;
struct SDescriptorSetLayout;
struct SDescriptorSet;
struct SImageSampler;
//struct SPushConstant;

struct IDeviceStore
{
	IAllocator& Allocator;
	Map<size_t, SMemoryBuffer*> Buffers;
	Map<size_t, SDescriptorSet*> DescriptorSets;
	Map<size_t, SDescriptorPool*> DescriptorPools;
	Map<size_t, SDescriptorSetLayout*> DescriptorSetLayouts;
	Map<size_t, SRenderPass*> RenderPasses;
	Map<size_t, SImageSampler*> ImageSamplers;
	Map<size_t, SPipeline*> Pipelines;
	Map<size_t, SShader*> Shaders;
	Map<size_t, SImage*> Images;
	//Map<size_t, SPushConstant*> PushConstants;

	IDeviceStore(IAllocator& InAllocator);
	~IDeviceStore();

	DELETE_COPY_AND_MOVE(IDeviceStore)

	bool DoesBufferExist(size_t Id);
	bool DoesDescriptorSetExist(size_t Id);
	bool DoesDescriptorPoolExist(size_t Id);
	bool DoesDescriptorSetLayoutExist(size_t Id);
	bool DoesRenderPassExist(size_t Id);
	bool DoesImageSamplerExist(size_t Id);
	bool DoesPipelineExist(size_t Id);
	bool DoesShaderExist(size_t Id);
	bool DoesImageExist(size_t Id);

	SMemoryBuffer* NewBuffer(size_t Id);
	SMemoryBuffer* GetBuffer(size_t Id);
	bool DeleteBuffer(size_t Id, bool Free = false);

	SDescriptorSet* NewDescriptorSet(size_t Id);
	SDescriptorSet* GetDescriptorSet(size_t Id);
	bool DeleteDescriptorSet(size_t Id, bool Free = false);

	SDescriptorPool* NewDescriptorPool(size_t Id);
	SDescriptorPool* GetDescriptorPool(size_t Id);
	bool DeleteDescriptorPool(size_t Id, bool Free = false);

	SDescriptorSetLayout* NewDescriptorSetLayout(size_t Id);
	SDescriptorSetLayout* GetDescriptorSetLayout(size_t Id);
	bool DeleteDescriptorSetLayout(size_t Id, bool Free = false);

	SRenderPass* NewRenderPass(size_t Id);
	SRenderPass* GetRenderPass(size_t Id);
	bool DeleteRenderPass(size_t Id, bool Free = false);

	SImageSampler* NewImageSampler(size_t Id);
	SImageSampler* GetImageSampler(size_t Id);
	bool DeleteImageSampler(size_t Id, bool Free = false);

	SPipeline* NewPipeline(size_t Id);
	SPipeline* GetPipeline(size_t Id);
	bool DeletePipeline(size_t Id, bool Free = false);

	SShader* NewShader(size_t Id);
	SShader* GetShader(size_t Id);
	bool DeleteShader(size_t Id, bool Free = false);

	SImage* NewImage(size_t Id);
	SImage* GetImage(size_t Id);
	bool DeleteImage(size_t Id, bool Free = false);
};

#endif // !ANGKASA1_RENDERER_API_DEVICE_H