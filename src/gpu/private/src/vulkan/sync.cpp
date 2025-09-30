#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Semaphore::info() const -> SemaphoreInfo const&
{
	return __self().info;
}

auto Semaphore::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto Semaphore::from(Device& device, SemaphoreInfo&& info) -> Semaphore
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);
		
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkSemaphore semaphore{};

	CHECK_OP(vkCreateSemaphore(vkdevice.device, &semaphoreInfo, nullptr, &semaphore))

	auto&& vksemaphore = *vkdevice.gpuResourcePool.stores.semaphores.emplace();

	vksemaphore.handle = semaphore;
	vksemaphore.info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{         
		vkdevice.setup_debug_name(vksemaphore);
	}

	return Semaphore{ &vksemaphore, &vkdevice };
}

auto Fence::info() const -> FenceInfo const&
{
	return __self().info;
}

auto Fence::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto Fence::value() const -> uint64
{
	auto&& device = __device();
	auto&& self = __self();
	
	uint64 value = {};

	ASSERTION(m_device != nullptr && "No device referenced for current Fence.");
	ASSERTION(self.handle != VK_NULL_HANDLE && "Attempting to retrieve value of an invalid Fence.");

	if (valid())
	{
		vkGetSemaphoreCounterValue(device.device, self.handle, &value);
	}

	return value;
}

auto Fence::signal(uint64 const value) const -> void
{
	if (valid())
	{
		auto&& device = __device();
		auto&& self = __self();

		uint64 currentValue = {};

		vkGetSemaphoreCounterValue(device.device, self.handle, &currentValue);
		currentValue = std::max(std::max(currentValue, value), currentValue + 1);

		VkSemaphoreSignalInfo signalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = self.handle,
			.value = currentValue,
		};
		vkSignalSemaphore(device.device, &signalInfo);
	}
}

auto Fence::signal() const -> void
{
	if (valid())
	{
		auto&& device = __device();
		auto&& self = __self();

		uint64 currentValue = {};
		vkGetSemaphoreCounterValue(device.device, self.handle, &currentValue);

		++currentValue;

		VkSemaphoreSignalInfo signalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = self.handle,
			.value = currentValue,
		};
		vkSignalSemaphore(device.device, &signalInfo);
	}
}

auto Fence::wait_for_value(uint64 value, uint64 timeout) const -> bool
{
	VkResult result = VK_SUCCESS;

	if (valid())
	{
		auto&& device = __device();
		auto&& self = __self();

		uint64 waitValue = value;
		VkSemaphoreWaitInfo waitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &self.handle,
			.pValues = &waitValue,
		};
		result = vkWaitSemaphores(device.device, &waitInfo, timeout);
	}

	return result == VK_SUCCESS;
}

auto Fence::from(Device& device, FenceInfo&& info) -> Fence
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	VkSemaphoreTypeCreateInfo timelineSemaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = info.initialValue
	};

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &timelineSemaphoreInfo
	};

	VkSemaphore timelineSemaphore = VK_NULL_HANDLE;

	CHECK_OP(vkCreateSemaphore(vkdevice.device, &semaphoreInfo, nullptr, &timelineSemaphore))

	auto&& vkfence = *vkdevice.gpuResourcePool.stores.fences.emplace();

	vkfence.handle = timelineSemaphore;
	vkfence.info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkfence);
	}

	return Fence{ &vkfence, &vkdevice };
}

auto Event::info() const -> EventInfo const&
{
	return __self().info;
}

auto Event::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto Event::from(Device& device, EventInfo&& info) -> Event
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	VkEventCreateInfo eventInfo{
		.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO
	};

	VkEvent event = VK_NULL_HANDLE;

	CHECK_OP(vkCreateEvent(vkdevice.device, &eventInfo, nullptr, &event))

	auto&& vkevent = *vkdevice.gpuResourcePool.stores.events.emplace();

	vkevent.handle = event;
	vkevent.info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkevent);
	}

	return Event{ &vkevent, &vkdevice };
}

auto Semaphore::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	DeviceImpl& device = static_cast<DeviceImpl&>(dvc);
	SemaphoreImpl& semaphore = static_cast<SemaphoreImpl&>(resource);

	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&semaphore](DeviceImpl& device) -> void
		{
			vkDestroySemaphore(device.device, semaphore.handle, nullptr);

			auto it = device.gpuResourcePool.stores.semaphores.get_iterator(&semaphore);
			device.gpuResourcePool.stores.semaphores.erase(it);
		}
	);
}

auto Fence::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	DeviceImpl& device = static_cast<DeviceImpl&>(dvc);
	FenceImpl& fence = static_cast<FenceImpl&>(resource);

	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&fence](DeviceImpl& device) -> void
		{
			vkDestroySemaphore(device.device, fence.handle, nullptr);

			auto it = device.gpuResourcePool.stores.fences.get_iterator(&fence);
			device.gpuResourcePool.stores.fences.erase(it);
		}
	);
}

auto Event::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	DeviceImpl& device = static_cast<DeviceImpl&>(dvc);
	EventImpl& event = static_cast<EventImpl&>(resource);

	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&event] (DeviceImpl& device) -> void
		{
			vkDestroyEvent(device.device, event.handle, nullptr);

			auto it = device.gpuResourcePool.stores.events.get_iterator(&event);
			device.gpuResourcePool.stores.events.erase(it);
		}
	);
}
}