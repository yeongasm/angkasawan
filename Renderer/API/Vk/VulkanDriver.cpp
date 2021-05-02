#pragma once
#include "VulkanDriver.h"
#include "Library/Algorithms/QuickSort.h"
#include "VulkanFunctions.h"
#include "ShaderToSPIRVCompiler.h"
#include "Src/spirv_reflect.h"

namespace gpu
{

#define GET_CURR_CMD_BUFFER() VkCommandBuffer& cmd = Ctx.CommandBuffers[Ctx.CurrentFrameIndex]

	static VulkanContext Ctx;
	//static Map<uint32, VulkanFramebuffer> g_Framebuffer;
	//static Map<uint32, VkRenderPass> g_RenderPass;
	//static Map<uint32, VulkanBuffer> g_Buffers;
	//static Map<uint32, VulkanPipeline> g_Pipelines;
	//static Map<uint32, VulkanDescriptorSet> g_DescriptorSets;
	//static Map<uint32, VulkanDescriptorPool> g_DescriptorPools;
	//static Map<uint32, VkDescriptorSetLayout> g_DescriptorSetLayout;
	//static Map<uint32, VulkanImage> g_Textures;
	//static Map<uint32, VkShaderModule> g_Shaders;
	//static Map<uint32, VkSampler> g_Samplers;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
		VkDebugUtilsMessageTypeFlagsEXT Type,
		const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
		void* UserData)
	{
		printf("::VALIDATION LAYER -- %s\n\n", CallbackData->pMessage);
		return VK_FALSE;
	}

	bool LoadVulkanLibrary()
	{
		Ctx.Dll = OS::LoadDllLibrary("vulkan-1.dll");
		if (!Ctx.Dll)
		{
			VKT_ASSERT(false); // Fail to load vulkan library.
			return false;
		}
		return true;
	}

	bool LoadVulkanModuleAndGlobalFunctions()
	{
#define VK_EXPORTED_FUNCTION(Func)											\
		if (!(Func = (PFN_##Func)OS::LoadProcAddress(Ctx.Dll, #Func)))	\
		{																	\
			VKT_ASSERT(false);												\
			return false;													\
		}
#include "VkFuncDecl.inl"

#define VK_GLOBAL_LEVEL_FUNCTION(Func)										\
		if (!(Func = (PFN_##Func)vkGetInstanceProcAddr(nullptr, #Func)))	\
		{																	\
			VKT_ASSERT(false);												\
			return false;													\
		}
#include "VkFuncDecl.inl"

		return true;
	}

	void FreeVulkanLibrary()
	{
		OS::FreeDllLibrary(Ctx.Dll);
	}

	bool LoadVulkanFunctions()
	{
#define VK_INSTANCE_LEVEL_FUNCTION(Func)										\
		if (!(Func = (PFN_##Func)vkGetInstanceProcAddr(Ctx.Instance, #Func)))	\
		{																		\
			VKT_ASSERT(false);													\
			return false;														\
		}
#include "VkFuncDecl.inl"
		return true;
	}

	bool LoadVulkanDeviceFunction()
	{
#define VK_DEVICE_LEVEL_FUNCTION(Func)										\
		if (!(Func = (PFN_##Func)vkGetDeviceProcAddr(Ctx.Device, #Func)))	\
		{																	\
			VKT_ASSERT(false);												\
			return false;													\
		}
#include "VkFuncDecl.inl"
		return true;
	}

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
	{
		FMemory::InitializeObject(CreateInfo);
		CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		CreateInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		CreateInfo.pfnUserCallback = DebugCallback;
	}

	bool CreateDebugMessenger()
	{
#if RENDERER_DEBUG_RENDER_DEVICE
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (vkCreateDebugUtilsMessengerEXT(Ctx.Instance, &createInfo, nullptr, &Ctx.DebugMessenger) != VK_SUCCESS)
		{
			VKT_ASSERT("Failed to set up debug messenger" && false);
			return false;
		}
#endif
		return true;
	}

	bool CreateVmAllocator()
	{
		VmaAllocatorCreateInfo allocatorInfo;
		FMemory::InitializeObject(allocatorInfo);

		allocatorInfo.physicalDevice	= Ctx.Gpu;
		allocatorInfo.device			= Ctx.Device;
		allocatorInfo.instance			= Ctx.Instance;
		allocatorInfo.vulkanApiVersion	= VK_API_VERSION_1_2;

		if (vmaCreateAllocator(&allocatorInfo, &Ctx.Allocator) != VK_SUCCESS)
		{
			VKT_ASSERT("Failed to create vulkan memory allocator!" && false);
			return false;
		}

		return true;
	}

	bool CreateVulkanInstance()
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

		if (vkCreateInstance(&createInfo, nullptr, &Ctx.Instance) != VK_SUCCESS)
		{
			VKT_ASSERT("Was not able to create vulkan instance. Vulkan is not supported by hardware!" && false);
			return false;
		}

		return true;
	}

	bool CreatePresentationSurface()
	{
		EngineImpl& engine = ao::FetchEngineCtx();

#ifdef VK_USE_PLATFORM_WIN32_KHR
		VkWin32SurfaceCreateInfoKHR createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = engine.Window.Handle;
		createInfo.hinstance = OS::GetHandleToModule(nullptr);

		if (vkCreateWin32SurfaceKHR(Ctx.Instance, &createInfo, nullptr, &Ctx.Surface) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create presentation surface!" && false);
			return false;
		}
#endif
		return true;
	}

	bool ChoosePhysicalDevice()
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
		if (vkEnumeratePhysicalDevices(Ctx.Instance, &numPhysicalDevices, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to enumerate physical devices!" && false);
			return false;
		}

		numPhysicalDevices = (numPhysicalDevices > 8) ? 8 : numPhysicalDevices;

		VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevices[8] = {};

		vkEnumeratePhysicalDevices(Ctx.Instance, &numPhysicalDevices, physicalDevices);

		for (size_t i = 0; i < static_cast<uint32>(numPhysicalDevices); i++)
		{
			if (!CheckPhysicalDeviceProperties(physicalDevices[i], Ctx.Surface))
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

		Ctx.Gpu = selectedDevice;
		vkGetPhysicalDeviceProperties(Ctx.Gpu, &Ctx.Properties);

		uint32 queueFamilyCount = 0;

		vkGetPhysicalDeviceQueueFamilyProperties(Ctx.Gpu, &queueFamilyCount, nullptr);
		if (!queueFamilyCount) { return false; }

		queueFamilyCount = (queueFamilyCount > 8) ? 8 : queueFamilyCount;

		VkQueueFamilyProperties properties[8] = {};
		vkGetPhysicalDeviceQueueFamilyProperties(Ctx.Gpu, &queueFamilyCount, properties);

		Ctx.TransferOp.Queue.FamilyIndex = -1;
		if (!GetTransferQueueFamilyIndex(
			Ctx.Gpu, 
			Ctx.TransferOp.Queue.FamilyIndex))
		{
			return false;
		}

		Ctx.GraphicsQueue.FamilyIndex = -1;
		Ctx.PresentQueue.FamilyIndex = -1;
		if (!GetGraphicsAndPresentQueueFamilyIndex(
			Ctx.Gpu, 
			Ctx.Surface, 
			Ctx.GraphicsQueue.FamilyIndex, 
			Ctx.PresentQueue.FamilyIndex))
		{
			return false;
		}

		if (Ctx.TransferOp.Queue.FamilyIndex == -1)
		{
			Ctx.TransferOp.Queue.FamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
		}

		return true;
	}

	bool CreateLogicalDevice()
	{
		float32 queuePriority = 1.0f;
		//float32 queuePriorities[3] = { 1.0f, 1.0f, 1.0f };
		//float32 queuePriorities[2] = { 1.0f, 1.0f };
		Array<VkDeviceQueueCreateInfo> queueCreateInfos;

		//uint32 gfxQueueCount = 1;
		//gfxQueueCount = (Ctx.TransferOp.Queue.FamilyIndex == Ctx.GraphicsQueue.FamilyIndex) ? gfxQueueCount++ : gfxQueueCount;
		//gfxQueueCount = (Ctx.PresentQueue.FamilyIndex == Ctx.GraphicsQueue.FamilyIndex) ? gfxQueueCount++ : gfxQueueCount;

		//if (gfxQueueCount > 1)
		//{
		//	VkDeviceQueueCreateInfo info = {};
		//	info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		//	info.queueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
		//	info.queueCount = gfxQueueCount;
		//	info.pQueuePriorities = queuePriorities;
		//	queueCreateInfos.Push(Move(info));
		//}
		//else
		//{
		//	VkDeviceQueueCreateInfo info = {};
		//	info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		//	info.queueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;
		//	info.queueCount = 1;
		//	info.pQueuePriorities = queuePriorities;
		//	queueCreateInfos.Push(info);

		//	info.queueFamilyIndex = Ctx.PresentQueue.FamilyIndex;
		//	queueCreateInfos.Push(info);
		//}
		
		{
			VkDeviceQueueCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			info.queueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
			info.queueCount = 1;
			info.pQueuePriorities = &queuePriority;
			queueCreateInfos.Push(Move(info));
		}

		{
			VkDeviceQueueCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			info.queueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;
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

		if (vkCreateDevice(Ctx.Gpu, &deviceCreateInfo, nullptr, &Ctx.Device) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create Vulkan device!" && false);
			return false;
		}

		return true;
	}

	void GetDeviceQueue()
	{
		//uint32 queueIndex = 0;
		vkGetDeviceQueue(Ctx.Device, Ctx.GraphicsQueue.FamilyIndex, 0, &Ctx.GraphicsQueue.Handle);
		vkGetDeviceQueue(Ctx.Device, Ctx.PresentQueue.FamilyIndex, 0, &Ctx.PresentQueue.Handle);

		//queueIndex = (Ctx.PresentQueue.FamilyIndex == Ctx.GraphicsQueue.FamilyIndex) ? queueIndex++ : queueIndex;

		//queueIndex = (Ctx.TransferOp.Queue.FamilyIndex == Ctx.GraphicsQueue.FamilyIndex) ? queueIndex++ : queueIndex;
		vkGetDeviceQueue(Ctx.Device, Ctx.TransferOp.Queue.FamilyIndex, 0, &Ctx.TransferOp.Queue.Handle);
	}

	bool CreateSwapchain()
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
			const VkSurfaceCapabilitiesKHR& SurfaceCapabilities
			) -> VkExtent2D
		{
			EngineImpl& engine = ao::FetchEngineCtx();
			if (SurfaceCapabilities.currentExtent.width == -1)
			{
				VkExtent2D swapChainExtent = { engine.Window.Width, engine.Window.Height };
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

		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Ctx.Gpu, Ctx.Surface, &surfaceCapabilities) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not check presentation surface capabilities!" && false);
			return false;
		}

		uint32 formatCount = 0;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(Ctx.Gpu, Ctx.Surface, &formatCount, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Error occured during presentation surface formats enumeration!" && false);
			return false;
		}

		if (formatCount > 16)
		{
			formatCount = 16;
		}

		VkSurfaceFormatKHR surfaceFormats[16] = {};
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(Ctx.Gpu, Ctx.Surface, &formatCount, surfaceFormats) != VK_SUCCESS)
		{
			VKT_ASSERT("Error occurred during presentation surface formats enumeration!" && false);
			return false;
		}

		uint32 presentModeCount = 0;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(Ctx.Gpu, Ctx.Surface, &presentModeCount, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Error occurred during presentation surface present modes enumeration!" && false);
			return false;
		}

		if (presentModeCount > 16)
		{
			presentModeCount = 16;
		}

		VkPresentModeKHR presentModes[16] = {};
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(Ctx.Gpu, Ctx.Surface, &presentModeCount, presentModes) != VK_SUCCESS)
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

		VkExtent2D imageExtent = getSwapChainExtent(surfaceCapabilities);

		VkSwapchainKHR oldSwapChain = Ctx.Swapchain.Handle;

		VkSwapchainCreateInfoKHR createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = Ctx.Surface;
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

		if (vkCreateSwapchainKHR(Ctx.Device, &createInfo, nullptr, &Ctx.Swapchain.Handle) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create swap chain!" && false);
			return false;
		}

		// Cache swap chain format into the context.
		Ctx.Swapchain.Format = desiredFormat;
		Ctx.Swapchain.NumOfImages = imageCount;
		Ctx.Swapchain.Extent = imageExtent;

		if (!Ctx.Swapchain.Images.Size())
		{
			// Get swap chain images.
			// Set up only once.
			Ctx.Swapchain.Images.Reserve(Ctx.Swapchain.NumOfImages);
			Ctx.Swapchain.ImageViews.Reserve(Ctx.Swapchain.NumOfImages);

			for (uint32 i = 0; i < Ctx.Swapchain.NumOfImages; i++)
			{
				Ctx.Swapchain.Images.Push(VkImage());
				Ctx.Swapchain.ImageViews.Push(VkImageView());
			}
		}
		vkGetSwapchainImagesKHR(Ctx.Device, Ctx.Swapchain.Handle, &Ctx.Swapchain.NumOfImages, Ctx.Swapchain.Images.First());

		if (oldSwapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(Ctx.Device, oldSwapChain, nullptr);
			// Destroy previous image views on re-creation.
			for (uint32 i = 0; i < Ctx.Swapchain.NumOfImages; i++)
			{
				vkDestroyImageView(Ctx.Device, Ctx.Swapchain.ImageViews[i], nullptr);
			}
		}

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = Ctx.Swapchain.Format.format;
		imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		// Create Image Views ...
		for (uint32 i = 0; i < Ctx.Swapchain.NumOfImages; i++)
		{
			imageViewCreateInfo.image = Ctx.Swapchain.Images[i];
			if (vkCreateImageView(Ctx.Device, &imageViewCreateInfo, nullptr, &Ctx.Swapchain.ImageViews[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not create image view for framebuffer!" && false);
				return false;
			}
		}

		return true;
	}

	bool CreateSyncObjects()
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
				if (vkCreateSemaphore(Ctx.Device, &semaphoreInfo, nullptr, &Ctx.Semaphores[i][j]) != VK_SUCCESS)
				{
					VKT_ASSERT("Could not create Semaphore!" && false);
					return false;
				}
			}

			if (vkCreateFence(Ctx.Device, &fenceInfo, nullptr, &Ctx.Fences[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not create Fence!" && false);
				return false;
			}
		}

		return true;
	}

	bool CreateDefaultRenderPass()
	{
		VkRenderPass& renderPass = Ctx.DefaultRenderPass;

		VkAttachmentDescription attachmentDesc = {};
		attachmentDesc.format = Ctx.Swapchain.Format.format;
		attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		//attachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		//attachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorAttachmentReference;

		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderpassCreateInfo = {};
		renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpassCreateInfo.attachmentCount = 1;
		renderpassCreateInfo.pAttachments = &attachmentDesc;
		renderpassCreateInfo.subpassCount = 1;
		renderpassCreateInfo.pSubpasses = &subpassDescription;
		renderpassCreateInfo.dependencyCount = 1;
		renderpassCreateInfo.pDependencies = &subpassDependency;

		if (vkCreateRenderPass(Ctx.Device, &renderpassCreateInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create render pass!" && false);
			return false;
		}

		return true;
	}

	bool CreateDefaultFramebuffer()
	{
		VulkanFramebuffer& framebuffer = Ctx.DefautltFramebuffer;

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = Ctx.DefaultRenderPass;
		createInfo.attachmentCount = 1;
		createInfo.width = Ctx.Swapchain.Extent.width;
		createInfo.height = Ctx.Swapchain.Extent.height;
		createInfo.layers = 1;

		for (uint32 i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
		{
			createInfo.pAttachments = &Ctx.Swapchain.ImageViews[i];
			if (vkCreateFramebuffer(Ctx.Device, &createInfo, nullptr, &framebuffer.Framebuffers[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not create a framebuffer!" && false);
				return false;
			}
		}

		return true;
	}

	bool CreateTransferOperation()
	{
		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;
		cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(Ctx.Device, &cmdPoolCreateInfo, nullptr, &Ctx.TransferOp.CommandPool) != VK_SUCCESS)
		{
			return false;
		}

		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.commandPool = Ctx.TransferOp.CommandPool;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(Ctx.Device, &cmdBufferAllocateInfo, &Ctx.TransferOp.TransferCmdBuffer) != VK_SUCCESS)
		{
			return false;
		}

		cmdBufferAllocateInfo.commandPool = Ctx.CommandPool;

		if (vkAllocateCommandBuffers(Ctx.Device, &cmdBufferAllocateInfo, &Ctx.TransferOp.GraphicsCmdBuffer) != VK_SUCCESS)
		{
			return false;
		}

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		if (vkCreateFence(Ctx.Device, &fenceCreateInfo, nullptr, &Ctx.TransferOp.Fence) != VK_SUCCESS)
		{
			return false;
		}

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		vkCreateSemaphore(Ctx.Device, &semaphoreCreateInfo, nullptr, &Ctx.TransferOp.Semaphore);

		return true;
	}

	bool CreateCommandPool()
	{
		VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(Ctx.Device, &createInfo, nullptr, &Ctx.CommandPool) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create command pool!" && false);
			return false;
		}

		return true;
	}

	bool AllocateCommandBuffers()
	{
		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};
		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.commandPool = Ctx.CommandPool;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

		if (vkAllocateCommandBuffers(Ctx.Device, &cmdBufferAllocateInfo, Ctx.CommandBuffers) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not allocate command buffers!" && false);
			return false;
		}

		return true;
	}

	bool Initialize()
	{
		if (!LoadVulkanLibrary())					return false;
		if (!LoadVulkanModuleAndGlobalFunctions())	return false;
		if (!CreateVulkanInstance())				return false;
		if (!LoadVulkanFunctions())					return false;

#if RENDERER_DEBUG_RENDER_DEVICE
		if (!CreateDebugMessenger())				return false;
#endif

		if (!CreatePresentationSurface())			return false;
		if (!ChoosePhysicalDevice())				return false;
		if (!CreateLogicalDevice())					return false;
		if (!LoadVulkanDeviceFunction())			return false;

		GetDeviceQueue();

		if (!CreateSwapchain())						return false;
		if (!CreateSyncObjects())					return false;

		if (!CreateDefaultRenderPass())				return false;
		if (!CreateDefaultFramebuffer())			return false;
		if (!CreateVmAllocator())					return false;
		if (!CreateCommandPool())					return false;
		if (!AllocateCommandBuffers())				return false;
		if (!CreateTransferOperation())				return false;

		Ctx.NextSwapchainImageIndex = 0;
		Ctx.CurrentFrameIndex = 0;

		return true;
	}

	void Terminate()
	{
		Flush();
		vkDestroyRenderPass(Ctx.Device, Ctx.DefaultRenderPass, nullptr);

		vmaDestroyAllocator(Ctx.Allocator);

		for (size_t i = 0; i < Ctx.Swapchain.NumOfImages; i++)
		{
			vkDestroyImageView(Ctx.Device, Ctx.Swapchain.ImageViews[i], nullptr);
		}
		
		//vkDestroyCommandPool(Ctx.Device, Ctx.CommandPool, nullptr);
		vkDestroyCommandPool(Ctx.Device, Ctx.TransferOp.CommandPool, nullptr);
		vkDestroyFence(Ctx.Device, Ctx.TransferOp.Fence, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyFence(Ctx.Device, Ctx.Fences[i], nullptr);
			for (uint32 j = 0; j < Semaphore_Type_Max; j++)
			{
				vkDestroySemaphore(Ctx.Device, Ctx.Semaphores[i][j], nullptr);
			}
		}
		
		if (Ctx.Swapchain.Handle != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(Ctx.Device, Ctx.Swapchain.Handle, nullptr);
		}

		vkDestroyDevice(Ctx.Device, nullptr);

#if RENDERER_DEBUG_RENDER_DEVICE
		vkDestroyDebugUtilsMessengerEXT(Ctx.Instance, Ctx.DebugMessenger, nullptr);
#endif
		vkDestroySurfaceKHR(Ctx.Instance, Ctx.Surface, nullptr);
		vkDestroyInstance(Ctx.Instance, nullptr);
		FreeVulkanLibrary();
	}

	void BlitToDefault(const IRFrameGraph& Graph)
	{
		GET_CURR_CMD_BUFFER();
		VkImage src = g_Textures[Graph.GetColorImage()].Handle;
		VkImage dst = Ctx.Swapchain.Images[Ctx.NextSwapchainImageIndex];

		VkImageSubresourceRange range = {};
		range.levelCount = 1;
		range.layerCount = 1;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
		barrier.dstQueueFamilyIndex = Ctx.PresentQueue.FamilyIndex;
		barrier.image = dst;
		barrier.subresourceRange = range;

		int32 width = static_cast<int32>(Ctx.Swapchain.Extent.width);
		int32 height = static_cast<int32>(Ctx.Swapchain.Extent.height);

		VkOffset3D srcOffset = { width, height, 1 };
		VkOffset3D dstOffset = { width, height, 1 };

		VkImageBlit region = {};
		region.srcOffsets[1] = srcOffset;
		region.dstOffsets[1] = dstOffset;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.layerCount = 1;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.layerCount = 1;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		vkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		vkCmdBlitImage(
			cmd, 
			src, 
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			dst, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			1, 
			&region, 
			VK_FILTER_LINEAR
		);

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		vkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);
	}

	void BindRenderpass(RenderPass& Pass)
	{
		GET_CURR_CMD_BUFFER();
		VkRenderPass& renderPass = g_RenderPass[Pass.RenderpassHandle];
		VulkanFramebuffer& framebuffer = g_Framebuffer[Pass.FramebufferHandle];

		static Array<VkClearValue> clearValues;
		bool hasDepthStencil = false;
		size_t numClearValues = Pass.ColorOutputs.Length();

		if (!Pass.Flags.Has(RenderPass_Bit_No_Color_Render))
		{
			numClearValues++;
		}

		if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output) && 
			Pass.DepthStencilOutput.Handle != INVALID_HANDLE) 
		{
			numClearValues++;
			hasDepthStencil = true;
		}
		else if(!Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
		{
			hasDepthStencil = true;
			numClearValues++;
		}

		for (size_t i = 0; i < numClearValues; i++)
		{
			VkClearValue& clearValue = clearValues.Insert(VkClearValue());
			clearValue.color = { 0, 0, 0, 1 };

			if (i == numClearValues - 1 && hasDepthStencil)
			{
				clearValue.depthStencil = { 1.0f, 0 };
			}
		}

		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffer.Framebuffers[Ctx.NextSwapchainImageIndex];
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = { Ctx.Swapchain.Extent.width, Ctx.Swapchain.Extent.height };
		beginInfo.clearValueCount = static_cast<uint32>(clearValues.Length());
		beginInfo.pClearValues = clearValues.First();

		vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		clearValues.Empty();
	}

	void BindPipeline(SRPipeline& Pipeline)
	{
		GET_CURR_CMD_BUFFER();
		VulkanPipeline& pipeline = g_Pipelines[Pipeline.Handle];
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Handle);
	}

	void UnbindRenderpass(RenderPass& Pass)
	{
		GET_CURR_CMD_BUFFER();
		vkCmdEndRenderPass(cmd);
	}

	void BindVertexBuffer(SRMemoryBuffer& Buffer, uint32 FirstBinding, size_t Offset)
	{
		GET_CURR_CMD_BUFFER();
		VkBuffer vbo = g_Buffers[Buffer.Handle].Handle;
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, FirstBinding, 1, &vbo, &offset);
	}

	void BindIndexBuffer(SRMemoryBuffer& Buffer, size_t Offset)
	{
		GET_CURR_CMD_BUFFER();
		VkBuffer ebo = g_Buffers[Buffer.Handle].Handle;
		vkCmdBindIndexBuffer(cmd, ebo, Offset, VK_INDEX_TYPE_UINT32);
	}

	void BindInstanceBuffer(SRMemoryBuffer& Buffer, uint32 FirstBinding, size_t Offset)
	{
		GET_CURR_CMD_BUFFER();
		VkBuffer ibo = g_Buffers[Buffer.Handle].Handle;
		vkCmdBindVertexBuffers(cmd, FirstBinding, 1, &ibo, &Offset);
	}

	void Draw(const DrawCommand& Command)
	{
		GET_CURR_CMD_BUFFER();
		if (Command.NumIndices)
		{
			vkCmdDrawIndexed(cmd, Command.NumIndices, Command.InstanceCount, Command.IndexOffset, 0, Command.InstanceOffset);
			return;
		}
		vkCmdDraw(cmd, Command.NumVertices, Command.InstanceCount, Command.VertexOffset, Command.InstanceOffset);
	}

	bool CreateShader(Shader& InShader, String& Code)
	{
		constexpr shaderc_shader_kind shaderType[Shader_Type_Max] = {
			shaderc_vertex_shader, 
			shaderc_fragment_shader, 
			shaderc_geometry_shader, 
			shaderc_compute_shader
		};

		Array<uint32> spirv;
		ShaderToSPIRVCompiler compiler;

		if (!compiler.CompileShader(InShader.Name.C_Str(), shaderType[InShader.Type],Code.C_Str(), spirv))
		{
			return false;
		}

		uint32 count = 0;
		spv_reflect::ShaderModule reflection(spirv.Length() * sizeof(uint32), spirv.First());
		reflection.EnumerateInputVariables(&count, nullptr);

		if (count)
		{
			static auto sortLambda = [](const ShaderAttrib& A, const ShaderAttrib& B) -> bool {
				return A.Location < B.Location;
			};

			ShaderAttrib attribute = {};
			Array<SpvReflectInterfaceVariable*> variables(count + 1);
			reflection.EnumerateInputVariables(&count, variables.First());
			variables[count] = {};

			for (auto var : variables)
			{
				if (!var) continue;

				if (var->type_description->op == SpvOpTypeMatrix)
				{
					for (uint32 i = 0; i < 4; i++)
					{
						attribute.Format = Shader_Attrib_Type_Vec4;
						attribute.Location = var->location + i;
						attribute.Offset = 0;
						InShader.Attributes.Push(Move(attribute));
					}
					continue;
				}
				else
				{
					switch (var->format)
					{
					case SPV_REFLECT_FORMAT_R32_SFLOAT:
						attribute.Format = Shader_Attrib_Type_Float;
						break;
					case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
						attribute.Format = Shader_Attrib_Type_Vec2;
						break;
					case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
						attribute.Format = Shader_Attrib_Type_Vec3;
						break;
					case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
						attribute.Format = Shader_Attrib_Type_Vec4;
						break;
					}
				}
				attribute.Location = var->location;
				attribute.Offset = 0;
				InShader.Attributes.Push(Move(attribute));
			}

			QuickSort(InShader.Attributes, sortLambda);

			//uint32 stride = 0;
			//for (ShaderAttrib& attrib : InShader.Attributes)
			//{
			//	attrib.Offset = stride;
			//	stride += getTypeStride(attrib.Format);
			//}
		}

		VkShaderModuleCreateInfo shaderCreateInfo = {};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = spirv.Length() * sizeof(uint32);
		shaderCreateInfo.pCode = spirv.First();

		VkShaderModule shaderHnd;

		if (vkCreateShaderModule(Ctx.Device, &shaderCreateInfo, nullptr, &shaderHnd) != VK_SUCCESS)
		{
			return false;
		}

		uint32 id = g_RandIdVk();
		InShader.Handle = id;
		g_Shaders.Insert(id, shaderHnd);

		return true;
	}

	void DestroyShader(Shader& InShader)
	{
		VkShaderModule& shaderHnd = g_Shaders[InShader.Handle];
		vkDestroyShaderModule(Ctx.Device, shaderHnd, nullptr);
		g_Shaders.Remove(InShader.Handle);
		InShader.Handle = INVALID_HANDLE;
	}

	bool CreateGraphicsPipeline(SRPipeline& Pipeline)
	{
		static auto getTypeStride = [](EShaderAttribFormat Format) -> uint32 {
			switch (Format)
			{
				case Shader_Attrib_Type_Float:
					return 4;
				case Shader_Attrib_Type_Vec2:
					return 8;
				case Shader_Attrib_Type_Vec3:
					return 12;
				case Shader_Attrib_Type_Vec4:
					return 16;
				default:
					break;
			}
			return 0;
		};

		if ((Pipeline.VertexShader->Handle   == INVALID_HANDLE) ||
			(Pipeline.FragmentShader->Handle == INVALID_HANDLE))
		{
			return false;
		}

		constexpr VkVertexInputRate inputRate[Vertex_Input_Rate_Max] = {
			VK_VERTEX_INPUT_RATE_VERTEX,
			VK_VERTEX_INPUT_RATE_INSTANCE,
		};

		Array<VkPipelineShaderStageCreateInfo> pipelineStages = {};

		// Pipeline shader stage declaration.
		// Vertex ...
		VkPipelineShaderStageCreateInfo vertexStage = {};
		vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStage.module = g_Shaders[Pipeline.VertexShader->Handle];
		vertexStage.pName = "main";
		pipelineStages.Push(Move(vertexStage));

		// Vertex bindings ...
		Array<VkVertexInputBindingDescription> bindingDescriptions;
		//VkVertexInputBindingDescription bindingDescription = {};
		//bindingDescription.binding = 0;
		//bindingDescription.stride = Pass.VertexStride;
		//bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Vertex attributes ...
		uint32 binding = 0;
		VkFormat format;
		Shader* vertexShader = Pipeline.VertexShader;
		ShaderAttrib* attribute = nullptr;
		Array<VkVertexInputAttributeDescription> vertexAttributes(vertexShader->Attributes.Length());

		for (auto& vtxInBind : Pipeline.VertexInputBindings)
		{
			bindingDescriptions.Push({
				vtxInBind.Binding,
				vtxInBind.Stride,
				inputRate[vtxInBind.Type]
			});
		}

		for (auto& vtxInBind : Pipeline.VertexInputBindings)
		{
			uint32 stride = 0;
			for (size_t i = vtxInBind.From; i <= vtxInBind.To; i++)
			{
				attribute = &vertexShader->Attributes[i];
				
				switch (attribute->Format)
				{
				case Shader_Attrib_Type_Vec2:
					format = VK_FORMAT_R32G32_SFLOAT;
					break;
				case Shader_Attrib_Type_Vec3:
					format = VK_FORMAT_R32G32B32_SFLOAT;
					break;
				default:
					format = VK_FORMAT_R32G32B32A32_SFLOAT;
					break;
				}

				attribute->Offset = stride;
				stride += getTypeStride(attribute->Format);
				binding = vtxInBind.Binding;
				vertexAttributes.Push({ attribute->Location, binding, format, attribute->Offset });
			}
		}

		// Fragment ...
		VkPipelineShaderStageCreateInfo fragmentStage = {};
		fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentStage.module = g_Shaders[Pipeline.FragmentShader->Handle];
		fragmentStage.pName = "main";
		pipelineStages.Push(Move(fragmentStage));

		VkPipelineVertexInputStateCreateInfo vertexStateCreateInfo = {};
		vertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions.First();
		vertexStateCreateInfo.pVertexAttributeDescriptions = vertexAttributes.First();
		vertexStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32>(bindingDescriptions.Length());
		vertexStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32>(vertexShader->Attributes.Length());

		constexpr VkPrimitiveTopology topology[Topology_Type_Max] = {
			VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
			VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
			VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = topology[Pipeline.Topology];
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Viewport and scissoring.
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float32>(Ctx.Swapchain.Extent.width);
		viewport.height = static_cast<float32>(Ctx.Swapchain.Extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = Ctx.Swapchain.Extent;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pScissors = &scissor;
		viewportStateCreateInfo.scissorCount = 1;

		// Dynamic state description.
		// TODO(Ygsm):
		// Study more about this.
		VkDynamicState dynamicStates[1] = { VK_DYNAMIC_STATE_VIEWPORT };
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = 1;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		constexpr VkPolygonMode polyMode[Polygon_Mode_Max] = {
			VK_POLYGON_MODE_FILL,
			VK_POLYGON_MODE_LINE,
			VK_POLYGON_MODE_POINT
		};

		constexpr VkFrontFace frontFace[Front_Face_Max] = {
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
			VK_FRONT_FACE_CLOCKWISE
		};

		constexpr VkCullModeFlags cullMode[Culling_Mode_Max] = {
			VK_CULL_MODE_NONE,
			VK_CULL_MODE_FRONT_BIT,
			VK_CULL_MODE_BACK_BIT,
			VK_CULL_MODE_FRONT_AND_BACK
		};

		// Rasterization state.
		VkPipelineRasterizationStateCreateInfo rasterStateCreateInfo = {};
		rasterStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterStateCreateInfo.polygonMode = polyMode[Pipeline.PolygonalMode];
		rasterStateCreateInfo.cullMode = cullMode[Pipeline.CullMode];
		rasterStateCreateInfo.frontFace = frontFace[Pipeline.FrontFace];
		rasterStateCreateInfo.lineWidth = 1.0f;
		//rasterStateCreateInfo.depthClampEnable = VK_FALSE; // Will be true for shadow rendering for some reason I do not know yet...

		// Multisampling state description
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
		multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		//multisampleCreateInfo.minSampleShading		= 1.0f;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

		// Blending state description.
		// NOTE(Ygsm):
		// Blending attachment state is needed for each COLOUR attachment that's used by the subpass.
		Array<VkPipelineColorBlendAttachmentState> colorBlendStates;
		VkPipelineColorBlendAttachmentState blendState = {};
		blendState.blendEnable = VK_TRUE;
		blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendState.colorBlendOp = VK_BLEND_OP_ADD;
		blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		for (uint32 i = 0; i < Pipeline.ColorOutputCount; i++)
		{
			colorBlendStates.Push(blendState);
		}

		if (Pipeline.HasDefaultColorOutput)
		{
			colorBlendStates.Push(blendState);
		}

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
		colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.attachmentCount = static_cast<uint32>(colorBlendStates.Length());
		colorBlendCreateInfo.pAttachments = colorBlendStates.First();
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;

		DescriptorLayout* layout = nullptr;
		Array<VkDescriptorSetLayout> descriptorSetLayouts;

		for (DescriptorLayout* layout : Pipeline.DescriptorLayouts)
		{
			descriptorSetLayouts.Push(g_DescriptorSetLayout[layout->Handle]);
		}

		VkPushConstantRange pushConstant = {};
		pushConstant.offset = 0;
		pushConstant.size = Ctx.Properties.limits.maxPushConstantsSize;
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		// Pipeline layour description.
		VkPipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pPushConstantRanges = &pushConstant;
		layoutCreateInfo.pushConstantRangeCount = 1;

		if (descriptorSetLayouts.Length())
		{
			layoutCreateInfo.setLayoutCount = static_cast<uint32>(descriptorSetLayouts.Length());
			layoutCreateInfo.pSetLayouts = descriptorSetLayouts.First();
		}

		VkPipelineLayout pipelineLayout;
		if (vkCreatePipelineLayout(Ctx.Device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create pipeline layout!=" && false);
			return false;
		}

		VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
		depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilCreateInfo.minDepthBounds = 0.0f;
		depthStencilCreateInfo.maxDepthBounds = 1.0f;
		depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;

		if (Pipeline.HasDepthStencil)
		{
			depthStencilCreateInfo.depthTestEnable = VK_TRUE;
			depthStencilCreateInfo.depthWriteEnable = VK_TRUE;

			//
			// TODO(Ygsm):
			// Need to revise depth stencil configuration in the pipeline.
			//

			//depthStencilCreateInfo.stencilTestEnable = VK_TRUE;
			//depthStencilCreateInfo.front = {};
			//depthStencilCreateInfo.back = {};
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = static_cast<uint32>(pipelineStages.Length());
		pipelineCreateInfo.pStages = pipelineStages.First();
		pipelineCreateInfo.pVertexInputState = &vertexStateCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pTessellationState = nullptr;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
		//pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = g_RenderPass[Pipeline.Renderpass->RenderpassHandle];
		pipelineCreateInfo.subpass = 0;							// TODO(Ygsm): Study more about this!
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// TODO(Ygsm): Study more about this!
		pipelineCreateInfo.basePipelineIndex = -1;				// TODO(Ygsm): Study more about this!

		if (Pipeline.HasDepthStencil)
		{
			pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
		}

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(Ctx.Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create graphics pipeline!" && false);
			return false;
		}

		uint32 key = g_RandIdVk();
		Pipeline.Handle = Handle<HPipeline>(key);
		auto& pipelineParam = g_Pipelines.Insert(key, {});
		pipelineParam.Handle = pipeline;
		pipelineParam.Layout = pipelineLayout;

		return true;
	}

	void DestroyGraphicsPipeline(SRPipeline& Pipeline)
	{
		VulkanPipeline& pipeline = g_Pipelines[Pipeline.Handle];

		vkDeviceWaitIdle(Ctx.Device);
		vkDestroyPipelineLayout(Ctx.Device, pipeline.Layout, nullptr);
		vkDestroyPipeline(Ctx.Device, pipeline.Handle, nullptr);

		g_Pipelines.Remove(Pipeline.Handle);
		Pipeline.Handle = INVALID_HANDLE;
	}

	bool CreateFramebuffer(RenderPass& Pass)
	{
		VkRenderPass& renderPass = g_RenderPass[Pass.RenderpassHandle];

		const uint32 width	= (!Pass.Width)  ? Ctx.Swapchain.Extent.width  : static_cast<uint32>(Pass.Width);
		const uint32 height = (!Pass.Height) ? Ctx.Swapchain.Extent.height : static_cast<uint32>(Pass.Height);
		const uint32 depth  = (!Pass.Depth)  ? 1 : static_cast<uint32>(Pass.Depth);

		size_t totalOutputs = (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output)) ?
			Pass.ColorOutputs.Length() + 1 : Pass.ColorOutputs.Length();

		Array<VkImageView> imageViews(totalOutputs);

		for (auto& pair : Pass.ColorOutputs)
		{
			RenderPass::AttachmentInfo& attachment = pair.Value;
			Texture texture;
			texture.Width = width;
			texture.Height = height;
			texture.Channels = 4;
			texture.Usage.Set(Image_Usage_Sampled);
			texture.Usage.Set(Image_Usage_Color_Attachment);

			CreateTexture(texture);
			attachment.Handle = texture.Handle;
			auto& imageParams = g_Textures[texture.Handle];
			imageViews.Push(imageParams.View);
		}

		if (!Pass.Flags.Has(RenderPass_Bit_No_Color_Render))
		{
			auto& imageParams = g_Textures[Pass.Owner.GetColorImage()];
			imageViews.Push(imageParams.View);
		}

		if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output))
		{
			Texture depthStencil;
			depthStencil.Width = width;
			depthStencil.Height = height;
			depthStencil.Channels = 4;
			depthStencil.Usage.Set(Image_Usage_Sampled);
			depthStencil.Usage.Set(Image_Usage_Depth_Stencil_Attachment);

			CreateTexture(depthStencil);
			Pass.DepthStencilOutput.Handle = depthStencil.Handle;
			auto& imageParams = g_Textures[depthStencil.Handle];
			imageViews.Push(imageParams.View);
		}
		else
		{
			if (!Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
			{
				auto& imageParams = g_Textures[Pass.Owner.GetDepthStencilImage()];
				imageViews.Push(imageParams.View);
			}
		}

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;
		framebufferInfo.pAttachments = imageViews.First();
		framebufferInfo.attachmentCount = static_cast<uint32>(imageViews.Length());
		framebufferInfo.renderPass = renderPass;

		VulkanFramebuffer framebuffer = {};
		for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
		{
			if (vkCreateFramebuffer(Ctx.Device, &framebufferInfo, nullptr, &framebuffer.Framebuffers[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Unable to create framebuffer" && false);
				return false;
			}
		}

		uint32 id = g_RandIdVk();
		Pass.FramebufferHandle = id;
		g_Framebuffer.Insert(id, framebuffer);

		return true;
	}

	void DestroyFramebuffer(RenderPass& Pass)
	{
		VulkanFramebuffer& framebuffer = g_Framebuffer[Pass.FramebufferHandle];

		vkDeviceWaitIdle(Ctx.Device);
		for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
		{
			vkDestroyFramebuffer(Ctx.Device, framebuffer.Framebuffers[i], nullptr);
			framebuffer.Framebuffers[i] = VK_NULL_HANDLE;
		}
		g_Framebuffer.Remove(Pass.FramebufferHandle);
		Pass.FramebufferHandle = INVALID_HANDLE;
	}

	bool CreateRenderpass(RenderPass& Pass)
	{
		uint32 offset = 0;
		uint32 colorAttCount = 0;
		const uint32 numPasses = Pass.Owner.GetNumRenderPasses();

		Array<VkAttachmentDescription> descriptions;
		Array<VkAttachmentReference> references;
		//Array<VkSubpassDependency> dependencies;

		if (!Pass.Flags.Has(RenderPass_Bit_No_Color_Render))
		{
			VkAttachmentDescription& desc = descriptions.Insert(VkAttachmentDescription());
			desc.format = VK_FORMAT_R8G8B8A8_SRGB;
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			if (!Pass.Order)
			{
				desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			if (Pass.Order == numPasses - 1)
			{
				desc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			}

			if ((Pass.Order > 0) && 
				(Pass.Order < numPasses - 1))
			{
				desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			VkAttachmentReference& ref = references.Insert(VkAttachmentReference());
			ref.attachment = offset++;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			//VkSubpassDependency& dep = dependencies.Insert(VkSubpassDependency());
			//dep.srcSubpass = VK_SUBPASS_EXTERNAL;
			//dep.dstSubpass = 0;
			//dep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			//dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			//dep.srcAccessMask = 0;
			//dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			//dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			colorAttCount++;
		}

		constexpr VkSampleCountFlagBits sampleBits[Sample_Count_Max] = {
			VK_SAMPLE_COUNT_1_BIT,
			VK_SAMPLE_COUNT_2_BIT,
			VK_SAMPLE_COUNT_4_BIT,
			VK_SAMPLE_COUNT_8_BIT,
			VK_SAMPLE_COUNT_16_BIT,
			VK_SAMPLE_COUNT_32_BIT,
			VK_SAMPLE_COUNT_64_BIT
		};

		for (size_t i = 0; i < Pass.ColorOutputs.Length(); i++, offset++)
		{
			VkAttachmentDescription& desc = descriptions.Insert(VkAttachmentDescription());
			desc.format = VK_FORMAT_R8G8B8A8_SRGB;
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference& ref = references.Insert(VkAttachmentReference());
			ref.attachment = offset;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			//VkSubpassDependency& dep = dependencies.Insert(VkSubpassDependency());
			//dep.srcSubpass = VK_SUBPASS_EXTERNAL;
			//dep.dstSubpass = 0;
			//dep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			//dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			////dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			//dep.srcAccessMask = 0;
			//dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			//dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			colorAttCount++;
		}

		// NOTE(Ygsm):
		// Since framebuffers only allow a single depth stencil attachment, we have to make a choice.
		// By default the default output's depth attachment is used but if it has it's own depth stencil attachment,
		// we set up that instead.

		// TODO(Ygsm):
		// The current set up does not include depth attachment that is used in subsequent passes as input.
		if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output) || 
			!Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
		{
			VkAttachmentDescription& desc = descriptions.Insert(VkAttachmentDescription());
			desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output))
			{
				desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			VkAttachmentReference& ref = references.Insert(VkAttachmentReference());
			ref.attachment = offset++;
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			//VkSubpassDependency* dep = (!Pass.Flags.Has(RenderPass_Bit_No_Color_Render)) ?
			//	&dependencies[0] : &dependencies.Insert(VkSubpassDependency());

			//dep->srcSubpass = VK_SUBPASS_EXTERNAL;
			//dep->dstSubpass = 0;
			//dep->srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			//dep->dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			//dep->srcAccessMask = 0;
			//dep->dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			//dep->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		}

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.pColorAttachments = references.First();
		subpassDescription.colorAttachmentCount = colorAttCount;

		if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output) ||
			!Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
		{
			subpassDescription.pDepthStencilAttachment = &references[offset - 1];
		}

		// Create actual renderpass.
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pAttachments = descriptions.First();
		renderPassInfo.attachmentCount = static_cast<uint32>(descriptions.Length());
		//renderPassInfo.pDependencies = dependencies.First();
		//renderPassInfo.dependencyCount = static_cast<uint32>(dependencies.Length());		
		renderPassInfo.pDependencies = nullptr;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.subpassCount = 1;

		VkRenderPass renderPass;

		if (vkCreateRenderPass(Ctx.Device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create render pass." && false);
			return false;
		}

		uint32 id = g_RandIdVk();
		Pass.RenderpassHandle = id;
		g_RenderPass.Insert(id, renderPass);

		return true;
	}

	void DestroyRenderpass(RenderPass& Pass)
	{
		VkRenderPass& renderPass = g_RenderPass[Pass.RenderpassHandle];
		vkDeviceWaitIdle(Ctx.Device);
		vkDestroyRenderPass(Ctx.Device, renderPass, nullptr);
		g_RenderPass.Remove(Pass.RenderpassHandle);
		Pass.RenderpassHandle = INVALID_HANDLE;
	}

	void CreateDescriptorPool(DescriptorPool& Pool)
	{
		uint32 maxSets = 0;
		constexpr VkDescriptorType types[Descriptor_Type_Max] = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_SAMPLER
		};

		Array<VkDescriptorPoolSize> poolSizes(Pool.Sizes.Length());

		for (auto& pair : Pool.Sizes)
		{
			VkDescriptorPoolSize& size = poolSizes.Insert(VkDescriptorPoolSize());
			size.type = types[pair.Key];
			size.descriptorCount = pair.Value;
			maxSets += size.descriptorCount;
		}

		VkDescriptorPoolCreateInfo info = {}; 
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets = maxSets;
		info.pPoolSizes = poolSizes.First();
		info.poolSizeCount = static_cast<uint32>(poolSizes.Length());

		VkDescriptorPool pool[MAX_FRAMES_IN_FLIGHT];
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkCreateDescriptorPool(Ctx.Device, &info, nullptr, &pool[i]);
		}

		uint32 id = g_RandIdVk();
		Pool.Handle = Handle<HSetPool>(id);
		VulkanDescriptorPool& descriptorPool = g_DescriptorPools.Insert(id, {});

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descriptorPool.Pool[i] = pool[i];
		}
	}

	void DestroyDescriptorPool(DescriptorPool& Pool)
	{
		vkDeviceWaitIdle(Ctx.Device);
		VulkanDescriptorPool& descriptorPool = g_DescriptorPools[Pool.Handle];
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyDescriptorPool(Ctx.Device, descriptorPool.Pool[i], nullptr);
		}
		g_DescriptorPools.Remove(Pool.Handle);
		Pool.Handle = INVALID_HANDLE;
	}

	bool CreateDescriptorSetLayout(DescriptorLayout& Layout)
	{
		constexpr VkDescriptorType types[Descriptor_Type_Max] = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_SAMPLER
		};

		Array<VkDescriptorSetLayoutBinding> layoutBindings(Layout.Bindings.Length());

		for (DescriptorBinding& b : Layout.Bindings)
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = b.Binding;
			binding.descriptorCount = b.DescriptorCount;
			binding.descriptorType = types[b.Type];

			if (b.ShaderStages.Has(Shader_Type_Vertex))
			{
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			}

			if (b.ShaderStages.Has(Shader_Type_Fragment))
			{
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			layoutBindings.Push(binding);
		}

		VkDescriptorSetLayoutCreateInfo descLayoutCreateInfo = {};
		descLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descLayoutCreateInfo.bindingCount = static_cast<uint32>(layoutBindings.Length());
		descLayoutCreateInfo.pBindings = layoutBindings.First();

		VkDescriptorSetLayout setLayout;

		if (vkCreateDescriptorSetLayout(Ctx.Device, &descLayoutCreateInfo, nullptr, &setLayout) != VK_SUCCESS)
		{
			return false;
		}

		uint32 id = g_RandIdVk();
		Layout.Handle = Handle<HSetLayout>(id);
		g_DescriptorSetLayout.Insert(id, setLayout);

		return true;
	}

	void DestroyDescriptorSetLayout(DescriptorLayout& Layout)
	{
		vkDeviceWaitIdle(Ctx.Device);
		VkDescriptorSetLayout& layout = g_DescriptorSetLayout[Layout.Handle];
		vkDestroyDescriptorSetLayout(Ctx.Device, layout, nullptr);
		g_DescriptorSetLayout.Remove(Layout.Handle);
		Layout.Handle = INVALID_HANDLE;
	}

	bool AllocateDescriptorSet(DescriptorSet& Set)
	{
		//constexpr VkDescriptorType types[Descriptor_Type_Max] = {
		//	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		//	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		//	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		//	VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		//	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		//	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
		//	VK_DESCRIPTOR_TYPE_SAMPLER
		//};

		VulkanDescriptorPool& poolParams = g_DescriptorPools[Set.Pool->Handle];
		VkDescriptorSetLayout layout = g_DescriptorSetLayout[Set.Layout->Handle];

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			allocInfo.descriptorPool = poolParams.Pool[i];
			if (vkAllocateDescriptorSets(Ctx.Device, &allocInfo, &descriptorSets[i]) != VK_SUCCESS)
			{
				return false;
			}
		}

		uint32 id = g_RandIdVk();
		Set.Handle = Handle<HSet>(id);
		VulkanDescriptorSet& descSetParams = g_DescriptorSets.Insert(id, {});

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descSetParams.Set[i] = descriptorSets[i];
		}

		return true;
	}

	void UpdateDescriptorSet(DescriptorSet& Set, DescriptorBinding& Binding, SRMemoryBuffer& Buffer)
	{
		constexpr VkDescriptorType types[Descriptor_Type_Max] = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_SAMPLER
		};

		VulkanDescriptorSet& descriptorSet = g_DescriptorSets[Set.Handle];

		VulkanBuffer& buffer = g_Buffers[Buffer.Handle];
		const size_t maxDeviceRange = static_cast<size_t>(MaxUniformBufferRange());
		const size_t totalSize = Binding.Size * MAX_FRAMES_IN_FLIGHT;

		VKT_ASSERT((totalSize <= maxDeviceRange) &&
			"Uniform buffer range cannot exceed device limit. Consider using storage buffers!");

		VkDescriptorBufferInfo info = {};
		info.buffer = buffer.Handle;
		info.offset = Buffer.Offset;
		info.range = totalSize;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			Binding.Offset[i] = Buffer.Offset + (Binding.Size * i);
		}
		Buffer.Offset += totalSize;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = Binding.Binding;
		// descriptorCount specifies the number of elements for a single binding.
		write.descriptorCount = 1;
		//write.descriptorCount = Binding.DescriptorCount;
		write.descriptorType = types[Binding.Type];
		write.pBufferInfo = &info;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = descriptorSet.Set[i];
			vkUpdateDescriptorSets(Ctx.Device, 1, &write, 0, nullptr);
		}
	}

	void UpdateDescriptorSetImage(DescriptorSet& Set, DescriptorBinding& Binding, Texture** Textures, uint32 Count)
	{
		constexpr VkDescriptorType types[Descriptor_Type_Max] = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_SAMPLER
		};

		Array<VkDescriptorImageInfo> imageInfos;
		VulkanImage* image = nullptr;
		VkSampler* sampler = nullptr;

		for (uint32 i = 0; i < Count; i++)
		{
			image = &g_Textures[Textures[i]->Handle];
			sampler = &g_Samplers[Textures[i]->Sampler->Handle];

			imageInfos.Push({
				*sampler,
				image->View,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});
		}

		VulkanDescriptorSet& descriptorSet = g_DescriptorSets[Set.Handle];

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = Binding.Binding;
		write.descriptorCount = Count;
		write.descriptorType = types[Binding.Type];
		write.pImageInfo = imageInfos.First();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = descriptorSet.Set[i];
			vkUpdateDescriptorSets(Ctx.Device, 1, &write, 0, nullptr);
		}
	}

	void UpdateDescriptorSetSampler(DescriptorSet& Set, DescriptorBinding& Binding, ImageSampler& Sampler)
	{
		constexpr VkDescriptorType types[Descriptor_Type_Max] = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
			VK_DESCRIPTOR_TYPE_SAMPLER
		};

		VkDescriptorImageInfo samplerInfo = {};
		samplerInfo.sampler = g_Samplers[Sampler.Handle];

		VulkanDescriptorSet& descriptorSet = g_DescriptorSets[Set.Handle];

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = Binding.Binding;
		write.descriptorCount = 1;
		write.descriptorType = types[Binding.Type];
		write.pImageInfo = &samplerInfo;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = descriptorSet.Set[i];
			vkUpdateDescriptorSets(Ctx.Device, 1, &write, 0, nullptr);
		}
	}

	void BindDescriptorSet(DescriptorSet& Set)
	{
		GET_CURR_CMD_BUFFER();
		const uint32 currFrameIdx = CurrentFrameIndex();
		VulkanPipeline& pipeline = g_Pipelines[*Set.Layout->Pipeline];
		VulkanDescriptorSet& descriptorSet = g_DescriptorSets[Set.Handle];
		static Array<uint32> offsets;

		for (DescriptorBinding& b : Set.Layout->Bindings)
		{
			if (b.Type != Descriptor_Type_Dynamic_Uniform_Buffer) { continue; }
			offsets.Push(static_cast<uint32>(b.Offset[currFrameIdx]));
		}

		vkCmdBindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline.Layout,
			Set.Slot,
			1,
			&descriptorSet.Set[CurrentFrameIndex()],
			static_cast<uint32>(offsets.Length()),
			offsets.First()
		);

		offsets.Empty();
	}

	bool CreateTexture(Texture& InTexture)
	{
		constexpr VkImageUsageFlags imgUsageFlags[] = {
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		};

		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
		switch (InTexture.Channels)
		{
		case 1:
			format = VK_FORMAT_R8_SRGB;
			break;
		case 2:
			format = VK_FORMAT_R8G8_SRGB;
			break;
		//case 3:
		//	format = VK_FORMAT_R8G8B8_SRGB;
		//	break;
		default:
			break;
		}

		if (InTexture.Usage.Has(Image_Usage_Depth_Stencil_Attachment))
		{
			format = VK_FORMAT_D24_UNORM_S8_UINT;
		}

		VkImageUsageFlags usageFlags = 0;
		for (uint32 i = 0; i < Image_Usage_Max; i++)
		{
			if (!InTexture.Usage.Has(i)) { continue; }
			usageFlags |= imgUsageFlags[i];
		}

		VkImageCreateInfo img = {};
		img.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		img.imageType = VK_IMAGE_TYPE_2D;
		img.format = format;
		img.samples = VK_SAMPLE_COUNT_1_BIT;
		img.extent = { InTexture.Width, InTexture.Height, 1 };
		img.tiling = VK_IMAGE_TILING_OPTIMAL;
		img.mipLevels = 1;
		img.usage = usageFlags;
		//img.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		img.arrayLayers = 1;
		img.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		img.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImageViewCreateInfo imgView = {};
		imgView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgView.format = format;
		imgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgView.subresourceRange.baseMipLevel = 0;
		imgView.subresourceRange.levelCount = 1;
		imgView.subresourceRange.baseArrayLayer = 0;
		imgView.subresourceRange.layerCount = 1;
		imgView.viewType = VK_IMAGE_VIEW_TYPE_2D;

		//if (InTexture.Usage.Has(Image_Usage_Color_Attachment))
		//{
		//	imgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//}

		if (InTexture.Usage.Has(Image_Usage_Depth_Stencil_Attachment))
		{
			imgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		//VkSamplerCreateInfo sampler = {};
		//sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		//sampler.magFilter = VK_FILTER_NEAREST;
		//sampler.minFilter = VK_FILTER_NEAREST;
		//sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		//sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		//sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		Handle<HImage> imgHandle = vk::CreateImage(img, imgView/*, sampler*/);

		if (imgHandle == INVALID_HANDLE)
		{
			return false;
		}

		InTexture.Handle = imgHandle;

		return true;
	}

	bool BeginTransfer()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		return vkBeginCommandBuffer(Ctx.TransferOp.TransferCmdBuffer, &beginInfo) == VK_SUCCESS;
	}

	bool BeginOwnershipTransfer()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		return vkBeginCommandBuffer(Ctx.TransferOp.GraphicsCmdBuffer, &beginInfo) == VK_SUCCESS;
	}

	void EndTransfer(bool Signal)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &Ctx.TransferOp.TransferCmdBuffer;

		if (Signal)
		{
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &Ctx.TransferOp.Semaphore;
		}

		vkEndCommandBuffer(Ctx.TransferOp.TransferCmdBuffer);
		vkQueueSubmit(Ctx.TransferOp.Queue.Handle, 1, &submitInfo, VK_NULL_HANDLE);
		//vkQueueSubmit(Ctx.TransferOp.Queue.Handle, 1, &submitInfo, Ctx.TransferOp.Fence);
		//vkWaitForFences(Ctx.Device, 1, &Ctx.TransferOp.Fence, VK_TRUE, UINT64_MAX);
		//vkResetFences(Ctx.Device, 1, &Ctx.TransferOp.Fence);
	}

	void EndOwnershipTransfer()
	{
		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &Ctx.TransferOp.GraphicsCmdBuffer;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &Ctx.TransferOp.Semaphore;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;

		vkEndCommandBuffer(Ctx.TransferOp.GraphicsCmdBuffer);
		vkQueueSubmit(Ctx.GraphicsQueue.Handle, 1, &submitInfo, VK_NULL_HANDLE);
		//vkQueueSubmit(Ctx.GraphicsQueue.Handle, 1, &submitInfo, Ctx.TransferOp.Fence);
		//vkWaitForFences(Ctx.Device, 1, &Ctx.TransferOp.Fence, VK_TRUE, UINT64_MAX);
		//vkResetFences(Ctx.Device, 1, &Ctx.TransferOp.Fence);
	}

	void TransferBuffer(SRMemoryTransferContext& TransferContext)
	{
		VulkanBuffer& src = g_Buffers[TransferContext.SrcBuffer.Handle];
		VulkanBuffer& dst = g_Buffers[TransferContext.DstBuffer->Handle];

		VkBufferCopy copy = {};
		copy.srcOffset = TransferContext.SrcOffset;
		copy.size = TransferContext.SrcSize;
		copy.dstOffset = TransferContext.DstOffset;

		vkCmdCopyBuffer(
			Ctx.TransferOp.TransferCmdBuffer,
			src.Handle,
			dst.Handle,
			1,
			&copy
		);

		VkBufferMemoryBarrier transferBarrier = {};

		transferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		transferBarrier.buffer = dst.Handle;
		transferBarrier.size = TransferContext.SrcSize;
		transferBarrier.offset = TransferContext.DstOffset;
		transferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		transferBarrier.srcQueueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;
		transferBarrier.dstQueueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
		
		vkCmdPipelineBarrier(
			Ctx.TransferOp.TransferCmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			1,
			&transferBarrier,
			0,
			nullptr
		);
	}

	void TransferTexture(SRTextureTransferContext& TransferContext)
	{
		VulkanBuffer& buffer = g_Buffers[TransferContext.Buffer.Handle];
		VulkanImage& img = g_Textures[TransferContext.Texture->Handle];

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		VkImageMemoryBarrier transferBarrier = {};
		transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		transferBarrier.image = img.Handle;
		transferBarrier.subresourceRange = subresourceRange;
		transferBarrier.srcAccessMask = 0;
		transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		transferBarrier.srcQueueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;
		transferBarrier.dstQueueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;

		vkCmdPipelineBarrier(
			Ctx.TransferOp.TransferCmdBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&transferBarrier
		);

		uint32 width = TransferContext.Texture->Width;
		uint32 height = TransferContext.Texture->Height;

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			Ctx.TransferOp.TransferCmdBuffer,
			buffer.Handle,
			img.Handle,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion
		);

		VkImageMemoryBarrier readBarrier = transferBarrier;
		readBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		readBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		readBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		readBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		readBarrier.srcQueueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;
		readBarrier.dstQueueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;

		vkCmdPipelineBarrier(
			Ctx.TransferOp.TransferCmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&readBarrier
		);
	}

	void TransferTextureOwnership(Texture& InTexture)
	{
		VulkanImage& img = g_Textures[InTexture.Handle];

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		VkImageMemoryBarrier ownershipTransfer = {};
		ownershipTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ownershipTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		ownershipTransfer.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ownershipTransfer.image = img.Handle;
		ownershipTransfer.subresourceRange = subresourceRange;
		ownershipTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		ownershipTransfer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		ownershipTransfer.srcQueueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
		ownershipTransfer.dstQueueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;

		vkCmdPipelineBarrier(
			Ctx.TransferOp.GraphicsCmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&ownershipTransfer
		);
	}

	void TransferBufferOwnership(SRMemoryBuffer& InBuffer)
	{
		VulkanBuffer& buffer = g_Buffers[InBuffer.Handle];

		VkBufferMemoryBarrier ownershipTransfer = {};

		ownershipTransfer.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		ownershipTransfer.buffer = buffer.Handle;
		ownershipTransfer.srcQueueFamilyIndex = Ctx.TransferOp.Queue.FamilyIndex;
		ownershipTransfer.dstQueueFamilyIndex = Ctx.GraphicsQueue.FamilyIndex;
		ownershipTransfer.offset = 0;
		ownershipTransfer.size = InBuffer.Size;
		ownershipTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		ownershipTransfer.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		if (InBuffer.Type.Has(Buffer_Type_Vertex))
		{
			ownershipTransfer.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		}

		if (InBuffer.Type.Has(Buffer_Type_Index))
		{
			ownershipTransfer.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
		}

		vkCmdPipelineBarrier(
			Ctx.TransferOp.GraphicsCmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0,
			0,
			nullptr,
			1,
			&ownershipTransfer,
			0,
			nullptr
		);
	}

	void DestroyTexture(Texture& InTexture)
	{
		VulkanImage& img = g_Textures[InTexture.Handle];
		vkDeviceWaitIdle(Ctx.Device);
		//vkDestroySampler(Ctx.Device, img.Sampler, nullptr);
		vkDestroyImageView(Ctx.Device, img.View, nullptr);
		vmaDestroyImage(Ctx.Allocator, img.Handle, img.Allocation);
		g_Textures.Remove(InTexture.Handle);
		InTexture.Handle = INVALID_HANDLE;
	}

	bool CreateSampler(ImageSampler& Sampler)
	{
		// TODO(Ygsm):
		// Figure out mip maps...
		//

		constexpr VkFilter imageFilters[] = {
			VK_FILTER_NEAREST,
			VK_FILTER_LINEAR,
			VK_FILTER_CUBIC_IMG
		};

		constexpr VkSamplerAddressMode addressMode[] = {
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
			VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
		};

		constexpr VkCompareOp compareOps[] = {
			VK_COMPARE_OP_NEVER,
			VK_COMPARE_OP_LESS,
			VK_COMPARE_OP_EQUAL,
			VK_COMPARE_OP_LESS_OR_EQUAL,
			VK_COMPARE_OP_GREATER,
			VK_COMPARE_OP_NOT_EQUAL,
			VK_COMPARE_OP_GREATER_OR_EQUAL,
			VK_COMPARE_OP_ALWAYS
		};

		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.minFilter = imageFilters[Sampler.MinFilter];
		createInfo.magFilter = imageFilters[Sampler.MagFilter];
		createInfo.addressModeU = addressMode[Sampler.AddressModeU];
		createInfo.addressModeV = addressMode[Sampler.AddressModeV];
		createInfo.addressModeW = addressMode[Sampler.AddressModeW];
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

		if (!Sampler.AnisotropyLvl)
		{
			const float32 hardwareAnisotropyLimit = Ctx.Properties.limits.maxSamplerAnisotropy;
			createInfo.anisotropyEnable = VK_TRUE;
			createInfo.maxAnisotropy = (Sampler.AnisotropyLvl > hardwareAnisotropyLimit) ? hardwareAnisotropyLimit : Sampler.AnisotropyLvl;
		}

		if (Sampler.CompareOp)
		{
			createInfo.compareEnable = VK_TRUE;
			createInfo.compareOp = compareOps[Sampler.CompareOp];
		}

		VkSampler sampler;
		if (vkCreateSampler(Ctx.Device, &createInfo, nullptr, &sampler) != VK_SUCCESS)
		{
			return false;
		}

		uint32 id = g_RandIdVk();
		Sampler.Handle = id;
		g_Samplers.Insert(id, sampler);

		return true;
	}

	void DestroySampler(ImageSampler& Sampler)
	{
		VkSampler& sampler = g_Samplers[Sampler.Handle];

		vkDeviceWaitIdle(Ctx.Device);
		vkDestroySampler(Ctx.Device, sampler, nullptr);
		g_Samplers.Remove(Sampler.Handle);
		Sampler.Handle = INVALID_HANDLE;
	}

	void SubmitPushConstant(SRPushConstant& PushConstant)
	{
		GET_CURR_CMD_BUFFER();
		VulkanPipeline& pipeline = g_Pipelines[PushConstant.Pipeline->Handle];
		vkCmdPushConstants(
			cmd, 
			pipeline.Layout, 
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
			0, 
			Push_Constant_Size, 
			&PushConstant.Data
		);
	}

	bool CreateBuffer(SRMemoryBuffer& Buffer, void* Data, size_t Size)
	{
		constexpr VkBufferUsageFlagBits bufferTypes[Buffer_Type_Max] = {
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};

		constexpr VmaMemoryUsage memoryUsage[Buffer_Locality_Max] = {
			VMA_MEMORY_USAGE_CPU_ONLY,
			VMA_MEMORY_USAGE_GPU_ONLY,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		};

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = Buffer.Size;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		for (uint32 i = 0; i < Buffer_Type_Max; i++)
		{
			if (!Buffer.Type.Has(i)) { continue; }
			bufferCreateInfo.usage |= bufferTypes[i];
		}

		VkBuffer bufferHnd;
		VmaAllocation allocation;
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = memoryUsage[Buffer.Locality];

		if (vmaCreateBuffer(
			Ctx.Allocator, 
			&bufferCreateInfo, 
			&allocCreateInfo, 
			&bufferHnd,
			&allocation, 
			nullptr
		) != VK_SUCCESS)
		{
			return false;
		}

		uint32 id = g_RandIdVk();
		Buffer.Handle = Handle<HBuffer>(id);
		VulkanBuffer& buf = g_Buffers.Insert(id, {});
		buf.Allocation = allocation;
		buf.Handle = bufferHnd;

		if (Buffer.Locality != Buffer_Locality_Gpu)
		{
			vmaMapMemory(Ctx.Allocator, buf.Allocation, reinterpret_cast<void**>(&buf.pData));
			vmaUnmapMemory(Ctx.Allocator, buf.Allocation);
		}

		if (Data && Size) { CopyToBuffer(Buffer, Data, Size); }

		return true;
	}

	bool CopyToBuffer(SRMemoryBuffer& Buffer, void* Data, size_t Size)
	{
		if (Buffer.Handle == INVALID_HANDLE) 
		{ 
			return false; 
		}

		VulkanBuffer& buffer = g_Buffers[Buffer.Handle];
		buffer.pData += Buffer.Offset;
		FMemory::Memcpy(buffer.pData, Data, Size);
		Buffer.Offset += Size;

		return true;
	}

	bool CopyToBuffer(SRMemoryBuffer& Buffer, void* Data, size_t Size, size_t Offset)
	{
		if (Buffer.Handle == INVALID_HANDLE)
		{
			return false;
		}

		VulkanBuffer& buffer = g_Buffers[Buffer.Handle];
		uint8* ptr = reinterpret_cast<uint8*>(buffer.pData + Offset);
		//buffer.pData += Offset;
		//FMemory::Memzero(buffer.pData, Buffer.Size - Offset);
		FMemory::Memcpy(ptr, Data, Size);
		//Buffer.Offset = Offset + Size;

		return true;
	}

	void DestroyBuffer(SRMemoryBuffer& Buffer)
	{
		VulkanBuffer& buffer = g_Buffers[Buffer.Handle];
		vkDeviceWaitIdle(Ctx.Device);
		vmaDestroyBuffer(Ctx.Allocator, buffer.Handle, buffer.Allocation);
		g_Buffers.Remove(Buffer.Handle);
		Buffer.Handle = INVALID_HANDLE;
	}

	void BeginFrame()
	{
		GET_CURR_CMD_BUFFER();
		//const uint32 currentFrame = CurrentFrameIndex();
		//VkCommandBuffer& cmdBuffer = Ctx.CommandBuffers[currentFrame];

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		vkResetCommandBuffer(cmd, 0);
		vkBeginCommandBuffer(cmd, &beginInfo);
	}

	void EndFrame()
	{
		GET_CURR_CMD_BUFFER();
		vkEndCommandBuffer(cmd);
	}

	void Clear()
	{
		const uint32 currentFrame = CurrentFrameIndex();

		vkWaitForFences(Ctx.Device, 1, &Ctx.Fences[currentFrame], VK_TRUE, UINT64_MAX);
		VkResult result = vkAcquireNextImageKHR(
			Ctx.Device, 
			Ctx.Swapchain.Handle, 
			UINT64_MAX, 
			Ctx.Semaphores[currentFrame][Semaphore_Type_Image_Available], 
			VK_NULL_HANDLE, 
			&Ctx.NextSwapchainImageIndex
		);

		switch (result)
		{
			case VK_SUCCESS:
			case VK_SUBOPTIMAL_KHR:
				break;
			case VK_ERROR_OUT_OF_DATE_KHR:
				OnWindowResize();
				break;
			default:
				VKT_ASSERT("Problem occured during swap chain image acquisation!" && false);
				Ctx.RenderFrame = false;
				return;
		}

		Ctx.RenderFrame = true;

		//for (auto& pair : Ctx.Store.FramePasses)
		//{
		//	auto& frameParams = pair.Value;
		//	VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];

		//	if (cmdBuffer == VK_NULL_HANDLE) { continue; }

		//	vkResetCommandBuffer(cmdBuffer, 0);
		//}
	}

	void Flush()
	{
		vkDeviceWaitIdle(Ctx.Device);
		vkDestroyCommandPool(Ctx.Device, Ctx.CommandPool, nullptr);
		Ctx.CommandPool = VK_NULL_HANDLE;

		VulkanFramebuffer& framebuffer = Ctx.DefautltFramebuffer;
		for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
		{
			vkDestroyFramebuffer(Ctx.Device, framebuffer.Framebuffers[i], nullptr);
			framebuffer.Framebuffers[i] = VK_NULL_HANDLE;
		}
	}

	void SwapBuffers()
	{
		const uint32 currentFrame = CurrentFrameIndex();
		const uint32 nextImageIndex = Ctx.NextSwapchainImageIndex;

		if (!Ctx.RenderFrame) { return; }

		if (Ctx.ImageFences[nextImageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(Ctx.Device, 1, &Ctx.ImageFences[nextImageIndex], VK_TRUE, UINT64_MAX);
		}

		Ctx.ImageFences[nextImageIndex] = Ctx.Fences[currentFrame];

		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		//VkSemaphore semaphore[2] = { Ctx.Semaphores[currentFrame][Semaphore_Type_Image_Available], Ctx.TransferOp.Semaphore };

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &Ctx.Semaphores[currentFrame][Semaphore_Type_Image_Available];
		//submitInfo.waitSemaphoreCount = 2;
		//submitInfo.pWaitSemaphores = semaphore;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.pCommandBuffers = &Ctx.CommandBuffers[currentFrame];
		submitInfo.commandBufferCount = 1;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &Ctx.Semaphores[currentFrame][Semaphore_Type_Render_Complete];
		
		vkResetFences(Ctx.Device, 1, &Ctx.Fences[currentFrame]);

		if (vkQueueSubmit(Ctx.GraphicsQueue.Handle, 1, &submitInfo, Ctx.Fences[currentFrame]) != VK_SUCCESS)
		{
			return;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &Ctx.Semaphores[currentFrame][Semaphore_Type_Render_Complete];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &Ctx.Swapchain.Handle;
		presentInfo.pImageIndices = &Ctx.NextSwapchainImageIndex;

		VkResult result = vkQueuePresentKHR(Ctx.PresentQueue.Handle, &presentInfo);

		Ctx.CurrentFrameIndex = (Ctx.CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

		switch (result)
		{
			case VK_SUCCESS:
				break;
			case VK_ERROR_OUT_OF_DATE_KHR:
			case VK_SUBOPTIMAL_KHR:
				OnWindowResize();
				return;
			default:
				VKT_ASSERT("Problem occured during image presentation!" && false);
				break;
		}
	}

	void OnWindowResize()
	{
		Flush();
		CreateSwapchain();
		CreateDefaultFramebuffer();
		CreateCommandPool();
		AllocateCommandBuffers();
	}

	uint32 SwapchainWidth()
	{
		return Ctx.Swapchain.Extent.width;
	}

	uint32 SwapchainHeight()
	{
		return Ctx.Swapchain.Extent.height;
	}

	uint32 CurrentFrameIndex()
	{
		return Ctx.CurrentFrameIndex;
	}

	uint32 MaxUniformBufferRange()
	{
		return Ctx.Properties.limits.maxUniformBufferRange;
	}

	size_t PadSizeToAlignedSize(size_t Size)
	{
		const size_t minUboAlignment = Ctx.Properties.limits.minUniformBufferOffsetAlignment;
		size_t size = Size;
		if (minUboAlignment)
		{
			size = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		return size;
	}

	namespace vk
	{
		Handle<HImage> CreateImage(VkImageCreateInfo& Img, VkImageViewCreateInfo& ImgView/*, VkSamplerCreateInfo& Sampler*/)
		{
			VkImage imgHandle;
			VkImageView imgViewHandle;
			//VkSampler samplerHandle;
			VmaAllocation allocation;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			if (vmaCreateImage(
				Ctx.Allocator,
				&Img,
				&allocInfo,
				&imgHandle,
				&allocation,
				nullptr
			) != VK_SUCCESS)
			{
				return INVALID_HANDLE;
			}

			ImgView.image = imgHandle;

			if (vkCreateImageView(Ctx.Device, &ImgView, nullptr, &imgViewHandle) != VK_SUCCESS)
			{
				vmaDestroyImage(Ctx.Allocator, imgHandle, allocation);
				return INVALID_HANDLE;
			}

			//if (vkCreateSampler(Ctx.Device, &Sampler, nullptr, &samplerHandle) != VK_SUCCESS)
			//{
			//	vkDestroyImageView(Ctx.Device, imgViewHandle, nullptr);
			//	vmaDestroyImage(Ctx.Allocator, imgHandle, allocation);
			//	return INVALID_HANDLE;
			//}

			uint32 id = g_RandIdVk();
			VulkanImage& img = g_Textures.Insert(id, {});
			img.Allocation = allocation;
			img.Handle = imgHandle;
			img.View = imgViewHandle;
			//img.Sampler = samplerHandle;

			return Handle<HImage>(id);
		}
	}
}
