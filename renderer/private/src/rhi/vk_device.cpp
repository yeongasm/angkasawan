#include "math/math.h"
#include "device.h"
#include "rhi/vulkan/vk_device.h"
#include "rhi/shader_compiler.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_messenger_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	auto translate_message_severity = [](VkDebugUtilsMessageSeverityFlagBitsEXT flag) -> rhi::ErrorSeverity
	{
		switch (flag)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return rhi::ErrorSeverity::Verbose;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return rhi::ErrorSeverity::Info;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return rhi::ErrorSeverity::Warning;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		default:
			return rhi::ErrorSeverity::Error;
		}
	};

	rhi::DeviceInitInfo* pInfo = static_cast<rhi::DeviceInitInfo*>(pUserData);
	rhi::ErrorSeverity sv = translate_message_severity(severity);
	pInfo->callback(sv, pCallbackData->pMessage, pInfo->data);

	return VK_TRUE;
}


VkDebugUtilsMessengerCreateInfoEXT populate_debug_messenger(void* data)
{
	return VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

		.pfnUserCallback = debug_util_messenger_callback,
		.pUserData = data
	};
}

namespace rhi
{

RHIObjTypeID get_next_rhi_object_id()
{
	static RHIObjTypeID::value_type _id = 0;
	return RHIObjTypeID{ ++_id };
}

void DeviceContext::clear_zombies()
{
	// The device has to stop using the resources before we can clean them up.
	/**
	* TODO(afiq):
	* Figure out a way to not wait for the device to idle before releasing the used resource.
	*/
	vkDeviceWaitIdle(device);

	for (const ZombieObject& zombie : cache.zombies)
	{
		RHIObjTypeID const type{ zombie.type };

		if (type == rhi_obj_type_id_v<VulkanBuffer>)
		{
			release_buffer(zombie.location);
		}
		else if (type == rhi_obj_type_id_v<VulkanImage>)
		{
			destroy_image(zombie.location);
		}
		else if (type == rhi_obj_type_id_v<VulkanSemaphore>)
		{
			destroy_semaphore(zombie.location);
		}
		else if (type == rhi_obj_type_id_v<VulkanPipeline>)
		{
			destroy_pipeline(zombie.location);
		}
		else if (type == rhi_obj_type_id_v<VulkanSwapchain>)
		{
			destroy_swapchain(zombie.location);
		}
		else if (type == rhi_obj_type_id_v<VulkanSurface>)
		{
			destroy_surface(zombie.location);
		}
		else if (type == rhi_obj_type_id_v<CommandArena>)
		{
			destroy_command_arena(zombie.location);
		}
	}
}

bool DeviceContext::create_descriptor_pool()
{
	VkDescriptorPoolSize bufferDescriptorPoolSize{
		.type				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount	= config.bufferDescriptorsCount
	};

	VkDescriptorPoolSize storageImageDescriptorPoolSize{
		.type				= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount	= config.storageImageDescriptorsCount
	};

	VkDescriptorPoolSize sampledImageDescriptorPoolSize{
		.type				= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount	= config.sampledImageDescriptorsCount
	};

	VkDescriptorPoolSize samplerDescriptorPoolSize{
		.type				= VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount	= config.samplerDescriptorCount
	};

	VkDescriptorPoolSize poolSizes[] = {
		bufferDescriptorPoolSize,
		storageImageDescriptorPoolSize,
		sampledImageDescriptorPoolSize,
		samplerDescriptorPoolSize
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo{
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext			= nullptr,
		.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets		= 1,
		.poolSizeCount	= static_cast<uint32>(std::size(poolSizes)),
		.pPoolSizes		= poolSizes,
	};

	return vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &cache.descriptorPool) == VK_SUCCESS;
}

bool DeviceContext::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding bufferDescriptorLayoutBinding{
		.binding			= 0,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount	= config.bufferDescriptorsCount,
		.stageFlags			= VK_SHADER_STAGE_ALL,
		.pImmutableSamplers = nullptr,
	};

	VkDescriptorSetLayoutBinding storageImageDescriptorLayoutBinding{
		.binding			= 1,
		.descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount	= config.storageImageDescriptorsCount,
		.stageFlags			= VK_SHADER_STAGE_ALL,
		.pImmutableSamplers = nullptr,
	};

	VkDescriptorSetLayoutBinding sampledImageDescriptorLayoutBinding{
		.binding			= 2,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount	= config.sampledImageDescriptorsCount,
		.stageFlags			= VK_SHADER_STAGE_ALL,
		.pImmutableSamplers = nullptr,
	};

	VkDescriptorSetLayoutBinding samplerDescriptorLayoutBinding{
		.binding			= 3,
		.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount	= config.samplerDescriptorCount,
		.stageFlags			= VK_SHADER_STAGE_ALL,
		.pImmutableSamplers = nullptr,
	};

	/*VkDescriptorSetLayoutBinding bufferAddressDescriptorLayoutBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_ALL,
		.pImmutableSamplers = nullptr,
	};*/

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
		bufferDescriptorLayoutBinding,
		storageImageDescriptorLayoutBinding,
		sampledImageDescriptorLayoutBinding,
		samplerDescriptorLayoutBinding/*,
		bufferAddressDescriptorLayoutBinding,*/
	};

	VkDescriptorBindingFlags descriptorBindingFlags[] = {
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT/*,
		VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT*/
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingCreateInfo{
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.pNext			= nullptr,
		.bindingCount	= static_cast<uint32>(std::size(descriptorBindingFlags)),
		.pBindingFlags	= descriptorBindingFlags,
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext			= &descriptorSetLayoutBindingCreateInfo,
		.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount	= static_cast<uint32>(std::size(descriptorSetLayoutBindings)),
		.pBindings		= descriptorSetLayoutBindings,
	};

	return vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &cache.descriptorSetLayout) == VK_SUCCESS;
}

bool DeviceContext::allocate_descriptor_set()
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{
		.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext				= nullptr,
		.descriptorPool		= cache.descriptorPool,
		.descriptorSetCount = 1u,
		.pSetLayouts		= &cache.descriptorSetLayout,
	};

	return vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &cache.descriptorSet) == VK_SUCCESS;
}

bool DeviceContext::create_pipeline_layouts()
{
	// The vulkan spec states that the size of a push constant must be a multiple of 4.
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPushConstantRange.html
	uint32 constexpr multiple	= 4u;
	uint32 const count			= static_cast<uint32>(config.pushConstantMaxSize / multiple) + 1u;

	cache.pipelineLayouts.reserve(count);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType			= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1u,
		.pSetLayouts	= &cache.descriptorSetLayout,
	};

	VkPipelineLayout layout0;
	if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout0) != VK_SUCCESS)
	{
		return false;
	}
	cache.pipelineLayouts.emplace(0u, std::move(layout0));

	for (uint32 i = 1u; i < count; ++i)
	{
		VkPushConstantRange range{
			.stageFlags = VK_SHADER_STAGE_ALL,
			.offset		= 0u,
			.size		= static_cast<uint32>(i * multiple)
		};

		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &range;

		VkPipelineLayout hlayout;
		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &hlayout) == VK_SUCCESS)
		{
			if (hlayout != VK_NULL_HANDLE)
			{
				cache.pipelineLayouts.emplace(range.size, hlayout);
			}
		}
	}

	return true;
}

VkPipelineLayout DeviceContext::get_appropriate_pipeline_layout(uint32 pushConstantSize, const uint32 max)
{
	uint32 num = std::min(pushConstantSize, max);
	if (pushConstantSize < max)
	{
		--num;
		for (uint32 i = 1; i < sizeof(uint32) * CHAR_BIT; i *= 2)
		{
			num |= num >> i;
		}
		++num;
	}
	return cache.pipelineLayouts[num];
}

/*DeviceErrorCode DeviceContext::acquire_next_swapchain_image(const SwapchainAcquireNextImageInfo& info, uint32& out)
{
	auto [_0, swapchain] = m_cache.swapchains.fetch_resource(info.swapchain.get());
	auto [_1, semaphore] = m_cache.semaphores.fetch_resource(info.semaphore.get());

	VkResult result = vkAcquireNextImageKHR(m_device, swapchain.swapchain, UINT64_MAX, semaphore.semaphore, VK_NULL_HANDLE, &out);
	if (result != VK_SUCCESS)
	{
		return vulkan_to_device_swapchain_error_code(result);
	}

	return DeviceErrorCode::Ok;
}*/

bool DeviceContext::initialize_resource_cache()
{
	// Readjust configuration values.
	config.sampledImageDescriptorsCount = std::min(std::min(properties.limits.maxDescriptorSetSampledImages, properties.limits.maxDescriptorSetStorageImages), config.sampledImageDescriptorsCount);
	config.storageImageDescriptorsCount	= std::min(std::min(properties.limits.maxDescriptorSetStorageImages, properties.limits.maxDescriptorSetSampledImages), config.bufferDescriptorsCount);
	config.bufferDescriptorsCount		= std::min(properties.limits.maxDescriptorSetStorageBuffers, config.bufferDescriptorsCount);
	config.samplerDescriptorCount		= std::min(properties.limits.maxDescriptorSetSamplers, config.samplerDescriptorCount);
	config.pushConstantMaxSize			= std::min(properties.limits.maxPushConstantsSize, config.pushConstantMaxSize);

	if (!create_descriptor_pool())			{ return false; }
	if (!create_descriptor_set_layout())	{ return false; }
	if (!allocate_descriptor_set())			{ return false; }
	if (!create_pipeline_layouts())			{ return false; }

	return true;
}

void DeviceContext::cleanup_resource_cache()
{
	// Don't need to call vkDeviceWaitIdle here since it's already invoked in the function below.
	clear_zombies();

	// Remove pre-allocated pipeline layouts.
	for (auto const& [_, layout] : cache.pipelineLayouts)
	{
		vkDestroyPipelineLayout(device, layout, nullptr);
	}
	cache.pipelineLayouts.clear();

	// Remove all descriptor related items.
	vkFreeDescriptorSets(device, cache.descriptorPool, 1, &cache.descriptorSet);
	cache.descriptorSet = VK_NULL_HANDLE;

	vkDestroyDescriptorSetLayout(device, cache.descriptorSetLayout, nullptr);
	cache.descriptorSetLayout = VK_NULL_HANDLE;

	vkDestroyDescriptorPool(device, cache.descriptorPool, nullptr);
	cache.descriptorPool = VK_NULL_HANDLE;
}

bool DeviceContext::create_vulkan_instance(DeviceInitInfo const& info)
{
	const auto& appVer = info.applicationVersion;
	const auto& engineVer = info.engineVersion;

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = info.applicationName.data(),
		.applicationVersion = VK_MAKE_API_VERSION(appVer.variant, appVer.major, appVer.minor, appVer.patch),
		.pEngineName = info.engineName.data(),
		.engineVersion = VK_MAKE_API_VERSION(engineVer.variant, engineVer.major, engineVer.minor, engineVer.patch),
		.apiVersion = VK_API_VERSION_1_3
	};

	std::vector<literal_t> extensions;
	extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if WIN32
	extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	if (validation)
	{
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	VkInstanceCreateInfo instanceInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = static_cast<uint32>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};

	if (validation)
	{
		literal_t layers[] = { "VK_LAYER_KHRONOS_validation" };
		instanceInfo.enabledLayerCount = 1;
		instanceInfo.ppEnabledLayerNames = layers;
		auto debugUtil = populate_debug_messenger(const_cast<DeviceInitInfo*>(&info));
		instanceInfo.pNext = &debugUtil;
	}

	return vkCreateInstance(&instanceInfo, nullptr, &instance) == VK_SUCCESS;
}

bool DeviceContext::create_debug_messenger(DeviceInitInfo const& info)
{
	if (validation)
	{
		auto debugUtilInfo = populate_debug_messenger(const_cast<DeviceInitInfo*>(&info));
		return vkCreateDebugUtilsMessengerEXT(instance, &debugUtilInfo, nullptr, &debugger) == VK_SUCCESS;
	}
	return true;
}

bool DeviceContext::choose_physical_device()
{
	constexpr uint32 MAX_PHYSICAL_DEVICE = 8u;

	auto score_device = [](VkPhysicalDevice& device) -> uint32
	{
		uint32 score = 0;
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		switch (properties.deviceType)
		{
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			score = 10000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			score = 1000;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			score = 100;
			break;
		default:
			score = 10;
			break;
		}

		score += static_cast<uint32>(properties.limits.maxMemoryAllocationCount / 1000u);
		score += static_cast<uint32>(properties.limits.maxBoundDescriptorSets / 1000u);
		score += static_cast<uint32>(properties.limits.maxDrawIndirectCount / 1000u);
		score += static_cast<uint32>(properties.limits.maxDrawIndexedIndexValue / 1000u);

		return score;
	};

	uint32 numDevices = 0;
	vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);

	if (!numDevices) { return false; }

	if (numDevices > MAX_PHYSICAL_DEVICE)
	{
		numDevices = MAX_PHYSICAL_DEVICE;
	}

	VkPhysicalDevice devices[MAX_PHYSICAL_DEVICE] = { VK_NULL_HANDLE };
	vkEnumeratePhysicalDevices(instance, &numDevices, devices);

	std::sort(
		std::begin(devices), 
		std::end(devices), 
		[score_device](VkPhysicalDevice& a, VkPhysicalDevice& b) -> bool
		{
			if (a == VK_NULL_HANDLE || b == VK_NULL_HANDLE)
			{
				return false;
			}
			uint32 aScore = score_device(a);
			uint32 bScore = score_device(b);
			return aScore > bScore;
		}
	);
	gpu = devices[0];

	vkGetPhysicalDeviceProperties(gpu, &properties);
	vkGetPhysicalDeviceFeatures(gpu, &features);

	return true;
}

void DeviceContext::get_device_queue_family_indices()
{
	constexpr uint32 MAX_QUEUE_FAMILY_SEARCH = 8u;
	/**
	* The Vulkan spec states that implementation MUST support at least ONE queue family.
	* That means the posibility of the GPU not having a queue family is non-existent.
	*/
	constexpr uint32 uint32Max = std::numeric_limits<uint32>::max();

	uint32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, nullptr);

	if (queueCount > MAX_QUEUE_FAMILY_SEARCH)
	{
		queueCount = MAX_QUEUE_FAMILY_SEARCH;
	}

	/*VkBool32 presentSupport[MAX_QUEUE_FAMILY_SEARCH];*/
	VkQueueFamilyProperties queueProperties[MAX_QUEUE_FAMILY_SEARCH];
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueCount, queueProperties);

	for (uint32 i = 0; i < queueCount; ++i)
	{
		if (mainQueue.familyIndex		!= uint32Max &&
			transferQueue.familyIndex	!= uint32Max &&
			computeQueue.familyIndex	!= uint32Max)
		{
			break;
		}

		VkQueueFamilyProperties const& property = queueProperties[i];
		/*vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu, i, m_swapchain.surface, &presentSupport[i]);*/
		if (property.queueCount)
		{
			// We look for a queue that's able to do graphics, transfer, compute and presentation.
			if (mainQueue.familyIndex == uint32Max &&
				/*presentSupport[i] &&*/
				(property.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0	&&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0	&&
				(property.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
			{
				mainQueue.familyIndex	= i;
				mainQueue.properties	= property;
			}
			// We look for a queue that can only do transfer operations.
			if (transferQueue.familyIndex == uint32Max &&
				(property.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0	&&
				(property.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0	&&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
			{
				transferQueue.familyIndex	= i;
				transferQueue.properties	= property;
			}
			// We look for a queue that is capable of doing compute but is not in the same family index as the main graphics queue.
			// This is for async compute.
			if (computeQueue.familyIndex == uint32Max &&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0	&&
				(mainQueue.familyIndex		!= i)					&&
				(transferQueue.familyIndex	!= i))
			{
				computeQueue.familyIndex	= i;
				computeQueue.properties		= property;
			}
		}
	}
	// If the transfer queue family index is still not set by this point, we fall back to the graphics queue.
	if (transferQueue.familyIndex == uint32Max)
	{
		transferQueue.familyIndex	= mainQueue.familyIndex;
		transferQueue.properties	= mainQueue.properties;
	}
	// If the compute queue family index is still not set by this point, we fall back to the graphics queue.
	if (computeQueue.familyIndex == uint32Max)
	{
		computeQueue.familyIndex	= mainQueue.familyIndex;
		computeQueue.properties		= mainQueue.properties;
	}
}

bool DeviceContext::create_logical_device()
{
	constexpr float32 priority = 1.f;
	size_t reserveCount = 3;

	if (mainQueue.familyIndex == transferQueue.familyIndex) 
	{ 
		--reserveCount; 
	}

	if (mainQueue.familyIndex == computeQueue.familyIndex) 
	{ 
		--reserveCount; 
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(reserveCount);

	// Always create a graphics queue.
	queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = mainQueue.familyIndex,
		.queueCount = 1,
		.pQueuePriorities = &priority
	});

	// If the transfer queue and the graphics queue are different, we create one.
	if (transferQueue.familyIndex != mainQueue.familyIndex)
	{
		queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = transferQueue.familyIndex,
			.queueCount = 1,
			.pQueuePriorities = &priority
		});
	}
	// If the compute queue and the graphics queue are different, we create one.
	if (computeQueue.familyIndex != mainQueue.familyIndex)
	{
		queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = computeQueue.familyIndex,
			.queueCount = 1,
			.pQueuePriorities = &priority
		});
	}

	literal_t extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceVulkan13Features deviceFeatures13{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.dynamicRendering = VK_TRUE
	};

	VkPhysicalDeviceVulkan12Features deviceFeatures12{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &deviceFeatures13,
		.drawIndirectCount = VK_TRUE,
		.descriptorIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
		.descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.timelineSemaphore = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE
	};

	VkPhysicalDeviceFeatures2 deviceFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &deviceFeatures12,
		.features = {
			.fullDrawIndexUint32 = VK_TRUE,
			.multiDrawIndirect = VK_TRUE,
			.multiViewport = VK_TRUE,
			.samplerAnisotropy = VK_TRUE
		}
	};

	VkDeviceCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &deviceFeatures,
		.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = extensions
	};

	return vkCreateDevice(gpu, &info, nullptr, &device) == VK_SUCCESS;
}

void DeviceContext::get_queue_handles()
{
	vkGetDeviceQueue(device, mainQueue.familyIndex, 0, &mainQueue.queue);
	vkGetDeviceQueue(device, transferQueue.familyIndex, 0, &transferQueue.queue);
	vkGetDeviceQueue(device, computeQueue.familyIndex, 0, &computeQueue.queue);
}

bool DeviceContext::create_device_allocator()
{
	VmaVulkanFunctions vmaVulkanFunctions{
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr
	};

	VmaAllocatorCreateInfo info{
		.flags							= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice					= gpu,
		.device							= device,
		.preferredLargeHeapBlockSize	= 0, // Sets it to lib internal default (256MiB).
		.pAllocationCallbacks			= nullptr,
		.pDeviceMemoryCallbacks			= nullptr,
		.pHeapSizeLimit					= nullptr,
		.pVulkanFunctions				= &vmaVulkanFunctions,
		.instance						= instance,
		.vulkanApiVersion				= VK_API_VERSION_1_3,
	};

	return vmaCreateAllocator(&info, &allocator) == VK_SUCCESS;
}

void DeviceContext::destroy_surface(uint32 location)
{
	auto [valid, surface] = cache.surfaces.fetch_resource(location);
	if (valid)
	{
		vkDestroySurfaceKHR(instance, surface.surface, nullptr);
		cache.surfaces.return_resource(location);
	}
}

void DeviceContext::destroy_swapchain(uint32 location)
{
	auto [valid, swapchain] = cache.swapchains.fetch_resource(location);
	if (valid)
	{
		for (auto& imageView : swapchain.imageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
		cache.swapchains.return_resource(location);
	}
}

void DeviceContext::destroy_image(uint32 location)
{
	auto [valid, image] = cache.images.fetch_resource(location);
	if (valid)
	{
		vmaDestroyImage(allocator, image.image, image.allocation);
		vkDestroyImageView(device, image.imageView, nullptr);
		cache.images.return_resource(location);
	}
}

void DeviceContext::destroy_semaphore(uint32 location)
{
	auto [valid, semaphore] = cache.semaphores.fetch_resource(location);
	if (valid)
	{
		vkDestroySemaphore(device, semaphore.semaphore, nullptr);
		cache.semaphores.return_resource(location);
	}
}

void DeviceContext::destroy_pipeline(uint32 location)
{
	auto [valid, pipeline] = cache.pipelines.fetch_resource(location);
	if (valid)
	{
		vkDestroyPipeline(device, pipeline.pipeline, nullptr);
		cache.pipelines.return_resource(location);
	}
}

void DeviceContext::release_buffer(uint32 location)
{
	auto [valid, buffer] = cache.buffers.fetch_resource(location);
	if (valid)
	{
		vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
		cache.buffers.return_resource(location);
	}
}

void DeviceContext::destroy_command_arena(uint32 location)
{
	auto [valid, cmdArena] = cache.commandArena.fetch_resource(location);

	if (valid)
	{
		vkDestroyCommandPool(device, cmdArena.commandPool, nullptr);
		cache.commandArena.return_resource(location);
	}
}

VkImageUsageFlags DeviceContext::translate_image_usage_flags(ImageUsage flags) const
{
	RhiFlag const mask = static_cast<RhiFlag>(flags);
	if (mask == 0)
	{
		return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
	}

	constexpr VkImageUsageFlagBits flagBits[] = {
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT,
		VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR
	};
	VkImageUsageFlags result{};
	for (uint32 i = 0; i < static_cast<uint32>(std::size(flagBits)); ++i)
	{
		uint32 const exist = (mask & (1 << i));
		auto const bit = flagBits[i];
		result |= (exist & bit);
	}
	return result;
}

VkImageAspectFlags DeviceContext::translate_image_aspect_flags(ImageAspect flags) const
{
	RhiFlag const mask = static_cast<RhiFlag>(flags);
	if (mask == 0)
	{
		return VK_IMAGE_ASPECT_NONE;
	}

	constexpr VkImageAspectFlags flagBits[] = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_ASPECT_STENCIL_BIT,
		VK_IMAGE_ASPECT_METADATA_BIT,
		VK_IMAGE_ASPECT_PLANE_0_BIT,
		VK_IMAGE_ASPECT_PLANE_1_BIT,
		VK_IMAGE_ASPECT_PLANE_2_BIT
	};
	VkImageUsageFlags result{};
	for (uint32 i = 0; i < static_cast<uint32>(std::size(flagBits)); ++i)
	{
		uint32 const exist = (mask & (1 << i));
		auto const bit = flagBits[i];
		result |= (exist & bit);
	}
	return result;
}

VkImageLayout DeviceContext::translate_image_layout(ImageLayout layout) const
{
	switch (layout)
	{;
	case ImageLayout::General:
		return VK_IMAGE_LAYOUT_GENERAL;
	case ImageLayout::Color_Attachment:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Stencil_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	case ImageLayout::Shader_Read_Only:
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case ImageLayout::Transfer_Src:
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case ImageLayout::Transfer_Dst:
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case ImageLayout::Preinitialized:
		return VK_IMAGE_LAYOUT_PREINITIALIZED;
	case ImageLayout::Depth_Read_Only_Stencil_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Attachment_Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	case ImageLayout::Depth_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	case ImageLayout::Depth_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
	case ImageLayout::Stencil_Attachment:
		return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
	case ImageLayout::Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
	case ImageLayout::Read_Only:
		return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
	case ImageLayout::Attachment:
		return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	case ImageLayout::Present_Src:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	case ImageLayout::Shared_Present:
		return VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
	case ImageLayout::Fragment_Density_Map:
		return VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	case ImageLayout::Fragment_Shading_Rate_Attachment:
		return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	case ImageLayout::Undefined:
	default:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

VkImageType	DeviceContext::translate_image_type(ImageType type) const
{
	switch (type)
	{
	case ImageType::Image_1D:
		return VK_IMAGE_TYPE_1D;
	case ImageType::Image_2D:
		return VK_IMAGE_TYPE_2D;
	case ImageType::Image_3D:
		return VK_IMAGE_TYPE_3D;
	default:
		return VK_IMAGE_TYPE_MAX_ENUM;
	}
}

VkImageViewType	DeviceContext::translate_image_view_type(ImageType type) const
{
	switch (type)
	{
	case ImageType::Image_1D:
		return VK_IMAGE_VIEW_TYPE_1D;
	case ImageType::Image_2D:
		return VK_IMAGE_VIEW_TYPE_2D;
	case ImageType::Image_3D:
		return VK_IMAGE_VIEW_TYPE_3D;
	default:
		return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	}
}

VkImageTiling DeviceContext::translate_image_tiling(ImageTiling tiling) const
{
	switch (tiling)
	{
	case ImageTiling::Optimal:
		return VK_IMAGE_TILING_OPTIMAL;
	case ImageTiling::Linear:
		return VK_IMAGE_TILING_LINEAR;
	default:
		return VK_IMAGE_TILING_MAX_ENUM;
	}
}

VkFormat DeviceContext::translate_image_format(ImageFormat format) const
{
	constexpr VkFormat formats[] = {
		VK_FORMAT_UNDEFINED,
		VK_FORMAT_R4G4_UNORM_PACK8,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_B5G6R5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,
		VK_FORMAT_A1R5G5B5_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_USCALED,
		VK_FORMAT_R8_SSCALED,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8_SRGB,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_USCALED,
		VK_FORMAT_R8G8_SSCALED,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8_SRGB,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_SNORM,
		VK_FORMAT_R8G8B8_USCALED,
		VK_FORMAT_R8G8B8_SSCALED,
		VK_FORMAT_R8G8B8_UINT,
		VK_FORMAT_R8G8B8_SINT,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_B8G8R8_UNORM,
		VK_FORMAT_B8G8R8_SNORM,
		VK_FORMAT_B8G8R8_USCALED,
		VK_FORMAT_B8G8R8_SSCALED,
		VK_FORMAT_B8G8R8_UINT,
		VK_FORMAT_B8G8R8_SINT,
		VK_FORMAT_B8G8R8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_USCALED,
		VK_FORMAT_R8G8B8A8_SSCALED,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SNORM,
		VK_FORMAT_B8G8R8A8_USCALED,
		VK_FORMAT_B8G8R8A8_SSCALED,
		VK_FORMAT_B8G8R8A8_UINT,
		VK_FORMAT_B8G8R8A8_SINT,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_A8B8G8R8_UNORM_PACK32,
		VK_FORMAT_A8B8G8R8_SNORM_PACK32,
		VK_FORMAT_A8B8G8R8_USCALED_PACK32,
		VK_FORMAT_A8B8G8R8_SSCALED_PACK32,
		VK_FORMAT_A8B8G8R8_UINT_PACK32,
		VK_FORMAT_A8B8G8R8_SINT_PACK32,
		VK_FORMAT_A8B8G8R8_SRGB_PACK32,
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_A2R10G10B10_SNORM_PACK32,
		VK_FORMAT_A2R10G10B10_USCALED_PACK32,
		VK_FORMAT_A2R10G10B10_SSCALED_PACK32,
		VK_FORMAT_A2R10G10B10_UINT_PACK32,
		VK_FORMAT_A2R10G10B10_SINT_PACK32,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_SNORM_PACK32,
		VK_FORMAT_A2B10G10R10_USCALED_PACK32,
		VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
		VK_FORMAT_A2B10G10R10_UINT_PACK32,
		VK_FORMAT_A2B10G10R10_SINT_PACK32,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_USCALED,
		VK_FORMAT_R16_SSCALED,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_USCALED,
		VK_FORMAT_R16G16_SSCALED,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SNORM,
		VK_FORMAT_R16G16B16_USCALED,
		VK_FORMAT_R16G16B16_SSCALED,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_USCALED,
		VK_FORMAT_R16G16B16A16_SSCALED,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R64_UINT,
		VK_FORMAT_R64_SINT,
		VK_FORMAT_R64_SFLOAT,
		VK_FORMAT_R64G64_UINT,
		VK_FORMAT_R64G64_SINT,
		VK_FORMAT_R64G64_SFLOAT,
		VK_FORMAT_R64G64B64_UINT,
		VK_FORMAT_R64G64B64_SINT,
		VK_FORMAT_R64G64B64_SFLOAT,
		VK_FORMAT_R64G64B64A64_UINT,
		VK_FORMAT_R64G64B64A64_SINT,
		VK_FORMAT_R64G64B64A64_SFLOAT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT
	};
	return formats[static_cast<std::underlying_type_t<ImageFormat>>(format)];
}

VkAttachmentLoadOp DeviceContext::translate_attachment_load_op(AttachmentLoadOp loadOp) const
{
	switch (loadOp)
	{
	case AttachmentLoadOp::Clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case AttachmentLoadOp::Dont_Care:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case AttachmentLoadOp::None:
		return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
	case AttachmentLoadOp::Load:
	default:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
}

VkAttachmentStoreOp DeviceContext::translate_attachment_store_op(AttachmentStoreOp storeOp) const
{
	switch (storeOp)
	{
	case AttachmentStoreOp::Dont_Care:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case AttachmentStoreOp::None:
		return VK_ATTACHMENT_STORE_OP_NONE;
	case AttachmentStoreOp::Store:
	default:
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
}

// --- Buffer usage flag conversion ---
VmaMemoryUsage DeviceContext::translate_memory_usage_flag(MemoryLocality locality) const
{
	switch (locality)
	{
	case MemoryLocality::Gpu:
		return VMA_MEMORY_USAGE_GPU_ONLY;
	case MemoryLocality::Cpu_To_Gpu:
		return VMA_MEMORY_USAGE_CPU_TO_GPU;
	case MemoryLocality::Cpu:
	default:
		return VMA_MEMORY_USAGE_CPU_ONLY;
	}
}

VkBufferUsageFlags DeviceContext::translate_buffer_usage_flags(BufferUsage flags) const
{
	RhiFlag const mask = static_cast<RhiFlag>(flags);
	constexpr VkBufferUsageFlagBits flagBits[] = {
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT
	};
	VkBufferUsageFlags result{};
	for (uint32 i = 0; i < static_cast<uint32>(std::size(flagBits)); ++i)
	{
		uint32 const exist = (mask & (1 << i));
		auto const bit = flagBits[i];
		result |= (exist & bit);
	}
	return result;
}

VkSampleCountFlagBits DeviceContext::translate_sample_count(SampleCount samples) const
{
	switch (samples)
	{
	case SampleCount::Sample_Count_1:
		return VK_SAMPLE_COUNT_1_BIT;
	case SampleCount::Sample_Count_2:
		return VK_SAMPLE_COUNT_2_BIT;
	case SampleCount::Sample_Count_4:
		return VK_SAMPLE_COUNT_4_BIT;
	case SampleCount::Sample_Count_8:
		return VK_SAMPLE_COUNT_8_BIT;
	case SampleCount::Sample_Count_16:
		return VK_SAMPLE_COUNT_16_BIT;
	case SampleCount::Sample_Count_32:
		return VK_SAMPLE_COUNT_32_BIT;
	case SampleCount::Sample_Count_64:
		return VK_SAMPLE_COUNT_64_BIT;
	default:
		return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	}
}

VkPresentModeKHR DeviceContext::translate_swapchain_presentation_mode(SwapchainPresentMode mode) const
{
	switch (mode)
	{
	case SwapchainPresentMode::Immediate:
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	case SwapchainPresentMode::Mailbox:
		return VK_PRESENT_MODE_MAILBOX_KHR;
	case SwapchainPresentMode::Fifo:
		return VK_PRESENT_MODE_FIFO_KHR;
	case SwapchainPresentMode::Fifo_Relaxed:
		return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	case SwapchainPresentMode::Shared_Demand_Refresh:
		return VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR;
	case SwapchainPresentMode::Shared_Continuous_Refresh:
		return VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR;
	default:
		return VK_PRESENT_MODE_MAX_ENUM_KHR;
	}
}

uint32 DeviceContext::clamp_swapchain_image_count(const uint32 current, const VkSurfaceCapabilitiesKHR& capability) const
{
	return std::min(std::max(current, capability.minImageCount), capability.maxImageCount);
}

VkExtent2D DeviceContext::get_swapchain_extent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, uint32 width, uint32 height) const
{
	if (surfaceCapabilities.currentExtent.width == -1)
	{
		VkExtent2D swapChainExtent{ width, height };
		if (swapChainExtent.width < surfaceCapabilities.minImageExtent.width)
		{
			swapChainExtent.width = surfaceCapabilities.minImageExtent.width;
		}

		if (swapChainExtent.height < surfaceCapabilities.minImageExtent.height)
		{
			swapChainExtent.height = surfaceCapabilities.minImageExtent.height;
		}

		if (swapChainExtent.width > surfaceCapabilities.maxImageExtent.width)
		{
			swapChainExtent.width = surfaceCapabilities.maxImageExtent.width;
		}

		if (swapChainExtent.height > surfaceCapabilities.maxImageExtent.height)
		{
			swapChainExtent.height = surfaceCapabilities.maxImageExtent.height;
		}
		return swapChainExtent;
	}
	return surfaceCapabilities.currentExtent;
}

VkShaderStageFlagBits DeviceContext::translate_shader_stage(ShaderType type) const
{
	switch (type)
	{
	case ShaderType::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderType::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderType::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderType::Tesselation_Control:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case ShaderType::Tesselation_Evaluation:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case ShaderType::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	case ShaderType::None:
	default:
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

VkFormat DeviceContext::translate_shader_attrib_format(ShaderAttribute::Format format) const
{
	switch (format)
	{
	case ShaderAttribute::Format::Int:
		return VK_FORMAT_R32_SINT;
	case ShaderAttribute::Format::Uint:
		return VK_FORMAT_R32_UINT;
	case ShaderAttribute::Format::Float:
		return VK_FORMAT_R32_SFLOAT;
	case ShaderAttribute::Format::Vec2:
		return VK_FORMAT_R32G32_SFLOAT;
	case ShaderAttribute::Format::Vec3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case ShaderAttribute::Format::Vec4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case ShaderAttribute::Format::Undefined:
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

VkCompareOp DeviceContext::translate_compare_op(CompareOp op) const
{
	switch (op)
	{
	case CompareOp::Less:
		return VK_COMPARE_OP_LESS;
	case CompareOp::Equal:
		return VK_COMPARE_OP_EQUAL;
	case CompareOp::Less_Or_Equal:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case CompareOp::Greater:
		return VK_COMPARE_OP_GREATER;
	case CompareOp::Not_Equal:
		return VK_COMPARE_OP_NOT_EQUAL;
	case CompareOp::Greater_Or_Equal:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case CompareOp::Always:
		return VK_COMPARE_OP_ALWAYS;
	case CompareOp::Never:
	default:
		return VK_COMPARE_OP_NEVER;
	}
}

VkPrimitiveTopology	DeviceContext::translate_topology(TopologyType topology) const
{
	switch (topology)
	{
	case TopologyType::Point:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case TopologyType::Line:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case TopologyType::Line_Strip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case TopologyType::Triange_Strip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case TopologyType::Triangle_Fan:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	case TopologyType::Triangle:
	default:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

VkPolygonMode DeviceContext::translate_polygon_mode(PolygonMode mode) const
{
	switch (mode)
	{
	case PolygonMode::Line:
		return VK_POLYGON_MODE_LINE;
	case PolygonMode::Point:
		return VK_POLYGON_MODE_POINT;
	case PolygonMode::Fill:
	default:
		return VK_POLYGON_MODE_FILL;
	}
}

VkCullModeFlags DeviceContext::translate_cull_mode(CullingMode mode) const
{
	switch (mode)
	{
	case CullingMode::None:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_NONE);
	case CullingMode::Front:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_FRONT_BIT);
	case CullingMode::Front_And_Back:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_FRONT_AND_BACK);
	case CullingMode::Back:
	default:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_BACK_BIT);
	}
}

VkFrontFace	DeviceContext::translate_front_face_dir(FrontFace face) const
{
	switch (face)
	{
	case FrontFace::Clockwise:
		return VK_FRONT_FACE_CLOCKWISE;
	case FrontFace::Counter_Clockwise:
	default:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

VkBlendFactor DeviceContext::translate_blend_factor(BlendFactor factor) const
{
	switch (factor)
	{
	case BlendFactor::One:
		return VK_BLEND_FACTOR_ONE;
	case BlendFactor::Src_Color:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case BlendFactor::One_Minus_Src_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case BlendFactor::Dst_Color:
		return VK_BLEND_FACTOR_DST_COLOR;
	case BlendFactor::One_Minus_Dst_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case BlendFactor::Src_Alpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case BlendFactor::One_Minus_Src_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case BlendFactor::Dst_Alpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case BlendFactor::One_Minus_Dst_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case BlendFactor::Constant_Color:
		return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case BlendFactor::One_Minus_Constant_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case BlendFactor::Constant_Alpha:
		return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case BlendFactor::One_Minus_Constant_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	case BlendFactor::Src_Alpha_Saturate:
		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	case BlendFactor::Src1_Color:
		return VK_BLEND_FACTOR_SRC1_COLOR;
	case BlendFactor::One_Minus_Src1_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
	case BlendFactor::Src1_Alpha:
		return VK_BLEND_FACTOR_SRC1_ALPHA;
	case BlendFactor::One_Minus_Src1_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	case BlendFactor::Zero:
	default:
		return VK_BLEND_FACTOR_ZERO;
	}
}
VkBlendOp DeviceContext::translate_blend_op(BlendOp op) const
{
	switch (op)
	{
	case BlendOp::Subtract:
		return VK_BLEND_OP_SUBTRACT;
	case BlendOp::Reverse_Subtract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case BlendOp::Add:
	default:
		return VK_BLEND_OP_ADD;
	}
}

VkFilter DeviceContext::translate_texel_filter(TexelFilter filter) const
{
	switch (filter)
	{
	case TexelFilter::Linear:
		return VK_FILTER_LINEAR;
	case TexelFilter::Cubic_Image:
		return VK_FILTER_CUBIC_IMG;
	case TexelFilter::Nearest:
	default:
		return VK_FILTER_NEAREST;
	}
}

VkPipelineStageFlags2 DeviceContext::translate_pipeline_stage_flags(PipelineStage stages) const
{
	VkFlags64 mask = static_cast<VkFlags64>(stages);
	if (!mask)
	{
		return VK_PIPELINE_STAGE_2_NONE;
	}
	constexpr VkPipelineStageFlagBits2 flagBits[] = {
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
		VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
		VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
		VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
		VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
		VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_2_HOST_BIT,
		VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_2_COPY_BIT,
		VK_PIPELINE_STAGE_2_RESOLVE_BIT,
		VK_PIPELINE_STAGE_2_BLIT_BIT,
		VK_PIPELINE_STAGE_2_CLEAR_BIT,
		VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
		VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
		VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT
	};
	VkPipelineStageFlags2 result = 0ull;
	for (size_t i = 0; i < std::size(flagBits); ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i));
		auto const bit = flagBits[i];
		result |= (exist & bit);
	}
	return result;
}

VkAccessFlags2 DeviceContext::translate_memory_access_flags(MemoryAccessType accesses) const
{
	VkFlags64 mask = static_cast<VkFlags64>(accesses);
	if (!mask)
	{
		return VK_ACCESS_2_NONE;
	}
	constexpr VkAccessFlagBits2 flagBits[] = {
		VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
		VK_ACCESS_2_INDEX_READ_BIT,
		VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
		VK_ACCESS_2_UNIFORM_READ_BIT,
		VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT,
		VK_ACCESS_2_SHADER_READ_BIT,
		VK_ACCESS_2_SHADER_WRITE_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_2_TRANSFER_READ_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_ACCESS_2_HOST_READ_BIT,
		VK_ACCESS_2_HOST_WRITE_BIT,
		VK_ACCESS_2_MEMORY_READ_BIT,
		VK_ACCESS_2_MEMORY_WRITE_BIT,
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
		VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT
	};
	VkAccessFlags2 result = 0ull;
	for (size_t i = 0; i < std::size(flagBits); ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i));
		auto const bit = flagBits[i];
		result |= (exist & bit);
	}
	return result;
}

//DeviceQueueType DeviceQueue::get_type() const
//{
//	return type;
//}

//bool DeviceQueue::create_command_pool(command::CommandPool& commandPool)
//{
//	// If the device already has the command pool, we don't recreate it.
//	if (commandPools.index_of(&commandPool) != -1			||
//		commandPool.data.slot() != DEVICE_INVALID_HANDLE	||
//		commandPool.data.valid())
//	{
//		return true;
//	}
//	
//	uint32 queueFamilyIndex = pContext->mainQueue.familyIndex; // Defaults to the main/graphics queue.
//
//	if (type == DeviceQueueType::Transfer)
//	{
//		queueFamilyIndex = pContext->transferQueue.familyIndex;
//	}
//
//	if (type == DeviceQueueType::Compute)
//	{
//		queueFamilyIndex = pContext->computeQueue.familyIndex;
//	}
//
//	VkCommandPoolCreateInfo commandPoolInfo{
//		.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//		.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
//		.queueFamilyIndex	= queueFamilyIndex
//	};
//
//	VkCommandPool hcommandPool;
//	VkResult result = vkCreateCommandPool(pContext->device, &commandPoolInfo, nullptr, &hcommandPool);
//	ASSERTION(result == VK_SUCCESS);
//
//	if (result != VK_SUCCESS)
//	{
//		return false;
//	}
//
//	auto [id, resource] = pContext->cache.commandArena.request_resource(&data.as<VulkanQueue>(), std::move(hcommandPool), CommandArena::CommandBufferStore{}, 0u);
//
//	new (&commandPool.data) RHIObjPtr{ id, &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };
//	// Assign the command pool to the queue's type.
//	commandPool.deviceContext = pContext;
//
//	return true;
//}

//void DeviceQueue::destroy_command_pool(command::CommandPool& commandPool)
//{
//	if (commandPool.data.valid())
//	{
//		// Queue up the pool into the zombie list.
//		pContext->cache.zombies.emplace(commandPool.data.slot(), commandPool.data.type_id());
//		new (&commandPool.data) RHIObjPtr{};
//	}
//}

//void DeviceQueue::wait_idle() const
//{
//	VulkanQueue& queue = data.as<VulkanQueue>();
//	vkQueueWaitIdle(queue.queue);
//}

//bool DeviceQueue::submit() const
//{
//	VulkanQueue& queue = data.as<VulkanQueue>();
//	VkSubmitInfo submitInfo{
//		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//	};
//	vkQueueSubmit(queue.queue, 1, &submitInfo, VK_NULL_HANDLE);
//}

RenderDevice::RenderDevice() :
	m_context{},
	/*m_mainQueue{},*/
	/*m_binarySemaphorePool{},*/
	m_api{ API::Vulkan }, 
	m_shadingLanguage{ ShaderLang::GLSL }
{}

RenderDevice::RenderDevice(API api, ShaderLang shadingLanguage) :
	m_context{},
	/*m_mainQueue{},*/
	/*m_binarySemaphorePool{}, */
	m_api { api }, 
	m_shadingLanguage{ shadingLanguage }
{}

bool RenderDevice::initialize(DeviceInitInfo const& info)
{
	m_context = static_cast<DeviceContext*>(ftl::allocate_memory(sizeof(DeviceContext)));
	new (m_context) DeviceContext{};

	m_context->init = info;
	m_context->config = info.config;
	m_context->validation = info.validation;

	volkInitialize();

	if (!m_context->create_vulkan_instance(m_context->init)) { return false; }

	volkLoadInstance(m_context->instance);

	if (!m_context->create_debug_messenger(m_context->init)) { return false; }
	if (!m_context->choose_physical_device()) { return false; }

	m_context->get_device_queue_family_indices();

	if (!m_context->create_logical_device()) { return false; }

	// Set up main queue.
	/*{
		new (&m_mainQueue.data) RHIObjPtr{ &m_context->mainQueue, rhi_obj_type_id_v<VulkanQueue> };
		m_mainQueue.pContext = m_context;
		m_mainQueue.type = DeviceQueueType::Main;

		m_mainQueue.properties.capabilities.graphics		= m_context->mainQueue.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
		m_mainQueue.properties.capabilities.compute			= m_context->mainQueue.properties.queueFlags & VK_QUEUE_COMPUTE_BIT;
		m_mainQueue.properties.capabilities.transfer		= m_context->mainQueue.properties.queueFlags & VK_QUEUE_TRANSFER_BIT;
		m_mainQueue.properties.capabilities.sparseBinding	= m_context->mainQueue.properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
		m_mainQueue.properties.capabilities.protectedMemory = m_context->mainQueue.properties.queueFlags & VK_QUEUE_PROTECTED_BIT;

		m_mainQueue.properties.queueCount			= m_context->mainQueue.properties.queueCount;
		m_mainQueue.properties.timestampValidBits	= m_context->mainQueue.properties.timestampValidBits;

		m_mainQueue.properties.minImageTransferGranularity.width	= m_context->mainQueue.properties.minImageTransferGranularity.width;
		m_mainQueue.properties.minImageTransferGranularity.height	= m_context->mainQueue.properties.minImageTransferGranularity.height;
		m_mainQueue.properties.minImageTransferGranularity.depth	= m_context->mainQueue.properties.minImageTransferGranularity.depth;
	}*/

	/**
	* To avoid a dispatch overhead (~7%), we tell volk to directly load device-related Vulkan entrypoints.
	*/
	volkLoadDevice(m_context->device);
	m_context->get_queue_handles();

	if (!m_context->create_device_allocator()) { return false; }

	if (!m_context->initialize_resource_cache())
	{
		return false;
	}

	return true;
}

void RenderDevice::terminate()
{
	m_context->cleanup_resource_cache();

	vmaDestroyAllocator(m_context->allocator);
	vkDestroyDevice(m_context->device, nullptr);

	if (m_context->validation)
	{
		vkDestroyDebugUtilsMessengerEXT(m_context->instance, m_context->debugger, nullptr);
	}
	m_context->gpu = VK_NULL_HANDLE;
	vkDestroyInstance(m_context->instance, nullptr);

	m_context->~DeviceContext();
	ftl::release_memory(m_context);
}

uint32 RenderDevice::stride_for_shader_attrib_format(ShaderAttribute::Format format) const
{
	switch (format)
	{
	case ShaderAttribute::Format::Int:
	case ShaderAttribute::Format::Uint:
	case ShaderAttribute::Format::Float:
		return 4;
	case ShaderAttribute::Format::Vec2:
		return 8;
	case ShaderAttribute::Format::Vec3:
		return 12;
	case ShaderAttribute::Format::Vec4:
		return 16;
	case ShaderAttribute::Format::Undefined:
	default:
		return 0;
	}
}

bool RenderDevice::is_img_format_color_format(ImageFormat format) const
{
	return format != ImageFormat::D24_Unorm_S8_Uint && format != ImageFormat::D16_Unorm_S8_Uint;
}

DeviceType RenderDevice::get_device_type() const
{
	switch (m_context->properties.deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:	 return DeviceType::Discrete_Gpu;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return DeviceType::Integrate_Gpu;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:	 return DeviceType::Virtual_Gpu;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:			 return DeviceType::Cpu;
	}
	return DeviceType::Other;
}

DeviceInfo RenderDevice::get_device_info() const
{
	static constexpr std::pair<uint32, literal_t> vendorIdToName[] = {
		{ 0x1002u, "AMD" },
		{ 0x1010u, "ImgTec" },
		{ 0x10DEu, "NVIDIA" },
		{ 0x13B5u, "ARM" },
		{ 0x5143u, "Qualcomm" },
		{ 0x8086u, "INTEL" }
	};

	std::string_view vendorName;
	std::for_each(
		std::begin(vendorIdToName),
		std::end(vendorIdToName),
		[&](const std::pair<uint32, literal_t> vendorInfo) -> void
		{
			if (m_context->properties.vendorID == vendorInfo.first)
			{
				vendorName = vendorInfo.second;
			}
		}
	);

	return DeviceInfo{
		.name = m_context->properties.deviceName,
		.type = get_device_type(),
		.vendor = vendorName,
		.vendorID = m_context->properties.vendorID,
		.deviceID = m_context->properties.deviceID,
		.apiVersion = {
			.major = VK_API_VERSION_MAJOR(m_context->properties.apiVersion),
			.minor = VK_API_VERSION_MINOR(m_context->properties.apiVersion),
			.patch = VK_API_VERSION_PATCH(m_context->properties.apiVersion)
		},
		.driverVersion = {
			.major = VK_API_VERSION_MAJOR(m_context->properties.driverVersion),
			.minor = VK_API_VERSION_MINOR(m_context->properties.driverVersion),
			.patch = VK_API_VERSION_PATCH(m_context->properties.driverVersion)
		}
	};
}

DeviceConfig const& RenderDevice::get_device_config() const
{
	return m_context->config;
}

API RenderDevice::get_api() const
{
	return m_api;
}

//DeviceQueue& RenderDevice::get_main_queue()
//{
//	return m_mainQueue;
//}

//DeviceQueue RenderDevice::get_async_transfer_queue() const
//{	
//	return DeviceQueue{ m_context->asyncTransferQueue.familyIndex };
//}
//
//DeviceQueue RenderDevice::get_async_compute_queue() const
//{
//	return DeviceQueue{ m_context->asyncComputeQueue.familyIndex };
//}

bool RenderDevice::create_surface_win32(Surface& surface)
{
	if (surface.hnd != device_invalid_handle_v<SurfaceHnd> ||
		surface.data.valid())
	{
		return true;
	}

	VkSurfaceKHR hsurface;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{
		.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance	= reinterpret_cast<HINSTANCE>(surface.info.instance),
		.hwnd		= reinterpret_cast<HWND>(surface.info.window)
	};

	VkResult result = vkCreateWin32SurfaceKHR(m_context->instance, &surfaceCreateInfo, nullptr, &hsurface);
	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	uint32 count = 1;
	VkSurfaceFormatKHR format;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_context->gpu, hsurface, &count, &format);

	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_context->gpu, hsurface, &capabilities);

	auto [id, resource] = m_context->cache.surfaces.request_resource(std::move(hsurface), std::move(format), std::move(capabilities));
	make_handle(surface.hnd, id);

	new (&surface.data) RHIObjPtr{ &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };

	return true;
}

void RenderDevice::destroy_surface(Surface& surface)
{
	if (surface.data.valid())
	{
		m_context->cache.zombies.emplace(surface.hnd.get(), surface.data.type_id());
		make_handle(surface.hnd, DEVICE_INVALID_HANDLE);
		new (&surface.data) RHIObjPtr{};
	}
}

bool RenderDevice::create_swapchain(Swapchain& swapchain, Swapchain* const pOldSwapchain)
{
	if (swapchain.hnd != device_invalid_handle_v<SwapchainHnd> ||
		swapchain.data.valid())
	{
		return true;
	}

	auto [valid0, surface] = m_context->cache.surfaces.fetch_resource(swapchain.surface.hnd.get());
	if (!valid0)
	{
		return false;
	}

	VkSwapchainKHR* pOld = nullptr;

	if (pOldSwapchain)
	{
		auto [valid1, sc] = m_context->cache.swapchains.fetch_resource(pOldSwapchain->hnd.get());
		if (!valid1)
		{
			return false;
		}
		pOld = &sc.swapchain;
	}

	VkExtent2D extent{
		std::min(swapchain.info.dimension.width, surface.capabilities.currentExtent.width),
		std::min(swapchain.info.dimension.height, surface.capabilities.currentExtent.height)
	};
	uint32 imageCount{ m_context->clamp_swapchain_image_count(swapchain.info.imageCount, surface.capabilities) };

	VkImageUsageFlags imageUsage = m_context->translate_image_usage_flags(swapchain.info.imageUsage);
	VkPresentModeKHR presentationMode = m_context->translate_swapchain_presentation_mode(swapchain.info.presentationMode);

	VkSwapchainKHR hswapchain;
	VkSwapchainCreateInfoKHR swapchainCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface				= surface.surface,
		.minImageCount			= imageCount,
		.imageFormat			= surface.format.format,
		.imageColorSpace		= surface.format.colorSpace,
		.imageExtent			= extent,
		.imageArrayLayers		= 1,
		.imageUsage				= imageUsage,
		.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount	= 1,
		.pQueueFamilyIndices	= &m_context->mainQueue.familyIndex,
		.preTransform			= VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode			= presentationMode,
		.clipped				= VK_TRUE,
		.oldSwapchain			= (pOldSwapchain != nullptr) ? *pOld : nullptr
	};

	VkResult result = vkCreateSwapchainKHR(m_context->device, &swapchainCreateInfo, nullptr, &hswapchain);
	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	auto [id, swpc] = m_context->cache.swapchains.request_resource(std::move(hswapchain), std::move(extent), imageCount, 0u);
	make_handle(swapchain.hnd, id);

	new (&swapchain.data) RHIObjPtr{ &swpc, rhi_obj_type_id_v<std::remove_reference_t<decltype(swpc)>> };

	swpc.images.reserve(imageCount);
	swpc.images.resize(imageCount);
	swpc.imageViews.reserve(imageCount);

	vkGetSwapchainImagesKHR(m_context->device, swpc.swapchain, &imageCount, swpc.images.data());

	VkImageViewCreateInfo imageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = surface.format.format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	for (VkImage& img : swpc.images)
	{
		VkImageView imgView;
		imageViewInfo.image = img;

		vkCreateImageView(m_context->device, &imageViewInfo, nullptr, &imgView);
		swpc.imageViews.emplace_back(imgView);
	}
	
	return true;
}

void RenderDevice::destroy_swapchain(Swapchain& swapchain)
{
	if (swapchain.data.valid())
	{
		m_context->cache.zombies.emplace(swapchain.hnd.get(), swapchain.data.type_id());
		make_handle(swapchain.hnd, DEVICE_INVALID_HANDLE);
		new (&swapchain.data) RHIObjPtr{};
	}
}

bool RenderDevice::update_next_swapchain_image(Swapchain& swapchain, Semaphore& semaphore)
{
	if (swapchain.hnd == device_invalid_handle_v<SwapchainHnd>	||
		semaphore.hnd == device_invalid_handle_v<SemaphoreHnd>	||
		!swapchain.data.valid()									||
		!semaphore.data.valid())
	{
		return false;
	}

	if (swapchain.state == SwapchainState::Error)
	{
		return false;
	}

	VulkanSemaphore& sem	= semaphore.data.as<VulkanSemaphore>();
	VulkanSwapchain& swpc	= swapchain.data.as<VulkanSwapchain>();

	VkResult result = vkAcquireNextImageKHR(m_context->device, swpc.swapchain, UINT64_MAX, sem.semaphore, VK_NULL_HANDLE, &swpc.nextImageIndex);
	
	switch (result)
	{
	case VK_SUCCESS:
		swapchain.state = SwapchainState::Ok;
		break;
	case VK_NOT_READY:
		swapchain.state = SwapchainState::Not_Ready;
		break;
	case VK_TIMEOUT:
		swapchain.state = SwapchainState::Timed_Out;
		break;
	case VK_SUBOPTIMAL_KHR:
		swapchain.state = SwapchainState::Suboptimal;
		break;
	default:
		swapchain.state = SwapchainState::Error;
		break;
	}

	return true;
}

/*bool RenderDevice::get_swapchain_image_count(SwapchainHnd const& hnd, uint32& count)
{
	auto [valid, swapchain] = m_context->cache.swapchains.fetch_resource(hnd.get());
	if (!valid)
	{
		return false;
	}

	count = swapchain.imageCount;

	return true;
}*/

/*bool RenderDevice::request_swapchain_images(SwapchainHnd const& hswapchain, SurfaceHnd const& hsurface)
{
	auto [valid0, swapchain] = m_context->cache.swapchains.fetch_resource(hswapchain.get());
	if (!valid0)
	{
		return false;
	}

	auto [valid1, surface] = m_context->cache.surfaces.fetch_resource(hsurface.get());
	if (!valid1)
	{
		return false;
	}

	swapchain.images.resize(swapchain.imageCount);
	vkGetSwapchainImagesKHR(m_context->device, swapchain.swapchain, &swapchain.imageCount, swapchain.images.data());

	VkImageViewCreateInfo imageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = surface.format.format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	for (VkImage& img : swapchain.images)
	{
		VkImageView imgView;
		imageViewInfo.image = img;

		vkCreateImageView(m_context->device, &imageViewInfo, nullptr, &imgView);
		swapchain.imageViews.emplace_back(imgView);
	}

	return true;
}*/

/*bool RenderDevice::create_semaphore(Semaphore& semaphore)
{
	VkSemaphoreType type{ VK_SEMAPHORE_TYPE_BINARY };

	VkSemaphoreTypeCreateInfo typeInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.flags = {}
	};

	uint32 value = 0u;

	if (semaphore.info.type == SemaphoreType::Timeline)
	{
		type = VK_SEMAPHORE_TYPE_TIMELINE;
		value = semaphore.info.initialValue;

		typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeInfo.initialValue = value;
		semaphoreInfo.pNext = &typeInfo;
	}

	VkSemaphore hsemaphore;
	VkResult result = vkCreateSemaphore(m_context->device, &semaphoreInfo, nullptr, &hsemaphore);
	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	auto [id, resource] = m_context->cache.semaphores.request_resource(std::move(hsemaphore), std::move(type), std::move(value));
	make_handle(semaphore.hnd, id);

	new (&semaphore.data) RHIObjPtr{ &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };

	return true;
}

void RenderDevice::destroy_semaphore(Semaphore& semaphore)
{
	if (semaphore.data.valid())
	{
		m_context->cache.zombies.emplace(semaphore.hnd.get(), semaphore.data.type_id());
		make_handle(semaphore.hnd, DEVICE_INVALID_HANDLE);
		new (&semaphore.data) RHIObjPtr{};
	}
}*/

bool RenderDevice::create_shader(Shader& shader)
{
	if (shader.hnd != device_invalid_handle_v<ShaderHnd> ||
		shader.data.valid())
	{
		return true;
	}

	VkShaderModule hshader;
	VkShaderModuleCreateInfo shaderInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shader.info.binaries.size() * sizeof(uint32),
		.pCode = shader.info.binaries.data()
	};
	VkResult result = vkCreateShaderModule(m_context->device, &shaderInfo, nullptr, &hshader);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	VkShaderStageFlagBits const stage = m_context->translate_shader_stage(shader.info.type);

	auto [id, resource] = m_context->cache.shaders.request_resource(std::move(hshader), std::move(stage));
	make_handle(shader.hnd, id);
	
	new (&shader.data) RHIObjPtr{ &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };
	
	shader.info.binaries.~vector();

	return true;
}

void RenderDevice::destroy_shader(Shader& shader)
{
	if (shader.data.valid())
	{
		m_context->cache.zombies.emplace(shader.hnd.get(), shader.data.type_id());
		make_handle(shader.hnd, DEVICE_INVALID_HANDLE);
		new (&shader.data) RHIObjPtr{};
	}
}

bool RenderDevice::create_raster_pipeline(RasterPipeline& pipeline)
{
	if (pipeline.hnd != device_invalid_handle_v<PipelineHnd> ||
		pipeline.data.valid())
	{
		return true;
	}

	const size_t shaderCount = pipeline.shaders.size();
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
	shaderStages.reserve(shaderCount);

	Shader* pVertexShader = nullptr;

	for (Shader* pShader : pipeline.shaders)
	{
		auto [valid, shdr] = m_context->cache.shaders.fetch_resource(pShader->hnd.get());
		if (!valid)
		{
			break;
		}

		if (pShader->info.type == ShaderType::Vertex)
		{
			pVertexShader = pShader;
		}

		shaderStages.push_back(
			VkPipelineShaderStageCreateInfo{
				.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage	= shdr.stage,
				.module = shdr.shader,
				.pName	= pShader->info.entryPoint.c_str()
			}
		);
	}

	if (shaderStages.size() != shaderCount || !pVertexShader)
	{
		return false;
	}

	std::span<PipelineInputBinding const> inputBindings{ 
		pipeline.info.inputBindings.data(), 
		pipeline.info.inputBindings.size()
	};

	std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	bindingDescriptions.reserve(inputBindings.size());
	attributeDescriptions.reserve(pVertexShader->info.attributes.size());

	for (PipelineInputBinding const& input : inputBindings)
	{
		bindingDescriptions.push_back(
			VkVertexInputBindingDescription{
				.binding	= input.binding,
				.stride		= input.stride,
				.inputRate	= (input.instanced == true) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX
			}
		);
		uint32 stride = 0;
		for (uint32 i = input.from; i <= input.to; ++i)
		{
			auto& attribute = pVertexShader->info.attributes[i];
			attributeDescriptions.push_back(
				VkVertexInputAttributeDescription{
					.location	= attribute.location,
					.binding	= input.binding,
					.format		= m_context->translate_shader_attrib_format(attribute.format),
					.offset		= stride,
				}
			);
			stride += stride_for_shader_attrib_format(attribute.format);
		}
	}

	VkPipelineVertexInputStateCreateInfo pipelineVertexState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount		= static_cast<uint32>(bindingDescriptions.size()),
		.pVertexBindingDescriptions			= bindingDescriptions.data(),
		.vertexAttributeDescriptionCount	= static_cast<uint32>(attributeDescriptions.size()),
		.pVertexAttributeDescriptions		= attributeDescriptions.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssembly{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology				= m_context->translate_topology(pipeline.info.topology),
		.primitiveRestartEnable = VK_FALSE
	};

	// NOTE(Afiq):
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkGraphicsPipelineCreateInfo.html
	// The vulkan spec states that a VkPipelineViewportStateCreateInfo is unnecessary for pipelines with dynamic states "VK_DYNAMIC_STATE_VIEWPORT" and "VK_DYNAMIC_STATE_SCISSOR".
	/*VkViewport viewport{ 0.f, 0.f, info.viewport.width, info.viewport.height, 0.f, 1.0f };

	VkRect2D scissor{
		.offset = {
			.x = info.scissor.offset.x,
			.y = info.scissor.offset.y
		},
		.extent = {
			.width = info.scissor.extent.width,
			.height = info.scissor.extent.height
		}
	};*/

	VkPipelineViewportStateCreateInfo pipelineViewportState{
		.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount	= 1,
		.scissorCount	= 1
	};

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo pipelineDynamicState{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount	= 2,
		.pDynamicStates		= states
	};

	VkPipelineRasterizationStateCreateInfo pipelineRasterState{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable	= pipeline.info.rasterization.enableDepthClamp,
		.polygonMode		= m_context->translate_polygon_mode(pipeline.info.rasterization.polygonalMode),
		.cullMode			= m_context->translate_cull_mode(pipeline.info.rasterization.cullMode),
		.frontFace			= m_context->translate_front_face_dir(pipeline.info.rasterization.frontFace),
		.lineWidth			= 1.f
	};

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleSate{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples	= VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading		= 1.f
	};

	size_t attachmentCount = pipeline.colorAttachments.size();
	std::vector<VkPipelineColorBlendAttachmentState> colorAttachmentBlendStates{};
	std::vector<VkFormat> colorAttachmentFormats{};

	colorAttachmentBlendStates.reserve(attachmentCount);
	colorAttachmentFormats.reserve(attachmentCount);

	for (ColorAttachment const& attachment: pipeline.colorAttachments)
	{
		colorAttachmentBlendStates.push_back(
			VkPipelineColorBlendAttachmentState{
				.blendEnable			= VK_TRUE,
				.srcColorBlendFactor	= m_context->translate_blend_factor(attachment.blendInfo.srcColorBlendFactor),
				.dstColorBlendFactor	= m_context->translate_blend_factor(attachment.blendInfo.dstColorBlendFactor),
				.colorBlendOp			= m_context->translate_blend_op(attachment.blendInfo.colorBlendOp),
				.srcAlphaBlendFactor	= m_context->translate_blend_factor(attachment.blendInfo.srcAlphaBlendFactor),
				.dstAlphaBlendFactor	= m_context->translate_blend_factor(attachment.blendInfo.dstAlphaBlendFactor),
				.alphaBlendOp			= m_context->translate_blend_op(attachment.blendInfo.alphaBlendOp),
				.colorWriteMask			= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			}
		);
		VkFormat fmt = m_context->translate_image_format(attachment.format);
		colorAttachmentFormats.push_back(fmt);
	}

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendState{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable		= VK_FALSE,
		.logicOp			= VK_LOGIC_OP_CLEAR,
		.attachmentCount	= static_cast<uint32>(attachmentCount),
		.pAttachments		= colorAttachmentBlendStates.data(),
		.blendConstants		= { 1.f, 1.f, 1.f, 1.f }
	};

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilState{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable		= pipeline.info.depthTest.enableDepthTest,
		.depthWriteEnable		= pipeline.info.depthTest.enableDepthWrite,
		.depthCompareOp			= m_context->translate_compare_op(pipeline.info.depthTest.depthTestCompareOp),
		.depthBoundsTestEnable	= pipeline.info.depthTest.enableDepthBoundsTest,
		.minDepthBounds			= pipeline.info.depthTest.minDepthBounds,
		.maxDepthBounds			= pipeline.info.depthTest.maxDepthBounds
	};

	VkPipelineRenderingCreateInfo pipelineRendering{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount		= static_cast<uint32>(attachmentCount),
		.pColorAttachmentFormats	= colorAttachmentFormats.data(),
		.depthAttachmentFormat		= VK_FORMAT_D16_UNORM,
		.stencilAttachmentFormat	= VK_FORMAT_S8_UINT
	};

	VkPipelineLayout hlayout = m_context->get_appropriate_pipeline_layout(pipeline.info.pushConstantSize, m_context->properties.limits.maxPushConstantsSize);

	VkPipeline hpipeline;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext					= &pipelineRendering,
		.stageCount				= static_cast<uint32>(shaderStages.size()),
		.pStages				= shaderStages.data(),
		.pVertexInputState		= &pipelineVertexState,
		.pInputAssemblyState	= &pipelineInputAssembly,
		.pTessellationState		= nullptr,
		.pViewportState			= &pipelineViewportState,
		.pRasterizationState	= &pipelineRasterState,
		.pMultisampleState		= &pipelineMultisampleSate,
		.pDepthStencilState		= &pipelineDepthStencilState,
		.pColorBlendState		= &pipelineColorBlendState,
		.pDynamicState			= &pipelineDynamicState,
		.layout					= hlayout,
		.renderPass				= nullptr,
		.subpass				= 0,
		.basePipelineHandle		= VK_NULL_HANDLE,
		.basePipelineIndex		= 0
	};

	// TODO(Afiq):
	// The only way this can fail is when we run out of host / device memory OR shader linkage has failed.
	VkResult result = vkCreateGraphicsPipelines(m_context->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &hpipeline);
	ASSERTION(result == VK_SUCCESS);
	if (result != VK_SUCCESS)
	{
		return false;
	}

	auto [id, resource] = m_context->cache.pipelines.request_resource(std::move(hpipeline), std::move(hlayout));
	make_handle(pipeline.hnd, id);
	
	new (&pipeline.data) RHIObjPtr{ &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };

	return true;
}

void RenderDevice::destroy_raster_pipeline(RasterPipeline& pipeline)
{
	if (pipeline.data.valid())
	{
		m_context->cache.zombies.emplace(pipeline.hnd.get(), pipeline.data.type_id());
		make_handle(pipeline.hnd, DEVICE_INVALID_HANDLE);
		new (&pipeline.data) RHIObjPtr{};
	}
}

bool RenderDevice::allocate_buffer(Buffer& buffer)
{
	if (buffer.hnd != device_invalid_handle_v<BufferHnd> ||
		buffer.data.valid())
	{
		return true;
	}

	VkBufferCreateInfo info{
		.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size			= buffer.info.size,
		.usage			= m_context->translate_buffer_usage_flags(buffer.info.usage),
		.sharingMode	= VK_SHARING_MODE_EXCLUSIVE
	};

	VmaAllocationCreateInfo allocInfo{ 
		.usage = m_context->translate_memory_usage_flag(buffer.info.locality) 
	};

	VkBuffer hbuffer;
	VmaAllocation allocation;
	VkResult result = vmaCreateBuffer(m_context->allocator, &info, &allocInfo, &hbuffer, &allocation, nullptr);
	ASSERTION(result == VK_SUCCESS);

	if (result != VK_SUCCESS)
	{
		return false;
	}

	if (buffer.info.locality != MemoryLocality::Gpu)
	{
		vmaMapMemory(m_context->allocator, allocation, &buffer.address);
		vmaUnmapMemory(m_context->allocator, allocation);
	}

	VkBufferDeviceAddressInfo addressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT,
		.buffer = hbuffer
	};
	VkDeviceAddress deviceAddress = vkGetBufferDeviceAddressKHR(m_context->device, &addressInfo);

	auto [id, resource] = m_context->cache.buffers.request_resource(std::move(hbuffer), std::move(deviceAddress), std::move(allocation));
	make_handle(buffer.hnd, id);

	new (&buffer.data) RHIObjPtr{ &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };

	return true;
}

void RenderDevice::release_buffer(Buffer& buffer)
{
	if (buffer.data.valid())
	{
		m_context->cache.zombies.emplace(buffer.hnd.get(), buffer.data.type_id());
		make_handle(buffer.hnd, DEVICE_INVALID_HANDLE);
		new (&buffer.data) RHIObjPtr{};
	}
}

bool RenderDevice::create_image(Image& image)
{
	constexpr uint32 MAX_IMAGE_MIP_LEVEL = 4u;

	if (image.hnd != device_invalid_handle_v<ImageHnd> ||
		image.data.valid())
	{
		return true;
	}

	VkFormat format = m_context->translate_image_format(image.info.format);
	VkImageUsageFlags usage = m_context->translate_image_usage_flags(image.info.imageUsage);

	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
	{
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	uint32 depth{ 1 };

	if (image.info.dimension.depth > 1)
	{
		depth = image.info.dimension.depth;
	}

	uint32 mipLevels{ 1 };

	if (image.info.mipLevel > 1)
	{
		const float32 width		= static_cast<const float32>(image.info.dimension.width);
		const float32 height	= static_cast<const float32>(image.info.dimension.height);

		mipLevels = static_cast<uint32>(math::floorf(math::log2f(std::max(width, height)))) + 1u;
		mipLevels = std::min(mipLevels, MAX_IMAGE_MIP_LEVEL);
	}

	VkImageCreateInfo imgInfo{
		.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType		= m_context->translate_image_type(image.info.type),
		.format			= format,
		.extent			= VkExtent3D{ image.info.dimension.width, image.info.dimension.width, depth },
		.mipLevels		= mipLevels,
		.arrayLayers	= 1,
		.samples		= m_context->translate_sample_count(image.info.samples),
		.tiling			= m_context->translate_image_tiling(image.info.tiling),
		.usage			= usage,
		.sharingMode	= VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VmaAllocationCreateInfo allocInfo{ .usage = VMA_MEMORY_USAGE_GPU_ONLY };
	VkImage himg;
	VmaAllocation allocation;

	VkResult result = vmaCreateImage(m_context->allocator, &imgInfo, &allocInfo, &himg, &allocation, nullptr);
	ASSERTION(result == VK_SUCCESS);

	if (result != VK_SUCCESS)
	{
		return false;
	}

	VkImageView himgView;
	VkImageViewCreateInfo imgViewInfo{
		.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image		= himg,
		.viewType	= m_context->translate_image_view_type(image.info.type),
		.format		= format,
		.subresourceRange = {
			.aspectMask		= aspectFlags,
			.baseMipLevel	= 0,
			.levelCount		= mipLevels,
			.baseArrayLayer = 0,
			.layerCount		= 1
		}
	};
	vkCreateImageView(m_context->device, &imgViewInfo, nullptr, &himgView);

	auto [id, resource] = m_context->cache.images.request_resource(std::move(himg), std::move(himgView), std::move(allocation));
	make_handle(image.hnd, id);

	new (&image.data) RHIObjPtr{ &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };

	return true;
}

void RenderDevice::destroy_image(Image& image)
{
	if (image.data.valid())
	{
		m_context->cache.zombies.emplace(image.hnd.get(), image.data.type_id());
		make_handle(image.hnd, DEVICE_INVALID_HANDLE);
		new (&image.data) RHIObjPtr{};
	}
}

void RenderDevice::cache_render_metadata(command::RenderMetadata& metadata)
{
	auto store_render_attachment_info = [this](command::RenderAttachment const& in, VkRenderingAttachmentInfo& out, VkClearValue clearValue) -> void
	{
		if (in.image)
		{
			out = {
				.sType				= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView			= in.image->data.as<VulkanImage>().imageView,
				.imageLayout		= m_context->translate_image_layout(in.layout),
				.resolveMode		= VkResolveModeFlagBits::VK_RESOLVE_MODE_NONE,
				.resolveImageView	= VK_NULL_HANDLE,
				.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.loadOp				= m_context->translate_attachment_load_op(in.loadOp),
				.storeOp			= m_context->translate_attachment_store_op(in.storeOp),
				.clearValue			= clearValue
			};
		}
	};

	VulkanRenderAttachments renderAttachments{};

	if (metadata.depthAttachment.has_value())
	{
		auto& attachment = metadata.depthAttachment.value();
		VkClearValue cv = { 
			.depthStencil = { 
				.depth = attachment.clearValue.depthStencil.depth 
			} 
		};
		store_render_attachment_info(attachment, renderAttachments.depthAttachment, cv);
	}

	if (metadata.stencilAttachment.has_value())
	{
		auto& attachment = metadata.stencilAttachment.value();
		VkClearValue cv = {
			.depthStencil = {
				.stencil = attachment.clearValue.depthStencil.stencil
			}
		};
		store_render_attachment_info(attachment, renderAttachments.stencilAttachment, cv);
	}

	for (uint32 i = 0; command::RenderAttachment const& attachment : metadata.colorAttachments)
	{
		VkClearValue cv = {
			.color = { 
				.float32 = { 
					attachment.clearValue.color.f32.r, 
					attachment.clearValue.color.f32.g, 
					attachment.clearValue.color.f32.b, 
					attachment.clearValue.color.f32.a 
				} 
			}
		};
		store_render_attachment_info(attachment, renderAttachments.colorAttachments[i], cv);
		++i;
		++renderAttachments.colorAttachmentCount;
	}

	auto [id, resource] = m_context->cache.renderingAttachments.request_resource(std::move(renderAttachments));
	new (&metadata.data) RHIObjPtr{ id, &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };
}

void RenderDevice::uncache_render_metadata(command::RenderMetadata& metadata)
{
	if (metadata.data.valid())
	{
		m_context->cache.renderingAttachments.return_resource(metadata.data.slot());
		new (&metadata.data) RHIObjPtr{};
	}
}

bool RenderDevice::create_command_pool(command::CommandPool& commandPool, rhi::DeviceQueueType type)
{
	// If the device already has the command pool, we don't recreate it.
	if (commandPool.data.slot() != DEVICE_INVALID_HANDLE	||
		commandPool.data.valid())
	{
		return true;
	}
	
	VulkanQueue* pQueue = &m_context->mainQueue;

	if (type == DeviceQueueType::Transfer)
	{
		pQueue = &m_context->transferQueue;
	}
	else if (type == DeviceQueueType::Compute)
	{
		pQueue = &m_context->computeQueue;
	}

	VkCommandPoolCreateInfo commandPoolInfo{
		.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex	= pQueue->familyIndex
	};

	VkCommandPool hcommandPool;
	VkResult result = vkCreateCommandPool(m_context->device, &commandPoolInfo, nullptr, &hcommandPool);
	ASSERTION(result == VK_SUCCESS);

	if (result != VK_SUCCESS)
	{
		return false;
	}

	auto [id, resource] = m_context->cache.commandArena.request_resource(pQueue, std::move(hcommandPool), CommandArena::CommandBufferStore{}, 0u);

	new (&commandPool.data) RHIObjPtr{ id, &resource, rhi_obj_type_id_v<std::remove_reference_t<decltype(resource)>> };
	// Assign the command pool to the queue's type.
	commandPool.deviceContext = m_context;

	return true;
}

void RenderDevice::destroy_command_pool(command::CommandPool& commandPool)
{
	if (commandPool.data.valid())
	{
		// Queue up the pool into the zombie list.
		m_context->cache.zombies.emplace(commandPool.data.slot(), commandPool.data.type_id());
		new (&commandPool.data) RHIObjPtr{};
	}
}

void RenderDevice::clear_resources()
{
	m_context->clear_zombies();
}

ShaderLang RenderDevice::get_shading_language() const
{
	return m_shadingLanguage;
}

/*uint32 RenderDevice::request_semaphores(uint32 count)
{
	size_t i = 0;
	BinarySemaphorePool::SemaphorePool* page = nullptr;
	if (!m_binarySemaphorePool.pages.capacity() ||
		(m_binarySemaphorePool.pages.back().current + count) > m_binarySemaphorePool.pages.back().pool.size())
	{
		i = m_binarySemaphorePool.pages.push(BinarySemaphorePool::SemaphorePool{});
		page = &m_binarySemaphorePool.pages[i];

		Semaphore const sem{ .info = { .type = SemaphoreType::Binary, .initialValue = 0 } };
		for (auto& semaphore : page->pool)
		{
			new (&semaphore) Semaphore{ sem };
			create_semaphore(semaphore);
		}
	}
	else
	{
		// Search references that were returned to the pool.
		if (m_binarySemaphorePool.freeIndices.size())
		{
			uint32 index = 0;
			for (size_t j = 0; j < m_binarySemaphorePool.freeIndices.size(); ++j)
			{
				index = static_cast<uint32>(m_binarySemaphorePool.freeIndices[j]);
				const BinarySemaphorePool::SemaphoreReference& reference = m_binarySemaphorePool.references[static_cast<size_t>(index)];
				// Only take references that have more semaphores than what was requested.
				if (reference.count >= count)
				{
					m_binarySemaphorePool.freeIndices.pop_at(j);
					break;
				}
			}
			return index;
		}

		page = m_binarySemaphorePool.pages.last();
		i = m_binarySemaphorePool.pages.size() - 1;
	}

	uint32 index = static_cast<uint32>(m_binarySemaphorePool.references.emplace(i, page->current, count));
	page->current += count;

	return index;
}*/

/*void RenderDevice::return_semaphores(uint32 index)
{
	m_binarySemaphorePool.freeIndices.emplace(index);
}*/

/*void RenderDevice::release_semaphores()
{
	for (auto& page : m_binarySemaphorePool.pages)
	{
		page.current = 0;
		for (auto& semaphore : page.pool)
		{
			destroy_semaphore(semaphore);
		}
	}
	m_binarySemaphorePool.pages.empty();
}*/

bool RenderDevice::compile_shader(ShaderCompileInfo const& compileInfo, ShaderInfo& shaderInfo, std::string* error)
{
	if (m_api == API::Vulkan)
	{
		if (m_shadingLanguage == ShaderLang::GLSL)
		{
			return shader_compiler::compile_glsl_to_spirv(compileInfo, shaderInfo, error);
		}
	}

	return false;
}

void RenderDevice::wait_idle() const
{
	vkDeviceWaitIdle(m_context->device);
}

DeviceConfig device_default_configuration()
{
	constexpr uint32 MAX_UINT32 = std::numeric_limits<uint32>::max();
	return DeviceConfig{
		.framesInFlight					= 2u,
		.swapchainImageCount			= 2u,
		.bufferDescriptorsCount			= MAX_UINT32,
		.storageImageDescriptorsCount	= MAX_UINT32,
		.sampledImageDescriptorsCount	= MAX_UINT32,
		.samplerDescriptorCount			= MAX_UINT32,
		.pushConstantMaxSize			= MAX_UINT32
	};
}

bool create_device(RenderDevice& device, DeviceInitInfo const& info)
{	
	new (&device) RenderDevice{ API::Vulkan, info.shadingLanguage };
	return device.initialize(info);
}

void destroy_device(RenderDevice& device)
{
	device.terminate();
}

}