#include "Device.h"
#include "API/Vk/ShaderToSPIRVCompiler.h"
#include "RenderAbstracts/Primitives.h"
#include "RenderAbstracts/FrameGraph.h"

static VkDebugUtilsMessengerEXT g_DebugMessenger;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	void* UserData)
{
	printf("::VALIDATION LAYER -- %s\n\n", CallbackData->pMessage);
	return VK_FALSE;
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
{
	CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	CreateInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	CreateInfo.pfnUserCallback = DebugCallback;
}

IRenderDevice::IRenderDevice() :
	Engine{},
	Dll{},
	Instance{},
	Gpu{},
	Device{},
	Surface{},
	Swapchain{},
	GraphicsQueue{},
	PresentQueue{},
	TransferQueue{},
	DefaultFramebuffer{},
	DefaultRenderPass{},
	Semaphores{},
	Fences{},
	ImageFences{},
	Properties{},
	Allocator{},
	CommandPool{},
	CommandBuffers{},
	DebugMessenger{},
	NextSwapchainImageIndex{},
	CurrentFrameIndex{},
	RenderFrame{}
{}

IRenderDevice::~IRenderDevice() {}

bool IRenderDevice::Initialize(const EngineImpl& Engine)
{
	if (!LoadVulkanLibrary()) return false;
	if (!LoadVulkanModuleAndGlobalFunc()) return false;
	if (!CreateVulkanInstance()) return false;
	if (!LoadVulkanFunctions()) return false;

	if (!CreateDebugMessenger()) return false;
	if (!CreatePresentationSurface(Engine.Window.Handle)) return false;
	if (!ChoosePhysicalDevice()) return false;
	if (!CreateLogicalDevice()) return false;
	if (!LoadVulkanDeviceFunction()) return false;

	GetDeviceQueues();

	if (!CreateSwapchain(Engine.Window.Extent.Width, Engine.Window.Extent.Height)) return false;
	if (!CreateSyncObjects()) return false;
	//if (!CreateDefaultRenderpass()) return false;
	//if (!CreateDefaultFramebuffer()) return false;
	if (!CreateAllocator()) return false;
	if (!CreateDefaultCommandPool()) return false;
	if (!AllocateCommandBuffers()) return false;
	//if (!CreateTransferOperation()) return false;

	return true;
}

void IRenderDevice::Terminate()
{
	//DestroyDefaultFramebuffer();
	//MoveToZombieList(DefaultRenderPass, EHandleType::Handle_Type_Renderpass);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		MoveToZombieList(CommandBuffers[i], CommandPool);
	}
	for (size_t i = 0; i < Swapchain.NumOfImages; i++)
	{
		MoveToZombieList(Swapchain.ImageViews[i], EHandleType::Handle_Type_Image_View);
	}
	MoveToZombieList(CommandPool, EHandleType::Handle_Type_Command_Pool);

	ClearZombieList();
	vmaDestroyAllocator(Allocator);

	for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyFence(Device, Fences[i], nullptr);
		for (uint32 j = 0; j < Semaphore_Type_Max; j++)
		{
			vkDestroySemaphore(Device, Semaphores[i][j], nullptr);
		}
	}

	if (Swapchain.Hnd != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(Device, Swapchain.Hnd, nullptr);
	}
#if RENDERER_DEBUG_RENDER_DEVICE
	if (DebugMessenger != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
	}
#endif

	vkDestroyDevice(Device, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);
	vkDestroyInstance(Instance, nullptr);
	FreeVulkanLibrary();
}

bool IRenderDevice::CreateSwapchain(uint32 Width, uint32 Height)
{
	static auto getSwapChainImageCount = [](
		const VkSurfaceCapabilitiesKHR& SurfaceCapabilities
		) -> uint32
	{
		uint32 imageCount = SurfaceCapabilities.minImageCount + 1;
		if ((SurfaceCapabilities.maxImageCount > 0) && (imageCount > SurfaceCapabilities.maxImageCount))
		{
			imageCount = SurfaceCapabilities.maxImageCount;
		}
		return imageCount;
	};

	static auto getSwapChainFormat = [](
		VkSurfaceFormatKHR* SurfaceFormats,
		uint32 Count
		) -> VkSurfaceFormatKHR
	{
		if ((Count == 1) && (SurfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			return { VK_FORMAT_R8G8B8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
		}

		for (uint32 i = 0; i < Count; i++)
		{
			if (SurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
			{
				return SurfaceFormats[i];
			}
		}

		return SurfaceFormats[0];
	};

	static auto getSwapChainExtent = [](
		const VkSurfaceCapabilitiesKHR& SurfaceCapabilities,
		uint32 SwapchainWidth,
		uint32 SwapchainHeight
		) -> VkExtent2D
	{
		if (SurfaceCapabilities.currentExtent.width == -1)
		{
			VkExtent2D swapChainExtent = { SwapchainWidth, SwapchainHeight };
			if (swapChainExtent.width < SurfaceCapabilities.minImageExtent.width)
			{
				swapChainExtent.width = SurfaceCapabilities.minImageExtent.width;
			}

			if (swapChainExtent.height < SurfaceCapabilities.minImageExtent.height)
			{
				swapChainExtent.height = SurfaceCapabilities.minImageExtent.height;
			}

			if (swapChainExtent.width > SurfaceCapabilities.maxImageExtent.width)
			{
				swapChainExtent.width = SurfaceCapabilities.maxImageExtent.width;
			}

			if (swapChainExtent.height > SurfaceCapabilities.maxImageExtent.height)
			{
				swapChainExtent.height = SurfaceCapabilities.maxImageExtent.height;
			}

			return swapChainExtent;
		}

		return SurfaceCapabilities.currentExtent;
	};

	static auto getSwapChainUsageFlags = [](
		const VkSurfaceCapabilitiesKHR& SurfaceCapabilities
		) -> VkImageUsageFlags
	{
		if (SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		{
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		return static_cast<VkImageUsageFlags>(-1);
	};

	static auto getSwapChainTransform = [](
		const VkSurfaceCapabilitiesKHR& SurfaceCapabilities
		) -> VkSurfaceTransformFlagBitsKHR
	{
		if (SurfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}

		return SurfaceCapabilities.currentTransform;
	};

	static auto getSwapChainPresentModes = [](
		VkPresentModeKHR* PresentModes,
		uint32 Count
		) -> VkPresentModeKHR
	{
		for (uint32 i = 0; i < Count; i++)
		{
			if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return PresentModes[i];
			}
		}

		for (uint32 i = 0; i < Count; i++)
		{
			if (PresentModes[i] == VK_PRESENT_MODE_FIFO_KHR)
			{
				return PresentModes[i];
			}
		}

		return static_cast<VkPresentModeKHR>(-1);
	};

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};

	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Gpu, Surface, &surfaceCapabilities) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not check presentation surface capabilities!" && false);
		return false;
	}

	uint32 formatCount = 0;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(Gpu, Surface, &formatCount, nullptr) != VK_SUCCESS)
	{
		VKT_ASSERT("Error occured during presentation surface formats enumeration!" && false);
		return false;
	}

	if (formatCount > 16)
	{
		formatCount = 16;
	}

	VkSurfaceFormatKHR surfaceFormats[16] = {};
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(Gpu, Surface, &formatCount, surfaceFormats) != VK_SUCCESS)
	{
		VKT_ASSERT("Error occurred during presentation surface formats enumeration!" && false);
		return false;
	}

	uint32 presentModeCount = 0;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(Gpu, Surface, &presentModeCount, nullptr) != VK_SUCCESS)
	{
		VKT_ASSERT("Error occurred during presentation surface present modes enumeration!" && false);
		return false;
	}

	if (presentModeCount > 16)
	{
		presentModeCount = 16;
	}

	VkPresentModeKHR presentModes[16] = {};
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(Gpu, Surface, &presentModeCount, presentModes) != VK_SUCCESS)
	{
		VKT_ASSERT("Error occurred during presentation surface present modes enumeration!" && false);
		return false;
	}

	VkSurfaceFormatKHR desiredFormat = getSwapChainFormat(surfaceFormats, formatCount);
	uint32 imageCount = getSwapChainImageCount(surfaceCapabilities);

	if (imageCount > MAX_SWAPCHAIN_IMAGE_ALLOWED)
	{
		imageCount = MAX_SWAPCHAIN_IMAGE_ALLOWED;
	}

	VkExtent2D imageExtent = getSwapChainExtent(surfaceCapabilities, Width, Height);

	VkSwapchainKHR oldSwapChain = Swapchain.Hnd;

	VkSwapchainCreateInfoKHR createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = desiredFormat.format;
	createInfo.imageColorSpace = desiredFormat.colorSpace;
	createInfo.imageExtent = imageExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = getSwapChainUsageFlags(surfaceCapabilities);
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = getSwapChainTransform(surfaceCapabilities);
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = getSwapChainPresentModes(presentModes, presentModeCount);
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapChain;

	if (vkCreateSwapchainKHR(Device, &createInfo, nullptr, &Swapchain.Hnd) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not create swap chain!" && false);
		return false;
	}

	// Cache swap chain format into the context.
	Swapchain.Format = desiredFormat;
	Swapchain.NumOfImages = imageCount;
	Swapchain.Extent = imageExtent;

	if (!Swapchain.Images.Size())
	{
		// Get swap chain images.
		// Set up only once.
		Swapchain.Images.Reserve(Swapchain.NumOfImages);
		Swapchain.ImageViews.Reserve(Swapchain.NumOfImages);

		for (uint32 i = 0; i < Swapchain.NumOfImages; i++)
		{
			Swapchain.Images.Push(VkImage());
			Swapchain.ImageViews.Push(VkImageView());
		}
	}
	vkGetSwapchainImagesKHR(Device, Swapchain.Hnd, &Swapchain.NumOfImages, Swapchain.Images.First());

	if (oldSwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(Device, oldSwapChain, nullptr);
		// Destroy previous image views on re-creation.
		for (uint32 i = 0; i < Swapchain.NumOfImages; i++)
		{
			vkDestroyImageView(Device, Swapchain.ImageViews[i], nullptr);
		}
	}

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = Swapchain.Format.format;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	// Create Image Views ...
	for (uint32 i = 0; i < Swapchain.NumOfImages; i++)
	{
		imageViewCreateInfo.image = Swapchain.Images[i];
		if (vkCreateImageView(Device, &imageViewCreateInfo, nullptr, &Swapchain.ImageViews[i]) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create image view for framebuffer!" && false);
			return false;
		}
	}

	return true;
}

bool IRenderDevice::CreateDefaultFramebuffer()
{
	VulkanFramebuffer& framebuffer = DefaultFramebuffer;

	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = DefaultRenderPass;
	createInfo.attachmentCount = 1;
	createInfo.width = Swapchain.Extent.width;
	createInfo.height = Swapchain.Extent.height;
	createInfo.layers = 1;

	for (uint32 i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
	{
		createInfo.pAttachments = &Swapchain.ImageViews[i];
		if (vkCreateFramebuffer(Device, &createInfo, nullptr, &framebuffer.Hnd[i]) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create a framebuffer!" && false);
			return false;
		}
	}

	return true;
}

void IRenderDevice::DestroyDefaultFramebuffer()
{
	for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
	{
		MoveToZombieList(DefaultFramebuffer.Hnd[i], EHandleType::Handle_Type_Framebuffer);
		DefaultFramebuffer.Hnd[i] = VK_NULL_HANDLE;
	}
}

void IRenderDevice::BeginFrame()
{
	RenderFrame = true;
	WaitFence(Fences[CurrentFrameIndex]);
	VkResult result = vkAcquireNextImageKHR(
		Device,
		Swapchain.Hnd,
		UINT64_MAX,
		Semaphores[CurrentFrameIndex][Semaphore_Type_Image_Available],
		VK_NULL_HANDLE,
		&NextSwapchainImageIndex
	);

	switch (result)
	{
	case VK_SUCCESS:
	case VK_SUBOPTIMAL_KHR:
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		OnWindowResize(Swapchain.Extent.width, Swapchain.Extent.height);
		break;
	default:
		VKT_ASSERT(false && "Problem occured during swap chain image acquisation!");
		RenderFrame = false;
		return;
	}

	VkCommandBuffer cmd = GetCommandBuffer();
	VkCommandBufferBeginInfo begin = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		nullptr
	};
	vkResetCommandBuffer(cmd, 0);
	vkBeginCommandBuffer(cmd, &begin);
}

bool IRenderDevice::RenderThisFrame() const
{
	return RenderFrame;
}

void IRenderDevice::EndFrame()
{
	vkEndCommandBuffer(GetCommandBuffer());
	
	if (ImageFences[NextSwapchainImageIndex] != VK_NULL_HANDLE)
	{
		WaitFence(ImageFences[NextSwapchainImageIndex]);
	}

	ImageFences[NextSwapchainImageIndex] = Fences[CurrentFrameIndex];

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &Semaphores[CurrentFrameIndex][Semaphore_Type_Image_Available];
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.pCommandBuffers = &GetCommandBuffer();
	submitInfo.commandBufferCount = 1;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &Semaphores[CurrentFrameIndex][Semaphore_Type_Render_Complete];

	vkResetFences(Device, 1, &Fences[CurrentFrameIndex]);
	VkResult res = vkQueueSubmit(GraphicsQueue.Hnd, 1, &submitInfo, Fences[CurrentFrameIndex]);
	if (res != VK_SUCCESS)
	{
		VKT_ASSERT(false && res);
		return;
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &Semaphores[CurrentFrameIndex][Semaphore_Type_Render_Complete];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &Swapchain.Hnd;
	presentInfo.pImageIndices = &NextSwapchainImageIndex;

	VkResult result = vkQueuePresentKHR(PresentQueue.Hnd, &presentInfo);

	CurrentFrameIndex = (CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

	switch (result)
	{
	case VK_SUCCESS:
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
	case VK_SUBOPTIMAL_KHR:
		OnWindowResize(Swapchain.Extent.width, Swapchain.Extent.height);
		break;
	default:
		VKT_ASSERT("Problem occured during image presentation!" && false);
		break;
	}
	ClearZombieList();
}

void IRenderDevice::DeviceWaitIdle()
{
	vkDeviceWaitIdle(Device);
}

void IRenderDevice::ClearZombieList()
{
	if (!ZombieList.Length()) { return; }

	DeviceWaitIdle();
	for (const ZombieObject& obj : ZombieList)
	{
		switch (obj.Type)
		{
		case EHandleType::Handle_Type_Buffer:
			vmaDestroyBuffer(Allocator, (VkBuffer)obj.Hnd, obj.Allocation);
			continue;
		case EHandleType::Handle_Type_Command_Buffer:
			vkFreeCommandBuffers(Device, obj.CommandPool, 1, (VkCommandBuffer*)&obj.Hnd);
			continue;
		case EHandleType::Handle_Type_Command_Pool:
			vkDestroyCommandPool(Device, (VkCommandPool)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Descriptor_Pool:
			vkDestroyDescriptorPool(Device, (VkDescriptorPool)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Descriptor_Set:
			vkFreeDescriptorSets(Device, obj.DescriptorPool, 1, (VkDescriptorSet*)&obj.Hnd);
			continue;
		case EHandleType::Handle_Type_Descriptor_Set_Layout:
			vkDestroyDescriptorSetLayout(Device, (VkDescriptorSetLayout)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Framebuffer:
			vkDestroyFramebuffer(Device, (VkFramebuffer)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Renderpass:
			vkDestroyRenderPass(Device, (VkRenderPass)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Image:
			vmaDestroyImage(Allocator, (VkImage)obj.Hnd, obj.Allocation);
			continue;
		case EHandleType::Handle_Type_Image_Sampler:
			vkDestroySampler(Device, (VkSampler)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Shader:
			vkDestroyShaderModule(Device, (VkShaderModule)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Pipeline:
			vkDestroyPipeline(Device, (VkPipeline)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Pipeline_Layout:
			vkDestroyPipelineLayout(Device, (VkPipelineLayout)obj.Hnd, nullptr);
			continue;
		case EHandleType::Handle_Type_Image_View:
		default:
			vkDestroyImageView(Device, (VkImageView)obj.Hnd, nullptr);
			continue;
		}
	}
	ZombieList.Empty();
}

VkCommandPool IRenderDevice::CreateCommandPool(uint32 QueueFamilyIndex, VkCommandPoolCreateFlags Flags)
{
	VkCommandPool hnd;
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.queueFamilyIndex = QueueFamilyIndex;
	info.flags = Flags;

	if (vkCreateCommandPool(Device, &info, nullptr, &hnd) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}
	return hnd;
}

VkCommandPool IRenderDevice::GetGraphicsCommandPool()
{
	return CommandPool;
}

VkCommandBuffer IRenderDevice::AllocateCommandBuffer(VkCommandPool PoolHnd, VkCommandBufferLevel Level, uint32 Count)
{
	VkCommandBuffer hnd;
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = PoolHnd;
	info.level = Level;
	info.commandBufferCount = Count;

	if (vkAllocateCommandBuffers(Device, &info, &hnd) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}
	return hnd;
}

void IRenderDevice::ResetCommandBuffer(VkCommandBuffer Hnd, VkCommandBufferResetFlags Flag)
{
	vkResetCommandBuffer(Hnd, Flag);
}

VkFence IRenderDevice::CreateFence(VkFenceCreateFlags Flag)
{
	VkFence hnd;
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = Flag;

	if (vkCreateFence(Device, &info, nullptr, &hnd) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}
	return hnd;
}

const IRenderDevice::VulkanSwapchain& IRenderDevice::GetSwapchain() const
{
	return Swapchain;
}

VkImage IRenderDevice::GetNextImageInSwapchain() const
{
	return Swapchain.Images[NextSwapchainImageIndex];
}

void IRenderDevice::WaitFence(VkFence Hnd, uint64 Timeout)
{
	vkWaitForFences(Device, 1, &Hnd, VK_TRUE, Timeout);
}

void IRenderDevice::ResetFence(VkFence Hnd)
{
	vkResetFences(Device, 1, &Hnd);
}

void IRenderDevice::DestroyFence(VkFence Hnd)
{
	vkDestroyFence(Device, Hnd, nullptr);
}

VkSemaphore IRenderDevice::CreateVkSemaphore(VkSemaphoreTypeCreateInfo* pSemaphoreType)
{
	VkSemaphore hnd;
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (pSemaphoreType) { info.pNext = pSemaphoreType; }

	if (vkCreateSemaphore(Device, &info, nullptr, &hnd) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}
	return hnd;
}

void IRenderDevice::DestroyVkSemaphore(VkSemaphore Hnd)
{
	vkDestroySemaphore(Device, Hnd, nullptr);
}

void IRenderDevice::BufferBarrier(
	VkCommandBuffer Cmd, 
	VkBuffer Hnd, 
	size_t Size, 
	size_t Offset, 
	VkAccessFlags SrcAccessMask, 
	VkAccessFlags DstAccessMask, 
	VkPipelineStageFlags SrcStageMask, 
	VkPipelineStageFlags DstStageMask, 
	uint32 SrcQueue, 
	uint32 DstQueue
)
{
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = Hnd;
	barrier.size = Size;
	barrier.offset = Offset;
	barrier.srcAccessMask = SrcAccessMask;
	barrier.dstAccessMask = DstAccessMask;
	barrier.srcQueueFamilyIndex = SrcQueue;
	barrier.dstQueueFamilyIndex = DstQueue;

	vkCmdPipelineBarrier(Cmd, SrcStageMask, DstStageMask, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void IRenderDevice::ImageBarrier(
	VkCommandBuffer Cmd, 
	VkImage Hnd, 
	VkImageSubresourceRange* pSubRange, 
	VkImageLayout OldLayout, 
	VkImageLayout NewLayout, 
	VkPipelineStageFlags SrcStageMask, 
	VkPipelineStageFlags DstStageMask, 
	uint32 SrcQueue, 
	uint32 DstQueue,
  VkAccessFlags SrcAccessMask,
  VkAccessFlags DstAccessMask
)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = OldLayout;
	barrier.newLayout = NewLayout;
	barrier.image = Hnd;
	barrier.srcAccessMask = SrcAccessMask;
  barrier.dstAccessMask = DstAccessMask;
	barrier.srcQueueFamilyIndex = SrcQueue;
	barrier.dstQueueFamilyIndex = DstQueue;

  if (pSubRange)
  {
	  barrier.subresourceRange = *pSubRange;
  }

	vkCmdPipelineBarrier(Cmd, SrcStageMask, DstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void IRenderDevice::WaitTimelineSempahore(VkSemaphore Hnd, uint64 Value, uint64 Timeout)
{
	VkSemaphoreWaitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	info.semaphoreCount = 1;
	info.pValues = &Value;
	info.pSemaphores = &Hnd;

	vkWaitSemaphores(Device, &info, Timeout);
}

void IRenderDevice::SignalTimelineSemaphore(VkSemaphore Hnd, uint64 Value)
{
	VkSemaphoreSignalInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
	info.semaphore = Hnd;
	info.value = Value;

	vkSignalSemaphore(Device, &info);
}

void IRenderDevice::BeginCommandBuffer(VkCommandBuffer Hnd, VkCommandBufferUsageFlags Flag)
{
	VkCommandBufferBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info.flags = Flag;
	vkBeginCommandBuffer(Hnd, &info);
}

void IRenderDevice::EndCommandBuffer(VkCommandBuffer Hnd)
{
	vkEndCommandBuffer(Hnd);
}

bool IRenderDevice::ToSpirV(const astl::String& Code, astl::Array<uint32>& SpirV, uint32 ShaderType)
{
	constexpr shaderc_shader_kind shaderType[] = {
		shaderc_vertex_shader,
		shaderc_fragment_shader,
		shaderc_geometry_shader,
		shaderc_compute_shader
	};

	auto shaderTypeStr = [](shaderc_shader_kind Type) -> const char* {
		switch (Type)
		{
		case shaderc_vertex_shader:
			return "VertexShader";
		case shaderc_fragment_shader:
			return "FragmentShader";
		case shaderc_geometry_shader:
			return "GeometryShader";
		case shaderc_compute_shader:
		default:
			break;
		}
		return "ComputeShader";
	};

	ShaderToSPIRVCompiler compiler;
	if (!compiler.CompileShader(
		shaderTypeStr(shaderType[ShaderType]), 
		shaderType[ShaderType], 
		Code.C_Str(), 
		SpirV
	))
  {
    printf(compiler.GetLastErrorMessage());
    return false;
  }

	return true;
}

bool IRenderDevice::LoadVulkanLibrary()
{
	Dll = OS::LoadDllLibrary("vulkan-1.dll");
	if (!Dll)
	{
		VKT_ASSERT(false); // Fail to load vulkan library.
		return false;
	}
	return true;
}

bool IRenderDevice::LoadVulkanModuleAndGlobalFunc()
{
#define VK_EXPORTED_FUNCTION(Func)									\
		if (!(Func = (PFN_##Func)OS::LoadProcAddress(Dll, #Func)))	\
		{															\
			VKT_ASSERT(false);										\
			return false;											\
		}
#include "Vk/VkFuncDecl.inl"

#define VK_GLOBAL_LEVEL_FUNCTION(Func)										\
		if (!(Func = (PFN_##Func)vkGetInstanceProcAddr(nullptr, #Func)))	\
		{																	\
			VKT_ASSERT(false);												\
			return false;													\
		}
#include "Vk/VkFuncDecl.inl"

	return true;
}

bool IRenderDevice::CreateVulkanInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pEngineName = "Angkasawan_Render_Engine";
	appInfo.pApplicationName = "Angkasawan";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

#if RENDERER_DEBUG_RENDER_DEVICE
	const char* extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};
	uint32 extensionCount = 3;
#else
	const char* extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};
	uint32 extensionCount = 2;
#endif

	VkInstanceCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.ppEnabledExtensionNames = extensions;
	createInfo.enabledExtensionCount = extensionCount;

#if RENDERER_DEBUG_RENDER_DEVICE
	const char* layers[] = { "VK_LAYER_LUNARG_monitor", "VK_LAYER_KHRONOS_validation" };
	createInfo.ppEnabledLayerNames = layers;
	createInfo.enabledLayerCount = 2;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	PopulateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
#else
	const char* layers[] = { "VK_LAYER_LUNARG_monitor" };
	createInfo.ppEnabledLayerNames = layers;
	createInfo.enabledLayerCount = 1;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &Instance) != VK_SUCCESS)
	{
		VKT_ASSERT("Was not able to create vulkan instance. Vulkan is not supported by hardware!" && false);
		return false;
	}

	return true;
}

bool IRenderDevice::LoadVulkanFunctions()
{
#define VK_INSTANCE_LEVEL_FUNCTION(Func)									\
		if (!(Func = (PFN_##Func)vkGetInstanceProcAddr(Instance, #Func)))	\
		{																	\
			VKT_ASSERT(false);												\
			return false;													\
		}
#include "Vk/VkFuncDecl.inl"
	return true;
}

bool IRenderDevice::CreateDebugMessenger()
{
#if RENDERER_DEBUG_RENDER_DEVICE
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	if (vkCreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
	{
		VKT_ASSERT("Failed to set up debug messenger" && false);
		return false;
	}
#endif
	return true;
}

bool IRenderDevice::ChoosePhysicalDevice()
{
	static auto CheckPhysicalDeviceProperties = [](
		VkPhysicalDevice& PhysicalDevice,
		VkSurfaceKHR& Surface) -> bool
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(PhysicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(PhysicalDevice, &deviceFeatures);

		uint32 majorVersion = VK_VERSION_MAJOR(deviceProperties.apiVersion);

		if ((majorVersion < 1) && (deviceProperties.limits.maxImageDimension2D < 4096))
		{
			VKT_ASSERT("Physical device doesn't support required parameters!" && false);
			return false;
		}

		return true;
	};

	static auto GetTransferQueueFamilyIndex = [](
		VkPhysicalDevice& Gpu,
		uint32& Index) -> bool
	{
		uint32 familyCount = 0;
		VkQueueFamilyProperties properties[8] = {};

		vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &familyCount, nullptr);
		if (!familyCount) { return false; }

		familyCount = (familyCount > 8) ? 8 : familyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &familyCount, properties);

		for (uint32 i = 0; i < familyCount; i++)
		{
			if (!properties[i].queueCount) { continue; }
			if (!(properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)) { continue; }

			if ((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ||
				(properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				continue;
			}

			Index = i;
			break;
		}

		return true;
	};

	static auto GetGraphicsAndPresentQueueFamilyIndex = [](
		VkPhysicalDevice& Gpu,
		VkSurfaceKHR& Surface,
		uint32& GfxIndex,
		uint32& PresentIndex) -> bool
	{
		uint32 familyCount = 0;
		VkQueueFamilyProperties properties[8] = {};
		VkBool32 presentSupport[8] = {};

		vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &familyCount, nullptr);
		if (!familyCount) { return false; }

		familyCount = (familyCount > 8) ? 8 : familyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &familyCount, properties);

		for (uint32 i = 0; i < familyCount; i++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(Gpu, i, Surface, &presentSupport[i]);
			if (!properties[i].queueCount) { continue; }
			if (!(properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) { continue; }

			GfxIndex = i;
			if (presentSupport[i])
			{
				PresentIndex = i;

			}
			break;
		}

		if (PresentIndex == -1)
		{
			for (uint32 i = 0; i < familyCount; i++)
			{
				if (!presentSupport[i]) { continue; }
				PresentIndex = i;
				break;
			}
		}

		return true;
	};

	uint32 numPhysicalDevices = 0;
	if (vkEnumeratePhysicalDevices(Instance, &numPhysicalDevices, nullptr) != VK_SUCCESS)
	{
		VKT_ASSERT("Unable to enumerate physical devices!" && false);
		return false;
	}

	numPhysicalDevices = (numPhysicalDevices > 8) ? 8 : numPhysicalDevices;

	VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevices[8] = {};

	vkEnumeratePhysicalDevices(Instance, &numPhysicalDevices, physicalDevices);

	for (size_t i = 0; i < static_cast<uint32>(numPhysicalDevices); i++)
	{
		if (!CheckPhysicalDeviceProperties(physicalDevices[i], Surface))
		{
			continue;
		}
		selectedDevice = physicalDevices[i];
		break;
	}

	if (selectedDevice == VK_NULL_HANDLE)
	{
		VKT_ASSERT("No physical devices on PC matches features requested by the engine!" && false);
		return false;
	}

	Gpu = selectedDevice;
	vkGetPhysicalDeviceProperties(Gpu, &Properties);

	uint32 queueFamilyCount = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &queueFamilyCount, nullptr);
	if (!queueFamilyCount) { return false; }

	queueFamilyCount = (queueFamilyCount > 8) ? 8 : queueFamilyCount;

	VkQueueFamilyProperties properties[8] = {};
	vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &queueFamilyCount, properties);

	TransferQueue.FamilyIndex = -1;
	if (!GetTransferQueueFamilyIndex(
		Gpu,
		TransferQueue.FamilyIndex))
	{
		return false;
	}

	GraphicsQueue.FamilyIndex = -1;
	PresentQueue.FamilyIndex = -1;
	if (!GetGraphicsAndPresentQueueFamilyIndex(
		Gpu,
		Surface,
		GraphicsQueue.FamilyIndex,
		PresentQueue.FamilyIndex))
	{
		return false;
	}

	if (TransferQueue.FamilyIndex == -1)
	{
		TransferQueue.FamilyIndex = GraphicsQueue.FamilyIndex;
	}

	return true;
}

bool IRenderDevice::CreateLogicalDevice()
{
	float32 queuePriority = 1.0f;
	astl::Array<VkDeviceQueueCreateInfo> queueCreateInfos;

	{
		VkDeviceQueueCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = GraphicsQueue.FamilyIndex;
		info.queueCount = 1;
		info.pQueuePriorities = &queuePriority;
		queueCreateInfos.Push(astl::Move(info));
	}

	{
		VkDeviceQueueCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = TransferQueue.FamilyIndex;
		info.queueCount = 1;
		info.pQueuePriorities = &queuePriority;
		queueCreateInfos.Push(astl::Move(info));
	}

	const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceVulkan12Features deviceFeatures12 = {};
  deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  deviceFeatures12.imagelessFramebuffer = VK_TRUE;
  deviceFeatures12.descriptorIndexing = VK_TRUE;
  deviceFeatures12.drawIndirectCount = VK_TRUE;
  deviceFeatures12.descriptorBindingPartiallyBound = VK_TRUE;

  VkPhysicalDeviceFeatures2 deviceFeatures = {};
  deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  deviceFeatures.features.samplerAnisotropy = VK_TRUE;
  deviceFeatures.pNext = &deviceFeatures12;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = &deviceFeatures;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.Length());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.First();
	deviceCreateInfo.ppEnabledExtensionNames = extensions;
	deviceCreateInfo.enabledExtensionCount = 1;

	if (vkCreateDevice(Gpu, &deviceCreateInfo, nullptr, &Device) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not create Vulkan device!" && false);
		return false;
	}

	return true;
}

bool IRenderDevice::CreatePresentationSurface(WndHandle Hwnd)
{
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = Hwnd;
	createInfo.hinstance = OS::GetHandleToModule(nullptr);

	if (vkCreateWin32SurfaceKHR(Instance, &createInfo, nullptr, &Surface) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not create presentation surface!" && false);
		return false;
	}
	return true;
}

void IRenderDevice::GetDeviceQueues()
{
	vkGetDeviceQueue(Device, GraphicsQueue.FamilyIndex, 0, &GraphicsQueue.Hnd);
	vkGetDeviceQueue(Device, PresentQueue.FamilyIndex, 0, &PresentQueue.Hnd);
	vkGetDeviceQueue(Device, TransferQueue.FamilyIndex, 0, &TransferQueue.Hnd);
}

bool IRenderDevice::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		for (uint32 j = 0; j < Semaphore_Type_Max; j++)
		{
			if (vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &Semaphores[i][j]) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not create Semaphore!" && false);
				return false;
			}
		}

		if (vkCreateFence(Device, &fenceInfo, nullptr, &Fences[i]) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create Fence!" && false);
			return false;
		}
	}

	return true;
}

bool IRenderDevice::CreateDefaultRenderpass()
{
	VkAttachmentDescription attachmentDesc = {};
	attachmentDesc.format = Swapchain.Format.format;
	attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;

	//VkSubpassDependency subpassDependency = {};
	//subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	//subpassDependency.dstSubpass = 0;
	//subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//subpassDependency.srcAccessMask = 0;
	//subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderpassCreateInfo = {};
	renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassCreateInfo.attachmentCount = 1;
	renderpassCreateInfo.pAttachments = &attachmentDesc;
	renderpassCreateInfo.subpassCount = 1;
	renderpassCreateInfo.pSubpasses = &subpassDescription;
	//renderpassCreateInfo.dependencyCount = 1;
	//renderpassCreateInfo.pDependencies = &subpassDependency;

	if (vkCreateRenderPass(Device, &renderpassCreateInfo, nullptr, &DefaultRenderPass) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not create render pass!" && false);
		return false;
	}

	return true;
}

bool IRenderDevice::CreateAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = Gpu;
	allocatorInfo.device = Device;
	allocatorInfo.instance = Instance;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;

	if (vmaCreateAllocator(&allocatorInfo, &Allocator) != VK_SUCCESS)
	{
		VKT_ASSERT("Failed to create vulkan memory allocator!" && false);
		return false;
	}

	return true;
}

//bool IRenderDevice::CreateTransferOperation()
//{
//	VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
//	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//	cmdPoolCreateInfo.queueFamilyIndex = TransferOp.Queue.FamilyIndex;
//	cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//
//	if (vkCreateCommandPool(Device, &cmdPoolCreateInfo, nullptr, &TransferOp.CommandPool) != VK_SUCCESS)
//	{
//		return false;
//	}
//
//	VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
//	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//	cmdBufferAllocateInfo.commandPool = TransferOp.CommandPool;
//	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//	cmdBufferAllocateInfo.commandBufferCount = 1;
//
//	if (vkAllocateCommandBuffers(Device, &cmdBufferAllocateInfo, &TransferOp.TransferCmdBuffer) != VK_SUCCESS)
//	{
//		return false;
//	}
//
//	cmdBufferAllocateInfo.commandPool = CommandPool;
//
//	if (vkAllocateCommandBuffers(Device, &cmdBufferAllocateInfo, &TransferOp.GraphicsCmdBuffer) != VK_SUCCESS)
//	{
//		return false;
//	}
//
//	VkFenceCreateInfo fenceCreateInfo = {};
//	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//
//	if (vkCreateFence(Device, &fenceCreateInfo, nullptr, &TransferOp.Fence) != VK_SUCCESS)
//	{
//		return false;
//	}
//
//	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
//	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
//
//	vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &TransferOp.Semaphore);
//
//	return true;
//}

bool IRenderDevice::CreateDefaultCommandPool()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = GraphicsQueue.FamilyIndex;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(Device, &createInfo, nullptr, &CommandPool) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not create command pool!" && false);
		return false;
	}

	return true;
}

bool IRenderDevice::AllocateCommandBuffers()
{
	VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
	cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocateInfo.commandPool = CommandPool;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(Device, &cmdBufferAllocateInfo, CommandBuffers) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not allocate command buffers!" && false);
		return false;
	}

	return true;
}

bool IRenderDevice::LoadVulkanDeviceFunction()
{
#define VK_DEVICE_LEVEL_FUNCTION(Func)									\
		if (!(Func = (PFN_##Func)vkGetDeviceProcAddr(Device, #Func)))	\
		{																\
			VKT_ASSERT(false);											\
			return false;												\
		}
#include "Vk/VkFuncDecl.inl"
	return true;
}

void IRenderDevice::FreeVulkanLibrary()
{
	OS::FreeDllLibrary(Dll);
}

IDeviceStore::IDeviceStore(/*IAllocator& InAllocator*/) :
	//Allocator(InAllocator),
	Buffers{},
	DescriptorSets{},
	DescriptorPools{},
	DescriptorSetLayouts{},
	RenderPasses{},
	ImageSamplers{},
	Pipelines{},
	Shaders{},
	Images{}//,
	//PushConstants{}
{}

IDeviceStore::~IDeviceStore() {}
/*
bool IDeviceStore::DoesBufferExist(size_t Id)
{
	for (auto& [id, resource] : Buffers)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesDescriptorSetExist(size_t Id)
{
	for (auto& [id, resource] : DescriptorSets)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesDescriptorPoolExist(size_t Id)
{
	for (auto& [id, resource] : DescriptorPools)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesDescriptorSetLayoutExist(size_t Id)
{
	for (auto& [id, resource] : DescriptorSetLayouts)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesRenderPassExist(size_t Id)
{
	for (auto& [id, resource] : RenderPasses)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesImageSamplerExist(size_t Id)
{
	for (auto& [id, resource] : ImageSamplers)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesPipelineExist(size_t Id)
{
	for (auto& [id, resource] : Pipelines)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesShaderExist(size_t Id)
{
	for (auto& [id, resource] : Shaders)
	{
		if (id == Id) { return true; }
	}
	return false;
}

bool IDeviceStore::DoesImageExist(size_t Id)
{
	for (auto& [id, resource] : Images)
	{
		if (id == Id) { return true; }
	}
	return false;
}*/

astl::Ref<SMemoryBuffer> IDeviceStore::NewBuffer(size_t Id)
{
	if (Buffers.Contains(Id)) { return NULLPTR; }
  astl::Ref<SMemoryBuffer> ref = Buffers.Insert(
    astl::Move(Id),
    astl::UniquePtr<SMemoryBuffer>(astl::IMemory::New<SMemoryBuffer>())
  );
  return ref;
}

astl::Ref<SMemoryBuffer> IDeviceStore::GetBuffer(size_t Id)
{
  if (!Buffers.Contains(Id)) { return NULLPTR; }
  return Buffers[Id];
}

bool IDeviceStore::DeleteBuffer(size_t Id)
{
  if (!Buffers.Contains(Id)) { return false; }
  Buffers.Remove(Id);
  return true;
}

astl::Ref<SDescriptorSet> IDeviceStore::NewDescriptorSet(size_t Id)
{
  if (DescriptorSets.Contains(Id)) { return NULLPTR; }
  astl::Ref<SDescriptorSet> ref = DescriptorSets.Insert(
    astl::Move(Id),
    astl::UniquePtr<SDescriptorSet>(astl::IMemory::New<SDescriptorSet>())
  );
  return ref;
}

astl::Ref<SDescriptorSet> IDeviceStore::GetDescriptorSet(size_t Id)
{
  if (!DescriptorSets.Contains(Id)) { return NULLPTR; }
  return DescriptorSets[Id];
}

bool IDeviceStore::DeleteDescriptorSet(size_t Id)
{

  if (!DescriptorSets.Contains(Id)) { return false; }
  DescriptorSets.Remove(Id);
  return true;
}

astl::Ref<SDescriptorPool> IDeviceStore::NewDescriptorPool(size_t Id)
{
  if (DescriptorPools.Contains(Id)) { return NULLPTR; }
  astl::Ref<SDescriptorPool> ref = DescriptorPools.Insert(
    astl::Move(Id),
    astl::UniquePtr<SDescriptorPool>(astl::IMemory::New<SDescriptorPool>())
  );
  return ref;
}

astl::Ref<SDescriptorPool> IDeviceStore::GetDescriptorPool(size_t Id)
{
  if (!DescriptorPools.Contains(Id)) { return NULLPTR; }
  return DescriptorPools[Id];
}

bool IDeviceStore::DeleteDescriptorPool(size_t Id)
{
  if (!DescriptorPools.Contains(Id)) { return false; }
  DescriptorPools.Remove(Id);
  return true;
}

astl::Ref<SDescriptorSetLayout> IDeviceStore::NewDescriptorSetLayout(size_t Id)
{
  if (DescriptorSetLayouts.Contains(Id)) { return NULLPTR; }
  astl::Ref<SDescriptorSetLayout> ref = DescriptorSetLayouts.Insert(
    astl::Move(Id),
    astl::UniquePtr<SDescriptorSetLayout>(astl::IMemory::New<SDescriptorSetLayout>())
  );
  return ref;
}

astl::Ref<SDescriptorSetLayout> IDeviceStore::GetDescriptorSetLayout(size_t Id)
{
  if (!DescriptorSetLayouts.Contains(Id)) { return NULLPTR; }
  return DescriptorSetLayouts[Id];
}

bool IDeviceStore::DeleteDescriptorSetLayout(size_t Id)
{
  if (!DescriptorSetLayouts.Contains(Id)) { return false; }
  DescriptorSetLayouts.Remove(Id);
  return true;
}

astl::Ref<SRenderPass> IDeviceStore::NewRenderPass(size_t Id)
{
  if (RenderPasses.Contains(Id)) { return NULLPTR; }
  astl::Ref<SRenderPass> ref = RenderPasses.Insert(
    astl::Move(Id),
    astl::UniquePtr<SRenderPass>(astl::IMemory::New<SRenderPass>())
  );
  return ref;
}

astl::Ref<SRenderPass> IDeviceStore::GetRenderPass(size_t Id)
{
  if (!RenderPasses.Contains(Id)) { return NULLPTR; }
  return RenderPasses[Id];
}

bool IDeviceStore::DeleteRenderPass(size_t Id)
{
  if (!RenderPasses.Contains(Id)) { return false; }
  RenderPasses.Remove(Id);
  return true;
}

astl::Ref<SImageSampler> IDeviceStore::NewImageSampler(size_t Id)
{
  if (ImageSamplers.Contains(Id)) { return NULLPTR; }
  astl::Ref<SImageSampler> ref = ImageSamplers.Insert(
    astl::Move(Id),
    astl::UniquePtr<SImageSampler>(astl::IMemory::New<SImageSampler>())
  );
  return ref;
}

astl::Ref<SImageSampler> IDeviceStore::GetImageSampler(size_t Id)
{
  if (!ImageSamplers.Contains(Id)) { return NULLPTR; }
  return ImageSamplers[Id];
}

astl::Ref<SImageSampler> IDeviceStore::GetImageSamplerWithHash(uint64 Hash)
{
  for (auto& [key, value] : ImageSamplers)
  {
    if (value->Hash == Hash)
    {
      return value;
    }
  }
  return NULLPTR;
}

bool IDeviceStore::DeleteImageSampler(size_t Id)
{
  if (!ImageSamplers.Contains(Id)) { return false; }
  ImageSamplers.Remove(Id);
  return true;
}

astl::Ref<SPipeline> IDeviceStore::NewPipeline(size_t Id)
{
  if (Pipelines.Contains(Id)) { return NULLPTR; }
  astl::Ref<SPipeline> ref = Pipelines.Insert(
    astl::Move(Id),
    astl::UniquePtr<SPipeline>(astl::IMemory::New<SPipeline>())
  );
  return ref;
}

astl::Ref<SPipeline> IDeviceStore::GetPipeline(size_t Id)
{
  if (!Pipelines.Contains(Id)) { return NULLPTR; }
  return Pipelines[Id];
}

bool IDeviceStore::DeletePipeline(size_t Id)
{
  if (!Pipelines.Contains(Id)) { return false; }
  Pipelines.Remove(Id);
  return true;
}

astl::Ref<SShader> IDeviceStore::NewShader(size_t Id)
{
  if (Shaders.Contains(Id)) { return NULLPTR; }
  astl::Ref<SShader> ref = Shaders.Insert(
    astl::Move(Id),
    astl::UniquePtr<SShader>(astl::IMemory::New<SShader>())
  );
  return ref;
}

astl::Ref<SShader> IDeviceStore::GetShader(size_t Id)
{
  if (!Shaders.Contains(Id)) { return NULLPTR; }
  return Shaders[Id];
}

bool IDeviceStore::DeleteShader(size_t Id)
{
  if (!Shaders.Contains(Id)) { return false; }
  Shaders.Remove(Id);
  return true;
}

astl::Ref<SImage> IDeviceStore::NewImage(size_t Id)
{
  if (Images.Contains(Id)) { return NULLPTR; }
  astl::Ref<SImage> ref = Images.Insert(
    astl::Move(Id),
    astl::UniquePtr<SImage>(astl::IMemory::New<SImage>())
  );
  return ref;
}

astl::Ref<SImage> IDeviceStore::GetImage(size_t Id)
{
  if (!Images.Contains(Id)) { return NULLPTR; }
	return Images[Id];
}

bool IDeviceStore::DeleteImage(size_t Id)
{
	if (!Images.Contains(Id)) { return false; }
	Images.Remove(Id);
	return true;
}

//SPushConstant* IDeviceStore::NewPushConstant(size_t Id)
//{
//	if (DoesPushConstantxist(Id)) { return nullptr; }
//	SPushConstant* resource = IAllocator::New<SPushConstant>(Allocator);
//	PushConstant.Insert(Id, resource);
//	return resource;
//}
//
//SPushConstant* IDeviceStore::GetPushConstantsize_t Id)
//{
//	if (!DoesPushConstantxist(Id)) { return nullptr; }
//	return PushConstant[Id];
//}
//
//bool IDeviceStore::DeletePushConstantsize_t Id, bool Free)
//{
//	if (!DoesPushConstantxist(Id)) { return false; }
//	if (Free)
//	{
//		SPushConstant* resource = PushConstant[Id];
//		IAllocator::Delete<SPushConstant>(resource, Allocator);
//	}
//	PushConstant.Remove(Id);
//	return true;
//}

const VkCommandBuffer& IRenderDevice::GetCommandBuffer() const
{
	return CommandBuffers[CurrentFrameIndex];
}

VmaAllocator& IRenderDevice::GetAllocator()
{
	return Allocator;
}

VkDevice IRenderDevice::GetDevice() const
{
	return Device;
}

const IRenderDevice::VulkanQueue& IRenderDevice::GetTransferQueue() const
{
	return TransferQueue;
}

const IRenderDevice::VulkanQueue& IRenderDevice::GetGraphicsQueue() const
{
	return GraphicsQueue;
}

const IRenderDevice::VulkanQueue& IRenderDevice::GetPresentationQueue() const
{
	return PresentQueue;
}

VkImageUsageFlags IRenderDevice::GetImageUsage(uint32 Index) const
{
	static constexpr VkImageUsageFlags usage[] = {
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	};
	return usage[Index];
}

VkSampleCountFlagBits IRenderDevice::GetSampleCount(uint32 Index) const
{
	static constexpr VkSampleCountFlagBits samples[] = {
		VK_SAMPLE_COUNT_1_BIT,
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_64_BIT
	};
	return samples[Index];
}

VkPipelineBindPoint IRenderDevice::GetPipelineBindPoint(uint32 Index) const
{
	static constexpr VkPipelineBindPoint bindPoints[] = {
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		VK_PIPELINE_BIND_POINT_COMPUTE,
	};
	return bindPoints[Index];
}

VkImageType IRenderDevice::GetImageType(uint32 Index) const
{
	static constexpr VkImageType types[] = {
		VK_IMAGE_TYPE_1D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_3D
	};
	return types[Index];
}

VkImageViewType IRenderDevice::GetImageViewType(uint32 Index) const
{
	static constexpr VkImageViewType types[] = {
		VK_IMAGE_VIEW_TYPE_1D,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_VIEW_TYPE_3D
	};
	return types[Index];
}

VkDescriptorType IRenderDevice::GetDescriptorType(uint32 Index) const
{
	static constexpr VkDescriptorType types[] = {
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_SAMPLER
	};
	return types[Index];
}

VkBufferUsageFlagBits IRenderDevice::GetBufferUsage(uint32 Index) const
{
	static constexpr VkBufferUsageFlagBits flags[] = {
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT
	};
	return flags[Index];
}

VkVertexInputRate IRenderDevice::GetVertexInputRate(uint32 Index) const
{
	static constexpr VkVertexInputRate rate[] = {
		VK_VERTEX_INPUT_RATE_VERTEX,
		VK_VERTEX_INPUT_RATE_INSTANCE,
	};
	return rate[Index];
}

VkPrimitiveTopology IRenderDevice::GetPrimitiveTopology(uint32 Index) const
{
	static constexpr VkPrimitiveTopology topology[] = {
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
	};
	return topology[Index];
}

VkPolygonMode IRenderDevice::GetPolygonMode(uint32 Index) const
{
	static constexpr VkPolygonMode mode[] = {
		VK_POLYGON_MODE_FILL,
		VK_POLYGON_MODE_LINE,
		VK_POLYGON_MODE_POINT
	};
	return mode[Index];
}

VkFrontFace IRenderDevice::GetFrontFaceMode(uint32 Index) const
{
	static constexpr VkFrontFace face[] = {
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FRONT_FACE_CLOCKWISE
	};
	return face[Index];
}

VkCullModeFlags IRenderDevice::GetCullMode(uint32 Index) const
{
	static constexpr VkCullModeFlags mode[] = {
		VK_CULL_MODE_NONE,
		VK_CULL_MODE_FRONT_BIT,
		VK_CULL_MODE_BACK_BIT,
		VK_CULL_MODE_FRONT_AND_BACK
	};
	return mode[Index];
}

VmaMemoryUsage IRenderDevice::GetMemoryUsage(uint32 Index) const
{
	static constexpr VmaMemoryUsage usage[] = {
		VMA_MEMORY_USAGE_CPU_ONLY,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	};
	return usage[Index];
}

VkSamplerAddressMode IRenderDevice::GetSamplerAddressMode(uint32 Index) const
{
	static constexpr VkSamplerAddressMode mode[] = {
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
	};
	return mode[Index];
}

VkFilter IRenderDevice::GetFilter(uint32 Index) const
{
	static constexpr VkFilter filter[] = {
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
		VK_FILTER_CUBIC_IMG
	};
	return filter[Index];
}

VkCompareOp IRenderDevice::GetCompareOp(uint32 Index) const
{
	static constexpr VkCompareOp compareOp[] = {
		VK_COMPARE_OP_NEVER,
		VK_COMPARE_OP_LESS,
		VK_COMPARE_OP_EQUAL,
		VK_COMPARE_OP_LESS_OR_EQUAL,
		VK_COMPARE_OP_GREATER,
		VK_COMPARE_OP_NOT_EQUAL,
		VK_COMPARE_OP_GREATER_OR_EQUAL,
		VK_COMPARE_OP_ALWAYS
	};
	return compareOp[Index];
}

VkFormat IRenderDevice::GetImageFormat(uint32 Format, uint32 Channels) const
{
  static constexpr VkFormat format[] = {
    VK_FORMAT_R8_SRGB,
    VK_FORMAT_R8G8_SRGB,
    VK_FORMAT_R8G8B8_SRGB,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_R8_UNORM,
    VK_FORMAT_R8G8_UNORM,
    VK_FORMAT_R8G8B8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM
  };
	return format[Format * Channels + (Channels - 1)];
}

const VkPhysicalDeviceProperties& IRenderDevice::GetPhysicalDeviceProperties() const
{
	return Properties;
}

void IRenderDevice::OnWindowResize(uint32 Width, uint32 Height)
{
	//DestroyDefaultFramebuffer();
	CreateSwapchain(Width, Height);
	//CreateDefaultFramebuffer();
}

const uint32 IRenderDevice::GetCurrentFrameIndex() const
{
	return CurrentFrameIndex;
}

const uint32 IRenderDevice::GetNextSwapchainImageIndex() const
{
	return NextSwapchainImageIndex;
}

IRenderDevice::ZombieObject::ZombieObject() :
	Hnd(nullptr), 
	Type(EHandleType::Handle_Type_None), 
	CommandPool{}, 
	DescriptorPool{}, 
	Allocation{}
{}

IRenderDevice::ZombieObject::ZombieObject(void* Handle, EHandleType Type) :
	Hnd(Handle), 
	Type(Type),
	CommandPool{},
	DescriptorPool{},
	Allocation{}
{}

IRenderDevice::ZombieObject::ZombieObject(void* Handle, VkCommandPool Pool) :
	Hnd(Handle), 
	Type(EHandleType::Handle_Type_Command_Buffer), 
	CommandPool(Pool), 
	DescriptorPool{}, 
	Allocation{}
{}

IRenderDevice::ZombieObject::ZombieObject(void* Handle, VkDescriptorPool Pool) :
	Hnd(Handle), 
	Type(EHandleType::Handle_Type_Descriptor_Set), 
	CommandPool{}, 
	DescriptorPool(Pool), 
	Allocation{}
{}

IRenderDevice::ZombieObject::ZombieObject(void* Handle, EHandleType Type, VmaAllocation Allocation) :
	Hnd(Handle), 
	Type(Type), 
	CommandPool{}, 
	DescriptorPool{}, 
	Allocation(Allocation)
{}

IRenderDevice::ZombieObject::~ZombieObject() {}
