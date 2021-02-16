#pragma once
#include "VulkanDriver.h"
#include "Engine/Interface.h"
#include "Library/Random/Xoroshiro.h"
#include "VulkanFunctions.h"

namespace vk
{

	/**
	* FUTURE PLANS(Ygsm):
	* We allocated buffer memory to buffer pages and try to allocate objects into it.
	* A single buffer page will behave like a linear allocator.
	* We'll probably use VMA for memory allocation needs.
	*/

	/**
	* TODO(Ygsm):
	* Each descriptor set should own a buffer.
	*/

#define MAX_PIPELINE_SHADER_STAGE	4
#define MAX_GPU_RESOURCE			static_cast<size_t>(1024)
#define MAX_GPU_RESOURCE_DBL		static_cast<size_t>(2018)

	static Xoroshiro32 g_RandIdVk(OS::GetPerfCounter());

	struct VulkanCompileTimeContext
	{
		using ColorBlendStatesArr	= Array<VkPipelineColorBlendAttachmentState>;
		using ClearValuesArr		= Array<VkClearValue>;

		OS::DllHandle		VulkanDll;
		ColorBlendStatesArr	ColorBlendStates;
		ClearValuesArr		ClearValues;

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
			VkDeviceSize	Offset;
			uint32			DataCount;
			uint8*			Data;
		};

		struct DescPoolParam
		{
			VkDescriptorPool	DescPool[MAX_FRAMES_IN_FLIGHT];
			Handle<HDescPool>	Next;
		};

		struct DescSetParam
		{
			VkDescriptorSet DescSet[MAX_FRAMES_IN_FLIGHT];
		};

		struct PipelineParams
		{
			VkPipeline		 Pipeline;
			VkPipelineLayout Layout;
		};

		struct Storage
		{
			template <typename ResourceType> 
			using StoreContainer = Map<uint32, ResourceType>;

			StoreContainer<VkShaderModule>	Shaders;
			StoreContainer<PipelineParams>	Pipelines;
			StoreContainer<BufferParam>		Buffers;
			StoreContainer<FramePassParams>	FramePasses;
			StoreContainer<ImageParam>		Images;
			StoreContainer<DescPoolParam>	DescriptorPools;
			StoreContainer<DescSetParam>	DescriptorSets;
			StoreContainer<VkDescriptorSetLayout> DescriptorLayout;

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

			VkImageType ImageType[Texture_Type_Max] = {
				VK_IMAGE_TYPE_1D,
				VK_IMAGE_TYPE_2D,
				VK_IMAGE_TYPE_3D
			};

			VkImageViewType ImageViewType[Texture_Type_Max] = {
				VK_IMAGE_VIEW_TYPE_1D,
				VK_IMAGE_VIEW_TYPE_2D,
				VK_IMAGE_VIEW_TYPE_3D
			};

			VkFormat Formats[7] = {
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_R32_SFLOAT,
				VK_FORMAT_R32G32_SFLOAT,
				VK_FORMAT_R32G32B32_SFLOAT,
				VK_FORMAT_R32G32B32A32_SFLOAT
			};

			VkBufferUsageFlagBits BufferTypes[Buffer_Type_Max] = {
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
			};

			VkDescriptorType DescriptorType[Descriptor_Type_Max] = {
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
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
		CreateInfo.messageSeverity = //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
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
		Ctx.ColorBlendStates.Reserve(MAX_FRAMEBUFFER_OUTPUTS);
		Ctx.ClearValues.Reserve(MAX_FRAMEBUFFER_OUTPUTS);

		Ctx.Store.Pipelines.Reserve(	MAX_GPU_RESOURCE);
		Ctx.Store.FramePasses.Reserve(	MAX_GPU_RESOURCE);
		Ctx.Store.Images.Reserve(		MAX_GPU_RESOURCE);
		Ctx.Store.Shaders.Reserve(	MAX_GPU_RESOURCE_DBL);

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

		uint32 id = g_RandIdVk();
		DefaultFramebuffer = static_cast<uint32>(id);
		Ctx.Store.FramePasses.Insert(id, {});

		if (!CreateDefaultRenderPass())				return false;
		if (!CreateDefaultFramebuffer())			return false;
		if (!CreateVmAllocator())					return false;

		NextImageIndex = 0;
		CurrentFrame = 0;

		return true;
	}

	void VulkanDriver::TerminateDriver()
	{
		Flush();

		auto& framePass = Ctx.Store.FramePasses[DefaultFramebuffer];
		vkDestroyRenderPass(Device, framePass.Renderpass, nullptr);

		Ctx.Store.Shaders.Release();
		Ctx.Store.Pipelines.Release();
		Ctx.Store.Images.Release();
		Ctx.Store.FramePasses.Release();

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

		for (auto& pair : Ctx.Store.FramePasses)
		{
			auto& frameParams = pair.Value;
			VkCommandBuffer* cmdBuffers = frameParams.CommandBuffers;
			vkFreeCommandBuffers(Device, CommandPool, MAX_FRAMES_IN_FLIGHT, cmdBuffers);
			FMemory::Memzero(cmdBuffers, MAX_FRAMES_IN_FLIGHT);

			for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
			{
				VkFramebuffer& framebuffer = frameParams.Framebuffers[i];

				if (framebuffer == VK_NULL_HANDLE) 
				{ 
					continue; 
				}

				vkDestroyFramebuffer(Device, framebuffer, nullptr);
				framebuffer = VK_NULL_HANDLE;
			}
		}

		if (CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(Device, CommandPool, nullptr);
			CommandPool = VK_NULL_HANDLE;
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

		for (auto& pair : Ctx.Store.FramePasses)
		{
			auto& frameParams = pair.Value;
			VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];

			if (cmdBuffer == VK_NULL_HANDLE) { continue; }

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

		const uint32 numCommandBuffers = static_cast<uint32>(CommandBuffersForSubmit.Length());

		VkSubmitInfo submitInfo = {};

		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount	= 1;
		submitInfo.pWaitSemaphores		= &Semaphores[CurrentFrame][Semaphore_Type_ImageAvailable];
		submitInfo.pWaitDstStageMask	= &waitDstStageMask;
		submitInfo.pCommandBuffers		= CommandBuffersForSubmit.First();
		submitInfo.commandBufferCount	= numCommandBuffers;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores	= &Semaphores[CurrentFrame][Semaphore_Type_RenderComplete];

		vkResetFences(Device, 1, &Fence[CurrentFrame]);

		if (vkQueueSubmit(Queues[Queue_Type_Graphics].Handle, 1, &submitInfo, Fence[CurrentFrame]) != VK_SUCCESS)
		{
			return false;
		}

		VkPresentInfoKHR presentInfo = {};

		presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount	= 1;
		presentInfo.pWaitSemaphores		= &Semaphores[CurrentFrame][Semaphore_Type_RenderComplete];
		presentInfo.swapchainCount		= 1;
		presentInfo.pSwapchains			= &SwapChain.Handle;
		presentInfo.pImageIndices		= &NextImageIndex;
		
		VkResult result = vkQueuePresentKHR(Queues[Queue_Type_Present].Handle, &presentInfo);

		CommandBuffersForSubmit.Empty();
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

		VkInstanceCreateInfo createInfo = {};

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
		vkGetPhysicalDeviceProperties(GPU, &GPUProperties);

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

		if (imageCount > MAX_SWAPCHAIN_IMAGE_ALLOWED)
		{
			imageCount = MAX_SWAPCHAIN_IMAGE_ALLOWED;
		}

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

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType	= VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format		= SwapChain.SurfaceFormat.format;
		imageViewCreateInfo.components	= { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		
		// Create Image Views ...
		for (uint32 i = 0; i < SwapChain.NumImages; i++)
		{

			imageViewCreateInfo.image = SwapChain.Images[i];

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
		auto& frameParams = Ctx.Store.FramePasses[DefaultFramebuffer];

		VkAttachmentDescription attachmentDesc = {};

		attachmentDesc.format			= SwapChain.SurfaceFormat.format;
		attachmentDesc.samples			= VK_SAMPLE_COUNT_1_BIT;
		//attachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDesc.loadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDesc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		//attachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDesc.initialLayout	= VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		attachmentDesc.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentReference = {};

		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};

		subpassDescription.pipelineBindPoint	= VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments	= &colorAttachmentReference;

		VkSubpassDependency subpassDependency = {};

		subpassDependency.srcSubpass	= VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass	= 0;
		subpassDependency.srcStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderpassCreateInfo = {};

		renderpassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderpassCreateInfo.attachmentCount	= 1;
		renderpassCreateInfo.pAttachments		= &attachmentDesc;
		renderpassCreateInfo.subpassCount		= 1;
		renderpassCreateInfo.pSubpasses			= &subpassDescription;
		renderpassCreateInfo.dependencyCount	= 1;
		renderpassCreateInfo.pDependencies		= &subpassDependency;

		if (vkCreateRenderPass(Device, &renderpassCreateInfo, nullptr, &frameParams.Renderpass) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create render pass!" && false);
			return false;
		}

		return true;
	}

	bool VulkanCommon::CreateDefaultFramebuffer()
	{
		auto& frameParams = Ctx.Store.FramePasses[DefaultFramebuffer];

		VkFramebufferCreateInfo createInfo = {};

		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = frameParams.Renderpass;
		createInfo.attachmentCount = 1;
		createInfo.width = SwapChain.Extent.width;
		createInfo.height = SwapChain.Extent.height;
		createInfo.layers = 1;

		for (uint32 i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
		{
			createInfo.pAttachments = &SwapChain.ImageViews[i];

			if (vkCreateFramebuffer(Device, &createInfo, nullptr, &frameParams.Framebuffers[i]) != VK_SUCCESS)
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

		return true;
	}

	bool VulkanDriver::AllocateCommandBuffers()
	{
		VkCommandBufferAllocateInfo cmdBufferAllocateInfo = {};

		cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocateInfo.commandPool = CommandPool;
		cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

		for (auto& pair : Ctx.Store.FramePasses)
		{
			VkCommandBuffer* commandBuffers = pair.Value.CommandBuffers;
			if (vkAllocateCommandBuffers(Device, &cmdBufferAllocateInfo, commandBuffers) != VK_SUCCESS)
			{
				VKT_ASSERT("Could not allocate command buffers!" && false);
				return false;
			}
		}

		return true;
	}

	bool VulkanDriver::CreateBuffer(HwBufferCreateInfo& CreateInfo)
	{
		uint32 id = g_RandIdVk();
		Ctx.Store.Buffers.Insert(id, {});
		*CreateInfo.Handle = id;

		auto& bufferParam = Ctx.Store.Buffers[id];

		size_t totalSize = CreateInfo.Count * CreateInfo.Size;
		VkBufferCreateInfo bufferCreateInfo = {};

		bufferCreateInfo.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size	= totalSize;
		bufferCreateInfo.usage	= Ctx.Flags.BufferTypes[CreateInfo.Type];

		VmaAllocationCreateInfo allocInfo = {};

		allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

		if (vmaCreateBuffer(Allocator, &bufferCreateInfo, &allocInfo, &bufferParam.Buffer, &bufferParam.Allocation, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Could not create vertex buffer" && false);
			return false;
		}

		bufferParam.DataCount = static_cast<uint32>(CreateInfo.Count);

		vmaMapMemory(Allocator, bufferParam.Allocation, reinterpret_cast<void**>(&bufferParam.Data));
		vmaUnmapMemory(Allocator, bufferParam.Allocation);

		if (CreateInfo.Data)
		{
			//void* data = nullptr;
			FMemory::Memcpy(bufferParam.Data, CreateInfo.Data, totalSize);
			bufferParam.Offset = totalSize;
		}

		return true;
	}

	void VulkanDriver::DestroyBuffer(Handle<HBuffer>& Hnd)
	{
		vkDeviceWaitIdle(Device);
		auto& bufferParam = Ctx.Store.Buffers[Hnd];
		vmaDestroyBuffer(Allocator, bufferParam.Buffer, bufferParam.Allocation);
		bufferParam.Buffer		= VK_NULL_HANDLE;
		bufferParam.Allocation	= VK_NULL_HANDLE;
		Ctx.Store.Buffers.Remove(Hnd);
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

	bool VulkanDriver::CreateDescriptorPool(HwDescriptorPoolCreateInfo& CreateInfo)
	{
		VkDescriptorPoolSize poolSize;
		poolSize.descriptorCount = CreateInfo.NumDescriptorsOfType;
		poolSize.type = Ctx.Flags.DescriptorType[CreateInfo.Type];

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.maxSets = CreateInfo.NumDescriptorsOfType;
		descriptorPoolCreateInfo.pPoolSizes = &poolSize;
		descriptorPoolCreateInfo.poolSizeCount = 1;

		uint32 passCount = MAX_FRAMES_IN_FLIGHT;
		VkDescriptorPool descPool[MAX_FRAMES_IN_FLIGHT];

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateDescriptorPool(Device, &descriptorPoolCreateInfo, nullptr, &descPool[i]) != VK_SUCCESS)
			{
				continue;
			}
			passCount--;
		}

		if (passCount)
		{
			return false;
		}

		uint32 id = g_RandIdVk();
		*CreateInfo.Handle = id;
		auto& descPoolParams = Ctx.Store.DescriptorPools.Insert(id, {});

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descPoolParams.DescPool[i] = descPool[i];
		}

		return true;
	}

	void VulkanDriver::DestroyDescriptorPool(Handle<HDescPool>& Hnd)
	{
		vkDeviceWaitIdle(Device);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorPool& descPool = Ctx.Store.DescriptorPools[Hnd].DescPool[i];
			vkDestroyDescriptorPool(Device, descPool, nullptr);
		}

		Ctx.Store.DescriptorPools.Remove(Hnd);
		new (&Hnd) Handle<HDescPool>(INVALID_HANDLE);
	}

	bool VulkanDriver::CreateDescriptorSetLayout(HwDescriptorSetLayoutCreateInfo& CreateInfo)
	{
		VkDescriptorSetLayoutBinding descSetLayoutBinding = {};
		descSetLayoutBinding.binding = CreateInfo.Binding;
		descSetLayoutBinding.descriptorCount = 1;
		descSetLayoutBinding.descriptorType = Ctx.Flags.DescriptorType[CreateInfo.Type];

		uint32 stageFlags = 0;

		if (CreateInfo.ShaderStages.Has(Shader_Type_Vertex))
		{
			stageFlags |= Ctx.Flags.ShaderStageFlags[Shader_Type_Vertex];
		}

		if (CreateInfo.ShaderStages.Has(Shader_Type_Fragment))
		{
			stageFlags |= Ctx.Flags.ShaderStageFlags[Shader_Type_Fragment];
		}

		descSetLayoutBinding.stageFlags = stageFlags;

		VkDescriptorSetLayoutCreateInfo descLayoutCreateInfo = {};
		descLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descLayoutCreateInfo.bindingCount = 1;
		descLayoutCreateInfo.pBindings = &descSetLayoutBinding;

		VkDescriptorSetLayout setLayout;

		if (vkCreateDescriptorSetLayout(Device, &descLayoutCreateInfo, nullptr, &setLayout) != VK_SUCCESS)
		{
			return false;
		}

		uint32 id = g_RandIdVk();
		*CreateInfo.Handle = id;
		Ctx.Store.DescriptorLayout.Insert(id, setLayout);

		return true;
	}

	void VulkanDriver::DestroyDescriptorSetLayout(Handle<HDescLayout>& Hnd)
	{
		vkDeviceWaitIdle(Device);
		VkDescriptorSetLayout& layout = Ctx.Store.DescriptorLayout[Hnd];
		vkDestroyDescriptorSetLayout(Device, layout, nullptr);
		Ctx.Store.DescriptorLayout.Remove(Hnd);
	}

	bool VulkanDriver::CreatePipeline(HwPipelineCreateInfo& CreateInfo)
	{
		VKT_ASSERT("Number of shaders exceeded amount allowed by driver!" && CreateInfo.Shaders.Length() <= MAX_PIPELINE_SHADER_STAGE);
		VkPipelineShaderStageCreateInfo pipelineShaders[MAX_PIPELINE_SHADER_STAGE];
		
		uint32 vertexAttribCount = 0;
		ShaderAttrib* pVertexAttrib = nullptr;

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

			if (shaderInformation.Type == Shader_Type_Vertex)
			{
				pVertexAttrib		= shaderInformation.Attributes;
				vertexAttribCount	= static_cast<uint32>(shaderInformation.AttributeCount);
			}
		}

		// Vertex attributes ...

		VKT_ASSERT("Vertex attribute count exceeds static array capability" && vertexAttribCount < 9);

		VkVertexInputBindingDescription bindingDescription = {};

		bindingDescription.binding	 = 0;
		bindingDescription.stride	 = CreateInfo.VertexStride;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertexAttributes[8];

		for (uint32 i = 0; i < vertexAttribCount; i++)
		{
			vertexAttributes[i].binding		= pVertexAttrib[i].Binding;
			vertexAttributes[i].location	= pVertexAttrib[i].Location;
			vertexAttributes[i].offset		= pVertexAttrib[i].Offset;
			vertexAttributes[i].format		= Ctx.Flags.Formats[pVertexAttrib[i].Format];
		}

		VkPipelineVertexInputStateCreateInfo vertexStateCreateInfo = {};

		vertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexStateCreateInfo.pVertexAttributeDescriptions = vertexAttributes;
		vertexStateCreateInfo.vertexBindingDescriptionCount = 1;
		vertexStateCreateInfo.vertexAttributeDescriptionCount = vertexAttribCount;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};

		inputAssemblyCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology				= Ctx.Flags.Topology[CreateInfo.Topology];
		inputAssemblyCreateInfo.primitiveRestartEnable	= VK_FALSE;

		// Viewport and scissoring.
		VkViewport viewport = {};
		
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;
		viewport.width		= static_cast<float32>(SwapChain.Extent.width);
		viewport.height		= static_cast<float32>(SwapChain.Extent.height);
		viewport.minDepth	= 0.0f;
		viewport.maxDepth	= 1.0f;

		VkRect2D scissor = {};

		scissor.offset = { 0, 0 };
		scissor.extent = SwapChain.Extent;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};

		viewportStateCreateInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.pViewports		= &viewport;
		viewportStateCreateInfo.viewportCount	= 1;
		viewportStateCreateInfo.pScissors		= &scissor;
		viewportStateCreateInfo.scissorCount	= 1;

		// Dynamic state description.
		// TODO(Ygsm):
		// Study more about this.
		VkDynamicState dynamicStates[1] = { VK_DYNAMIC_STATE_VIEWPORT };
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};

		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = 1;
		dynamicStateCreateInfo.pDynamicStates = dynamicStates;

		// Rasterization state.
		VkPipelineRasterizationStateCreateInfo rasterStateCreateInfo = {};

		rasterStateCreateInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterStateCreateInfo.polygonMode	= Ctx.Flags.PolygonMode[CreateInfo.PolyMode];
		rasterStateCreateInfo.cullMode		= Ctx.Flags.CullingMode[CreateInfo.CullMode];
		rasterStateCreateInfo.frontFace		= Ctx.Flags.FrontFace[CreateInfo.FrontFace];
		rasterStateCreateInfo.lineWidth		= 1.0f;
		//rasterStateCreateInfo.depthClampEnable = VK_FALSE; // Will be true for shadow rendering for some reason I do not know yet...

		// Multisampling state description
		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};

		multisampleCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleCreateInfo.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.sampleShadingEnable	= VK_FALSE;
		//multisampleCreateInfo.minSampleShading		= 1.0f;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable		= VK_FALSE;

		// Blending state description.
		// NOTE(Ygsm):
		// Blending attachment state is needed for each COLOUR attachment that's used by the subpass.

		VkPipelineColorBlendAttachmentState colorBlendState = {};

		colorBlendState.blendEnable			= VK_TRUE;
		colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendState.colorBlendOp		= VK_BLEND_OP_ADD;
		colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendState.alphaBlendOp		= VK_BLEND_OP_ADD;
		colorBlendState.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		for (uint32 i = 0; i < CreateInfo.NumColorAttachments; i++)
		{
			Ctx.ColorBlendStates.Push(colorBlendState);
		}

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};

		colorBlendCreateInfo.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendCreateInfo.attachmentCount	= CreateInfo.NumColorAttachments;
		colorBlendCreateInfo.pAttachments		= Ctx.ColorBlendStates.First();
		colorBlendCreateInfo.logicOpEnable		= VK_FALSE;
		colorBlendCreateInfo.logicOp			= VK_LOGIC_OP_COPY;

		//
		// NOTE(Ygsm):
		// The code below is only a temporary solution .........
		//

		Array<VkDescriptorSetLayout> descriptorSetLayouts;

		for (size_t i = 0; i < CreateInfo.DescriptorLayouts.Length(); i++)
		{
			Handle<HDescLayout> hnd = CreateInfo.DescriptorLayouts[i];
			VkDescriptorSetLayout layout = Ctx.Store.DescriptorLayout[hnd];
			descriptorSetLayouts.Push(layout);
		}

		// Pipeline layour description.
		VkPipelineLayoutCreateInfo layoutCreateInfo = {};

		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		if (descriptorSetLayouts.Length())
		{
			layoutCreateInfo.setLayoutCount = static_cast<uint32>(descriptorSetLayouts.Length());
			layoutCreateInfo.pSetLayouts = descriptorSetLayouts.First();
		}

		//
		// NOTE(Ygsm):
		// The code above is only a temporary solution .........
		//

		VkPipelineLayout pipelineLayout;
		if (vkCreatePipelineLayout(Device, &layoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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

		if (CreateInfo.HasDepth)
		{
			depthStencilCreateInfo.depthTestEnable	= VK_TRUE;
			depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
		}

		//
		// TODO(Ygsm):
		// Need to revise depth stencil configuration in the pipeline.
		//

		if (CreateInfo.HasStencil)
		{
			depthStencilCreateInfo.stencilTestEnable = VK_TRUE;
			depthStencilCreateInfo.front = {};
			depthStencilCreateInfo.back  = {};
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};

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
		pipelineCreateInfo.renderPass = Ctx.Store.FramePasses[CreateInfo.FramePassHandle].Renderpass;
		pipelineCreateInfo.subpass = 0;							// TODO(Ygsm): Study more about this!
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// TODO(Ygsm): Study more about this!
		pipelineCreateInfo.basePipelineIndex = -1;				// TODO(Ygsm): Study more about this!

		if (CreateInfo.HasDepth || CreateInfo.HasStencil)
		{
			pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
		}

		VkPipeline pipeline;
		if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
		{
			Ctx.ColorBlendStates.Empty();
			VKT_ASSERT("Could not create graphics pipeline!" && false);
			return false;
		}

		*CreateInfo.Handle = g_RandIdVk();
		auto& pipelineParam = Ctx.Store.Pipelines.Insert(*CreateInfo.Handle, {});
		pipelineParam.Pipeline = pipeline;
		pipelineParam.Layout = pipelineLayout;

		Ctx.ColorBlendStates.Empty();

		return true;
	}

	void VulkanDriver::DestroyPipeline(Handle<HPipeline>& Hnd)
	{
		vkDeviceWaitIdle(Device);
		auto& pipelineParam = Ctx.Store.Pipelines[Hnd];
		vkDestroyPipelineLayout(Device, pipelineParam.Layout, nullptr);
		vkDestroyPipeline(Device, pipelineParam.Pipeline, nullptr);
		//Ctx.Store.Pipelines.Remove(Hnd);
		//new (&Hnd) Handle<HPipeline>(INVALID_HANDLE);
	}

	void VulkanDriver::ReleasePipeline(Handle<HPipeline>& Hnd)
	{
		Ctx.Store.Pipelines.Remove(Hnd);
		new (&Hnd) Handle<HPipeline>(INVALID_HANDLE);
	}

	//bool VulkanDriver::AddCommandBufferEntry(HwCmdBufferAllocInfo& AllocateInfo)
	//{
	//	uint32 id = g_RandIdVk();
	//	new (AllocateInfo.Handle) Handle<HCmdBuffer>(id);

	//	auto& storage = Ctx.Store.CommandBuffers.Insert(id, {});
	//	for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	//	{
	//		storage.Push(VkCommandBuffer());
	//	}

	//	return true;
	//}

	//void VulkanDriver::FreeCommandBuffer(Handle<HCmdBuffer>& Hnd)
	//{
	//	vkDeviceWaitIdle(Device);
	//	VkCommandBuffer* cmdBuffer = Ctx.Store.CommandBuffers[Hnd].First();
	//	//size_t numCmdBuffers = Ctx.Store.CommandBuffers[Hnd].Length();
	//	vkFreeCommandBuffers(Device, CommandPool, MAX_FRAMES_IN_FLIGHT, cmdBuffer);
	//	//CommandBuffers.PopAt(Hnd, false);
	//	Ctx.Store.CommandBuffers.Remove(Hnd);
	//	new (&Hnd) Handle<HCmdBuffer>(INVALID_HANDLE);
	//}

	//void VulkanDriver::PresentImageOCnScreen(Handle<HImage>& Hnd)
	//{
	//	//
	//	// TODO(Ygsm):
	//	//
	//	auto& imageParams = Ctx.Store.Images[Hnd];
	//	VkCommandBuffer& cmdBuffer = DefFramebuffer.CmdBuffers[CurrentFrame];
	//	VkCommandBufferBeginInfo cmdBufferBeginInfo;
	//	FMemory::InitializeObject(cmdBufferBeginInfo);

	//	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	//	VkRenderPassBeginInfo renderPassBeginInfo;
	//	FMemory::InitializeObject(renderPassBeginInfo);

	//	const VkClearValue clearValue = {
	//		{
	//			0, 0, 0, 0
	//		}
	//	};

	//	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	//	renderPassBeginInfo.framebuffer = DefFramebuffer.Framebuffers[CurrentFrame];
	//	renderPassBeginInfo.renderPass = DefFramebuffer.Renderpass;
	//	renderPassBeginInfo.renderArea.extent = SwapChain.Extent;
	//	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	//	renderPassBeginInfo.pClearValues = &clearValue;
	//	renderPassBeginInfo.clearValueCount = 1;

	//	VkImageBlit region;
	//	FMemory::InitializeObject(region);

	//	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//	region.srcSubresource.layerCount = 1;
	//	region.srcOffsets[1] = { static_cast<int32>(SwapChain.Extent.width), static_cast<int32>(SwapChain.Extent.height), 1 };

	//	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//	region.dstSubresource.layerCount = 1;
	//	region.dstOffsets[1] = { static_cast<int32>(SwapChain.Extent.width), static_cast<int32>(SwapChain.Extent.height), 1 };

	//	vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
	//	vkCmdBlitImage(
	//		cmdBuffer,
	//		imageParams.Image,
	//		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	//		SwapChain.Images[CurrentFrame],
	//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//		1,
	//		&region,
	//		VK_FILTER_NEAREST);
	//	vkEndCommandBuffer(cmdBuffer);
	//	CommandBuffersForSubmit.Push(cmdBuffer);
	//}

	void VulkanDriver::RecordCommandBuffer(HwCmdBufferRecordInfo& RecordInfo)
	{
		auto& frameParams = Ctx.Store.FramePasses[RecordInfo.FramePassHandle];

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};

		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		float32* color = RecordInfo.ClearColor;

		for (uint32 i = 0; i < RecordInfo.NumOutputs; i++)
		{
			VkClearValue& clearValue = Ctx.ClearValues.Insert(VkClearValue());
			clearValue.color = { color[Color_Channel_Red], color[Color_Channel_Green], color[Color_Channel_Blue], color[Color_Channel_Alpha] };

			if (i == RecordInfo.NumOutputs - 1 && RecordInfo.HasDepthStencil)
			{
				FMemory::Memzero(&clearValue, sizeof(VkClearValue));
				clearValue.depthStencil = { 1.0f, 0 };
			}
		}

		//const VkClearValue clearValue = {
		//	{
		//		color[Color_Channel_Red],
		//		color[Color_Channel_Green],
		//		color[Color_Channel_Blue],
		//		color[Color_Channel_Alpha]
		//	}
		//};

		//VkImageSubresourceRange imageSubresourceRange;
		//FMemory::InitializeObject(imageSubresourceRange);

		//imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		//imageSubresourceRange.levelCount = 1;
		//imageSubresourceRange.layerCount = 1;

		VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];

		vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);

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

		VkRenderPassBeginInfo renderPassBeginInfo = {};

		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass	= frameParams.Renderpass;
		renderPassBeginInfo.framebuffer = frameParams.Framebuffers[NextImageIndex];
		renderPassBeginInfo.renderArea.offset = { offset[Surface_Offset_X], offset[Surface_Offset_Y] };
		renderPassBeginInfo.renderArea.extent = { extent[Surface_Extent_Width], extent[Surface_Extent_Height] };
		renderPassBeginInfo.clearValueCount = RecordInfo.NumOutputs;
		renderPassBeginInfo.pClearValues = Ctx.ClearValues.First();

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		//VkViewport viewport = {};
		//viewport.x = 0.0f;
		//viewport.y = 0.0f;
		//viewport.width = static_cast<float32>(SwapChain.Extent.width);
		//viewport.height = static_cast<float32>(SwapChain.Extent.height);
		//viewport.minDepth = 0.0f;
		//viewport.maxDepth = 1.0f;

		//vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		if (RecordInfo.PipelineHandle != INVALID_HANDLE)
		{
			auto& pipelineParam = Ctx.Store.Pipelines[RecordInfo.PipelineHandle];
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineParam.Pipeline);
		}

		Ctx.ClearValues.Empty();
	}

	void VulkanDriver::UnrecordCommandBuffer(HwCmdBufferUnrecordInfo& UnrecordInfo)
	{
		auto& frameParams = Ctx.Store.FramePasses[UnrecordInfo.FramePassHandle];
		VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];

		if (UnrecordInfo.UnbindRenderpass)
		{
			vkCmdEndRenderPass(cmdBuffer);
		}

		if (UnrecordInfo.Unrecord)
		{
			vkEndCommandBuffer(cmdBuffer);
		}
	}

	void VulkanDriver::BindVboAndEbo(HwBindVboEboInfo& BindInfo)
	{
		auto& frameParams = Ctx.Store.FramePasses[BindInfo.FramePassHandle];
		auto& vboParams = Ctx.Store.Buffers[BindInfo.Vbo];

		VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];
		VkBuffer& vbo = vboParams.Buffer;
		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vbo, &offset);

		if (BindInfo.Ebo != INVALID_HANDLE)
		{
			auto& eboParams = Ctx.Store.Buffers[BindInfo.Ebo];
			VkBuffer& ebo = eboParams.Buffer;
			vkCmdBindIndexBuffer(cmdBuffer, ebo, 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void VulkanDriver::Draw(HwDrawInfo& DrawInfo)
	{
		auto& frameParams = Ctx.Store.FramePasses[DrawInfo.FramePassHandle];
		VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];

		if (DrawInfo.IndexCount)
		{
			vkCmdDrawIndexed(cmdBuffer, DrawInfo.IndexCount, 1, DrawInfo.IndexOffset, 0, 0);
		}
		else
		{
			vkCmdDraw(cmdBuffer, DrawInfo.VertexCount, 1, DrawInfo.VertexOffset, 0);
		}
	}

	void VulkanDriver::BlitImage(HwBlitInfo& BlitInfo)
	{
		auto& frameParams = Ctx.Store.FramePasses[BlitInfo.FramePassHandle];

		VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];
		VkImage& srcImage = Ctx.Store.Images[BlitInfo.Source].Image;
		VkImage& dstImage = Ctx.Store.Images[BlitInfo.Destination].Image;

		VkImageBlit region = {};
		region.srcSubresource.mipLevel		= 1;
		region.srcSubresource.layerCount	= 1;
		region.srcSubresource.aspectMask	= VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel		= 1;
		region.dstSubresource.layerCount	= 1;
		region.dstSubresource.aspectMask	= VK_IMAGE_ASPECT_COLOR_BIT;

		vkCmdBlitImage(cmdBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
	}

	void VulkanDriver::BlitImageToSwapchain(HwBlitInfo& BlitInfo)
	{
		auto& frameParams = Ctx.Store.FramePasses[BlitInfo.FramePassHandle];

		VkImage& srcImage = Ctx.Store.Images[BlitInfo.Source].Image;
		VkImage& dstImage = SwapChain.Images[NextImageIndex];

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};

		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		const VkClearValue clearValue = {
			{
				0.0f, 0.0f, 0.0f, 0.0f
			}
		};

		VkCommandBuffer& cmdBuffer = frameParams.CommandBuffers[CurrentFrame];

		vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);

		VkImageSubresourceRange subresourceRange = {};

		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		//VkMemoryBarrier memoryBarrier = {};

		//memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		//memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		//memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		VkImageMemoryBarrier imageBarrier = {};

		imageBarrier.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.srcAccessMask		 = VK_ACCESS_MEMORY_READ_BIT;
		imageBarrier.dstAccessMask		 = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.oldLayout			 = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier.newLayout			 = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.srcQueueFamilyIndex = Queues[Queue_Type_Present].FamilyIndex;
		imageBarrier.dstQueueFamilyIndex = Queues[Queue_Type_Graphics].FamilyIndex;
		imageBarrier.image				 = dstImage;
		imageBarrier.subresourceRange	 = subresourceRange;

		VkImageBlit region = {};
		
		int32 width		= static_cast<int32>(SwapChain.Extent.width);
		int32 height	= static_cast<int32>(SwapChain.Extent.height);

		VkOffset3D srcOffset = { width, height, 1 };
		VkOffset3D dstOffset = { width, height, 1 };

		region.srcOffsets[1] = srcOffset;
		region.dstOffsets[1] = dstOffset;

		region.srcSubresource.mipLevel	 = 0;
		region.srcSubresource.layerCount = 1;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel	 = 0;
		region.dstSubresource.layerCount = 1;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imageBarrier);

		vkCmdBlitImage(cmdBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

		imageBarrier.srcAccessMask	= VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
		imageBarrier.oldLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier.newLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		//memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		//memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&imageBarrier
		);

		vkEndCommandBuffer(cmdBuffer);
	}

	bool VulkanDriver::CreateImage(HwImageCreateStruct& CreateInfo)
	{
		bool success = true;
		uint32 id = g_RandIdVk();
		*CreateInfo.Handle = id;

		Ctx.Store.Images.Insert(id, {});
		auto& imageParam = Ctx.Store.Images[id];

		VkImageCreateInfo imageInfo = {};

		imageInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType		= Ctx.Flags.ImageType[CreateInfo.Type];
		imageInfo.format		= Ctx.Flags.Formats[CreateInfo.Usage];
		imageInfo.samples		= Ctx.Flags.SampleCounts[CreateInfo.Samples];
		imageInfo.extent.width	= CreateInfo.Width;
		imageInfo.extent.height = CreateInfo.Height;
		imageInfo.extent.depth	= CreateInfo.Depth;
		imageInfo.tiling		= VK_IMAGE_TILING_OPTIMAL;
		imageInfo.mipLevels		= CreateInfo.MipLevels;
		imageInfo.usage			= CreateInfo.UsageFlagBits;
		imageInfo.arrayLayers	= 1;
		imageInfo.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		VmaAllocationCreateInfo allocInfo = {};

		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		if (vmaCreateImage(Allocator, &imageInfo, &allocInfo, &imageParam.Image, &imageParam.Allocation, nullptr) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create image" && false);
			success = false;
			goto BeforeImageCreateEnd;
		}

		VkImageViewCreateInfo imageViewInfo;
		FMemory::InitializeObject(imageViewInfo);

		imageViewInfo.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewInfo.format							= Ctx.Flags.Formats[CreateInfo.Usage];
		imageViewInfo.subresourceRange.baseMipLevel		= 0;
		imageViewInfo.subresourceRange.levelCount		= CreateInfo.MipLevels;
		imageViewInfo.subresourceRange.baseArrayLayer	= 0;
		imageViewInfo.subresourceRange.layerCount		= 1;
		imageViewInfo.image								= imageParam.Image;
		imageViewInfo.viewType							= Ctx.Flags.ImageViewType[CreateInfo.Type];

		if ((CreateInfo.UsageFlagBits & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if ((CreateInfo.UsageFlagBits & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
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
		vkDeviceWaitIdle(Device);
		auto& imageParam = Ctx.Store.Images[Hnd];

		if (imageParam.Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(Device, imageParam.Sampler, nullptr);
		}

		vkDestroyImageView(Device, imageParam.ImageView, nullptr);
		vmaDestroyImage(Allocator, imageParam.Image, imageParam.Allocation);
		Ctx.Store.Images.Remove(Hnd);
		Hnd = INVALID_HANDLE;
	}

	bool VulkanDriver::CreateFramebuffer(HwFramebufferCreateInfo& CreateInfo)
	{
		bool hasDepthStencil = false;
		uint32 attOffset = 0;

		auto& frameParams = Ctx.Store.FramePasses[CreateInfo.Handle];

		const uint32 width	= (!CreateInfo.Width)	? SwapChain.Extent.width : static_cast<uint32>(CreateInfo.Width);
		const uint32 height = (!CreateInfo.Height)	? SwapChain.Extent.height : static_cast<uint32>(CreateInfo.Height);
		const uint32 depth	= (!CreateInfo.Depth)	? 1 : static_cast<uint32>(CreateInfo.Depth);

		const uint32 totalOutputs = CreateInfo.NumOutputs;
		const uint32 colorAttachmentCount = CreateInfo.NumColorOutputs;

		VKT_ASSERT("Must not exceed maximum attachment allowed in framebuffer" && totalOutputs <= MAX_FRAMEBUFFER_OUTPUTS);
		
		HwImageCreateStruct imageInfo;

		VkSamplerCreateInfo samplerInfo;
		FMemory::InitializeObject(samplerInfo);

		samplerInfo.sType			= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter		= VK_FILTER_LINEAR;
		samplerInfo.minFilter		= VK_FILTER_LINEAR;
		samplerInfo.mipmapMode		= VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU	= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV	= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW	= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.mipLodBias		= 0.0f;
		samplerInfo.maxAnisotropy	= 1.0f;
		samplerInfo.minLod			= 0.0f;
		samplerInfo.maxLod			= 1.0f;
		samplerInfo.borderColor		= VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		for (size_t i = 0; i < totalOutputs; i++)
		{
			HwAttachmentInfo& attachment = CreateInfo.Outputs[i];

			FMemory::Memzero(&imageInfo, sizeof(HwImageCreateStruct));
			FMemory::Memcpy(&imageInfo, &attachment, sizeof(HwAttachmentInfo));

			imageInfo.Width		= width;
			imageInfo.Height	= height;
			imageInfo.Depth		= depth;
			imageInfo.MipLevels = 1;
			imageInfo.Samples	= CreateInfo.Samples;
			imageInfo.UsageFlagBits = VK_IMAGE_USAGE_SAMPLED_BIT;

			switch (attachment.Usage)
			{
				case Texture_Usage_Color:
				case Texture_Usage_Color_HDR:
					imageInfo.UsageFlagBits |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
					break;
				case Texture_Usage_Depth_Stencil:
					imageInfo.UsageFlagBits |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
					break;
				default:
					break;
			}

			// Should probably check for fail here ...
			if (!CreateImage(imageInfo)) { VKT_ASSERT(false); }

			VkSampler& sampler = Ctx.Store.Images[*attachment.Handle].Sampler;
			vkCreateSampler(Device, &samplerInfo, nullptr, &sampler);
		}

		VkImageView imgViewAtt[MAX_FRAMEBUFFER_OUTPUTS];

		if (CreateInfo.DefaultOutputs[Default_Output_Color] != INVALID_HANDLE)
		{
			auto& imageParams = Ctx.Store.Images[CreateInfo.DefaultOutputs[Default_Output_Color]];
			imgViewAtt[attOffset++] = imageParams.ImageView;
		}

		for (size_t i = 0; i < totalOutputs; i++, attOffset++)
		{
			HwAttachmentInfo& attachment = CreateInfo.Outputs[i];
			auto& imageParams = Ctx.Store.Images[*attachment.Handle];

			if (attachment.Usage == Texture_Usage_Depth_Stencil)
			{
				hasDepthStencil = true;
			}

			imgViewAtt[attOffset] = imageParams.ImageView;
		}

		if (CreateInfo.DefaultOutputs[Default_Output_DepthStencil] != INVALID_HANDLE && !hasDepthStencil)
		{
			auto& imageParams = Ctx.Store.Images[CreateInfo.DefaultOutputs[Default_Output_DepthStencil]];
			imgViewAtt[attOffset++] = imageParams.ImageView;
		}

		VkFramebufferCreateInfo framebufferInfo = {};

		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;
		framebufferInfo.pAttachments = imgViewAtt;
		framebufferInfo.attachmentCount = attOffset;
		framebufferInfo.renderPass = frameParams.Renderpass;

		for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
		{
			if (vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &frameParams.Framebuffers[i]) != VK_SUCCESS)
			{
				VKT_ASSERT("Unable to create framebuffer" && false);
				return false;
			}
		}

		return true;
	};

	void VulkanDriver::DestroyFramebuffer(Handle<HFramePass>& Hnd)
	{
		auto& frameParams = Ctx.Store.FramePasses[Hnd];

		vkDeviceWaitIdle(Device);
		for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
		{
			vkDestroyFramebuffer(Device, frameParams.Framebuffers[i], nullptr);
			frameParams.Framebuffers[i] = VK_NULL_HANDLE;
		}

		//Hnd = INVALID_HANDLE;
	};

	bool VulkanDriver::CreateRenderPass(HwRenderpassCreateInfo& CreateInfo)
	{
		uint32 offset = 0;
		uint32 totalColorAttCount = 0;
		uint32 dependencyCount = 0;

		uint32 id = g_RandIdVk();

		*CreateInfo.Handle = id;
		Ctx.Store.FramePasses.Insert(id, {});

		auto& frameParams = Ctx.Store.FramePasses[id];

		VkAttachmentDescription attachmentDescs[MAX_FRAMEBUFFER_OUTPUTS];
		VkAttachmentReference	attachmentRef[MAX_FRAMEBUFFER_OUTPUTS];
		VkSubpassDependency		dependencies[MAX_FRAMEBUFFER_OUTPUTS];

		FMemory::Memzero(attachmentDescs,	MAX_FRAMEBUFFER_OUTPUTS * sizeof(VkAttachmentDescription));
		FMemory::Memzero(attachmentRef,		MAX_FRAMEBUFFER_OUTPUTS * sizeof(VkAttachmentReference));
		FMemory::Memzero(dependencies,		MAX_FRAMEBUFFER_OUTPUTS * sizeof(VkSubpassDependency));

		if (!CreateInfo.DefaultOutputs.Has(RenderPass_Bit_No_Color_Render))
		{
			VkAttachmentDescription& desc = attachmentDescs[offset];
			VkAttachmentReference& ref = attachmentRef[offset];
			VkSubpassDependency& dep = dependencies[offset];

			desc = {};
			ref = {};
			dep = {};

			desc.format = VK_FORMAT_R8G8B8A8_SRGB;
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			switch (CreateInfo.Order.Value())
			{
				case RenderPass_Order_Value_First:
					desc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
					desc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
					desc.finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					break;
				case RenderPass_Order_Value_FirstLast:
					desc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
					desc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
					desc.finalLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					break;
				case RenderPass_Order_Value_Last:
					desc.initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					desc.finalLayout	= VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					break;
				default:
					desc.initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					desc.finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					break;
			}

			ref.attachment = offset++;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			dep.srcSubpass = VK_SUBPASS_EXTERNAL;
			dep.dstSubpass = 0;
			dep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dep.srcAccessMask = 0;
			dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencyCount++;
			totalColorAttCount++;
		}

		for (size_t i = 0; i < CreateInfo.NumColorOutputs; i++, offset++)
		{
			VkAttachmentDescription& attDesc = attachmentDescs[offset];
			VkAttachmentReference& ref = attachmentRef[offset];
			VkSubpassDependency& dep = dependencies[offset];

			attDesc = {};
			ref = {};
			dep = {};

			attDesc.format = VK_FORMAT_R8G8B8A8_SRGB;
			attDesc.samples = Ctx.Flags.SampleCounts[CreateInfo.Samples];
			attDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			ref.attachment = offset;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			dep.srcSubpass = VK_SUBPASS_EXTERNAL;
			dep.dstSubpass = 0;
			dep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			//dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dep.srcAccessMask = 0;
			dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencyCount++;

			totalColorAttCount++;
		}

		// NOTE(Ygsm):
		// Since framebuffers only allow a single depth stencil attachment, we have to make a choice.
		// By default the default output's depth attachment is used but if it has it's own depth stencil attachment,
		// we set up that instead.

		// TODO(Ygsm):
		// The current set up does not include depth attachment that is used in subsequent passes as input.
		if (CreateInfo.HasDepthStencilAttachment || !CreateInfo.DefaultOutputs.Has(RenderPass_Bit_No_DepthStencil_Render))
		{
			VkAttachmentDescription& desc = attachmentDescs[offset];
			VkAttachmentReference& ref = attachmentRef[offset];

			desc.format			= VK_FORMAT_D24_UNORM_S8_UINT;
			desc.samples		= Ctx.Flags.SampleCounts[CreateInfo.Samples];
			desc.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			if (CreateInfo.HasDepthStencilAttachment)
			{
				desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				dependencyCount++;
			}

			uint32 dependencyOffset = (!CreateInfo.DefaultOutputs.Has(RenderPass_Bit_No_Color_Render)) ? 0 : offset;

			VkSubpassDependency& dep = dependencies[dependencyOffset];

			ref.attachment	= offset++;
			ref.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			dep.srcSubpass = VK_SUBPASS_EXTERNAL;
			dep.dstSubpass = 0;
			dep.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dep.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dep.srcAccessMask = 0;
			dep.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		}

		VkSubpassDescription subpassDescription = {};

		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.pColorAttachments = attachmentRef;
		subpassDescription.colorAttachmentCount = totalColorAttCount;

		if (CreateInfo.HasDepthStencilAttachment || !CreateInfo.DefaultOutputs.Has(RenderPass_Bit_No_DepthStencil_Render))
		{
			subpassDescription.pDepthStencilAttachment = &attachmentRef[offset - 1];
		}

		// Create actual renderpass.
		VkRenderPassCreateInfo renderPassInfo = {};

		renderPassInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount	= offset;
		renderPassInfo.pAttachments		= attachmentDescs;
		renderPassInfo.pSubpasses		= &subpassDescription;
		renderPassInfo.subpassCount		= 1;
		renderPassInfo.pDependencies	= dependencies;
		renderPassInfo.dependencyCount	= dependencyCount;

		if (vkCreateRenderPass(Device, &renderPassInfo, nullptr, &frameParams.Renderpass) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create render pass." && false);
			return false;
		}

		return true;
	}

	void VulkanDriver::DestroyRenderPass(Handle<HFramePass>& Hnd)
	{
		auto& frameParams = Ctx.Store.FramePasses[Hnd];

		vkDeviceWaitIdle(Device);
		vkDestroyRenderPass(Device, frameParams.Renderpass, nullptr);
		frameParams.Renderpass = VK_NULL_HANDLE;
	}

	void VulkanDriver::ReleaseFramePass(Handle<HFramePass>& Hnd)
	{
		Ctx.Store.FramePasses.Remove(Hnd);
	}

	void VulkanDriver::PushCmdBufferForSubmit(Handle<HFramePass>& Hnd)
	{
		auto& frameParams = Ctx.Store.FramePasses[Hnd];
		VkCommandBuffer& commandBuffer = frameParams.CommandBuffers[CurrentFrame];
		CommandBuffersForSubmit.Push(commandBuffer);
	}

	bool VulkanDriver::AllocateDescriptorSet(HwDescriptorSetAllocateInfo& AllocInfo)
	{
		auto& poolParams = Ctx.Store.DescriptorPools[AllocInfo.Pool];

		VkDescriptorSetLayout& layout = Ctx.Store.DescriptorLayout[AllocInfo.Layout];

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			allocInfo.descriptorPool = poolParams.DescPool[i];
			vkAllocateDescriptorSets(Device, &allocInfo, &descriptorSets[i]);
		}

		uint32 id = g_RandIdVk();
		*AllocInfo.Handle = id;
		auto& descSetParams = Ctx.Store.DescriptorSets.Insert(id, {});

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descSetParams.DescSet[i] = descriptorSets[i];
		}

		return true;
	}

	void VulkanDriver::MapDescSetToBuffer(HwDescriptorSetMapInfo& MapInfo)
	{
		auto& descSetParams = Ctx.Store.DescriptorSets[MapInfo.DescriptorSetHandle];
		auto& bufferParams = Ctx.Store.Buffers[MapInfo.BufferHandle];

		const uint32 maxDeviceRange = GPUProperties.limits.maxUniformBufferRange;
		const uint32 mappingRange = MapInfo.Range * MAX_FRAMES_IN_FLIGHT;
		uint32 remainingRange = mappingRange - maxDeviceRange;

		if (remainingRange <= 0)
		{
			remainingRange = 0;
		}

		// NOTE(Ygsm):
		// Not sure if this would work...
		Array<VkDescriptorBufferInfo> bufferInfos;

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = bufferParams.Buffer;

		bufferInfo.offset = MapInfo.Offset;
		bufferInfo.range = mappingRange - remainingRange;
		bufferInfos.Push(bufferInfo);

		if (remainingRange > 0)
		{
			bufferInfo.offset = mappingRange - remainingRange;
			bufferInfo.range = remainingRange;
			bufferInfos.Push(bufferInfo);
		}

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType		= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstBinding	= MapInfo.Binding;
		descriptorWrite.descriptorType = Ctx.Flags.DescriptorType[MapInfo.DescriptorType];
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo		= bufferInfos.First();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			//bufferInfo.offset = i * MapInfo.Range;
			descriptorWrite.dstSet = descSetParams.DescSet[i];
			vkUpdateDescriptorSets(Device, 1, &descriptorWrite, 0, nullptr);
		}
	}

	void VulkanDriver::BindDescriptorSets(HwDescriptorSetBindInfo& BindInfo)
	{
		auto& pipelineParam = Ctx.Store.Pipelines[BindInfo.PipelineHandle];
		auto& descriptorSetParam = Ctx.Store.DescriptorSets[BindInfo.DescriptorSetHandle];
		auto& framePassParams = Ctx.Store.FramePasses[BindInfo.FramePassHandle];

		VkCommandBuffer commandBuffer = framePassParams.CommandBuffers[CurrentFrame];
		VkDescriptorSet descriptorSet = descriptorSetParam.DescSet[CurrentFrame];

		VkDescriptorType type = Ctx.Flags.DescriptorType[BindInfo.DescriptorType];

		uint32 dynamicOffsetCount = 0;
		uint32* dynamicOffset = nullptr;

		if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
			dynamicOffsetCount = 1;
			dynamicOffset = &BindInfo.Offset;
		}

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineParam.Layout,
			0,
			1,
			&descriptorSet,
			dynamicOffsetCount,
			dynamicOffset);
	}

	void VulkanDriver::MapDataToDescriptorSet(HwDataToDescriptorSetMapInfo& MapInfo)
	{
		auto& descSetParam = Ctx.Store.DescriptorSets[MapInfo.DescriptorSetHandle];
		auto& bufferParam = Ctx.Store.Buffers[MapInfo.BufferHandle];
		uint8* ptr = reinterpret_cast<uint8*>(bufferParam.Data + MapInfo.Offset);
		FMemory::Memcpy(ptr, MapInfo.Data, MapInfo.Size);
	}

	size_t VulkanDriver::PadDataSizeForUniform(size_t Size)
	{
		const size_t minUboAlignment = GPUProperties.limits.minUniformBufferOffsetAlignment;
		size_t alignedSize = Size;
		if (minUboAlignment)
		{
			alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		return alignedSize;
	}
}