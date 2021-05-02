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

	bool Initialize(const EngineImpl& Engine);
	void Terminate();

	bool CreateSwapchain(uint32 Width, uint32 Height);

	/**
	* Default framebuffer will always take it's width and height from the swapchain.
	*/
	bool CreateDefaultFramebuffer();
	void DestroyDefaultFramebuffer();

	void BeginFrame();
	void EndFrame();

	/**
	* Returns the current frame's command buffer.
	* Extendable to multi-threaded command buffer recording.
	*/
	VkCommandBuffer GetCommandBuffer() const;

	VmaAllocator& GetAllocator();

	VkDevice GetDevice() const;

	VkDescriptorType GetDescriptorType(uint32 Index);

	/**
	* Current frame's index.
	*/
	const uint32 GetCurrentFrameIndex() const;

private:

	IRenderDevice() = delete;
	~IRenderDevice() = delete;
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
	VulkanTransfer TransferOp;
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
	bool CreateTransferOperation();
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
struct SPushConstant;

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
	Map<size_t, SPushConstant*> PushConstants;

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