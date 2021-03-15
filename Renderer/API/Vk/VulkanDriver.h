#pragma once
#ifndef LEARNVK_RENDERER_API_VK_VULKAN_LOADER
#define LEARNVK_RENDERER_API_VK_VULKAN_LOADER

#include "Vk.h"
#include "API/Definitions.h"
#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"
#include "Library/Containers/Map.h"
#include "Library/Containers/Bitset.h"
#include "Assets/GPUHandles.h"
#include "API/RendererFlagBits.h"
#include "Engine/Interface.h"
#include "RenderAbstracts/Primitives.h"
#include "RenderAbstracts/FrameGraph.h"
#include "RenderAbstracts/DrawCommand.h"
#include "RenderAbstracts/DescriptorSets.h"
#include "RenderAbstracts/RenderMemory.h"
#include "RenderAbstracts/GPUMemory.h"

namespace gpu
{

	enum ESemaphoreType : uint32
	{
		Semaphore_Type_Image_Available = 0,
		Semaphore_Type_Render_Complete = 1,
		Semaphore_Type_Max = 2
	};

	struct VulkanQueue
	{
		VkQueue Handle;
		uint32 FamilyIndex;
	};

	struct VulkanSwapchain
	{
		VkSwapchainKHR Handle;
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
		VkCommandBuffer CommandBuffer;
		VulkanQueue Queue;
	};

	struct VulkanFramepass
	{
		VkFramebuffer Framebuffers[MAX_SWAPCHAIN_IMAGE_ALLOWED];
		//VkCommandBuffer CmdBuffers[MAX_FRAMES_IN_FLIGHT];
		VkRenderPass Renderpass;
	};

	struct VulkanImage
	{
		VkImage Handle;
		VkImageView View;
		VkSampler Sampler;
		VmaAllocation Allocation;
	};

	struct VulkanBuffer
	{
		VkBuffer Handle;
		VmaAllocation Allocation;
		uint8* pData;
	};

	struct VulkanPipeline
	{
		VkPipeline Handle;
		VkPipelineLayout Layout;
	};

	struct VulkanDescriptorPool
	{
		VkDescriptorPool Pool[MAX_FRAMES_IN_FLIGHT];
	};

	struct VulkanDescriptorSet
	{
		VkDescriptorSet Set[MAX_FRAMES_IN_FLIGHT];
	};

	struct VulkanContext
	{
		OS::DllHandle Dll;
		VkInstance Instance;
		VkPhysicalDevice Gpu;
		VkDevice Device;
		VkSurfaceKHR Surface;
		VulkanSwapchain Swapchain;
		VulkanQueue GraphicsQueue;
		VulkanQueue PresentQueue;
		VulkanTransfer TransferOp;
		VulkanFramepass DefaultPass;
		VkSemaphore Semaphores[MAX_FRAMES_IN_FLIGHT][Semaphore_Type_Max];
		VkFence Fences[MAX_FRAMES_IN_FLIGHT];
		VkFence ImageFences[MAX_SWAPCHAIN_IMAGE_ALLOWED];
		VkPhysicalDeviceProperties Properties;
		VmaAllocator Allocator;
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffers[MAX_FRAMES_IN_FLIGHT];
		uint32 NextSwapchainImageIndex;
		uint32 CurrentFrameIndex;
		bool RenderFrame;

#if RENDERER_DEBUG_RENDER_DEVICE
		VkDebugUtilsMessengerEXT DebugMessenger;
#endif
	};

	bool Initialize();
	void Terminate();
	void BlitToDefault(const IRFrameGraph& Graph);
	void BindRenderpass(RenderPass& Pass);
	void BindPipeline(RenderPass& Pass);
	void UnbindRenderpass(RenderPass& Pass);
	void BindVertexBuffer(const DrawCommand& Cmd);
	void BindIndexBuffer(const DrawCommand& Cmd);
	void Draw(const DrawCommand& Command);
	//bool CreateFrameImage(FrameImages& Images);
	//void DestroyFrameImage(FrameImages& Images);
	bool CreateShader(Shader& InShader, String& Code);
	void DestroyShader(Shader& InShader);
	bool CreateGraphicsPipeline(RenderPass& Pass);
	void DestroyGraphicsPipeline(RenderPass& Pass);
	bool CreateFramebuffer(RenderPass& Pass);
	void DestroyFramebuffer(RenderPass& Pass, bool Release = true);
	bool CreateRenderpass(RenderPass& Pass);
	void DestroyRenderpass(RenderPass& Pass);
	void CreateDescriptorPool(DescriptorPool& Pool);
	void DestroyDescriptorPool(DescriptorPool& Pool);
	bool CreateDescriptorSetLayout(DescriptorLayout& Layout);
	void DestroyDescriptorSetLayout(DescriptorLayout& Layout);
	bool AllocateDescriptorSet(DescriptorSet& Set);
	void UpdateDescriptorSet(DescriptorSet& Set, DescriptorBinding& Binding, SRMemoryBuffer& Buffer);
	//void UpdateDescriptorSet(DescriptorSet& Set, DescriptorBinding& Binding);
	void BindDescriptorSetInstance(DescriptorSetInstance& Instance);
	bool CreateTexture(Texture& InTexture);
	void DestroyTexture(Texture& InTexture);

	/**
	* Creates a new buffer.
	* Copies data to buffer if Data and Size is specified.
	*/
	bool CreateBuffer(SRMemoryBuffer& Buffer, void* Data = nullptr, size_t Size = 0);

	/**
	* Copies contents to the specified buffer.
	* Updates the offset in the buffer abstraction object.
	*/
	bool CopyToBuffer(SRMemoryBuffer& Buffer, void* Data, size_t Size);

	/**
	* Copies contents to the specified buffer.
	* Additionally, frees everything after the specified offset and updates the buffer abstraction object
	* internal offset to become the tail of the buffer after the copy.
	*/
	bool CopyToBuffer(SRMemoryBuffer& Buffer, void* Data, size_t Size, size_t Offset);
	void DestroyBuffer(SRMemoryBuffer& Buffer);

	bool BeginTransfer();
	void EndTransfer();
	void TransferBuffer(SRMemoryTransferContext& TransferContext);
	void TransferTexture(Texture& InTexture, SRMemoryBuffer& Buffer);

	void BeginFrame();
	void EndFrame();
	void Clear();
	void Flush();
	void SwapBuffers();
	void OnWindowResize();
	uint32 SwapchainWidth();
	uint32 SwapchainHeight();
	uint32 CurrentFrameIndex();
	uint32 MaxUniformBufferRange();
	size_t PadSizeToAlignedSize(size_t Size);

	namespace vk
	{
		Handle<HImage> CreateImage(VkImageCreateInfo& Img, VkImageViewCreateInfo& ImgView, VkSamplerCreateInfo& Sampler);
	}
}

#endif // !LEARNVK_RENDERER_API_VK_VULKAN_LOADER