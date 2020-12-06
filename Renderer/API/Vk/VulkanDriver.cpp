#pragma once
#include "VulkanDriver.h"
#include "Engine/Interface.h"
#include "Library/Random/Xoroshiro.h"
#include "VulkanFunctions.h"

namespace vk
{

#define MAX_PIPELINE_SHADER_STAGE 4
#define MAX_ATTACHMENTS_IN_FRAMEBUFFER 8
#define MAX_GPU_RESOURCE static_cast<size_t>(1024)
#define MAX_GPU_RESOURCE_DBL static_cast<size_t>(2018)

	static Xoroshiro32 g_RandIdVk(OS::GetPerfCounter());

	struct VulkanCompileTimeContext
	{

		OS::DllHandle				VulkanDll;

		struct ImageParam
		{
			VkImage			Image;
			VkImageView		ImageView;
			VkSampler		Sampler;
			VmaAllocation	Allocation;
		};

		struct BufferParam
		{
			VkBuffer		Buffer;
			VmaAllocation	Allocation;
		};

		struct FrameParam
		{
			VkFramebuffer	Framebuffer;
			VkRenderPass	Renderpass;
		};

		struct Storage
		{
			template <typename ResourceType> 
			using StoreContainer	= Map<uint32, ResourceType>;
			using CmdBufferArr		= StaticArray<VkCommandBuffer, static_cast<size_t>(MAX_FRAMES_IN_FLIGHT)>;
			using FramebufferArr	= StaticArray<VkFramebuffer, static_cast<size_t>(MAX_FRAMES_IN_FLIGHT)>;

			StoreContainer<VkShaderModule>	Shaders;
			StoreContainer<VkPipeline>		Pipelines;
			StoreContainer<CmdBufferArr>	CommandBuffers;
			StoreContainer<VkBuffer>		Buffers;
			StoreContainer<FrameParam>		Framebuffers;
			StoreContainer<ImageParam>		Images;

		} Store;

		struct FlagEnums
		{
			VkShaderStageFlagBits ShaderStageFlags[Shader_Type_Max] = { 
				VK_SHADER_STAGE_VERTEX_BIT, 
				VK_SHADER_STAGE_FRAGMENT_BIT 
			};

			VkPrimitiveTopology Topology[Topology_Type_Max] = {
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
				VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
			};

			VkPolygonMode PolygonMode[Polygon_Mode_Max] = {
				VK_POLYGON_MODE_FILL,
				VK_POLYGON_MODE_LINE,
				VK_POLYGON_MODE_POINT
			};

			VkFrontFace FrontFace[Front_Face_Max] = {
				VK_FRONT_FACE_COUNTER_CLOCKWISE,
				VK_FRONT_FACE_CLOCKWISE
			};

			VkCullModeFlagBits CullingMode[Culling_Mode_Max] = {
				VK_CULL_MODE_NONE,
				VK_CULL_MODE_FRONT_BIT,
				VK_CULL_MODE_BACK_BIT,
				VK_CULL_MODE_FRONT_AND_BACK
			};

			VkCommandBufferLevel CmdBufferLevel[2] = {
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				VK_COMMAND_BUFFER_LEVEL_SECONDARY
			};

			VkSampleCountFlagBits SampleCounts[Sample_Count_Max] = {
				VK_SAMPLE_COUNT_1_BIT,
				VK_SAMPLE_COUNT_2_BIT,
				VK_SAMPLE_COUNT_4_BIT,
				VK_SAMPLE_COUNT_8_BIT,
				VK_SAMPLE_COUNT_16_BIT,
				VK_SAMPLE_COUNT_32_BIT,
				VK_SAMPLE_COUNT_64_BIT
			};

			VkImageType ImageType[Attachment_Dimension_Max] = {
				VK_IMAGE_TYPE_1D,
				VK_IMAGE_TYPE_2D,
				VK_IMAGE_TYPE_3D
			};

			VkImageViewType ImageViewType[Attachment_Dimension_Max] = {
				VK_IMAGE_VIEW_TYPE_1D,
				VK_IMAGE_VIEW_TYPE_2D,
				VK_IMAGE_VIEW_TYPE_3D
			};

			VkImageUsageFlagBits ImageUsage[Attachment_Type_Max] = {
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
			};

		} Flags;

#if RENDERER_DEBUG_RENDER_DEVICE
		VkDebugUtilsMessengerEXT DebugMessenger;
#endif
	} Ctx;


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
		Ctx.VulkanDll = OS::LoadDllLibrary("vulkan-1.dll");
		if (!Ctx.VulkanDll)
		{
			// Unable to load vulkan dynamic library;
			VKT_ASSERT(false);
			return false;
		}
		return true;
	}

	bool LoadVulkanModuleAndGlobalFunctions()
	{
#define VK_EXPORTED_FUNCTION(Func)											\
		if (!(Func = (PFN_##Func)OS::LoadProcAddress(Ctx.VulkanDll, #Func)))	\
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
		OS::FreeDllLibrary(Ctx.VulkanDll);
	}

	bool LoadVulkanFunctions(VkInstance& Instance)
	{
#define VK_INSTANCE_LEVEL_FUNCTION(Func)									\
		if (!(Func = (PFN_##Func)vkGetInstanceProcAddr(Instance, #Func)))	\
		{																	\
			VKT_ASSERT(false);												\
			return false;													\
		}
#include "VkFuncDecl.inl"
		return true;
	}

	bool LoadVulkanDeviceFunction(VkDevice& Device)
	{
#define VK_DEVICE_LEVEL_FUNCTION(Func)									\
		if (!(Func = (PFN_##Func)vkGetDeviceProcAddr(Device, #Func)))	\
		{																\
			VKT_ASSERT(false);											\
			return false;												\
		}
#include "VkFuncDecl.inl"
		return true;
	}

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
	{
		FMemory::InitializeObject(CreateInfo);
		CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		CreateInfo.pfnUserCallback = DebugCallback;
	}

	bool CreateDebugMessenger(VkInstance& Instance)
	{
#if RENDERER_DEBUG_RENDER_DEVICE
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (vkCreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &Ctx.DebugMessenger) != VK_SUCCESS)
		{
			VKT_ASSERT("Failed to set up debug messenger" && false);
			return false;
		}
#endif
		return true;
	}

	bool VulkanDriver::CreateVmAllocator()
	{
		VmaAllocatorCreateInfo allocatorInfo;
		FMemory::InitializeObject(allocatorInfo);

		allocatorInfo.physicalDevice	= GPU;
		allocatorInfo.device			= Device;
		allocatorInfo.instance			= Instance;
		allocatorInfo.vulkanApiVersion	= VK_API_VERSION_1_2;

		if (vmaCreateAllocator(&allocatorInfo, &Allocator) != VK_SUCCESS)
		{
			VKT_ASSERT("Failed to create vulkan memory allocator!" && false);
			return false;
		}

		return true;
	}

	bool VulkanDriver::InitializeDriver()
	{
		/**
		* NOTE(Ygsm):
		* Application should terminate if the driver fails to initialize.
		*/

		if (!LoadVulkanLibrary())					return false;
		if (!LoadVulkanModuleAndGlobalFunctions())	return false;
		if (!CreateVulkanInstance())				return false;
		if (!LoadVulkanFunctions(Instance))			return false;

#if RENDERER_DEBUG_RENDER_DEVICE
		if (!CreateDebugMessenger(Instance))		return false;
#endif

		if (!CreatePresentationSurface())			return false;
		if (!ChoosePhysicalDevice())				return false;
		if (!CreateLogicalDevice())					return false;
		if (!LoadVulkanDeviceFunction(Device))		return false;
		if (!GetDeviceQueue())						return false;
		if (!CreateSwapchain())						return false;
		if (!CreateSyncObjects())					return false;
		if (!CreateDefaultRenderPass())				return false;
		if (!CreateVmAllocator())					return false;

		Ctx.Store.CommandBuffers.Reserve(32);
		Ctx.Store.Pipelines.Reserve(		MAX_GPU_RESOURCE);
		Ctx.Store.Framebuffers.Reserve(		MAX_GPU_RESOURCE);
		Ctx.Store.Images.Reserve(			MAX_GPU_RESOURCE);
		Ctx.Store.Shaders.Reserve(		MAX_GPU_RESOURCE_DBL);

		NextImageIndex = 0;
		CurrentFrame = 0;

		return true;
	}

	void VulkanDriver::TerminateDriver()
	{
		Flush();

		Ctx.Store.CommandBuffers.Release();
		Ctx.Store.Shaders.Release();
		Ctx.Store.Pipelines.Release();
		Ctx.Store.Images.Release();
		Ctx.Store.Framebuffers.Release();

		vmaDestroyAllocator(Allocator);

		for (size_t i = 0; i < SwapChain.NumImages; i++)
		{
			vkDestroyImageView(Device, SwapChain.ImageViews[i], nullptr);
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyFence(Device, Fence[i], nullptr);
			for (uint32 j = 0; j < Semaphore_Max; j++)
			{
				vkDestroySemaphore(Device, Semaphores[i][j], nullptr);
			}
		}
		
		if (SwapChain.Handle != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(Device, SwapChain.Handle, nullptr);
		}

		vkDestroyDevice(Device, nullptr);

#if RENDERER_DEBUG_RENDER_DEVICE
		vkDestroyDebugUtilsMessengerEXT(Instance, Ctx.DebugMessenger, nullptr);
#endif
		vkDestroySurfaceKHR(Instance, Surface, nullptr);
		vkDestroyInstance(Instance, nullptr);
		FreeVulkanLibrary();
	}

	void VulkanDriver::Flush()
	{
		vkDeviceWaitIdle(Device);

		if (Ctx.Store.CommandBuffers.Length())
		{
			for (auto& pair : Ctx.Store.CommandBuffers)
			{
				VkCommandBuffer* cmdBuffers = pair.Value.First();
				vkFreeCommandBuffers(Device, CommandPool, MAX_FRAMES_IN_FLIGHT, cmdBuffers);
				pair.Value.Empty();
			}
		}

		if (CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(Device, CommandPool, nullptr);
			CommandPool = VK_NULL_HANDLE;
		}

		if (Ctx.Store.Framebuffers.Length())
		{
			for (auto& pair : Ctx.Store.Framebuffers)
			{
				auto& params = pair.Value;
				VkFramebuffer& framebuffer = params.Framebuffer;

				if (framebuffer == VK_NULL_HANDLE)
				{
					continue;
				}

				vkDestroyFramebuffer(Device, framebuffer, nullptr);
				framebuffer = VK_NULL_HANDLE;
			}
		}
	}

	void VulkanDriver::OnWindowResize()
	{
		Flush();
		CreateSwapchain();
		CreateDefaultFramebuffer();
		CreateCommandPool();
		AllocateCommandBuffers();
	}

	void VulkanDriver::Clear()
	{
		vkWaitForFences(Device, 1, &Fence[CurrentFrame], VK_TRUE, UINT64_MAX);
		VkResult result = vkAcquireNextImageKHR(Device, SwapChain.Handle, UINT64_MAX, Semaphores[CurrentFrame][Semaphore_Type_ImageAvailable], VK_NULL_HANDLE, &NextImageIndex);

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
			RenderFrame = false;
			return;
		}

		RenderFrame = true;
		for (auto pair : Ctx.Store.CommandBuffers)
		{
			auto& cmdBufferArr = pair.Value;
			VkCommandBuffer& cmdBuffer = cmdBufferArr[CurrentFrame];

			if (cmdBuffer == VK_NULL_HANDLE)
			{
				continue;
			}

			vkResetCommandBuffer(cmdBuffer, 0);
		}
	}

	/**
	* Swap buffer will be exclusively used to present images onto screen.
	* There will be a step before swapping buffers that copies the contents of the final renderpass onto the default framebuffer.
	*/
	bool VulkanDriver::SwapBuffers()
	{
		if (!RenderFrame)
		{
			return false;
		}

		if (ImageFences[NextImageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(Device, 1, &ImageFences[NextImageIndex], VK_TRUE, UINT64_MAX);
		}
		ImageFences[NextImageIndex] = Fence[CurrentFrame];

		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo;
		FMemory::InitializeObject(submitInfo);

		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount	= 1;
		submitInfo.pWaitSemaphores		= &Semaphores[CurrentFrame][Semaphore_Type_ImageAvailable];
		submitInfo.pWaitDstStageMask	= &waitDstStageMask;
		submitInfo.pCommandBuffers		= &DefFramebuffer.CmdBuffers[CurrentFrame];
		submitInfo.commandBufferCount	= 1;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores	= &Semaphores[CurrentFrame][Semaphore_Type_RenderComplete];

		vkResetFences(Device, 1, &Fence[CurrentFrame]);

		if (vkQueueSubmit(Queues[Queue_Type_Graphics].Handle, 1, &submitInfo, Fence[CurrentFrame]) != VK_SUCCESS)
		{
			return false;
		}

		VkPresentInfoKHR presentInfo;
		FMemory::InitializeObject(presentInfo);

		presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount	= 1;
		presentInfo.pWaitSemaphores		= &Semaphores[CurrentFrame][Semaphore_Type_RenderComplete];
		presentInfo.swapchainCount		= 1;
		presentInfo.pSwapchains			= &SwapChain.Handle;
		presentInfo.pImageIndices		= &NextImageIndex;
		
		VkResult result = vkQueuePresentKHR(Queues[Queue_Type_Present].Handle, &presentInfo);

		CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		switch (result)
		{
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			OnWindowResize();
			return false;
		default:
			VKT_ASSERT("Problem occured during image presentation!" && false);
			break;
		}

		return true;
	}

	uint32 VulkanDriver::FindMemoryType(uint32 TypeFilter, VkMemoryPropertyFlags Properties)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(GPU, &memoryProperties);

		for (uint32 i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((TypeFilter & (1 << i) && memoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
			{
				return i;
			}
		}

		return -1;
	}

	/**
	* Creates a Vulkan instance.
	*/
	bool VulkanCommon::CreateVulkanInstance()
	{
		VkApplicationInfo appInfo;
		FMemory::InitializeObject(appInfo);

		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pEngineName			= "RenderDevice";
		appInfo.pApplicationName	= "AngkasawanRenderEngine";
		appInfo.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion			= VK_API_VERSION_1_2;

		/**
		* NOTE(Ygsm):
		* Should we check for supported extensions?
		* Do GPU nowadays don't support surface extensions?
		*/

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

		VkInstanceCreateInfo createInfo;
		FMemory::InitializeObject(createInfo);

		createInfo.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo			= &appInfo;
		createInfo.ppEnabledExtensionNames	= extensions;
		createInfo.enabledExtensionCount	= extensionCount;

#if RENDERER_DEBUG_RENDER_DEVICE
		const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
		createInfo.ppEnabledLayerNames	= layers;
		createInfo.enabledLayerCount	= 1;

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

	bool VulkanCommon::ChoosePhysicalDevice()
	{
		static auto CheckPhysicalDeviceProperties = [](
			VkPhysicalDevice& PhysicalDevice,
			VkSurfaceKHR& Surface,
			uint32& GraphicsQueueFamilyIndex,
			uint32& PresentQueueFamilyIndex) -> bool
		{
			uint32 queueFamilyCount = 0;
			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;

			vkGetPhysicalDeviceProperties(PhysicalDevice, &deviceProperties);
			vkGetPhysicalDeviceFeatures(PhysicalDevice, &deviceFeatures);

			uint32 majorVersion = VK_VERSION_MAJOR(deviceProperties.apiVersion);

			if ((majorVersion < 1) && (deviceProperties.limits.maxImageDimension2D < 4096))
			{
				VKT_ASSERT("Physical device doesn't suport required parameters!" && false);
				return false;
			}

			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);
			
			if (!queueFamilyCount)
			{
				// Physical device does not have any queue families!
				return false;
			}

			if (queueFamilyCount > 16)
			{
				// Don't think there will ever be more than 16 queue families in a GPU.
				queueFamilyCount = 16;
			}

			VkQueueFamilyProperties queueFamilyProperties[16] = {};
			VkBool32 queuePresentSupport[16] = {};

			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, queueFamilyProperties);

			for (uint32 i = 0; i < queueFamilyCount; i++)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, Surface, &queuePresentSupport[i]);

				if ((queueFamilyProperties[i].queueCount > 0) &&
					(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
					(queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
				{
					if (GraphicsQueueFamilyIndex == -1)
					{
						GraphicsQueueFamilyIndex = i;
					}

					if (queuePresentSupport[i])
					{
						GraphicsQueueFamilyIndex = i;
						PresentQueueFamilyIndex = i;
						return true;
					}
				}
			}

			// If we don't have a queue that supports both graphics and present, we have to use separate queues.
			for (uint32 i = 0; i < queueFamilyCount; i++)
			{
				if (queuePresentSupport[i])
				{
					PresentQueueFamilyIndex = i;
					return true;
				}
			}

			return false;
		};

		uint32 numPhysicalDevices = 0;

		if (vkEnumeratePhysicalDevices(Instance, &numPhysicalDevices, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to enumerate physical devices!" && false);
			return false;
		}

		if (numPhysicalDevices > 8)
		{
			numPhysicalDevices = 8;
		}

		VkPhysicalDevice physicalDevices[8] = {};
		if (vkEnumeratePhysicalDevices(Instance, &numPhysicalDevices, physicalDevices) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to enumerate physical devices!" && false);
			return false;
		}

		uint32 graphicsQueueFamilyIndex = -1;
		uint32 presentQueueFamilyIndex  = -1;

		VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;

		for (size_t i = 0; i < static_cast<uint32>(numPhysicalDevices); i++)
		{
			if (!CheckPhysicalDeviceProperties(physicalDevices[i], Surface, graphicsQueueFamilyIndex, presentQueueFamilyIndex))
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

		GPU = selectedDevice;
		Queues[Queue_Type_Present].FamilyIndex = presentQueueFamilyIndex;
		Queues[Queue_Type_Graphics].FamilyIndex = graphicsQueueFamilyIndex;

#if _DEBUG
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(GPU, &deviceProperties);
		printf("GPU Name: %s\n", deviceProperties.deviceName);
		printf("GPU API Version: %d\n", deviceProperties.apiVersion);
		printf("GPU Driver Version: %d\n\n", deviceProperties.driverVersion);
#endif

		return true;
	}

	bool VulkanCommon::CreateLogicalDevice()
	{
		float32 queuePriority = 1.0f;
		Array<VkDeviceQueueCreateInfo> queueCreateInfos;

		{
			VkDeviceQueueCreateInfo qCreateInfo;
			FMemory::InitializeObject(qCreateInfo);

			qCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			qCreateInfo.queueFamilyIndex = Queues[Queue_Type_Graphics].FamilyIndex;
			qCreateInfo.queueCount = 1;
			qCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.Push(qCreateInfo);

			if (Queues[Queue_Type_Graphics].FamilyIndex != Queues[Queue_Type_Present].FamilyIndex)
			{
				qCreateInfo.queueFamilyIndex = Queues[Queue_Type_Present].FamilyIndex;
				queueCreateInfos.Push(qCreateInfo);
			}
		}

		const char* extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo deviceCreateInfo;
		FMemory::InitializeObject(deviceCreateInfo);

		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.Length());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.First();
		deviceCreateInfo.ppEnabledExtensionNames = &extensions;
		deviceCreateInfo.enabledExtensionCount = 1;

		if (vkCreateDevice(GPU, &deviceCreateInfo, nullptr, &Device) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create Vulkan device!" && false);
			return false;
		}

		return true;
	}

	bool VulkanCommon::CreatePresentationSurface()
	{
		EngineImpl& engine = ao::FetchEngineCtx();

#ifdef VK_USE_PLATFORM_WIN32_KHR
		VkWin32SurfaceCreateInfoKHR createInfo;
		FMemory::InitializeObject(createInfo);

		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = engine.Window.Handle;
		createInfo.hinstance = OS::GetHandleToModule(nullptr);

		if (vkCreateWin32SurfaceKHR(Instance, &createInfo, nullptr, &Surface) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create presentation surface!" && false);
			return false;
		}
#endif
		return true;
	}

	bool VulkanCommon::GetDeviceQueue()
	{
		vkGetDeviceQueue(Device, Queues[Queue_Type_Graphics].FamilyIndex, 0, &Queues[Queue_Type_Graphics].Handle);
		vkGetDeviceQueue(Device, Queues[Queue_Type_Present].FamilyIndex, 0, &Queues[Queue_Type_Present].Handle);
		return true;
	}

	bool VulkanCommon::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo;
		FMemory::InitializeObject(semaphoreInfo);
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo;
		FMemory::InitializeObject(fenceInfo);
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		ImageFences.Reserve(SwapChain.NumImages);
		for (uint32 i = 0; i < SwapChain.NumImages; i++)
		{
			ImageFences.Push(VkFence());
		}

		for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			for (uint32 j = 0; j < Semaphore_Max; j++)
			{
				if (vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &Semaphores[i][j]) != VK_SUCCESS)
				{
					VKT_ASSERT("Could not create Semaphore!" && false);
					return false;
				}
			}

			if (vkCreateFence(Device, &fenceInfo, nullptr, &Fence[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not create Fence!" && false);
				return false;
			}
		}

		return true;
	}

	bool VulkanCommon::CreateSwapchain()
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
				return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			}

			for (uint32 i = 0; i < Count; i++)
			{
				if (SurfaceFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
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

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		FMemory::InitializeObject(surfaceCapabilities);

		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPU, Surface, &surfaceCapabilities) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not check presentation surface capabilities!" && false);
			return false;
		}

		uint32 formatCount = 0;
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, Surface, &formatCount, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Error occured during presentation surface formats enumeration!" && false);
			return false;
		}

		if (formatCount > 16)
		{
			formatCount = 16;
		}

		VkSurfaceFormatKHR surfaceFormats[16] = {};
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, Surface, &formatCount, surfaceFormats) != VK_SUCCESS)
		{
			VKT_ASSERT("Error occurred during presentation surface formats enumeration!" && false);
			return false;
		}

		uint32 presentModeCount = 0;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(GPU, Surface, &presentModeCount, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Error occurred during presentation surface present modes enumeration!" && false);
			return false;
		}

		if (presentModeCount > 16)
		{
			presentModeCount = 16;
		}

		VkPresentModeKHR presentModes[16] = {};
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(GPU, Surface, &presentModeCount, presentModes) != VK_SUCCESS)
		{
			VKT_ASSERT("Error occurred during presentation surface present modes enumeration!" && false);
			return false;
		}

		VkSurfaceFormatKHR desiredFormat = getSwapChainFormat(surfaceFormats, formatCount);
		uint32 imageCount = getSwapChainImageCount(surfaceCapabilities);
		VkExtent2D imageExtent = getSwapChainExtent(surfaceCapabilities);

		VkSwapchainKHR oldSwapChain = SwapChain.Handle;

		VkSwapchainCreateInfoKHR createInfo;
		FMemory::InitializeObject(createInfo);

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

		if (vkCreateSwapchainKHR(Device, &createInfo, nullptr, &SwapChain.Handle) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create swap chain!" && false);
			return false;
		}

		// Cache swap chain format into the context.
		SwapChain.SurfaceFormat = desiredFormat;
		SwapChain.NumImages = imageCount;
		SwapChain.Extent = imageExtent;

		// Get swap chain images.
		// Set up only once.
		if (!SwapChain.Images.Size())
		{
			SwapChain.Images.Reserve(SwapChain.NumImages);
			SwapChain.ImageViews.Reserve(SwapChain.NumImages);

			for (uint32 i = 0; i < SwapChain.NumImages; i++)
			{
				SwapChain.Images.Push(VkImage());
				SwapChain.ImageViews.Push(VkImageView());
			}
		}
		vkGetSwapchainImagesKHR(Device, SwapChain.Handle, &SwapChain.NumImages, SwapChain.Images.First());

		if (oldSwapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(Device, oldSwapChain, nullptr);

			// Destroy previous image views on re-creation.
			for (uint32 i = 0; i < SwapChain.NumImages; i++)
			{
				vkDestroyImageView(Device, SwapChain.ImageViews[i], nullptr);
			}
		}

		// Create Image Views ...
		for (uint32 i = 0; i < SwapChain.NumImages; i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo;
			FMemory::InitializeObject(imageViewCreateInfo);

			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = SwapChain.Images[i];
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = SwapChain.SurfaceFormat.format;
			imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
			imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			if (vkCreateImageView(Device, &imageViewCreateInfo, nullptr, &SwapChain.ImageViews[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not create image view for framebuffer!" && false);
				return false;
			}
		}

		return true;
	}

	bool VulkanCommon::CreateDefaultRenderPass()
	{
		VkAttachmentDescription attachmentDesc;
		FMemory::InitializeObject(attachmentDesc);

		attachmentDesc.format = SwapChain.SurfaceFormat.format;
		attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentReference;
		FMemory::InitializeObject(colorAttachmentReference);

		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription;
		FMemory::InitializeObject(subpassDescription);

		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorAttachmentReference;

		VkSubpassDependency subpassDependency;
		FMemory::InitializeObject(subpassDependency);

		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderpassCreateInfo;
		FMemory::InitializeObject(renderpassCreateInfo);

		renderpassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpassCreateInfo.attachmentCount = 1;
		renderpassCreateInfo.pAttachments = &attachmentDesc;
		renderpassCreateInfo.subpassCount = 1;
		renderpassCreateInfo.pSubpasses = &subpassDescription;
		renderpassCreateInfo.dependencyCount = 1;
		renderpassCreateInfo.pDependencies = &subpassDependency;

		if (vkCreateRenderPass(Device, &renderpassCreateInfo, nullptr, &DefFramebuffer.Renderpass) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create render pass!" && false);
			return false;
		}

		return true;
	}

	bool VulkanCommon::CreateDefaultFramebuffer()
	{
		for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkFramebufferCreateInfo createInfo;
			FMemory::InitializeObject(createInfo);

			createInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			createInfo.renderPass		= DefFramebuffer.Renderpass;
			createInfo.attachmentCount	= 1;
			createInfo.pAttachments		= &SwapChain.ImageViews[i];
			createInfo.width			= SwapChain.Extent.width;
			createInfo.height			= SwapChain.Extent.height;
			createInfo.layers			= 1;

			if (vkCreateFramebuffer(Device, &createInfo, nullptr, &DefFramebuffer.Framebuffers[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not create a framebuffer!" && false);
				return false;
			}
		}

		return true;
	}

	bool VulkanDriver::CreateCommandPool()
	{
		VkCommandPoolCreateInfo createInfo;
		FMemory::InitializeObject(createInfo);

		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = Queues[Queue_Type_Graphics].FamilyIndex;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if (vkCreateCommandPool(Device, &createInfo, nullptr, &CommandPool) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create command pool!" && false);
			return false;
		}

		if ((vkGetSwapchainImagesKHR(Device, SwapChain.Handle, &SwapChain.NumImages, nullptr) != VK_SUCCESS) || !SwapChain.NumImages)
		{
			VKT_ASSERT("Could not get the number of swap chain images!" && false);
			return false;
		}

		return true;
	}

	bool VulkanDriver::AllocateCommandBuffers()
	{
		{
			/**
			* Default framebuffer command buffer.
			*/

			VkCommandBufferAllocateInfo cmdBufferAllocateInfo;
			FMemory::InitializeObject(cmdBufferAllocateInfo);

			cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferAllocateInfo.commandPool = CommandPool;
			cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

			if (vkAllocateCommandBuffers(Device, &cmdBufferAllocateInfo, DefFramebuffer.CmdBuffers) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not allocate command buffers!" && false);
				return false;
			}
		}

		for (auto& pair : Ctx.Store.CommandBuffers)
		{
			VkCommandBufferAllocateInfo cmdBufferAllocateInfo;
			FMemory::InitializeObject(cmdBufferAllocateInfo);

			cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferAllocateInfo.commandPool = CommandPool;
			cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

			if (vkAllocateCommandBuffers(Device, &cmdBufferAllocateInfo, pair.Value.First()) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not allocate command buffers!" && false);
				return false;
			}
		}
		return true;
	}

	bool VulkanDriver::CreateShader(HwShaderCreateInfo& CreateInfo)
	{
		VkShaderModuleCreateInfo shaderCreateInfo;
		FMemory::InitializeObject(shaderCreateInfo);

		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = CreateInfo.DWordBuf->Length() * sizeof(uint32);
		shaderCreateInfo.pCode = CreateInfo.DWordBuf->First();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(this->Device, &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			return false;
		}

		*CreateInfo.Handle = g_RandIdVk();
		Ctx.Store.Shaders.Insert(*CreateInfo.Handle, shaderModule);

		return true;
	}

	void VulkanDriver::DestroyShader(Handle<HShader>& Hnd)
	{
		VkShaderModule& shaderModule = Ctx.Store.Shaders[Hnd];
		vkDestroyShaderModule(Device, shaderModule, nullptr);
		Ctx.Store.Shaders.Remove(Hnd);
		new (&Hnd) Handle<HShader>(INVALID_HANDLE);
	}

	bool VulkanDriver::CreatePipeline(HwPipelineCreateInfo& CreateInfo)
	{
		VKT_ASSERT("Number of shaders exceeded amount allowed by driver!" && CreateInfo.Shaders.Length() <= MAX_PIPELINE_SHADER_STAGE);
		VkPipelineShaderStageCreateInfo pipelineShaders[MAX_PIPELINE_SHADER_STAGE];

		// Pipeline shader stage declaration.
		for (size_t i = 0; i < CreateInfo.Shaders.Length(); i++)
		{
			HwPipelineCreateInfo::ShaderInfo& shaderInformation = CreateInfo.Shaders[i];
			VkPipelineShaderStageCreateInfo& pipelineCreateInfo = pipelineShaders[i];
			FMemory::InitializeObject(pipelineCreateInfo);

			pipelineCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			pipelineCreateInfo.stage	= Ctx.Flags.ShaderStageFlags[shaderInformation.Type];
			pipelineCreateInfo.module	= Ctx.Store.Shaders[shaderInformation.Handle];
			pipelineCreateInfo.pName	= "main";
		}

		// Baking VkPipelineVertexInputStateCreateInfo for tutorial's sake.
		// TODO(Ygsm):
		// This part needs to be dynamic in the future.

		VkPipelineVertexInputStateCreateInfo vertexStateCreateInfo;
		FMemory::InitializeObject(vertexStateCreateInfo);

		vertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
		FMemory::InitializeObject(inputAssemblyCreateInfo);

		inputAssemblyCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology				= Ctx.Flags.Topology[CreateInfo.Topology];
		inputAssemblyCreateInfo.primitiveRestartEnable	= VK_FALSE;

		// Viewport and scissoring.
		VkViewport viewport;
		FMemory::InitializeObject(viewport);
		
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;
		viewport.width		= static_cast<float32>(SwapChain.Extent.width);
		viewport.height		= static_cast<float32>(SwapChain.Extent.height);
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;

		VkRect2D scissor;
		FMemory::InitializeObject(scissor);

		scissor.offset = { 0, 0 };
		scissor.extent = SwapChain.Extent;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
		FMemory::InitializeObject(viewportStateCreateInfo);

		viewportStateCreateInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.pViewports		= &viewport;
		viewportStateCreateInfo.viewportCount	= 1;
		viewportStateCreateInfo.pScissors		= &scissor;
		viewportStateCreateInfo.scissorCount	= 1;

		// Dynamic state description.
		// TODO(Ygsm):
		// Study more about this.
		//VkDynamicState dynamicStates[1] = { VK_DYNAMIC_STATE_VIEWPORT };
		//VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
		//FMemory::InitializeObject(dynamicStateCreateInfo);

		//dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		//dynamicStateCreateInfo.dynamicStateCount = 1;
		//dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		// Rasterization state.
		VkPipelineRasterizationStateCreateInfo rasterStateCreateInfo;
		FMemory::InitializeObject(rasterStateCreateInfo);

		rasterStateCreateInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterStateCreateInfo.polygonMode	= Ctx.Flags.PolygonMode[CreateInfo.PolyMode];
		rasterStateCreateInfo.cullMode		= Ctx.Flags.CullingMode[CreateInfo.CullMode];
		rasterStateCreateInfo.frontFace		= Ctx.Flags.FrontFace[CreateInfo.FrontFace];
		rasterStateCreateInfo.lineWidth		= 1.0f;
		//rasterStateCreateInfo.depthClampEnable = VK_FALSE; // Will be true for shadow rendering for some reason I do not know yet...

		// Multisampling state description
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo;
		FMemory::InitializeObject(multisampleCreateInfo);

		multisampleCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.sampleShadingEnable	= VK_FALSE;
		//multisampleCreateInfo.minSampleShading		= 1.0f;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable		= VK_FALSE;

		// Blending state description.
		// TODO(Ygsm):
		// Need to study this part more.
		VkPipelineColorBlendAttachmentState colorBlendState;
		FMemory::InitializeObject(colorBlendState);

		colorBlendState.blendEnable			= VK_FALSE;
		//colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		//colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		//colorBlendState.colorBlendOp		= VK_BLEND_OP_ADD;
		//colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		//colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		//colorBlendState.alphaBlendOp		= VK_BLEND_OP_ADD;
		colorBlendState.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
		FMemory::InitializeObject(colorBlendCreateInfo);

		colorBlendCreateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.attachmentCount	= 1;
		colorBlendCreateInfo.pAttachments		= &colorBlendState;
		colorBlendCreateInfo.logicOpEnable		= VK_FALSE;
		colorBlendCreateInfo.logicOp			= VK_LOGIC_OP_COPY;

		// Pipeline layour description.
		VkPipelineLayoutCreateInfo layoutCreateInfo;
		FMemory::InitializeObject(layoutCreateInfo);

		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		VkPipelineLayout pipelineLayout;
		if (vkCreatePipelineLayout(Device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create pipeline layout!=" && false);
			return false;
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
		FMemory::InitializeObject(pipelineCreateInfo);

		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = static_cast<uint32>(CreateInfo.Shaders.Length());
		pipelineCreateInfo.pStages = pipelineShaders;
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
		pipelineCreateInfo.renderPass = Ctx.Store.Framebuffers[CreateInfo.FramebufferHandle].Renderpass;
		pipelineCreateInfo.subpass = 0;							// TODO(Ygsm): Study more about this!
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// TODO(Ygsm): Study more about this!
		pipelineCreateInfo.basePipelineIndex = -1;				// TODO(Ygsm): Study more about this!

		VkPipeline graphicsPipeline;
		if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create graphics pipeline!" && false);
			return false;
		}

		*CreateInfo.Handle = g_RandIdVk();
		Ctx.Store.Pipelines.Insert(*CreateInfo.Handle, graphicsPipeline);

		vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);

		return true;
	}

	void VulkanDriver::DestroyPipeline(Handle<HPipeline>& Hnd)
	{
		vkDeviceWaitIdle(Device);
		VkPipeline& graphicsPipeline = Ctx.Store.Pipelines[Hnd];
		vkDestroyPipeline(Device, graphicsPipeline, nullptr);
		Ctx.Store.Pipelines.Remove(Hnd);
		new (&Hnd) Handle<HPipeline>(INVALID_HANDLE);
	}

	bool VulkanDriver::AddCommandBufferEntry(HwCmdBufferAllocInfo& AllocateInfo)
	{
		uint32 id = g_RandIdVk();
		new (AllocateInfo.Handle) Handle<HCmdBuffer>(id);

		auto& storage = Ctx.Store.CommandBuffers.Insert(id, {});
		for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			storage.Push(VkCommandBuffer());
		}

		return true;
	}

	void VulkanDriver::FreeCommandBuffer(Handle<HCmdBuffer>& Hnd)
	{
		vkDeviceWaitIdle(Device);
		VkCommandBuffer* cmdBuffer = Ctx.Store.CommandBuffers[Hnd].First();
		//size_t numCmdBuffers = Ctx.Store.CommandBuffers[Hnd].Length();
		vkFreeCommandBuffers(Device, CommandPool, MAX_FRAMES_IN_FLIGHT, cmdBuffer);
		//CommandBuffers.PopAt(Hnd, false);
		Ctx.Store.CommandBuffers.Remove(Hnd);
		new (&Hnd) Handle<HCmdBuffer>(INVALID_HANDLE);
	}

	void VulkanDriver::RecordCommandBuffer(HwCmdBufferRecordInfo& RecordInfo)
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo;
		FMemory::InitializeObject(cmdBufferBeginInfo);

		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		float32* color = RecordInfo.ClearColor;
		const VkClearValue clearValue = {
			{
				color[Color_Channel_Red],
				color[Color_Channel_Green],
				color[Color_Channel_Blue],
				color[Color_Channel_Alpha]
			}
		};

		VkImageSubresourceRange imageSubresourceRange;
		FMemory::InitializeObject(imageSubresourceRange);

		imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageSubresourceRange.levelCount = 1;
		imageSubresourceRange.layerCount = 1;

		VkPipeline pipeline = Ctx.Store.Pipelines[*RecordInfo.PipelineHandle];
		VkCommandBuffer& cmdBuffer = Ctx.Store.CommandBuffers[*RecordInfo.CommandBufferHandle][CurrentFrame];

		vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);

		if (Queues[Queue_Type_Graphics].Handle != Queues[Queue_Type_Present].Handle)
		{
			VkImageMemoryBarrier barrierFromPresentToDraw;
			FMemory::InitializeObject(barrierFromPresentToDraw);

			barrierFromPresentToDraw.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrierFromPresentToDraw.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrierFromPresentToDraw.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrierFromPresentToDraw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrierFromPresentToDraw.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrierFromPresentToDraw.srcQueueFamilyIndex = Queues[Queue_Type_Present].FamilyIndex;
			barrierFromPresentToDraw.dstQueueFamilyIndex = Queues[Queue_Type_Graphics].FamilyIndex;
			barrierFromPresentToDraw.image = SwapChain.Images[NextImageIndex];
			barrierFromPresentToDraw.subresourceRange = imageSubresourceRange;

			vkCmdPipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrierFromPresentToDraw);
		}

		int32* offset  = RecordInfo.SurfaceOffset;
		uint32* extent = RecordInfo.SurfaceExtent;

		if (!extent[Surface_Extent_Width])
		{
			extent[Surface_Extent_Width] = SwapChain.Extent.width;
		}

		if (!extent[Surface_Extent_Height])
		{
			extent[Surface_Extent_Height] = SwapChain.Extent.height;
		}

		VkRenderPassBeginInfo renderPassBeginInfo;
		FMemory::InitializeObject(renderPassBeginInfo);

		auto& frameParam = Ctx.Store.Framebuffers[RecordInfo.FramebufferHandle];

		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass	= frameParam.Renderpass;
		renderPassBeginInfo.framebuffer = frameParam.Framebuffers[NextImageIndex];
		renderPassBeginInfo.renderArea.offset = { offset[Surface_Offset_X], offset[Surface_Offset_Y] };
		renderPassBeginInfo.renderArea.extent = { extent[Surface_Extent_Width], extent[Surface_Extent_Height] };
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdDraw(cmdBuffer, 3, 1, 0, 0);	// NOTE(Ygsm): I'm assuming you need to specify the number of vertices in the mesh.
		vkCmdEndRenderPass(cmdBuffer);

		if (Queues[Queue_Type_Graphics].Handle != Queues[Queue_Type_Present].Handle)
		{
			VkImageMemoryBarrier barrierFromDrawToPresent;
			FMemory::InitializeObject(barrierFromDrawToPresent);

			barrierFromDrawToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrierFromDrawToPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrierFromDrawToPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrierFromDrawToPresent.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrierFromDrawToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrierFromDrawToPresent.srcQueueFamilyIndex = Queues[Queue_Type_Graphics].FamilyIndex;
			barrierFromDrawToPresent.dstQueueFamilyIndex = Queues[Queue_Type_Present].FamilyIndex;
			barrierFromDrawToPresent.image = SwapChain.Images[NextImageIndex];
			barrierFromDrawToPresent.subresourceRange = imageSubresourceRange;

			vkCmdPipelineBarrier(cmdBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrierFromDrawToPresent);
		}

		vkEndCommandBuffer(cmdBuffer);
	}

	bool VulkanDriver::CreateImage(HwImageCreateStruct& CreateInfo)
	{
		bool success = true;
		uint32 id = g_RandIdVk();
		*CreateInfo.Handle = id;

		Ctx.Store.Images.Insert(id, {});
		auto& imageParam = Ctx.Store.Images[id];

		VkImageCreateInfo imageInfo;
		FMemory::InitializeObject(imageInfo);

		imageInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType		= CreateInfo.Dimension;
		imageInfo.extent.width	= CreateInfo.Width;
		imageInfo.extent.height = CreateInfo.Height;
		imageInfo.extent.depth	= CreateInfo.Depth;
		imageInfo.format		= CreateInfo.Format;
		imageInfo.tiling		= VK_IMAGE_TILING_OPTIMAL;
		imageInfo.mipLevels		= CreateInfo.MipLevels;
		imageInfo.usage			= CreateInfo.UsageFlags;
		imageInfo.samples		= CreateInfo.Samples;
		imageInfo.arrayLayers	= 1;
		imageInfo.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = CreateInfo.InitialLayout;
		
		VmaAllocationCreateInfo allocInfo;
		FMemory::InitializeObject(allocInfo);

		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		if (vmaCreateImage(
			Allocator,
			&imageInfo,
			&allocInfo,
			&imageParam.Image,
			&imageParam.Allocation,
			nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create image" && false);
			success = false;
			goto BeforeImageCreateEnd;
		}

		VkImageViewCreateInfo imageViewInfo;
		FMemory::InitializeObject(imageViewInfo);

		imageViewInfo.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.format							= CreateInfo.Format;
		imageViewInfo.subresourceRange.baseMipLevel		= 0;
		imageViewInfo.subresourceRange.levelCount		= CreateInfo.MipLevels;
		imageViewInfo.subresourceRange.baseArrayLayer	= 0;
		imageViewInfo.subresourceRange.layerCount		= 1;
		imageViewInfo.image								= imageParam.Image;
		imageViewInfo.viewType							= Ctx.Flags.ImageViewType[CreateInfo.Dimension];

		if (CreateInfo.UsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (CreateInfo.UsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		if (vkCreateImageView(
			Device, 
			&imageViewInfo, 
			nullptr, 
			&imageParam.ImageView) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create image's image view" && false);
			success = false;
			goto BeforeImageCreateEnd;
		}

		// NOTE(Ygsm):
		// Not sure if we should include samplers in image creation.
		//
		//if (CreateInfo.UsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT != VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
		//{
		//	goto BeforeImageCreateEnd;
		//}

		//VkSamplerCreateInfo samplerInfo;
		//FMemory::InitializeObject(samplerInfo);

		//samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	BeforeImageCreateEnd:

		if (!success)
		{
			*CreateInfo.Handle = INVALID_HANDLE;
			Ctx.Store.Images.Remove(id);
		}

		return success;
	}

	void VulkanDriver::DestroyImage(Handle<HImage>& Hnd)
	{
		auto& imageParam = Ctx.Store.Images[Hnd];
		vkDestroyImageView(Device, imageParam.ImageView, nullptr);
		vmaDestroyImage(Allocator, imageParam.Image, imageParam.Allocation);
	}

	bool VulkanDriver::CreateFramebuffer(HwFramebufferCreateInfo& CreateInfo)
	{
		bool success = true;
		uint32 id = g_RandIdVk();

		*CreateInfo.Handle = id;
		Ctx.Store.Framebuffers.Insert(id, {});

		auto& framebufferParam = Ctx.Store.Framebuffers[id];

		const uint32 width	= (!CreateInfo.Width)	? SwapChain.Extent.width : static_cast<uint32>(CreateInfo.Width);
		const uint32 height = (!CreateInfo.Height)	? SwapChain.Extent.height : static_cast<uint32>(CreateInfo.Height);
		const uint32 depth	= (!CreateInfo.Depth)	? 1 : static_cast<uint32>(CreateInfo.Depth);

		const size_t numOfAttachments = CreateInfo.Attachments.Length();
		uint32 inputAttachmentCount = 0;
		uint32 outputAttachmentCount = 0;
		VKT_ASSERT("Must not exceed maximum attachment allowed in framebuffer" && numOfAttachments <= MAX_ATTACHMENTS_IN_FRAMEBUFFER);
		
		HwImageCreateStruct imageInfo;

		//
		// NOTE(Ygsm):
		// Since we're not targeting mobile (not using more than a single subpass), there is no need for VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
		//

		for (size_t i = 0; i < numOfAttachments; i++)
		{
			HwAttachmentInfo& attachment = CreateInfo.Attachments[i];

			// Input attachments mean the image was already created and we can skip this step.
			if (attachment.Type == Attachment_Usage_Input)
			{
				inputAttachmentCount++;
				continue;
			}

			FMemory::InitializeObject(imageInfo);

			imageInfo.Handle = attachment.Handle;
			imageInfo.Width = width;
			imageInfo.Height = height;
			imageInfo.Depth = depth;
			imageInfo.MipLevels = 1;
			imageInfo.Dimension = Ctx.Flags.ImageType[attachment.Dimension];
			imageInfo.Samples = Ctx.Flags.SampleCounts[CreateInfo.Samples];

			//
			// Note(Ygsm):
			// This makes sense right since they will be sampled at some point?
			//
			imageInfo.UsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT;

			switch (attachment.Type)
			{
				case Attachment_Type_Color:
					imageInfo.UsageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
					imageInfo.Format = VK_FORMAT_R8G8B8A8_SRGB;
					break;
				case Attachment_Type_Depth_Stencil:
					imageInfo.UsageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					imageInfo.Format = VK_FORMAT_D24_UNORM_S8_UINT;
					break;
				default:
					break;
			}

			if (!CreateImage(imageInfo))
			{
				success = false;
				goto FramebufferCreateBeforeEnd;
			}

			if (attachment.Type == Attachment_Type_Color)
			{
				outputAttachmentCount++;
			}

			// Create samplers after creating the image?
		}

		// Stuck here !!! Help needed !!!

		VkAttachmentDescription attachmentDescs[MAX_ATTACHMENTS_IN_FRAMEBUFFER];

		for (size_t i = 0; i < numOfAttachments; i++)
		{
			HwAttachmentInfo& attachment = CreateInfo.Attachments[i];
			VkAttachmentDescription& attDesc = attachmentDescs[i];
			FMemory::InitializeObject(attDesc);

			attDesc.samples			= Ctx.Flags.SampleCounts[CreateInfo.Samples];
			attDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			attDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;

			switch (attachment.Type)
			{
				case Attachment_Type_Color:
					attDesc.format			= VK_FORMAT_R8G8B8A8_SRGB;
					attDesc.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					break;
				case Attachment_Type_Depth_Stencil:
					attDesc.format			= VK_FORMAT_D24_UNORM_S8_UINT;
					attDesc.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					break;
				default:
					break;
			}

			switch (attachment.Usage)
			{
				case Attachment_Usage_Input: 
				{
					attDesc.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					attDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
					attDesc.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;

					if (attachment.Type == Attachment_Type_Depth_Stencil)
					{
						attDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_LOAD;
						attDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
					}

					break;
				}
				case Attachment_Usage_Output:
				{
					attDesc.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
					attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

					if (attachment.Type == Attachment_Type_Depth_Stencil)
					{
						attDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
						attDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_STORE;
					}

					break;
				}
				default:
					break;
			}
		}

		// TODO(Ygsm):
		// Create renderpass and it's subpasses.
		VkSubpassDescription subpassDescription;
		FMemory::InitializeObject(subpassDescription);

		subpassDescription.pipelineBindPoint	= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.inputAttachmentCount = inputAttachmentCount;
		subpassDescription.colorAttachmentCount = outputAttachmentCount;

		// Create subpass dependencies.
		VkSubpassDependency dependencies[MAX_ATTACHMENTS_IN_FRAMEBUFFER];

		for (size_t i = 0; i < numOfAttachments; i++)
		{

		}

		// Create actual renderpass.
		VkRenderPassCreateInfo renderPassInfo;
		FMemory::InitializeObject(renderPassInfo);

		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32>(numOfAttachments);
		renderPassInfo.pAttachments = attachmentDescs;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		// dependencies ...

		vkCreateRenderPass(Device, &renderPassInfo, nullptr, &framebufferParam.Renderpass);

		VkImageView imgViewAtt[MAX_ATTACHMENTS_IN_FRAMEBUFFER];

		for (size_t i = 0; i < numOfAttachments; i++)
		{
			HwAttachmentInfo& attachment = CreateInfo.Attachments[i];
			auto& imageParam = Ctx.Store.Images[*attachment.Handle];
			imgViewAtt[i] = imageParam.ImageView;
		}

		VkFramebufferCreateInfo framebufferInfo;
		FMemory::InitializeObject(framebufferInfo);

		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.width = CreateInfo.Width;
		framebufferInfo.height = CreateInfo.Height;
		framebufferInfo.pAttachments = imgViewAtt;
		framebufferInfo.attachmentCount = numOfAttachments;
		framebufferInfo.layers = 1;
		framebufferInfo.renderPass = framebufferParam.Renderpass;

		if (vkCreateFramebuffer(
			Device, 
			&framebufferInfo, 
			nullptr, 
			&framebufferParam.Framebuffer) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create framebuffer" && false);
			success = false;
		}

	FramebufferCreateBeforeEnd:

		if (!success)
		{
			*CreateInfo.Handle = INVALID_HANDLE;
			Ctx.Store.Framebuffers.Remove(id);
		}

		return success;
	};

	void VulkanDriver::DestroyFramebuffer(Handle<HFramebuffer>& Hnd)
	{
		// destroy framebuffer.
		// destroy renderpass.
		// destroy image and it's image view.
	};

}