module;

#include "lib/array.h"
#include "lib/string.h"
#include "vulkan/vk.h"

module forge.api;

import forge.common;

VKAPI_ATTR VkBool32 VKAPI_CALL forge_debug_util_messenger_callback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
	[[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData)
{
	auto translate_message_severity = [](VkDebugUtilsMessageSeverityFlagBitsEXT flag) -> frg::ErrorSeverity
	{
		switch (flag)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return frg::ErrorSeverity::Verbose;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return frg::ErrorSeverity::Info;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return frg::ErrorSeverity::Warning;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		default:
			return frg::ErrorSeverity::Error;
		}
	};

	frg::DeviceInitInfo* pInfo = static_cast<frg::DeviceInitInfo*>(pUserData);
	frg::ErrorSeverity sv = translate_message_severity(severity);
	pInfo->callback(sv, pCallbackData->pMessage);

	return VK_TRUE;
}

namespace frg::api
{
auto populate_debug_messenger(void* data) -> VkDebugUtilsMessengerCreateInfoEXT
{
	return VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

		.pfnUserCallback = forge_debug_util_messenger_callback,
		.pUserData = data
	};
}

auto Context::create_vulkan_instance(frg::DeviceInitInfo const& info) -> bool
{
	const auto& appVer = info.appVersion;
	const auto& engineVer = info.engineVersion;

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = info.appName.data(),
		.applicationVersion = VK_MAKE_API_VERSION(appVer.variant, appVer.major, appVer.minor, appVer.patch),
		.pEngineName = info.engineName.data(),
		.engineVersion = VK_MAKE_API_VERSION(engineVer.variant, engineVer.major, engineVer.minor, engineVer.patch),
		.apiVersion = VK_API_VERSION_1_3
	};

	lib::array<literal_t> extensions;
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
		literal_t layers[] = { "VK_LAYER_LUNARG_monitor", "VK_LAYER_KHRONOS_validation" };
		instanceInfo.enabledLayerCount = 2;
		instanceInfo.ppEnabledLayerNames = layers;
		auto debugUtil = populate_debug_messenger(const_cast<DeviceInitInfo*>(&info));
		instanceInfo.pNext = &debugUtil;
	}

	return vkCreateInstance(&instanceInfo, nullptr, &instance) == VK_SUCCESS;
}

auto Context::create_debug_messenger(frg::DeviceInitInfo const& info) -> bool
{
	if (validation)
	{
		auto debugUtilInfo = populate_debug_messenger(const_cast<DeviceInitInfo*>(&info));
		return vkCreateDebugUtilsMessengerEXT(instance, &debugUtilInfo, nullptr, &debugger) == VK_SUCCESS;
	}
	return true;
}

auto Context::choose_physical_device(frg::DeviceInitInfo const& initInfo) -> bool
{
	constexpr uint32 MAX_PHYSICAL_DEVICE = 8u;
	struct PhysicalDevices
	{
		std::array<VkPhysicalDevice, MAX_PHYSICAL_DEVICE> devices = {};
		size_t count = 0;
	};
	std::array<PhysicalDevices, 5> deviceTable = {};
	std::array<VkPhysicalDeviceType, 5> priority = {
		VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
		VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
		VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
		VK_PHYSICAL_DEVICE_TYPE_CPU,
		VK_PHYSICAL_DEVICE_TYPE_OTHER
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

	for (uint32 i = 0; i < numDevices; ++i)
	{
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(devices[i], &prop);

		PhysicalDevices& physDevice = deviceTable[prop.deviceType];
		if (physDevice.count < MAX_PHYSICAL_DEVICE)
		{
			physDevice.devices[physDevice.count++] = devices[i];
		}
	}

	auto score_device = [&](VkPhysicalDevice& device) -> uint32
	{
		uint32 score = 0;
		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(device, &properties);

		score += static_cast<uint32>(properties.limits.maxMemoryAllocationCount / 1000u);
		score += static_cast<uint32>(properties.limits.maxBoundDescriptorSets / 1000u);
		score += static_cast<uint32>(properties.limits.maxDrawIndirectCount / 1000u);
		score += static_cast<uint32>(properties.limits.maxDrawIndexedIndexValue / 1000u);

		return score;
	};

	for (PhysicalDevices& entry : deviceTable)
	{
		std::sort(
			std::begin(entry.devices),
			std::begin(entry.devices) + entry.count,
			[score_device, this](VkPhysicalDevice& a, VkPhysicalDevice& b) -> bool
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
	}

	auto preferredDeviceIndex = static_cast<std::underlying_type_t<DeviceType>>(initInfo.preferredDevice);
	if (deviceTable[preferredDeviceIndex].count)
	{
		gpu = deviceTable[preferredDeviceIndex].devices[0];
	}
	else
	{
		for (VkPhysicalDeviceType type : priority)
		{
			if (deviceTable[type].count)
			{
				gpu = deviceTable[type].devices[0];
				break;
			}
		}
	}

	vkGetPhysicalDeviceProperties(gpu, &properties);
	vkGetPhysicalDeviceFeatures(gpu, &features);

	return true;
}

auto Context::get_device_queue_family_indices() -> void
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
		if (mainQueue.familyIndex != uint32Max &&
			transferQueue.familyIndex != uint32Max &&
			computeQueue.familyIndex != uint32Max)
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
				(property.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 &&
				(property.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0)
			{
				mainQueue.familyIndex = i;
				mainQueue.properties = property;
			}
			// We look for a queue that can only do transfer operations.
			if (transferQueue.familyIndex == uint32Max &&
				(property.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 &&
				(property.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
			{
				transferQueue.familyIndex = i;
				transferQueue.properties = property;
			}
			// We look for a queue that is capable of doing compute but is not in the same family index as the main graphics queue.
			// This is for async compute.
			if (computeQueue.familyIndex == uint32Max &&
				(property.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 &&
				(mainQueue.familyIndex != i) &&
				(transferQueue.familyIndex != i))
			{
				computeQueue.familyIndex = i;
				computeQueue.properties = property;
			}
		}
	}
	// If the transfer queue family index is still not set by this point, we fall back to the graphics queue.
	if (transferQueue.familyIndex == uint32Max)
	{
		transferQueue.familyIndex = mainQueue.familyIndex;
		transferQueue.properties = mainQueue.properties;
	}
	// If the compute queue family index is still not set by this point, we fall back to the graphics queue.
	if (computeQueue.familyIndex == uint32Max)
	{
		computeQueue.familyIndex = mainQueue.familyIndex;
		computeQueue.properties = mainQueue.properties;
	}
}

auto Context::create_logical_device() -> bool
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

	lib::array<VkDeviceQueueCreateInfo> queueCreateInfos;
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
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE
	};

	VkPhysicalDeviceVulkan12Features deviceFeatures12{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &deviceFeatures13,
		.drawIndirectCount = VK_TRUE,
		.shaderFloat16 = VK_TRUE,
		.shaderInt8 = VK_TRUE,
		.descriptorIndexing = VK_TRUE,
		.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
		.shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
		.descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
		.scalarBlockLayout = VK_TRUE,
		.timelineSemaphore = VK_TRUE,
		.bufferDeviceAddress = VK_TRUE,
		.vulkanMemoryModel = VK_TRUE
	};

	VkPhysicalDeviceVulkan11Features deviceFeatures11{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = &deviceFeatures12,
		.shaderDrawParameters = VK_TRUE
	};

	VkPhysicalDeviceFeatures2 deviceFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &deviceFeatures11,
		.features = {
			.fullDrawIndexUint32 = VK_TRUE,
			.geometryShader = VK_TRUE,
			.tessellationShader = VK_TRUE,
			.logicOp = VK_TRUE,
			.multiDrawIndirect = VK_TRUE,
			.multiViewport = VK_TRUE,
			.samplerAnisotropy = VK_TRUE,
			.textureCompressionBC = VK_TRUE,
			.shaderUniformBufferArrayDynamicIndexing = VK_TRUE,
			.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
			.shaderStorageBufferArrayDynamicIndexing = VK_TRUE,
			.shaderStorageImageArrayDynamicIndexing = VK_TRUE,
			.shaderFloat64 = VK_TRUE,
			.shaderInt64 = VK_TRUE,
			.shaderInt16 = VK_TRUE
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

auto Context::get_queue_handles() -> void
{
	vkGetDeviceQueue(device, mainQueue.familyIndex, 0, &mainQueue.queue);
	vkGetDeviceQueue(device, transferQueue.familyIndex, 0, &transferQueue.queue);
	vkGetDeviceQueue(device, computeQueue.familyIndex, 0, &computeQueue.queue);
}

auto Context::create_device_allocator() -> bool
{
	VmaVulkanFunctions vmaVulkanFunctions{
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr
	};
	VmaAllocatorCreateInfo info{
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = gpu,
		.device = device,
		.preferredLargeHeapBlockSize = 0, // Sets it to lib internal default (256MiB).
		.pAllocationCallbacks = nullptr,
		.pDeviceMemoryCallbacks = nullptr,
		.pHeapSizeLimit = nullptr,
		.pVulkanFunctions = &vmaVulkanFunctions,
		.instance = instance,
		.vulkanApiVersion = VK_API_VERSION_1_3,
	};
	return vmaCreateAllocator(&info, &allocator) == VK_SUCCESS;
}

auto Context::create_descriptor_pool(frg::DeviceConfig const& config) -> bool
{
	VkDescriptorPoolSize bufferPoolSize{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = config.maxBuffers + 1
	};

	VkDescriptorPoolSize storageImagePoolSize{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = config.maxImages
	};

	VkDescriptorPoolSize sampledImagePoolSize{
		.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = config.maxImages
	};

	VkDescriptorPoolSize samplerPoolSize{
		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = config.maxSamplers
	};

	VkDescriptorPoolSize poolSizes[] = {
		bufferPoolSize,
		storageImagePoolSize,
		sampledImagePoolSize,
		samplerPoolSize
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		.maxSets = 1,
		.poolSizeCount = static_cast<uint32>(std::size(poolSizes)),
		.pPoolSizes = poolSizes,
	};

	return vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorCache.descriptorPool) == VK_SUCCESS;
}

auto Context::create_descriptor_set_layout(frg::DeviceConfig const& config) -> bool
{
	VkDescriptorSetLayoutBinding storageImageLayout{
		.binding = STORAGE_IMAGE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding combinedImageSamplerLayout{
		.binding = COMBINED_IMAGE_SAMPLER_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding sampledImageLayout{
		.binding = SAMPLED_IMAGE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding samplerLayout{
		.binding = SAMPLER_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = config.maxSamplers,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding bufferDeviceAddressLayout{
		.binding = BUFFER_DEVICE_ADDRESS_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
		storageImageLayout,
		combinedImageSamplerLayout,
		sampledImageLayout,
		samplerLayout,
		bufferDeviceAddressLayout
	};

	VkDescriptorBindingFlags bindingFlags[] = {
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlagsCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = static_cast<uint32>(std::size(bindingFlags)),
		.pBindingFlags = bindingFlags,
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &layoutBindingFlagsCreateInfo,
		.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
		.bindingCount = static_cast<uint32>(std::size(descriptorSetLayoutBindings)),
		.pBindings = descriptorSetLayoutBindings,
	};

	return vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorCache.descriptorSetLayout) == VK_SUCCESS;
}

auto Context::allocate_descriptor_set() -> bool
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorCache.descriptorPool,
		.descriptorSetCount = 1u,
		.pSetLayouts = &descriptorCache.descriptorSetLayout,
	};
	return vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorCache.descriptorSet) == VK_SUCCESS;
}

auto Context::create_pipeline_layouts(frg::DeviceConfig const& config) -> bool
{
	// The vulkan spec states that the size of a push constant must be a multiple of 4.
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPushConstantRange.html
	uint32 constexpr multiple = 4u;
	uint32 const count = static_cast<uint32>(config.pushConstantMaxSize / multiple) + 1u;

	descriptorCache.pipelineLayouts.reserve(count);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1u,
		.pSetLayouts = &descriptorCache.descriptorSetLayout,
	};

	VkPipelineLayout layout0;
	if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout0) != VK_SUCCESS)
	{
		return false;
	}
	descriptorCache.pipelineLayouts.emplace(0u, std::move(layout0));

	for (uint32 i = 1u; i < count; ++i)
	{
		VkPushConstantRange range{
			.stageFlags = VK_SHADER_STAGE_ALL,
			.offset = 0u,
			.size = static_cast<uint32>(i * multiple)
		};

		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &range;

		VkPipelineLayout hlayout;
		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &hlayout) == VK_SUCCESS)
		{
			descriptorCache.pipelineLayouts.emplace(range.size, hlayout);
		}
	}
	return true;
}

auto Context::initialize_descriptor_cache(frg::DeviceConfig const& config) -> bool
{
	if (!create_descriptor_pool(config))
	{
		return false;
	}
	if (!create_descriptor_set_layout(config))
	{
		return false;
	}
	if (!allocate_descriptor_set())
	{
		return false;
	}
	if (!create_pipeline_layouts(config))
	{
		return false;
	}

	// Set up buffer device address.
	{
		// Set up buffer device address.
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = config.maxBuffers * sizeof(VkDeviceAddress),
			.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};

		VmaAllocationCreateInfo allocInfo{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		};

		if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &descriptorCache.bdaBuffer, &descriptorCache.bdaAllocation, nullptr) == VK_SUCCESS)
		{
			vmaMapMemory(allocator, descriptorCache.bdaAllocation, reinterpret_cast<void**>(&descriptorCache.bdaHostAddress));

			VkDescriptorBufferInfo descriptorBufferInfo{
				.buffer = descriptorCache.bdaBuffer,
				.offset = 0,
				.range = VK_WHOLE_SIZE,
			};

			VkWriteDescriptorSet bdaWriteInfo{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = descriptorCache.descriptorSet,
				.dstBinding = BUFFER_DEVICE_ADDRESS_BINDING,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &descriptorBufferInfo
			};

			vkUpdateDescriptorSets(device, 1, &bdaWriteInfo, 0, nullptr);
		}
	}

	if constexpr (frg::ENABLE_DEBUG_RESOURCE_NAMES)
	{
		VkDebugUtilsObjectNameInfoEXT debugObjectNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(descriptorCache.descriptorPool),
			.pObjectName = "<descriptor_pool>:application",
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

		debugObjectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
		debugObjectNameInfo.objectHandle = reinterpret_cast<uint64_t>(descriptorCache.descriptorSetLayout);
		debugObjectNameInfo.pObjectName = "<descriptor_set_layout>:application";
		vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

		debugObjectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
		debugObjectNameInfo.objectHandle = reinterpret_cast<uint64_t>(descriptorCache.descriptorSet);
		debugObjectNameInfo.pObjectName = "<descriptor_set>:application";
		vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

		lib::string layoutName{ 256 };
		for (auto&& [size, layout] : descriptorCache.pipelineLayouts)
		{
			layoutName.format("<pipeline_layout>:push_constant_size = {} bytes", size);

			debugObjectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
			debugObjectNameInfo.objectHandle = reinterpret_cast<uint64_t>(layout);
			debugObjectNameInfo.pObjectName = layoutName.c_str();
			vkSetDebugUtilsObjectNameEXT(device, &debugObjectNameInfo);

			layoutName.clear();
		}
	}

	return true;
}

auto Context::initialize(frg::DeviceInitInfo const& info, frg::DeviceConfig& config) -> bool
{
	validation = info.validation;

	volkInitialize();

	if (!create_vulkan_instance(info))
	{
		return false;
	}

	volkLoadInstance(instance);

	if (!create_debug_messenger(info))
	{
		return false;
	}

	if (!choose_physical_device(info))
	{
		return false;
	}

	get_device_queue_family_indices();

	if (!create_logical_device())
	{
		return false;
	}

	/**
	* To avoid a dispatch overhead (~7%), we tell volk to directly load device-related Vulkan entrypoints.
	*/
	volkLoadDevice(device);
	get_queue_handles();

	if (!create_device_allocator())
	{
		return false;
	}

	// Readjust configuration values.
	config.maxImages = std::min(std::min(properties.limits.maxDescriptorSetSampledImages, properties.limits.maxDescriptorSetStorageImages), config.maxImages);
	config.maxBuffers = std::min(properties.limits.maxDescriptorSetStorageBuffers, config.maxBuffers);
	config.maxSamplers = std::min(properties.limits.maxDescriptorSetSamplers, config.maxSamplers);
	config.pushConstantMaxSize = std::min(properties.limits.maxPushConstantsSize, config.pushConstantMaxSize);

	if (!initialize_descriptor_cache(config))
	{
		return false;
	}

	return true;
}

auto Context::terminate() -> void
{
	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, nullptr);

	if (validation)
	{
		vkDestroyDebugUtilsMessengerEXT(instance, debugger, nullptr);
	}

	gpu = nullptr;
	vkDestroyInstance(instance, nullptr);
}

auto Context::push_constant_pipeline_layout(uint32 const pushConstantSize, uint32 const max) -> VkPipelineLayout
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
	return descriptorCache.pipelineLayouts[num];
}


auto translate_memory_usage(frg::MemoryUsage const usage) -> VmaAllocationCreateFlags
{
	uint32 const inputFlags = static_cast<uint32 const>(usage);
	VmaAllocationCreateFlagBits bits[] = {
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
		VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT,
		VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT
	};
	VmaAllocationCreateFlags result = 0;
	for (uint32 i = 0; i < static_cast<uint32>(std::size(bits)); ++i)
	{
		uint32 const exist = (inputFlags & (1u << i)) != 0u;
		result |= exist * bits[i];
	}
	return result;
}

auto translate_buffer_usage_flags(frg::BufferUsage const flags) -> VkBufferUsageFlags
{
	uint32 const mask = static_cast<uint32 const>(flags);
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

auto translate_format(frg::Format const format) -> VkFormat
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
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_BC1_RGB_UNORM_BLOCK,
		VK_FORMAT_BC1_RGB_SRGB_BLOCK,
		VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
		VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
		VK_FORMAT_BC2_UNORM_BLOCK,
		VK_FORMAT_BC2_SRGB_BLOCK,
		VK_FORMAT_BC3_UNORM_BLOCK,
		VK_FORMAT_BC3_SRGB_BLOCK,
		VK_FORMAT_BC4_UNORM_BLOCK,
		VK_FORMAT_BC4_SNORM_BLOCK,
		VK_FORMAT_BC5_UNORM_BLOCK,
		VK_FORMAT_BC5_SNORM_BLOCK,
		VK_FORMAT_BC6H_UFLOAT_BLOCK,
		VK_FORMAT_BC6H_SFLOAT_BLOCK,
		VK_FORMAT_BC7_UNORM_BLOCK,
		VK_FORMAT_BC7_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
		VK_FORMAT_EAC_R11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11_SNORM_BLOCK,
		VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
		VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
		VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
		VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
		VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
		VK_FORMAT_G8B8G8R8_422_UNORM,
		VK_FORMAT_B8G8R8G8_422_UNORM,
		VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
		VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
		VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
		VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
		VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
		VK_FORMAT_R10X6_UNORM_PACK16,
		VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
		VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
		VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
		VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
		VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
		VK_FORMAT_R12X4_UNORM_PACK16,
		VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
		VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
		VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
		VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
		VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
		VK_FORMAT_G16B16G16R16_422_UNORM,
		VK_FORMAT_B16G16R16G16_422_UNORM,
		VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
		VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,
		VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
		VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,
		VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
		VK_FORMAT_G8_B8R8_2PLANE_444_UNORM,
		VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16,
		VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16,
		VK_FORMAT_G16_B16R16_2PLANE_444_UNORM,
		VK_FORMAT_A4R4G4B4_UNORM_PACK16,
		VK_FORMAT_A4B4G4R4_UNORM_PACK16,
		VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK,
		VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK,
		VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG,
		VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,
		VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
		VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,
		VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG
	};
	return formats[static_cast<std::underlying_type_t<Format>>(format)];
}

auto translate_image_usage_flags(frg::ImageUsage const usage) -> VkImageUsageFlags
{
	uint32 const mask = static_cast<uint32 const>(usage);
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
		uint32 const exist = (mask & (1u << i)) != 0u;
		auto const bit = flagBits[i];
		result |= exist * bit;
	}
	return result;
}

auto translate_image_type(frg::ImageType const type) -> VkImageType
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

auto translate_sample_count(frg::SampleCount const samples) -> VkSampleCountFlagBits
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

auto translate_image_tiling(frg::ImageTiling const tiling) -> VkImageTiling
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

auto translate_image_view_type(frg::ImageType const type) -> VkImageViewType
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

auto translate_swapchain_presentation_mode(frg::SwapchainPresentMode const mode) -> VkPresentModeKHR
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

auto translate_color_space(frg::ColorSpace const colorSpace) -> VkColorSpaceKHR
{
	switch (colorSpace)
	{
	case frg::ColorSpace::Display_P3_Non_Linear:
		return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
	case frg::ColorSpace::Extended_Srgb_Linear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
	case frg::ColorSpace::Display_P3_Linear:
		return VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT;
	case frg::ColorSpace::Dci_P3_Non_Linear:
		return VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT;
	case frg::ColorSpace::Bt709_Linear:
		return VK_COLOR_SPACE_BT709_LINEAR_EXT;
	case frg::ColorSpace::Bt709_Non_Linear:
		return VK_COLOR_SPACE_BT709_NONLINEAR_EXT;
	case frg::ColorSpace::Bt2020_Linear:
		return VK_COLOR_SPACE_BT2020_LINEAR_EXT;
	case frg::ColorSpace::Hdr10_St2084:
		return VK_COLOR_SPACE_HDR10_ST2084_EXT;
	case frg::ColorSpace::Dolby_Vision:
		return VK_COLOR_SPACE_DOLBYVISION_EXT;
	case frg::ColorSpace::Hdr10_Hlg:
		return VK_COLOR_SPACE_HDR10_HLG_EXT;
	case frg::ColorSpace::Adobe_Rgb_Linear:
		return VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT;
	case frg::ColorSpace::Adobe_Rgb_Non_Linear:
		return VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;
	case frg::ColorSpace::Pass_Through:
		return VK_COLOR_SPACE_PASS_THROUGH_EXT;
	case frg::ColorSpace::Extended_Srgb_Non_Linear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
	case frg::ColorSpace::Display_Native_Amd:
		return VK_COLOR_SPACE_DISPLAY_NATIVE_AMD;
	case frg::ColorSpace::Srgb_Non_Linear:
	default:
		return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}
}

auto translate_shader_stage(frg::ShaderType const type) -> VkShaderStageFlagBits
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

auto translate_texel_filter(frg::TexelFilter const filter) -> VkFilter
{
	switch (filter)
	{
	case frg::TexelFilter::Nearest:
		return VK_FILTER_NEAREST;
	case frg::TexelFilter::Cubic_Image:
		return VK_FILTER_CUBIC_IMG;
	default:
	case frg::TexelFilter::Linear:
		return VK_FILTER_LINEAR;
	}
}

auto translate_mipmap_mode(frg::MipmapMode const mode) -> VkSamplerMipmapMode
{
	switch (mode)
	{
	case frg::MipmapMode::Nearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case frg::MipmapMode::Linear:
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

auto translate_sampler_address_mode(frg::SamplerAddress const address) -> VkSamplerAddressMode
{
	switch (address)
	{
	case frg::SamplerAddress::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case frg::SamplerAddress::Mirrored_Repeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case frg::SamplerAddress::Clamp_To_Border:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case frg::SamplerAddress::Mirror_Clamp_To_Edge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case frg::SamplerAddress::Clamp_To_Edge:
	default:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
}

auto translate_compare_op(frg::CompareOp const op) -> VkCompareOp
{
	switch (op)
	{
	case frg::CompareOp::Less:
		return VK_COMPARE_OP_LESS;
	case frg::CompareOp::Equal:
		return VK_COMPARE_OP_EQUAL;
	case frg::CompareOp::Less_Or_Equal:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case frg::CompareOp::Greater:
		return VK_COMPARE_OP_GREATER;
	case frg::CompareOp::Not_Equal:
		return VK_COMPARE_OP_NOT_EQUAL;
	case frg::CompareOp::Greater_Or_Equal:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case frg::CompareOp::Always:
		return VK_COMPARE_OP_ALWAYS;
	case frg::CompareOp::Never:
	default:
		return VK_COMPARE_OP_NEVER;
	}
}

auto translate_border_color(frg::BorderColor const color) -> VkBorderColor
{
	switch (color)
	{
	case frg::BorderColor::Int_Transparent_Black:
		return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	case frg::BorderColor::Float_Opaque_Black:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	case frg::BorderColor::Int_Opaque_Black:
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	case frg::BorderColor::Float_Opaque_White:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	case frg::BorderColor::Int_Opaque_White:
		return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	case frg::BorderColor::Float_Transparent_Black:
	default:
		return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	}
}

auto translate_attachment_load_op(frg::AttachmentLoadOp const loadOp) -> VkAttachmentLoadOp
{
	switch (loadOp)
	{
	case frg::AttachmentLoadOp::Clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	case frg::AttachmentLoadOp::Dont_Care:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case frg::AttachmentLoadOp::None:
		return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
	case frg::AttachmentLoadOp::Load:
	default:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}
}

auto translate_attachment_store_op(frg::AttachmentStoreOp const storeOp) -> VkAttachmentStoreOp
{
	switch (storeOp)
	{
	case frg::AttachmentStoreOp::Dont_Care:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	case frg::AttachmentStoreOp::None:
		return VK_ATTACHMENT_STORE_OP_NONE;
	case frg::AttachmentStoreOp::Store:
	default:
		return VK_ATTACHMENT_STORE_OP_STORE;
	}
}

auto translate_shader_attrib_format(frg::Format const format) -> VkFormat
{
	switch (format)
	{
	case frg::Format::R32_Int:
		return VK_FORMAT_R32_SINT;
	case frg::Format::R32_Uint:
		return VK_FORMAT_R32_UINT;
	case frg::Format::R32_Float:
		return VK_FORMAT_R32_SFLOAT;
	case frg::Format::R32G32_Float:
		return VK_FORMAT_R32G32_SFLOAT;
	case frg::Format::R32G32B32_Float:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case frg::Format::R32G32B32A32_Float:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case frg::Format::Undefined:
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

auto translate_topology(frg::TopologyType const topology) -> VkPrimitiveTopology
{
	switch (topology)
	{
	case frg::TopologyType::Point:
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case frg::TopologyType::Line:
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case frg::TopologyType::Line_Strip:
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case frg::TopologyType::Triange_Strip:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case frg::TopologyType::Triangle_Fan:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	case frg::TopologyType::Triangle:
	default:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

auto translate_polygon_mode(frg::PolygonMode const mode) -> VkPolygonMode
{
	switch (mode)
	{
	case frg::PolygonMode::Line:
		return VK_POLYGON_MODE_LINE;
	case frg::PolygonMode::Point:
		return VK_POLYGON_MODE_POINT;
	case frg::PolygonMode::Fill:
	default:
		return VK_POLYGON_MODE_FILL;
	}
}

auto translate_cull_mode(frg::CullingMode const mode) -> VkCullModeFlags
{
	switch (mode)
	{
	case frg::CullingMode::None:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_NONE);
	case frg::CullingMode::Front:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_FRONT_BIT);
	case frg::CullingMode::Front_And_Back:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_FRONT_AND_BACK);
	case frg::CullingMode::Back:
	default:
		return static_cast<VkCullModeFlags>(VK_CULL_MODE_BACK_BIT);
	}
}

auto translate_front_face_dir(frg::FrontFace const face) -> VkFrontFace
{
	switch (face)
	{
	case frg::FrontFace::Clockwise:
		return VK_FRONT_FACE_CLOCKWISE;
	case frg::FrontFace::Counter_Clockwise:
	default:
		return VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
}

auto translate_blend_factor(frg::BlendFactor const factor) -> VkBlendFactor
{
	switch (factor)
	{
	case frg::BlendFactor::One:
		return VK_BLEND_FACTOR_ONE;
	case frg::BlendFactor::Src_Color:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case frg::BlendFactor::One_Minus_Src_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case frg::BlendFactor::Dst_Color:
		return VK_BLEND_FACTOR_DST_COLOR;
	case frg::BlendFactor::One_Minus_Dst_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case frg::BlendFactor::Src_Alpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case frg::BlendFactor::One_Minus_Src_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case frg::BlendFactor::Dst_Alpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case frg::BlendFactor::One_Minus_Dst_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	case frg::BlendFactor::Constant_Color:
		return VK_BLEND_FACTOR_CONSTANT_COLOR;
	case frg::BlendFactor::One_Minus_Constant_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case frg::BlendFactor::Constant_Alpha:
		return VK_BLEND_FACTOR_CONSTANT_ALPHA;
	case frg::BlendFactor::One_Minus_Constant_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	case frg::BlendFactor::Src_Alpha_Saturate:
		return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
	case frg::BlendFactor::Src1_Color:
		return VK_BLEND_FACTOR_SRC1_COLOR;
	case frg::BlendFactor::One_Minus_Src1_Color:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
	case frg::BlendFactor::Src1_Alpha:
		return VK_BLEND_FACTOR_SRC1_ALPHA;
	case frg::BlendFactor::One_Minus_Src1_Alpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
	case frg::BlendFactor::Zero:
	default:
		return VK_BLEND_FACTOR_ZERO;
	}
}

auto translate_blend_op(frg::BlendOp const op) -> VkBlendOp
{
	switch (op)
	{
	case frg::BlendOp::Subtract:
		return VK_BLEND_OP_SUBTRACT;
	case frg::BlendOp::Reverse_Subtract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case frg::BlendOp::Add:
	default:
		return VK_BLEND_OP_ADD;
	}
}

auto translate_image_aspect_flags(frg::ImageAspect flags) -> VkImageAspectFlags
{
	uint32 const mask = static_cast<uint32>(flags);

	constexpr VkImageAspectFlagBits bits[] = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_ASPECT_STENCIL_BIT,
		VK_IMAGE_ASPECT_METADATA_BIT,
		VK_IMAGE_ASPECT_PLANE_0_BIT,
		VK_IMAGE_ASPECT_PLANE_1_BIT,
		VK_IMAGE_ASPECT_PLANE_2_BIT
	};

	uint32 constexpr numBits = static_cast<size_t>(std::size(bits));

	VkImageUsageFlags result = 0;

	for (uint32 i = 0; i < numBits; ++i)
	{
		uint32 const exist = (mask & (1 << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_image_layout(frg::ImageLayout layout) -> VkImageLayout
{
	switch (layout)
	{
	case frg::ImageLayout::General:
		return VK_IMAGE_LAYOUT_GENERAL;
	case frg::ImageLayout::Color_Attachment:
		return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case frg::ImageLayout::Depth_Stencil_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case frg::ImageLayout::Depth_Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	case frg::ImageLayout::Shader_Read_Only:
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case frg::ImageLayout::Transfer_Src:
		return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case frg::ImageLayout::Transfer_Dst:
		return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case frg::ImageLayout::Preinitialized:
		return VK_IMAGE_LAYOUT_PREINITIALIZED;
	case frg::ImageLayout::Depth_Read_Only_Stencil_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	case frg::ImageLayout::Depth_Attachment_Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
	case frg::ImageLayout::Depth_Attachment:
		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	case frg::ImageLayout::Depth_Read_Only:
		return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
	case frg::ImageLayout::Stencil_Attachment:
		return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
	case frg::ImageLayout::Stencil_Read_Only:
		return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
	case frg::ImageLayout::Read_Only:
		return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
	case frg::ImageLayout::Attachment:
		return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	case frg::ImageLayout::Present_Src:
		return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	case frg::ImageLayout::Shared_Present:
		return VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
	case frg::ImageLayout::Fragment_Density_Map:
		return VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	case frg::ImageLayout::Fragment_Shading_Rate_Attachment:
		return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	case frg::ImageLayout::Undefined:
	default:
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

auto translate_pipeline_stage_flags(frg::PipelineStage stages) -> VkPipelineStageFlags2
{
	uint64 const mask = static_cast<uint64>(stages);

	constexpr VkPipelineStageFlagBits2 bits[] = {
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
		VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
		VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV,
		VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV,
		VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR
	};

	uint64 constexpr numBits = static_cast<uint64>(std::size(bits));

	VkPipelineStageFlags2 result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;

		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_memory_access_flags(frg::MemoryAccessType accesses) -> VkAccessFlags2
{
	uint64 mask = static_cast<uint64>(accesses);

	constexpr VkAccessFlagBits2 bits[] = {
		VK_ACCESS_2_HOST_READ_BIT,
		VK_ACCESS_2_HOST_WRITE_BIT,
		VK_ACCESS_2_MEMORY_READ_BIT,
		VK_ACCESS_2_MEMORY_WRITE_BIT
	};

	uint64 constexpr numBits = static_cast<uint64>(std::size(bits));

	VkAccessFlags2 result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_shader_stage_flags(frg::ShaderStage shaderStage) -> VkShaderStageFlags
{
	uint64 mask = static_cast<uint64>(shaderStage);

	constexpr VkShaderStageFlagBits bits[] = {
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_SHADER_STAGE_ALL_GRAPHICS,
		VK_SHADER_STAGE_ALL
	};

	uint64 constexpr numBits = static_cast<uint64>(std::size(bits));

	VkShaderStageFlags result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

auto stride_for_shader_attrib_format(frg::Format const format) -> uint32
{
	switch (format)
	{
	case frg::Format::R32_Int:
	case frg::Format::R32_Uint:
	case frg::Format::R32_Float:
		return 4;
	case frg::Format::R32G32_Float:
		return 8;
	case frg::Format::R32G32B32_Float:
		return 12;
	case frg::Format::R32G32B32A32_Float:
		return 16;
	case frg::Format::Undefined:
	default:
		return 0;
	}
}

auto sampler_info_packed_uint64(frg::SamplerInfo const& info) -> uint64
{
	uint64 packed = 0;
	uint64 const minf = static_cast<uint64>(info.minFilter);
	uint64 const magf = static_cast<uint64>(info.magFilter);
	uint64 const mipm = static_cast<uint64>(info.mipmapMode);
	uint64 const admu = static_cast<uint64>(info.addressModeU);
	uint64 const admv = static_cast<uint64>(info.addressModeV);
	uint64 const admw = static_cast<uint64>(info.addressModeW);
	uint64 const cmpe = (info.compareOp != CompareOp::Never) ? 1ull : 0ull;
	uint64 const anie = (info.maxAnisotropy > 0.f) ? 1ull : 0ull;
	uint64 const cmpo = static_cast<uint64>(info.compareOp);
	uint64 const bcol = static_cast<uint64>(info.borderColor);
	uint64 const cord = (info.unnormalizedCoordinates) ? 1ull : 0ull;

	uint64 const mplb = static_cast<uint64>(info.mipLodBias);
	uint64 const maxa = static_cast<uint64>(info.maxAnisotropy);
	uint64 const minl = static_cast<uint64>(info.minLod);
	uint64 const maxl = static_cast<uint64>(info.maxLod);

	packed |= (0x000000000000000FULL & minf);
	packed |= (0x00000000000000F0ULL & magf);
	packed |= (0x0000000000000F00ULL & mipm);
	packed |= (0x000000000000F000ULL & bcol);
	packed |= (0x00000000000F0000ULL & admu);
	packed |= (0x0000000000F00000ULL & admv);
	packed |= (0x000000000F000000ULL & admw);
	packed |= (0x00000000F0000000ULL & cmpo);
	packed |= (0x0000000F00000000ULL & mplb);
	packed |= (0x000000F000000000ULL & maxa);
	packed |= (0x00000F0000000000ULL & minl);
	packed |= (0x0000F00000000000ULL & maxl);
	packed |= (0x2000000000000000ULL & anie);
	packed |= (0x4000000000000000ULL & cmpe);
	packed |= (0x8000000000000000ULL & cord);

	return packed;
}

auto vk_to_rhi_color_space(VkColorSpaceKHR colorSpace) -> frg::ColorSpace
{
	switch (colorSpace)
	{
	case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
		return frg::ColorSpace::Display_P3_Non_Linear;
	case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
		return frg::ColorSpace::Extended_Srgb_Linear;
	case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
		return frg::ColorSpace::Display_P3_Linear;
	case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
		return frg::ColorSpace::Dci_P3_Non_Linear;
	case VK_COLOR_SPACE_BT709_LINEAR_EXT:
		return frg::ColorSpace::Bt709_Linear;
	case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
		return frg::ColorSpace::Bt709_Non_Linear;
	case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
		return frg::ColorSpace::Bt2020_Linear;
	case VK_COLOR_SPACE_HDR10_ST2084_EXT:
		return frg::ColorSpace::Hdr10_St2084;
	case VK_COLOR_SPACE_DOLBYVISION_EXT:
		return frg::ColorSpace::Dolby_Vision;
	case VK_COLOR_SPACE_HDR10_HLG_EXT:
		return frg::ColorSpace::Hdr10_Hlg;
	case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
		return frg::ColorSpace::Adobe_Rgb_Linear;
	case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
		return frg::ColorSpace::Adobe_Rgb_Non_Linear;
	case VK_COLOR_SPACE_PASS_THROUGH_EXT:
		return frg::ColorSpace::Pass_Through;
	case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
		return frg::ColorSpace::Extended_Srgb_Non_Linear;
	case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:
		return frg::ColorSpace::Display_Native_Amd;
	case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
	default:
		return frg::ColorSpace::Srgb_Non_Linear;
	}
}
}