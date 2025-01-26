#include "vulkan/vkgpu.hpp"

struct VolkInit
{
	VolkInit()
	{
		volkInitialize();
	}
};
static VolkInit volkInit{};

VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_messenger_callback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT type,
	[[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData)
{
	if (pCallbackData && 
		(pCallbackData->messageIdNumber == 0x675DC32EU ||
		pCallbackData->messageIdNumber == 0x00000000U))
	{
		return VK_FALSE;
	}

	auto translate_message_severity = [](VkDebugUtilsMessageSeverityFlagBitsEXT flag) -> gpu::ErrorSeverity
	{
		switch (flag)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return gpu::ErrorSeverity::Verbose;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return gpu::ErrorSeverity::Info;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return gpu::ErrorSeverity::Warning;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		default:
			return gpu::ErrorSeverity::Error;
		}
	};

	gpu::DeviceInitInfo* pInfo = static_cast<gpu::DeviceInitInfo*>(pUserData);
	gpu::ErrorSeverity sv = translate_message_severity(severity);

	pInfo->callback(sv, pCallbackData->pMessage);

	return VK_FALSE;
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

namespace gpu
{
using DestroyFuncPtr = void(vk::DeviceImpl::*)(uint64 const);

constexpr DestroyFuncPtr resourceDestroyFn[] = {
	&vk::DeviceImpl::destroy_memory_block,
	&vk::DeviceImpl::destroy_binary_semaphore,
	&vk::DeviceImpl::destroy_timeline_semaphore,
	&vk::DeviceImpl::destroy_event,
	&vk::DeviceImpl::destroy_buffer,
	&vk::DeviceImpl::destroy_image,
	&vk::DeviceImpl::destroy_sampler,
	&vk::DeviceImpl::destroy_swapchain,
	&vk::DeviceImpl::destroy_shader,
	&vk::DeviceImpl::destroy_pipeline,
	&vk::DeviceImpl::destroy_command_pool
};

auto Device::info() const -> DeviceInfo const&
{
	return m_info;
}

auto Device::config() const -> DeviceConfig const&
{
	return m_config;
}

auto Device::wait_idle() const -> void
{
	auto&& self = to_device(this);
	vkDeviceWaitIdle(self.device);
}

auto Device::cpu_timeline() const -> uint64
{
	return m_cpuTimeline.load(std::memory_order_acquire);
}

auto Device::gpu_timeline() const -> uint64
{
	return m_gpuTimeline->value();
}

auto Device::submit(SubmitInfo const& info) -> bool
{
	auto&& self = to_device(this);
	auto&& gpuTimeline = to_impl(*m_gpuTimeline);
	
	self.flush_submit_info_buffers();

	m_cpuTimeline.fetch_add(1, std::memory_order_relaxed);

	VkQueue queue = self.mainQueue.queue;

	if (info.queue == DeviceQueue::Transfer)
	{
		queue = self.transferQueue.queue;
	}
	else if (info.queue == DeviceQueue::Compute)
	{
		queue = self.computeQueue.queue;
	}

	for (auto&& submittedCmdBuffer : info.commandBuffers)
	{
		if (submittedCmdBuffer->valid())
		{
			vk::CommandBufferImpl& vkcmdbuffer = to_impl(*submittedCmdBuffer);
			self.submitCommandBuffers.push_back(vkcmdbuffer.handle);
		}
	}

	for (auto&& [fence, waitValue] : info.waitFences)
	{
		vk::FenceImpl& timelineSemaphore = to_impl(*fence);
		self.waitSemaphores.push_back(timelineSemaphore.handle);
		self.waitTimelineSemaphoreValues.push_back(waitValue);
		self.waitDstStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	for (auto&& waitSemaphore : info.waitSemaphores)
	{
		vk::SemaphoreImpl& semaphore = to_impl(*waitSemaphore);
		self.waitSemaphores.push_back(semaphore.handle);
		self.waitTimelineSemaphoreValues.push_back(0);
		self.waitDstStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	self.signalSemaphores.push_back(gpuTimeline.handle);
	self.signalTimelineSemaphoreValues.push_back(m_cpuTimeline.load(std::memory_order_acquire));

	for (auto&& [fence, signalValue] : info.signalFences)
	{
		vk::FenceImpl& timelineSemaphore = to_impl(*fence);
		self.signalSemaphores.push_back(timelineSemaphore.handle);
		self.signalTimelineSemaphoreValues.push_back(signalValue);
	}

	for (auto&& signalSemaphore : info.signalSemaphores)
	{
		vk::SemaphoreImpl& semaphore = to_impl(*signalSemaphore);
		self.signalSemaphores.push_back(semaphore.handle);
		self.signalTimelineSemaphoreValues.push_back(0);
	}

	VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.waitSemaphoreValueCount = static_cast<uint32>(self.waitTimelineSemaphoreValues.size()),
		.pWaitSemaphoreValues = self.waitTimelineSemaphoreValues.data(),
		.signalSemaphoreValueCount = static_cast<uint32>(self.signalTimelineSemaphoreValues.size()),
		.pSignalSemaphoreValues = self.signalTimelineSemaphoreValues.data()
	};

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = &timelineSemaphoreSubmitInfo,
		.waitSemaphoreCount = static_cast<uint32>(self.waitSemaphores.size()),
		.pWaitSemaphores = self.waitSemaphores.data(),
		.pWaitDstStageMask = self.waitDstStageMasks.data(),
		.commandBufferCount = static_cast<uint32>(self.submitCommandBuffers.size()),
		.pCommandBuffers = self.submitCommandBuffers.data(),
		.signalSemaphoreCount = static_cast<uint32>(self.signalSemaphores.size()),
		.pSignalSemaphores = self.signalSemaphores.data()
	};

	return vkQueueSubmit(queue, 1, &submitInfo, nullptr) == VK_SUCCESS;
}

auto Device::present(PresentInfo const& info) -> bool
{
	auto&& self = to_device(this);
	
	self.flush_submit_info_buffers();

	for (auto&& swapchain : info.swapchains)
	{
		vk::SwapchainImpl& vkswapchain = to_impl(*swapchain);
		vk::SemaphoreImpl& presentSemaphore = to_impl(*vkswapchain.current_present_semaphore());

		self.presentSwapchains.push_back(vkswapchain.handle);
		self.presentImageIndices.push_back(vkswapchain.current_image_index());
		self.waitSemaphores.push_back(presentSemaphore.handle);
	}

	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = static_cast<uint32>(self.waitSemaphores.size()),
		.pWaitSemaphores = self.waitSemaphores.data(),
		.swapchainCount = static_cast<uint32>(self.presentSwapchains.size()),
		.pSwapchains = self.presentSwapchains.data(),
		.pImageIndices = self.presentImageIndices.data(),
	};

	return vkQueuePresentKHR(self.mainQueue.queue, &presentInfo) == VK_SUCCESS;
}

auto Device::clear_garbage() -> void
{
	auto&& self = to_device(this);

	std::scoped_lock lock{ self.gpuResourcePool.zombieMutex };

	uint64 const gpuTimeline = self.gpu_timeline();

	while (!self.gpuResourcePool.zombies.empty())
	{
		auto&& zombie = self.gpuResourcePool.zombies.back();

		if (zombie.timeline > gpuTimeline)
		{
			break;
		}

		uint32 const typeId = std::to_underlying(zombie.resourceType);

		(self.*resourceDestroyFn[typeId])(zombie.resourceId);

		self.gpuResourcePool.zombies.pop_back();
	}
}

auto Device::from(DeviceInitInfo const& info) -> std::expected<std::unique_ptr<Device>, std::string_view>
{
	auto vkdevice = std::make_unique<vk::DeviceImpl>();

	if (!vkdevice->initialize(info))
	{
		auto& deleter = vkdevice.get_deleter();
		deleter(vkdevice.release());

		return std::unexpected{ "Failed to create GPU device." };
	}

	return std::unique_ptr<Device>{ std::move(vkdevice) };
}

auto Device::destroy(std::unique_ptr<Device>& device) -> void
{
	auto vkdevice = static_cast<vk::DeviceImpl*>(device.get());

	vkdevice->terminate();

	auto& deleter = device.get_deleter();
	deleter(device.release());
}

namespace vk
{
auto DeviceImpl::initialize(DeviceInitInfo const& info) -> bool
{
	static constexpr std::pair<uint32, literal_t> vendorIdToName[] = {
		{ 0x1002u, "AMD" },
		{ 0x1010u, "ImgTec" },
		{ 0x10DEu, "NVIDIA" },
		{ 0x13B5u, "ARM" },
		{ 0x5143u, "Qualcomm" },
		{ 0x8086u, "INTEL" }
	};

	static constexpr DeviceType deviceTypeMap[] = {
		DeviceType::Other,
		DeviceType::Integrate_Gpu,
		DeviceType::Discrete_Gpu,
		DeviceType::Virtual_Gpu,
		DeviceType::Cpu
	};

	/**
	* m_initInfo caches the original init values including the config.
	*/
	m_initInfo = info;
	/**
	* m_config's value will be updated to reflect the device's capabilities.
	*/
	m_config = info.config;

	if (!create_vulkan_instance())
	{
		return false;
	}

	volkLoadInstance(instance);

	if (!create_debug_messenger())
	{
		return false;
	}

	if (!choose_physical_device())
	{
		return false;
	}

	get_physical_device_subgroup_properties();
	get_device_queue_family_indices();

	if (!create_logical_device())
	{
		return false;
	}

	volkLoadDevice(device);

	get_queue_handles();

	if (!create_device_allocator())
	{
		return false;
	}
	
	// Readjust configuration values.
	m_config.maxImages	= std::min(std::min(properties.limits.maxDescriptorSetSampledImages, properties.limits.maxDescriptorSetStorageImages), m_config.maxImages);
	m_config.maxBuffers = std::min(properties.limits.maxDescriptorSetStorageBuffers, m_config.maxBuffers);
	m_config.maxSamplers = std::min(properties.limits.maxDescriptorSetSamplers, m_config.maxSamplers);
	m_config.pushConstantMaxSize = std::min(properties.limits.maxPushConstantsSize, m_config.pushConstantMaxSize);

	if (!initialize_descriptor_cache())
	{
		return false;
	}

	m_info = {
		.name = m_initInfo.name,
		.type = deviceTypeMap[properties.deviceType],
		.api = API::Vulkan,
		.vendorID = properties.vendorID,
		.deviceID = properties.deviceID,
		.deviceName = properties.deviceName,
		.apiVersion = {
			.major = VK_API_VERSION_MAJOR(properties.apiVersion),
			.minor = VK_API_VERSION_MINOR(properties.apiVersion),
			.patch = VK_API_VERSION_PATCH(properties.apiVersion)
		},
		.driverVersion = {
			.major = VK_API_VERSION_MAJOR(properties.driverVersion),
			.minor = VK_API_VERSION_MINOR(properties.driverVersion),
			.patch = VK_API_VERSION_PATCH(properties.driverVersion)
		}
	};

	std::for_each(
		std::begin(vendorIdToName),
		std::end(vendorIdToName),
		[&](const std::pair<uint32, literal_t> vendorInfo) -> void
		{
			if (properties.vendorID == vendorInfo.first)
			{
				m_info.vendor = vendorInfo.second;
			}
		}
	);

	m_gpuTimeline = Fence::from(*this, { .name = "device gpu timeline" });

	return true;
}

auto DeviceImpl::terminate() -> void
{
	flush_submit_info_buffers();
	cleanup_resource_pool();

	vmaDestroyAllocator(allocator);
	vkDestroyDevice(device, nullptr);

	if (m_initInfo.validation)
	{
		vkDestroyDebugUtilsMessengerEXT(instance, debugger, nullptr);
	}

	gpu = VK_NULL_HANDLE;
	vkDestroyInstance(instance, nullptr);
}

auto DeviceImpl::push_constant_pipeline_layout(uint32 const pushConstantSize, uint32 const max) -> VkPipelineLayout
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

auto DeviceImpl::setup_debug_name(SwapchainImpl const& swapchain) -> void
{
	auto&& name = swapchain.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
			.objectHandle = reinterpret_cast<uint64_t>(swapchain.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(ShaderImpl const& shader) -> void
{
	auto&& name = shader.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SHADER_MODULE,
			.objectHandle = reinterpret_cast<uint64_t>(shader.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(BufferImpl const& buffer) -> void
{
	auto&& name = buffer.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_BUFFER,
			.objectHandle = reinterpret_cast<uint64_t>(buffer.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(ImageImpl const& image) -> void
{
	auto&& name = image.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT imageDebugNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE,
			.objectHandle = reinterpret_cast<uint64_t>(image.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &imageDebugNameInfo);

		lib::string imageViewDebugName = lib::format("<image_view>:{}", image.info().name.c_str());
		VkDebugUtilsObjectNameInfoEXT imageViewDebugNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
			.objectHandle = reinterpret_cast<uint64_t>(image.imageView),
			.pObjectName = imageViewDebugName.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &imageViewDebugNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(SamplerImpl const& sampler) -> void
{
	auto&& name = sampler.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SAMPLER,
			.objectHandle = reinterpret_cast<uint64_t>(sampler.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(PipelineImpl const& pipeline) -> void
{
	lib::string const* name = nullptr;

	if (pipeline.type() == PipelineType::Rasterization)
	{
		name = &pipeline.as<RasterPipeline>()->info().name;
	}

	if (name && name->size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_PIPELINE,
			.objectHandle = reinterpret_cast<uint64_t>(pipeline.handle),
			.pObjectName = name->c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(CommandPoolImpl const& commandPool) -> void
{
	auto&& name = commandPool.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(commandPool.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(CommandBufferImpl const& commandBuffer) -> void
{
	auto&& name = commandBuffer.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
			.objectHandle = reinterpret_cast<uint64_t>(commandBuffer.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(SemaphoreImpl const& semaphore) -> void
{
	auto&& name = semaphore.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SEMAPHORE,
			.objectHandle = reinterpret_cast<uint64_t>(semaphore.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(FenceImpl const& fence) -> void
{
	auto&& name = fence.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SEMAPHORE,
			.objectHandle = reinterpret_cast<uint64_t>(fence.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(EventImpl const& event) -> void
{
	auto&& name = event.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_EVENT,
			.objectHandle = reinterpret_cast<uint64_t>(event.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::setup_debug_name(MemoryBlockImpl const& event) -> void
{
	auto const& name = event.info().name;
	if (name.size())
	{
		VkDebugUtilsObjectNameInfoEXT debugResourceNameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY,
			.objectHandle = reinterpret_cast<uint64_t>(event.handle),
			.pObjectName = name.c_str(),
		};
		vkSetDebugUtilsObjectNameEXT(device, &debugResourceNameInfo);
	}
}

auto DeviceImpl::create_vulkan_instance() -> bool
{
	const auto& appVer = m_initInfo.appVersion;
	const auto& engineVer = m_initInfo.engineVersion;

	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = m_initInfo.appName.data(),
		.applicationVersion = VK_MAKE_API_VERSION(appVer.variant, appVer.major, appVer.minor, appVer.patch),
		.pEngineName = m_initInfo.engineName.data(),
		.engineVersion = VK_MAKE_API_VERSION(engineVer.variant, engineVer.major, engineVer.minor, engineVer.patch),
		.apiVersion = VK_API_VERSION_1_3
	};

	lib::array<literal_t> extensions;
	extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef WIN32
	extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	if (m_initInfo.validation)
	{
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	VkInstanceCreateInfo instanceInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = static_cast<uint32>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};

	literal_t layers[] = { "VK_LAYER_LUNARG_monitor", "VK_LAYER_KHRONOS_validation" };

	VkBool32 activate = true;

	auto debugUtil = populate_debug_messenger(const_cast<DeviceInitInfo*>(&m_initInfo));

	VkLayerSettingEXT validationLayerSync = {
		.pLayerName = "VK_LAYER_KHRONOS_validation",
        .pSettingName = "syncval_shader_accesses_heuristic",
        .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
        .valueCount = 1u,
        .pValues = &activate		
	};

	VkLayerSettingsCreateInfoEXT const validationLayerSettings = {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .pNext = &debugUtil,
        .settingCount = 1u,
        .pSettings = &validationLayerSync
	};

	VkValidationFeatureEnableEXT const validationFeatures[] =
    {
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };

	VkValidationFeaturesEXT validationInfo = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext = &validationLayerSettings,
        .enabledValidationFeatureCount = static_cast<uint32_t>(std::size(validationFeatures)),
        .pEnabledValidationFeatures = validationFeatures,
        .disabledValidationFeatureCount = 0U,
        .pDisabledValidationFeatures = nullptr
    };

	if (m_initInfo.validation)
	{
		instanceInfo.pNext = &validationInfo;
		instanceInfo.enabledLayerCount = 2;
		instanceInfo.ppEnabledLayerNames = layers;
		instanceInfo.pNext = &validationInfo;
	}

	return vkCreateInstance(&instanceInfo, nullptr, &instance) == VK_SUCCESS;
}

auto DeviceImpl::create_debug_messenger() -> bool
{
	if (m_initInfo.validation)
	{
		auto debugUtilInfo = populate_debug_messenger(const_cast<DeviceInitInfo*>(&m_initInfo));
		return vkCreateDebugUtilsMessengerEXT(instance, &debugUtilInfo, nullptr, &debugger) == VK_SUCCESS;
	}
	return true;
}

auto DeviceImpl::choose_physical_device() -> bool
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
	}

	auto preferredDeviceIndex = std::to_underlying(m_initInfo.preferredDevice);
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

	vulkan13Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;

	vulkan12Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
	vulkan12Properties.pNext = &vulkan13Properties;

	vulkan11Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
	vulkan11Properties.pNext = &vulkan12Properties;

	VkPhysicalDeviceProperties2 vulkanProperties2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &vulkan11Properties
	};

	vkGetPhysicalDeviceProperties2(gpu, &vulkanProperties2);

	vkGetPhysicalDeviceProperties(gpu, &properties);
	vkGetPhysicalDeviceFeatures(gpu, &features);

	return true;
}

auto DeviceImpl::get_physical_device_subgroup_properties() -> void
{
	subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

	VkPhysicalDeviceProperties2 deviceProperties2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &subgroupProperties
	};

	vkGetPhysicalDeviceProperties2(gpu, &deviceProperties2);
}

auto DeviceImpl::get_device_queue_family_indices() -> void
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

auto DeviceImpl::create_logical_device() -> bool
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
		.subgroupSizeControl = VK_TRUE,
		.computeFullSubgroups = VK_TRUE,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
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

auto DeviceImpl::get_queue_handles() -> void
{
	vkGetDeviceQueue(device, mainQueue.familyIndex, 0, &mainQueue.queue);
	vkGetDeviceQueue(device, transferQueue.familyIndex, 0, &transferQueue.queue);
	vkGetDeviceQueue(device, computeQueue.familyIndex, 0, &computeQueue.queue);
}

auto DeviceImpl::create_device_allocator() -> bool
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

auto DeviceImpl::initialize_descriptor_cache() -> bool
{
	if (!create_descriptor_pool())
	{
		return false;
	}
	if (!create_descriptor_set_layout())
	{
		return false;
	}
	if (!allocate_descriptor_set())
	{
		return false;
	}
	if (!create_pipeline_layouts())
	{
		return false;
	}

	// Set up buffer device address.
	{
		// Set up buffer device address.
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = m_config.maxBuffers * sizeof(VkDeviceAddress),
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


	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
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

auto DeviceImpl::create_descriptor_pool() -> bool
{
	VkDescriptorPoolSize bufferPoolSize{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = m_config.maxBuffers + 1
	};

	VkDescriptorPoolSize storageImagePoolSize{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = m_config.maxImages
	};

	VkDescriptorPoolSize sampledImagePoolSize{
		.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = m_config.maxImages
	};

	VkDescriptorPoolSize samplerPoolSize{
		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = m_config.maxSamplers
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

auto DeviceImpl::create_descriptor_set_layout() -> bool
{
	VkDescriptorSetLayoutBinding storageImageLayout{
		.binding = STORAGE_IMAGE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = m_config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding combinedImageSamplerLayout{
		.binding = COMBINED_IMAGE_SAMPLER_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = m_config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding sampledImageLayout{
		.binding = SAMPLED_IMAGE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = m_config.maxImages,
		.stageFlags = VK_SHADER_STAGE_ALL
	};

	VkDescriptorSetLayoutBinding samplerLayout{
		.binding = SAMPLER_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = m_config.maxSamplers,
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

auto DeviceImpl::allocate_descriptor_set() -> bool
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

auto DeviceImpl::create_pipeline_layouts() -> bool
{
	// The vulkan spec states that the size of a push constant must be a multiple of 4.
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPushConstantRange.html
	uint32 constexpr multiple = 4u;
	uint32 const count = static_cast<uint32>(m_config.pushConstantMaxSize / multiple) + 1u;

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

auto DeviceImpl::flush_submit_info_buffers() -> void
{
	waitSemaphores.clear();
	signalSemaphores.clear();
	waitDstStageMasks.clear();
	waitTimelineSemaphoreValues.clear();
	signalTimelineSemaphoreValues.clear();
	submitCommandBuffers.clear();
	presentSwapchains.clear();
	presentImageIndices.clear();
}

auto DeviceImpl::destroy_memory_block(uint64 const id) -> void
{
	using memory_block_index = decltype(gpuResourcePool.memoryBlocks)::index;

	memory_block_index const index = memory_block_index::from(id);
	MemoryBlockImpl& memoryBlock = gpuResourcePool.memoryBlocks[index];

	vmaFreeMemory(allocator, memoryBlock.handle);

	gpuResourcePool.memoryBlocks.erase(index);
}

auto DeviceImpl::destroy_binary_semaphore(uint64 const id) -> void
{
	using semaphore_index = decltype(gpuResourcePool.binarySemaphore)::index;

	semaphore_index const index = semaphore_index::from(id);
	SemaphoreImpl& semaphore = gpuResourcePool.binarySemaphore[index];

	vkDestroySemaphore(device, semaphore.handle, nullptr);

	gpuResourcePool.binarySemaphore.erase(index);
}

auto DeviceImpl::destroy_timeline_semaphore(uint64 const id) -> void
{
	using semaphore_index = decltype(gpuResourcePool.timelineSemaphore)::index;

	semaphore_index const index = semaphore_index::from(id);
	FenceImpl& fence = gpuResourcePool.timelineSemaphore[index];

	vkDestroySemaphore(device, fence.handle, nullptr);

	gpuResourcePool.timelineSemaphore.erase(index);
}

auto DeviceImpl::destroy_event(uint64 const id) -> void
{
	using event_index = decltype(gpuResourcePool.events)::index;

	event_index const index = event_index::from(id);
	EventImpl& event = gpuResourcePool.events[index];

	vkDestroyEvent(device, event.handle, nullptr);

	gpuResourcePool.events.erase(index);
}

auto DeviceImpl::destroy_buffer(uint64 const id) -> void
{
	using buffer_index = decltype(gpuResourcePool.buffers)::index;
	using memory_block_index = decltype(gpuResourcePool.memoryBlocks)::index;

	buffer_index const index = buffer_index::from(id);
	BufferImpl& buffer = gpuResourcePool.buffers[index];

	memory_block_index const memoryBlockId = memory_block_index::from(buffer.allocationBlock.id());

	MemoryBlockImpl& memoryBlock = gpuResourcePool.memoryBlocks[memoryBlockId];

	if (!buffer.is_transient())
	{
		vmaDestroyBuffer(allocator, buffer.handle, memoryBlock.handle);
		// Buffer owns the memory allocation and erases it from the resource pool.
		gpuResourcePool.memoryBlocks.erase(memoryBlockId);
	}
	else
	{
		vkDestroyBuffer(device, buffer.handle, nullptr);
	}

	gpuResourcePool.buffers.erase(index);
}

auto DeviceImpl::destroy_image(uint64 const id) -> void
{
	using image_index = decltype(gpuResourcePool.images)::index;
	using memory_block_index = decltype(gpuResourcePool.memoryBlocks)::index;

	image_index const index = image_index::from(id);
	ImageImpl& image = gpuResourcePool.images[index];

	if (!image.is_transient())
	{
		vkDestroyImageView(device, image.imageView, nullptr);

		if (!image.is_swapchain_image())
		{
			memory_block_index const memoryBlockId = memory_block_index::from(image.allocationBlock.id());

			MemoryBlockImpl& memoryBlock = gpuResourcePool.memoryBlocks[memoryBlockId];

			vmaDestroyImage(allocator, image.handle, memoryBlock.handle);

			// Image owns the memory allocation and erases it from the resource pool.
			gpuResourcePool.memoryBlocks.erase(memoryBlockId);
		}
	}
	else
	{
		vkDestroyImageView(device, image.imageView, nullptr);
		vkDestroyImage(device, image.handle, nullptr);
	}
	gpuResourcePool.images.erase(index);
}

auto DeviceImpl::destroy_sampler(uint64 const id) -> void
{
	using sampler_index = decltype(gpuResourcePool.samplers)::index;

	sampler_index const index = sampler_index::from(id);
	SamplerImpl& sampler = gpuResourcePool.samplers[index];

	gpuResourcePool.samplerCache.erase(sampler.info_packed());
	vkDestroySampler(device, sampler.handle, nullptr);

	gpuResourcePool.samplers.erase(index);
}

auto DeviceImpl::destroy_swapchain(uint64 const id) -> void
{
	using swapchain_index = decltype(gpuResourcePool.swapchains)::index;

	swapchain_index const index = swapchain_index::from(id);
	SwapchainImpl& swapchain = gpuResourcePool.swapchains[index];

	Surface* pSurface = swapchain.pSurface;

	vkDestroySwapchainKHR(device, swapchain.handle, nullptr);

	gpuResourcePool.swapchains.erase(index);
	
	/**
	* fetch_sub returns the previously held value of the atomic variable prior to the operation.
	* That means, if it returns 2, the current value of the atomic refCount is 1 and the only reference to the surface is itself.
	*/
	if (pSurface->refCount.fetch_sub(1, std::memory_order_acq_rel) == 2)
	{
		vkDestroySurfaceKHR(instance, pSurface->handle, nullptr);
		gpuResourcePool.surfaces.erase(pSurface->id);
	}
}

auto DeviceImpl::destroy_shader(uint64 const id) -> void
{
	using shader_index = decltype(gpuResourcePool.shaders)::index;

	shader_index const index = shader_index::from(id);
	ShaderImpl& shader = gpuResourcePool.shaders[index];

	vkDestroyShaderModule(device, shader.handle, nullptr);

	gpuResourcePool.shaders.erase(index);
}

auto DeviceImpl::destroy_pipeline(uint64 const id) -> void
{
	using pipeline_index = decltype(gpuResourcePool.pipelines)::index;

	pipeline_index const index = pipeline_index::from(id);
	PipelineImpl& pipeline = gpuResourcePool.pipelines[index];

	//vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
	vkDestroyPipeline(device, pipeline.handle, nullptr);

	gpuResourcePool.pipelines.erase(index);
}

auto DeviceImpl::destroy_command_pool(uint64 const id) -> void
{
	using command_pool_index = decltype(gpuResourcePool.commandPools)::index;

	command_pool_index const index = command_pool_index::from(id);
	std::unique_ptr<CommandPoolImpl>& cmdPool = gpuResourcePool.commandPools[index];

	std::array<VkCommandBuffer, MAX_COMMAND_BUFFER_PER_POOL> cmdBuffers;

	for (uint32 i = 0; i < cmdPool->commandBufferPool.commandBufferCount; ++i)
	{
		cmdBuffers[i] = cmdPool->commandBufferPool.commandBuffers[i].handle;
	}

	vkFreeCommandBuffers(device, cmdPool->handle, cmdPool->commandBufferPool.commandBufferCount, cmdBuffers.data());
	vkDestroyCommandPool(device, cmdPool->handle, nullptr);

	gpuResourcePool.commandPools.erase(index);
}

auto DeviceImpl::clear_descriptor_cache() -> void
{
	vmaUnmapMemory(allocator, descriptorCache.bdaAllocation);
	vmaDestroyBuffer(allocator, descriptorCache.bdaBuffer, descriptorCache.bdaAllocation);
	// Remove pre-allocated pipeline layouts.
	for (auto const& [_, layout] : descriptorCache.pipelineLayouts)
	{
		vkDestroyPipelineLayout(device, layout, nullptr);
	}
	descriptorCache.pipelineLayouts.clear();

	// Remove all descriptor related items.
	vkFreeDescriptorSets(device, descriptorCache.descriptorPool, 1, &descriptorCache.descriptorSet);
	vkDestroyDescriptorSetLayout(device, descriptorCache.descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorCache.descriptorPool, nullptr);

	descriptorCache.descriptorSet = VK_NULL_HANDLE;
	descriptorCache.descriptorSetLayout = VK_NULL_HANDLE;
	descriptorCache.descriptorPool = VK_NULL_HANDLE;
}

auto DeviceImpl::cleanup_resource_pool() -> void
{
	wait_idle();

	clear_garbage();
	clear_descriptor_cache();
}

auto translate_image_usage_flags(ImageUsage flags) -> VkImageUsageFlags
{
	uint32 const mask = static_cast<uint32>(flags);
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

auto translate_swapchain_presentation_mode(SwapchainPresentMode mode) -> VkPresentModeKHR
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

auto translate_color_space(ColorSpace const colorSpace) -> VkColorSpaceKHR
{
	switch (colorSpace)
	{
	case ColorSpace::Display_P3_Non_Linear:
		return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
	case ColorSpace::Extended_Srgb_Linear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
	case ColorSpace::Display_P3_Linear:
		return VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT;
	case ColorSpace::Dci_P3_Non_Linear:
		return VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT;
	case ColorSpace::Bt709_Linear:
		return VK_COLOR_SPACE_BT709_LINEAR_EXT;
	case ColorSpace::Bt709_Non_Linear:
		return VK_COLOR_SPACE_BT709_NONLINEAR_EXT;
	case ColorSpace::Bt2020_Linear:
		return VK_COLOR_SPACE_BT2020_LINEAR_EXT;
	case ColorSpace::Hdr10_St2084:
		return VK_COLOR_SPACE_HDR10_ST2084_EXT;
	case ColorSpace::Dolby_Vision:
		return VK_COLOR_SPACE_DOLBYVISION_EXT;
	case ColorSpace::Hdr10_Hlg:
		return VK_COLOR_SPACE_HDR10_HLG_EXT;
	case ColorSpace::Adobe_Rgb_Linear:
		return VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT;
	case ColorSpace::Adobe_Rgb_Non_Linear:
		return VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT;
	case ColorSpace::Pass_Through:
		return VK_COLOR_SPACE_PASS_THROUGH_EXT;
	case ColorSpace::Extended_Srgb_Non_Linear:
		return VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT;
	case ColorSpace::Display_Native_Amd:
		return VK_COLOR_SPACE_DISPLAY_NATIVE_AMD;
	case ColorSpace::Srgb_Non_Linear:
	default:
		return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}
}

auto translate_shader_stage(ShaderType type) -> VkShaderStageFlagBits
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
	case ShaderType::Task:
		return VK_SHADER_STAGE_TASK_BIT_NV;
	case ShaderType::Mesh:
		return VK_SHADER_STAGE_MESH_BIT_NV;
	case ShaderType::Ray_Generation:
		return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	case ShaderType::Any_Hit:
		return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
	case ShaderType::Closest_Hit:
		return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	case ShaderType::Ray_Miss:
		return VK_SHADER_STAGE_MISS_BIT_KHR;
	case ShaderType::Intersection:
		return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
	case ShaderType::Callable:
		return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
	case ShaderType::None:
	default:
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

auto translate_buffer_usage_flags(BufferUsage flags) -> VkBufferUsageFlags
{
	uint32 const mask = static_cast<uint32>(flags);
	constexpr VkBufferUsageFlagBits flagBits[] = {
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
	};
	VkBufferUsageFlags result{};
	for (uint32 i = 0; i < static_cast<uint32>(std::size(flagBits)); ++i)
	{
		uint32 const exist = (mask & (1u << i)) != 0u;
		auto const bit = flagBits[i];
		result |= exist * bit;
	}
	return result;
}

auto translate_image_type(ImageType type) -> VkImageType
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

auto translate_image_view_type(ImageType type) -> VkImageViewType
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

auto translate_image_tiling(ImageTiling tiling) -> VkImageTiling
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

auto translate_format(Format format) -> VkFormat
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
	return formats[std::to_underlying(format)];
}

auto translate_texel_filter(TexelFilter filter) -> VkFilter
{
	switch (filter)
	{
	case TexelFilter::Nearest:
		return VK_FILTER_NEAREST;
	case TexelFilter::Cubic_Image:
		return VK_FILTER_CUBIC_IMG;
	default:
	case TexelFilter::Linear:
		return VK_FILTER_LINEAR;
	}
}

auto translate_mipmap_mode(MipmapMode mode) -> VkSamplerMipmapMode
{
	switch (mode)
	{
	case MipmapMode::Nearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case MipmapMode::Linear:
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

auto translate_sampler_address_mode(SamplerAddress address) -> VkSamplerAddressMode
{
	switch (address)
	{
	case SamplerAddress::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case SamplerAddress::Mirrored_Repeat:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case SamplerAddress::Clamp_To_Border:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case SamplerAddress::Mirror_Clamp_To_Edge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case SamplerAddress::Clamp_To_Edge:
	default:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
}

auto translate_compare_op(CompareOp op) -> VkCompareOp
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

auto translate_border_color(BorderColor color) -> VkBorderColor
{
	switch (color)
	{
	case BorderColor::Int_Transparent_Black:
		return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	case BorderColor::Float_Opaque_Black:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	case BorderColor::Int_Opaque_Black:
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	case BorderColor::Float_Opaque_White:
		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	case BorderColor::Int_Opaque_White:
		return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	case BorderColor::Float_Transparent_Black:
	default:
		return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	}
}

auto translate_attachment_load_op(AttachmentLoadOp loadOp) -> VkAttachmentLoadOp
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

auto translate_attachment_store_op(AttachmentStoreOp storeOp) -> VkAttachmentStoreOp
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


auto translate_sample_count(SampleCount samples) -> VkSampleCountFlagBits
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

auto stride_for_shader_attrib_format(Format format) -> uint32
{
	switch (format)
	{
	case Format::R32_Int:
	case Format::R32_Uint:
	case Format::R32_Float:
		return 4;
	case Format::R32G32_Float:
		return 8;
	case Format::R32G32B32_Float:
		return 12;
	case Format::R32G32B32A32_Float:
		return 16;
	case Format::Undefined:
	default:
		return 0;
	}
}

auto vk_to_rhi_color_space(VkColorSpaceKHR colorSpace) -> ColorSpace
{
	switch (colorSpace)
	{
	case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
		return ColorSpace::Display_P3_Non_Linear;
	case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
		return ColorSpace::Extended_Srgb_Linear;
	case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:
		return ColorSpace::Display_P3_Linear;
	case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:
		return ColorSpace::Dci_P3_Non_Linear;
	case VK_COLOR_SPACE_BT709_LINEAR_EXT:
		return ColorSpace::Bt709_Linear;
	case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:
		return ColorSpace::Bt709_Non_Linear;
	case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
		return ColorSpace::Bt2020_Linear;
	case VK_COLOR_SPACE_HDR10_ST2084_EXT:
		return ColorSpace::Hdr10_St2084;
	case VK_COLOR_SPACE_DOLBYVISION_EXT:
		return ColorSpace::Dolby_Vision;
	case VK_COLOR_SPACE_HDR10_HLG_EXT:
		return ColorSpace::Hdr10_Hlg;
	case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:
		return ColorSpace::Adobe_Rgb_Linear;
	case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:
		return ColorSpace::Adobe_Rgb_Non_Linear;
	case VK_COLOR_SPACE_PASS_THROUGH_EXT:
		return ColorSpace::Pass_Through;
	case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
		return ColorSpace::Extended_Srgb_Non_Linear;
	case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:
		return ColorSpace::Display_Native_Amd;
	case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
	default:
		return ColorSpace::Srgb_Non_Linear;
	}
}

auto get_image_create_info(ImageInfo const& info) -> VkImageCreateInfo
{
	constexpr uint32 MAX_IMAGE_MIP_LEVEL = 4u;

	VkFormat const format = translate_format(info.format);
	VkImageUsageFlags const usage = translate_image_usage_flags(info.imageUsage);

	uint32 depth{ 1 };

	if (info.dimension.depth > 1)
	{
		depth = info.dimension.depth;
	}

	uint32 mipLevels{ 1 };

	if (info.mipLevel > 1)
	{
		const float32 width = static_cast<const float32>(info.dimension.width);
		const float32 height = static_cast<const float32>(info.dimension.height);

		mipLevels = static_cast<uint32>(std::floorf(std::log2f(std::max(width, height)))) + 1u;
		mipLevels = std::min(mipLevels, MAX_IMAGE_MIP_LEVEL);
	}

	return VkImageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = translate_image_type(info.type),
		.format = format,
		.extent = {
			.width = info.dimension.width,
			.height = info.dimension.height,
			.depth = depth
		},
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = translate_sample_count(info.samples),
		.tiling = translate_image_tiling(info.tiling),
		.usage = usage,
		.sharingMode = translate_sharing_mode(info.sharingMode),
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
}

auto get_buffer_create_info(BufferInfo const& info) -> VkBufferCreateInfo
{
	return VkBufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = info.size,
		.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | translate_buffer_usage_flags(info.bufferUsage),
		.sharingMode = translate_sharing_mode(info.sharingMode)
	};
}

auto translate_shader_attrib_format(Format format) -> VkFormat
{
	switch (format)
	{
	case Format::R32_Int:
		return VK_FORMAT_R32_SINT;
	case Format::R32_Uint:
		return VK_FORMAT_R32_UINT;
	case Format::R32_Float:
		return VK_FORMAT_R32_SFLOAT;
	case Format::R32G32_Float:
		return VK_FORMAT_R32G32_SFLOAT;
	case Format::R32G32B32_Float:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case Format::R32G32B32A32_Float:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case Format::Undefined:
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

auto translate_topology(TopologyType topology) -> VkPrimitiveTopology
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

auto translate_polygon_mode(PolygonMode mode) -> VkPolygonMode
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

auto translate_cull_mode(CullingMode mode) -> VkCullModeFlags
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

auto translate_front_face_dir(FrontFace face) -> VkFrontFace
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

auto translate_blend_factor(BlendFactor factor) -> VkBlendFactor
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

auto translate_blend_op(BlendOp op) -> VkBlendOp
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

auto sampler_info_packed_uint64(SamplerInfo const& info) -> uint64
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

auto translate_image_aspect_flags(ImageAspect flags) -> VkImageAspectFlags
{
	uint32 const mask = (uint32)flags;

	constexpr VkImageAspectFlagBits bits[] = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_ASPECT_STENCIL_BIT,
		VK_IMAGE_ASPECT_METADATA_BIT,
		VK_IMAGE_ASPECT_PLANE_0_BIT,
		VK_IMAGE_ASPECT_PLANE_1_BIT,
		VK_IMAGE_ASPECT_PLANE_2_BIT
	};

	uint32 constexpr numBits = (uint32)std::size(bits);

	VkImageUsageFlags result = 0;

	for (uint32 i = 0; i < numBits; ++i)
	{
		uint32 const exist = (mask & (1 << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_image_layout(ImageLayout layout) -> VkImageLayout
{
	switch (layout)
	{
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

auto translate_pipeline_stage_flags(PipelineStage stages) -> VkPipelineStageFlags2
{
	uint64 const mask = (uint64)stages;

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

	uint64 constexpr numBits = (uint64)std::size(bits);

	VkPipelineStageFlags2 result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;

		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_memory_access_flags(MemoryAccessType accesses) -> VkAccessFlags2
{
	uint64 mask = (uint64)accesses;

	constexpr VkAccessFlagBits2 bits[] = {
		VK_ACCESS_2_HOST_READ_BIT,
		VK_ACCESS_2_HOST_WRITE_BIT,
		VK_ACCESS_2_MEMORY_READ_BIT,
		VK_ACCESS_2_MEMORY_WRITE_BIT
	};

	uint64 constexpr numBits = (uint64)std::size(bits);

	VkAccessFlags2 result = 0;

	for (uint64 i = 0; i < numBits; ++i)
	{
		VkFlags64 const exist = (mask & (1ull << i)) != 0;
		result |= (exist * bits[i]);
	}

	return result;
}

auto translate_shader_stage_flags(ShaderStage shaderStage) -> VkShaderStageFlags
{
	//uint64 mask = static_cast<uint64>(shaderStage);

	//constexpr VkShaderStageFlagBits bits[] = {
	//	VK_SHADER_STAGE_VERTEX_BIT,
	//	VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
	//	VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
	//	VK_SHADER_STAGE_GEOMETRY_BIT,
	//	VK_SHADER_STAGE_FRAGMENT_BIT,
	//	VK_SHADER_STAGE_COMPUTE_BIT,
	//	VK_SHADER_STAGE_ALL_GRAPHICS,
	//	VK_SHADER_STAGE_ALL,
	//	VK_SHADER_STAGE_RAYGEN_BIT_KHR,
	//	VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
	//	VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
	//	VK_SHADER_STAGE_MISS_BIT_KHR,
	//	VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
	//	VK_SHADER_STAGE_CALLABLE_BIT_KHR,
	//	VK_SHADER_STAGE_TASK_BIT_NV,
	//	VK_SHADER_STAGE_MESH_BIT_NV
	//};

	//uint64 constexpr numBits = static_cast<uint64>(std::size(bits));

	//VkShaderStageFlags result = 0;

	//for (uint64 i = 0; i < numBits; ++i)
	//{
	//	VkFlags64 const exist = (mask & (1ull << i)) != 0;
	//	result |= (exist * bits[i]);
	//}

	//return result;

	return static_cast<VkShaderStageFlags>(shaderStage);
}

auto translate_sharing_mode(SharingMode sharingMode) -> VkSharingMode
{
	return sharingMode == SharingMode::Concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
}

auto translate_memory_usage(MemoryUsage usage) -> VmaAllocationCreateFlags
{
	uint32 const inputFlags = static_cast<uint32>(usage);
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

auto to_error_message(VkResult result) -> std::string_view
{
	switch (result)
	{
	case VK_NOT_READY:
		return "VK_NOT_READY - A fence or query has not yet completed";
	case VK_TIMEOUT:
		return "VK_TIMEOUT - A wait operation has not completed in the specified time";
	case VK_EVENT_SET:
		return "VK_EVENT_SET - An event is signaled";
	case VK_EVENT_RESET:
		return "VK_EVENT_RESET - An event is unsignaled";
	case VK_INCOMPLETE:
		return "VK_INCOMPLETE - A return array was too small for the result";
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		return "VK_ERROR_OUT_OF_HOST_MEMORY - A host memory allocation has failed";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		return "VK_ERROR_OUT_OF_DEVICE_MEMORY - A device memory allocation has failed";
	case VK_ERROR_INITIALIZATION_FAILED:
		return "VK_ERROR_INITIALIZATION_FAILED - Initialization of an object could not be completed for implementation-specific reasons";
	case VK_ERROR_DEVICE_LOST:
		return "VK_ERROR_DEVICE_LOST - The logical or physical device has been lost. See https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#devsandqueues-lost-device";
	case VK_ERROR_MEMORY_MAP_FAILED:
		return "VK_ERROR_MEMORY_MAP_FAILED - Mapping of a memory object has failed";
	case VK_ERROR_LAYER_NOT_PRESENT:
		return "VK_ERROR_LAYER_NOT_PRESENT - A requested layer is not present or could not be loaded";
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		return "VK_ERROR_EXTENSION_NOT_PRESENT - A requested extension is not supported";
	case VK_ERROR_FEATURE_NOT_PRESENT:
		return "VK_ERROR_FEATURE_NOT_PRESENT - A requested feature is not supported";
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		return "VK_ERROR_INCOMPATIBLE_DRIVER - The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons";
	case VK_ERROR_TOO_MANY_OBJECTS:
		return "VK_ERROR_TOO_MANY_OBJECTS - Too many objects of the type have already been created";
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		return "VK_ERROR_FORMAT_NOT_SUPPORTED - A requested format is not supported on this device";
	case VK_ERROR_FRAGMENTED_POOL:
		return "VK_ERROR_FRAGMENTED_POOL - A pool allocation has failed due to fragmentation of the pool�s memory";
	case VK_ERROR_UNKNOWN:
		return "VK_ERROR_UNKNOWN - An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred";
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		return "VK_ERROR_OUT_OF_POOL_MEMORY - A pool memory allocation has failed";
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:
		return "VK_ERROR_INVALID_EXTERNAL_HANDLE - An external handle is not a valid handle of the specified type";
	case VK_ERROR_FRAGMENTATION:
		return "VK_ERROR_FRAGMENTATION - A descriptor pool creation has failed due to fragmentation";
	case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
		return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS - A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid";
	case VK_PIPELINE_COMPILE_REQUIRED:
		return "VK_PIPELINE_COMPILE_REQUIRED - A requested pipeline creation would have required compilation, but the application requested compilation to not be performed";
	case VK_ERROR_SURFACE_LOST_KHR:
		return "VK_ERROR_SURFACE_LOST_KHR - A surface is no longer available";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR - The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again";
	case VK_SUBOPTIMAL_KHR:
		return "VK_SUBOPTIMAL_KHR - A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully";
	case VK_ERROR_OUT_OF_DATE_KHR:
		return "VK_ERROR_OUT_OF_DATE_KHR - A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR - The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image";
	case VK_ERROR_VALIDATION_FAILED_EXT:
		return "VK_ERROR_VALIDATION_FAILED_EXT - A command failed because invalid usage was detected by the implementation or a validation-layer";
	case VK_ERROR_INVALID_SHADER_NV:
		return "VK_ERROR_INVALID_SHADER_NV - One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled";
	case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
		return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT - ";
	case VK_ERROR_NOT_PERMITTED_KHR:
		return "VK_ERROR_NOT_PERMITTED_KHR - The driver implementation has denied a request to acquire a priority above the default priority (VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT) because the application does not have sufficient privileges";
	case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
		return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT - An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exclusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application�s control";
	case VK_THREAD_IDLE_KHR:
		return "VK_THREAD_IDLE_KHR - A deferred operation is not complete but there is currently no work for this thread to do at the time of this call";
	case VK_THREAD_DONE_KHR:
		return "VK_THREAD_DONE_KHR - A deferred operation is not complete but there is no work remaining to assign to additional threads";
	case VK_OPERATION_DEFERRED_KHR:
		return "VK_OPERATION_DEFERRED_KHR - A deferred operation was requested and at least some of the work was deferred";
	case VK_OPERATION_NOT_DEFERRED_KHR:
		return "VK_OPERATION_NOT_DEFERRED_KHR - A deferred operation was requested and no operations were deferred";
	case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
		return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT - An image creation failed because internal resources required for compression are exhausted";
	case VK_RESULT_MAX_ENUM:
	default:
		return "VK_SUCCESS - Command successfully completed";
	}
}
}

auto to_device(Device const* device) -> vk::DeviceImpl const&
{
	return *static_cast<vk::DeviceImpl const*>(device);
}

auto to_device(Device const& device) -> vk::DeviceImpl const&
{
	return to_device(&device);
}

auto to_device(Device* device) -> vk::DeviceImpl&
{
	return *static_cast<vk::DeviceImpl*>(device);
}

auto to_device(Device& device) -> vk::DeviceImpl&
{
	return to_device(&device);
}

auto to_impl(MemoryBlock& memoryBlock) -> vk::MemoryBlockImpl&
{
	return static_cast<vk::MemoryBlockImpl&>(memoryBlock);
}

auto to_impl(Semaphore& semaphore) -> vk::SemaphoreImpl&
{
	return static_cast<vk::SemaphoreImpl&>(semaphore);
}

auto to_impl(Fence& fence) -> vk::FenceImpl&
{
	return static_cast<vk::FenceImpl&>(fence);
}

auto to_impl(Event& event) -> vk::EventImpl&
{
	return static_cast<vk::EventImpl&>(event);
}

auto to_impl(Image& image) -> vk::ImageImpl&
{
	return static_cast<vk::ImageImpl&>(image);
}

auto to_impl(Sampler& sampler) -> vk::SamplerImpl&
{
	return static_cast<vk::SamplerImpl&>(sampler);
}

auto to_impl(Buffer& buffer) -> vk::BufferImpl&
{
	return static_cast<vk::BufferImpl&>(buffer);
}

auto to_impl(Shader& shader) -> vk::ShaderImpl&
{
	return static_cast<vk::ShaderImpl&>(shader);
}

auto to_impl(Pipeline& pipeline) -> vk::PipelineImpl&
{
	return static_cast<vk::PipelineImpl&>(pipeline);
}

auto to_impl(Swapchain& swapchain) -> vk::SwapchainImpl&
{
	return static_cast<vk::SwapchainImpl&>(swapchain);
}

auto to_impl(CommandBuffer& cmdBuffer) -> vk::CommandBufferImpl&
{
	return static_cast<vk::CommandBufferImpl&>(cmdBuffer);
}

auto to_impl(CommandPool& cmdPool) -> vk::CommandPoolImpl&
{
	return static_cast<vk::CommandPoolImpl&>(cmdPool);
}

auto to_impl(MemoryBlock const& memoryBlock) -> vk::MemoryBlockImpl const&
{
	return static_cast<vk::MemoryBlockImpl const&>(memoryBlock);
}

auto to_impl(Semaphore const& semaphore) -> vk::SemaphoreImpl const&
{
	return static_cast<vk::SemaphoreImpl const&>(semaphore);
}

auto to_impl(Fence const& fence) -> vk::FenceImpl const&
{
	return static_cast<vk::FenceImpl const&>(fence);
}

auto to_impl(Event const& event) -> vk::EventImpl const&
{
	return static_cast<vk::EventImpl const&>(event);
}

auto to_impl(Image const& image) -> vk::ImageImpl const&
{
	return static_cast<vk::ImageImpl const&>(image);
}

auto to_impl(Sampler const& sampler) -> vk::SamplerImpl const&
{
	return static_cast<vk::SamplerImpl const&>(sampler);
}

auto to_impl(Buffer const& buffer) -> vk::BufferImpl const&
{
	return static_cast<vk::BufferImpl const&>(buffer);
}

auto to_impl(Shader const& shader) -> vk::ShaderImpl const&
{
	return static_cast<vk::ShaderImpl const&>(shader);
}

auto to_impl(Pipeline const& pipeline) -> vk::PipelineImpl const&
{
	return static_cast<vk::PipelineImpl const&>(pipeline);
}

auto to_impl(Swapchain const& swapchain) -> vk::SwapchainImpl const&
{
	return static_cast<vk::SwapchainImpl const&>(swapchain);
}

auto to_impl(CommandBuffer const& cmdBuffer) -> vk::CommandBufferImpl const&
{
	return static_cast<vk::CommandBufferImpl const&>(cmdBuffer);
}

auto to_impl(CommandPool const& cmdPool) -> vk::CommandPoolImpl const&
{
	return static_cast<vk::CommandPoolImpl const&>(cmdPool);
}
}