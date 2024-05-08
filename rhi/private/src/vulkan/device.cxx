module;

#include "vulkan/vk.h"
#include "lib/string.h"

module forge;

namespace frg
{
auto next_resource_type_id() -> resource_type_id
{
	using resource_type_id_t = resource_type_id::value_type;

	static resource_type_id_t id = 0;

	return resource_type_id{ id++ };
}

constexpr resource_type_id SEMAPHORE_TYPE_ID		= resource_type_id_v<Semaphore>;
constexpr resource_type_id FENCE_TYPE_ID			= resource_type_id_v<Fence>;
constexpr resource_type_id BUFFER_TYPE_ID			= resource_type_id_v<Buffer>;
constexpr resource_type_id IMAGE_TYPE_ID			= resource_type_id_v<Image>;
constexpr resource_type_id SAMPLER_TYPE_ID			= resource_type_id_v<Sampler>;
constexpr resource_type_id SWAPCHAIN_TYPE_ID		= resource_type_id_v<Swapchain>;
constexpr resource_type_id SHADER_TYPE_ID			= resource_type_id_v<Shader>;
constexpr resource_type_id PIPELINE_TYPE_ID			= resource_type_id_v<Pipeline>;
constexpr resource_type_id COMMAND_POOL_TYPE_ID		= resource_type_id_v<CommandPool>;
constexpr resource_type_id COMMAND_BUFFER_TYPE_ID	= resource_type_id_v<CommandBuffer>;
constexpr resource_type_id MAX_TYPE_ID				= resource_type_id_v<struct _MAX_DEVICE_RESOURCE>;

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
	vkDeviceWaitIdle(m_apiContext.device);
}

auto Device::cpu_timeline() const -> uint64
{
	return m_cpuTimeline.load(std::memory_order_relaxed);
}

auto Device::gpu_timeline() const -> uint64
{
	return m_gpuTimeline->value();
}

auto Device::submit(SubmitInfo const& info) -> bool
{
	m_apiContext.flush_submit_info_buffers();

	VkQueue queue = m_apiContext.mainQueue.queue;

	if (info.queue == DeviceQueue::Transfer)
	{
		queue = m_apiContext.transferQueue.queue;
	}
	else if (info.queue == DeviceQueue::Compute)
	{
		queue = m_apiContext.computeQueue.queue;
	}

	for (Resource<CommandBuffer> const& submittedCmdBuffer : info.commandBuffers)
	{
		if (submittedCmdBuffer->current_state() == CommandBufferState::Executable)
		{
			api::Semaphore& semaphore = submittedCmdBuffer->m_completionTimeline->m_impl;
			api::CommandBuffer& cmdBuffer = submittedCmdBuffer->m_impl;

			m_apiContext.submitCommandBuffers.push_back(cmdBuffer.handle);

			m_apiContext.signalSemaphores.push_back(semaphore.handle);
			m_apiContext.signalTimelineSemaphoreValues.push_back(submittedCmdBuffer->m_recordingTimeline);

			submittedCmdBuffer->m_state = CommandBufferState::Pending;
		}
	}

	for (auto&& [fence, waitValue] : info.waitFences)
	{
		api::Semaphore& timelineSemaphore = fence->m_impl;

		m_apiContext.waitSemaphores.push_back(timelineSemaphore.handle);
		m_apiContext.waitTimelineSemaphoreValues.push_back(waitValue);
		m_apiContext.waitDstStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	for (Resource<Semaphore> const& waitSemaphore : info.waitSemaphores)
	{
		api::Semaphore& semaphore = waitSemaphore->m_impl;

		m_apiContext.waitSemaphores.push_back(semaphore.handle);
		m_apiContext.waitTimelineSemaphoreValues.push_back(0);
		m_apiContext.waitDstStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	}

	// Signal CPU timeline value.
	uint64 const cpuTimeline = m_cpuTimeline.fetch_add(1, std::memory_order_relaxed) + 1;

	// Signal GPU timeline fence.
	m_apiContext.signalSemaphores.push_back(m_gpuTimeline->m_impl.handle);
	m_apiContext.signalTimelineSemaphoreValues.push_back(cpuTimeline);

	for (auto&& [fence, signalValue] : info.signalFences)
	{
		api::Semaphore& timelineSemaphore = fence->m_impl;

		m_apiContext.signalSemaphores.push_back(timelineSemaphore.handle);
		m_apiContext.signalTimelineSemaphoreValues.push_back(signalValue);
	}

	for (Resource<Semaphore> const& signalSemaphore : info.signalSemaphores)
	{
		api::Semaphore& semaphore = signalSemaphore->m_impl;

		m_apiContext.signalSemaphores.push_back(semaphore.handle);
		m_apiContext.signalTimelineSemaphoreValues.push_back(0);
	}

	VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
		.waitSemaphoreValueCount = static_cast<uint32>(m_apiContext.waitTimelineSemaphoreValues.size()),
		.pWaitSemaphoreValues = m_apiContext.waitTimelineSemaphoreValues.data(),
		.signalSemaphoreValueCount = static_cast<uint32>(m_apiContext.signalTimelineSemaphoreValues.size()),
		.pSignalSemaphoreValues = m_apiContext.signalTimelineSemaphoreValues.data()
	};

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = &timelineSemaphoreSubmitInfo,
		.waitSemaphoreCount = static_cast<uint32>(m_apiContext.waitSemaphores.size()),
		.pWaitSemaphores = m_apiContext.waitSemaphores.data(),
		.pWaitDstStageMask = m_apiContext.waitDstStageMasks.data(),
		.commandBufferCount = static_cast<uint32>(m_apiContext.submitCommandBuffers.size()),
		.pCommandBuffers = m_apiContext.submitCommandBuffers.data(),
		.signalSemaphoreCount = static_cast<uint32>(m_apiContext.signalSemaphores.size()),
		.pSignalSemaphores = m_apiContext.signalSemaphores.data()
	};

	return vkQueueSubmit(queue, 1, &submitInfo, nullptr) == VK_SUCCESS;
}

auto Device::present(PresentInfo const& info) -> bool
{
	
}

auto Device::initialize(DeviceInitInfo const& info) -> bool
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

	DeviceConfig config = info.config;

	bool initialized = m_apiContext.initialize(info, config);

	if (initialized)
	{
		m_info = {
			.name = info.name,
			.type = deviceTypeMap[m_apiContext.properties.deviceType],
			.api = API::Vulkan,
			.shaderLang = info.shadingLanguage,
			.vendorID = m_apiContext.properties.vendorID,
			.deviceID = m_apiContext.properties.deviceID,
			.deviceName = m_apiContext.properties.deviceName,
			.apiVersion = {
				.major = VK_API_VERSION_MAJOR(m_apiContext.properties.apiVersion),
				.minor = VK_API_VERSION_MINOR(m_apiContext.properties.apiVersion),
				.patch = VK_API_VERSION_PATCH(m_apiContext.properties.apiVersion)
			},
			.driverVersion = {
				.major = VK_API_VERSION_MAJOR(m_apiContext.properties.driverVersion),
				.minor = VK_API_VERSION_MINOR(m_apiContext.properties.driverVersion),
				.patch = VK_API_VERSION_PATCH(m_apiContext.properties.driverVersion)
			}
		};

		std::for_each(
			std::begin(vendorIdToName),
			std::end(vendorIdToName),
			[&](const std::pair<uint32, literal_t> vendorInfo) -> void
			{
				if (m_apiContext.properties.vendorID == vendorInfo.first)
				{
					m_info.vendor = vendorInfo.second;
				}
			}
		);

		m_initInfo = info;
		m_config = config;

		m_gpuTimeline = Fence::from(*this, { .name = lib::format("{}_device_timeline", m_info.name.data()) });

		// Increment the CPU timeline because theoratically, the device *IS* already running on the CPU side.
		m_cpuTimeline.fetch_add(1, std::memory_order_relaxed);
	}

	return initialized;
}

auto Device::terminate() -> void
{
	// TODO(afiq):
	// 1. Put remaining resources in the zombie list ...
	// 2. Clear resources in the zombie list ...
	m_apiContext.terminate();
}

//auto _Instance_::create_device(DeviceInitInfo const& info) -> std::optional<lib::ref<Device>>
//{
//	auto&& [key, device] = m_devices.emplace(lib::hash_string_view{ info.name }, std::make_unique<Device>());
//	if (!device->initialize(info))
//	{
//		m_devices.erase(key);
//
//		return std::nullopt;
//	}
//	return std::make_optional(device.get());
//}
//
//auto _Instance_::destroy_device(Device& device) -> void
//{
//	auto pair = m_devices.at(device.m_info.name);
//	device.terminate();
//	m_devices.erase(pair->first);
//}
//
//std::unique_ptr<_Instance_> _Instance_::_instance = {};
//
//auto create_instance() -> Instance
//{
//	if (!_Instance_::_instance)
//	{
//		_Instance_::_instance = std::make_unique<_Instance_>();
//	}
//	return _Instance_::_instance.get();
//}
//
//auto destroy_instance() -> void
//{
//	_Instance_& instance = *_Instance_::_instance;
//	if (instance.m_devices.size())
//	{
//		// TODO(afiq):
//		// Once an exception handler / assert system is implemented, use that to report the unfreed devices.
//	}
//	instance.m_devices.clear();
//}
}