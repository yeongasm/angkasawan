#include "Device.h"

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
	if (!LoadVulkanFunctions()) return false;

	GetDeviceQueues();

	if (!CreateSwapchain(Engine.Window.Extent.Width, Engine.Window.Extent.Height)) return false;
	if (!CreateSyncObjects()) return false;
	if (!CreateDefaultRenderpass()) return false;
	if (!CreateDefaultFramebuffer()) return false;
	if (!CreateAllocator()) return false;
	if (!CreateCommandPool()) return false;
	if (!AllocateCommandBuffers()) return false;
	//if (!CreateTransferOperation()) return false;

	return true;
}

void IRenderDevice::Terminate()
{
	DestroyDefaultFramebuffer();
	vkDestroyRenderPass(Device, DefaultRenderPass, nullptr);
	vmaDestroyAllocator(Allocator);

	for (size_t i = 0; i < Swapchain.NumOfImages; i++)
	{
		vkDestroyImageView(Device, Swapchain.ImageViews[i], nullptr);
	}

	vkDestroyCommandPool(Device, CommandPool, nullptr);
	//vkDestroyCommandPool(Device, TransferOp.CommandPool, nullptr);
	//vkDestroyFence(Device, TransferOp.Fence, nullptr);

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

	if (DebugMessenger != VK_NULL_HANDLE)
	{
		vkDestroyDebugUtilsMessengerEXT(Instance, DebugMessenger, nullptr);
	}

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
	vkDeviceWaitIdle(Device);
	for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
	{
		vkDestroyFramebuffer(Device, DefaultFramebuffer.Hnd[i], nullptr);
		DefaultFramebuffer.Hnd[i] = VK_NULL_HANDLE;
	}
}

void IRenderDevice::BeginFrame()
{
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

void IRenderDevice::EndFrame()
{
	vkEndCommandBuffer(GetCommandBuffer());
}

void IRenderDevice::DeviceWaitIdle()
{
	vkDeviceWaitIdle(Device);
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

void IRenderDevice::DestroyCommandPool(VkCommandPool Hnd)
{
	vkDestroyCommandPool(Device, Hnd, nullptr);
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
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	if (vkCreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &DebugMessenger) != VK_SUCCESS)
	{
		VKT_ASSERT("Failed to set up debug messenger" && false);
		return false;
	}
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
	Array<VkDeviceQueueCreateInfo> queueCreateInfos;

	{
		VkDeviceQueueCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = GraphicsQueue.FamilyIndex;
		info.queueCount = 1;
		info.pQueuePriorities = &queuePriority;
		queueCreateInfos.Push(Move(info));
	}

	{
		VkDeviceQueueCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = TransferQueue.FamilyIndex;
		info.queueCount = 1;
		info.pQueuePriorities = &queuePriority;
		queueCreateInfos.Push(Move(info));
	}

	const char* extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.Length());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.First();
	deviceCreateInfo.ppEnabledExtensionNames = &extensions;
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

bool IRenderDevice::CreateCommandPool()
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

IDeviceStore::IDeviceStore(IAllocator& InAllocator) :
	Allocator(InAllocator),
	Buffers{},
	DescriptorSets{},
	DescriptorPools{},
	DescriptorSetLayouts{},
	RenderPasses{},
	ImageSamplers{},
	Pipelines{},
	Shaders{},
	Images{},
	PushConstants{}
{}

IDeviceStore::~IDeviceStore() {}

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
}

SMemoryBuffer* IDeviceStore::NewBuffer(size_t Id)
{
	if (DoesBufferExist(Id)) { return nullptr; }
	SMemoryBuffer* resource = IAllocator::New<SMemoryBuffer>(Allocator);
	Buffers.Insert(Id, resource);
	return resource;
}

SMemoryBuffer* IDeviceStore::GetBuffer(size_t Id)
{
	if (!DoesBufferExist(Id)) { return nullptr; }
	return Buffers[Id];
}

bool IDeviceStore::DeleteBuffer(size_t Id, bool Free)
{
	if (!DoesBufferExist(Id)) { return false; }
	if (Free) 
	{ 
		SMemoryBuffer* resource = Buffers[Id];
		IAllocator::Delete<SMemoryBuffer>(resource, Allocator);
	}
	Buffers.Remove(Id);
	return true;
}

SDescriptorSet* IDeviceStore::NewDescriptorSet(size_t Id)
{
	if (DoesDescriptorSetExist(Id)) { return nullptr; }
	SDescriptorSet* resource = IAllocator::New<SDescriptorSet>(Allocator);
	DescriptorSets.Insert(Id, resource);
	return resource;
}

SDescriptorSet* IDeviceStore::GetDescriptorSet(size_t Id)
{
	if (!DoesDescriptorSetExist(Id)) { return nullptr; }
	return DescriptorSets[Id];
}

bool IDeviceStore::DeleteDescriptorSet(size_t Id, bool Free)
{
	if (!DoesDescriptorSetExist(Id)) { return false; }
	if (Free)
	{
		SDescriptorSet* resource = DescriptorSets[Id];
		IAllocator::Delete<SDescriptorSet>(resource, Allocator);
	}
	DescriptorSets.Remove(Id);
	return true;
}

SDescriptorPool* IDeviceStore::NewDescriptorPool(size_t Id)
{
	if (DoesDescriptorPoolExist(Id)) { return nullptr; }
	SDescriptorPool* resource = IAllocator::New<SDescriptorPool>(Allocator);
	DescriptorPools.Insert(Id, resource);
	return resource;
}

SDescriptorPool* IDeviceStore::GetDescriptorPool(size_t Id)
{
	if (!DoesDescriptorPoolExist(Id)) { return nullptr; }
	return DescriptorPools[Id];
}

bool IDeviceStore::DeleteDescriptorPool(size_t Id, bool Free)
{
	if (!DoesDescriptorPoolExist(Id)) { return false; }
	if (Free)
	{
		SDescriptorPool* resource = DescriptorPools[Id];
		IAllocator::Delete<SDescriptorPool>(resource, Allocator);
	}
	DescriptorPools.Remove(Id);
	return true;
}

SDescriptorSetLayout* IDeviceStore::NewDescriptorSetLayout(size_t Id)
{
	if (DoesDescriptorSetLayoutExist(Id)) { return nullptr; }
	SDescriptorSetLayout* resource = IAllocator::New<SDescriptorSetLayout>(Allocator);
	DescriptorSetLayouts.Insert(Id, resource);
	return resource;
}

SDescriptorSetLayout* IDeviceStore::GetDescriptorSetLayout(size_t Id)
{
	if (!DoesDescriptorSetLayoutExist(Id)) { return nullptr; }
	return DescriptorSetLayouts[Id];
}

bool IDeviceStore::DeleteDescriptorSetLayout(size_t Id, bool Free)
{
	if (!DoesDescriptorSetLayoutExist(Id)) { return false; }
	if (Free)
	{
		SDescriptorSetLayout* resource = DescriptorSetLayouts[Id];
		IAllocator::Delete<SDescriptorSetLayout>(resource, Allocator);
	}
	DescriptorSetLayouts.Remove(Id);
	return true;
}

SRenderPass* IDeviceStore::NewRenderPass(size_t Id)
{
	if (DoesRenderPassExist(Id)) { return nullptr; }
	SRenderPass* resource = IAllocator::New<SRenderPass>(Allocator);
	RenderPasses.Insert(Id, resource);
	return resource;
}

SRenderPass* IDeviceStore::GetRenderPass(size_t Id)
{
	if (!DoesRenderPassExist(Id)) { return nullptr; }
	return RenderPasses[Id];
}

bool IDeviceStore::DeleteRenderPass(size_t Id, bool Free)
{
	if (!DoesRenderPassExist(Id)) { return false; }
	if (Free)
	{
		SRenderPass* resource = RenderPasses[Id];
		IAllocator::Delete<SRenderPass>(resource, Allocator);
	}
	RenderPasses.Remove(Id);
	return true;
}

SImageSampler* IDeviceStore::NewImageSampler(size_t Id)
{
	if (DoesImageSamplerExist(Id)) { return nullptr; }
	SImageSampler* resource = IAllocator::New<SImageSampler>(Allocator);
	ImageSamplers.Insert(Id, resource);
	return resource;
}

SImageSampler* IDeviceStore::GetImageSampler(size_t Id)
{
	if (!DoesImageSamplerExist(Id)) { return nullptr; }
	return ImageSamplers[Id];
}

bool IDeviceStore::DeleteImageSampler(size_t Id, bool Free)
{
	if (!DoesImageSamplerExist(Id)) { return false; }
	if (Free)
	{
		SImageSampler* resource = ImageSamplers[Id];
		IAllocator::Delete<SImageSampler>(resource, Allocator);
	}
	ImageSamplers.Remove(Id);
	return true;
}

SPipeline* IDeviceStore::NewPipeline(size_t Id)
{
	if (DoesPipelineExist(Id)) { return nullptr; }
	SPipeline* resource = IAllocator::New<SPipeline>(Allocator);
	Pipelines.Insert(Id, resource);
	return resource;
}

SPipeline* IDeviceStore::GetPipeline(size_t Id)
{
	if (!DoesPipelineExist(Id)) { return nullptr; }
	return Pipelines[Id];
}

bool IDeviceStore::DeletePipeline(size_t Id, bool Free)
{
	if (!DoesPipelineExist(Id)) { return false; }
	if (Free)
	{
		SPipeline* resource = Pipelines[Id];
		IAllocator::Delete<SPipeline>(resource, Allocator);
	}
	Pipelines.Remove(Id);
	return true;
}

SShader* IDeviceStore::NewShader(size_t Id)
{
	if (DoesShaderExist(Id)) { return nullptr; }
	SShader* resource = IAllocator::New<SShader>(Allocator);
	Shaders.Insert(Id, resource);
	return resource;
}

SShader* IDeviceStore::GetShader(size_t Id)
{
	if (!DoesShaderExist(Id)) { return nullptr; }
	return Shaders[Id];
}

bool IDeviceStore::DeleteShader(size_t Id, bool Free)
{
	if (!DoesShaderExist(Id)) { return false; }
	if (Free)
	{
		SShader* resource = Shaders[Id];
		IAllocator::Delete<SShader>(resource, Allocator);
	}
	Shaders.Remove(Id);
	return true;
}

SImage* IDeviceStore::NewImage(size_t Id)
{
	if (DoesImageExist(Id)) { return nullptr; }
	SImage* resource = IAllocator::New<SImage>(Allocator);
	Images.Insert(Id, resource);
	return resource;
}

SImage* IDeviceStore::GetImage(size_t Id)
{
	if (!DoesImageExist(Id)) { return nullptr; }
	return Images[Id];
}

bool IDeviceStore::DeleteImage(size_t Id, bool Free)
{
	if (!DoesImageExist(Id)) { return false; }
	if (Free)
	{
		SImage* resource = Images[Id];
		IAllocator::Delete<SImage>(resource, Allocator);
	}
	Images.Remove(Id);
	return true;
}

VkCommandBuffer IRenderDevice::GetCommandBuffer() const
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

VmaMemoryUsage IRenderDevice::GetMemoryUsage(uint32 Index) const
{
	static constexpr VmaMemoryUsage usage[] = {
		VMA_MEMORY_USAGE_CPU_ONLY,
		VMA_MEMORY_USAGE_GPU_ONLY,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	};
	return usage[Index];
}

const VkPhysicalDeviceProperties& IRenderDevice::GetPhysicalDeviceProperties() const
{
	return Properties;
}

const uint32 IRenderDevice::GetCurrentFrameIndex() const
{
	return CurrentFrameIndex;
}
