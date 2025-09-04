#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Semaphore::info() const -> SemaphoreInfo const&
{
	return m_info;
}

auto Semaphore::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
}

auto Semaphore::from(Device& device, SemaphoreInfo&& info) -> handle_type
{
	auto&& vkdevice = to_device(device);
		
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkSemaphore handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateSemaphore(vkdevice.device, &semaphoreInfo, nullptr, &handle))

	auto it = vkdevice.gpuResourcePool.stores.binarySemaphore.emplace();

	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::SemaphoreImpl>,
		.id 	= ++vkdevice.gpuResourcePool.idCounter.others
	};

	auto id = std::bit_cast<uint64>(meta);

	vkdevice.gpuResourcePool.caches.binarySemaphore.emplace(id, it);
	vkdevice.begin_referencing(id);

	auto&& vksemaphore = *it;

	vksemaphore.handle = handle;
	vksemaphore.m_info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{         
		vkdevice.setup_debug_name(vksemaphore);
	}

	return handle_type{ vkdevice, id };
}

auto Semaphore::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a Semaphore with an invalid id.");

	auto&& vkdevice = to_device(device);
	auto&& vksemaphore = *vkdevice.gpuResourcePool.caches.binarySemaphore[id];

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vksemaphore, id](vk::DeviceImpl& device) -> void
		{
			vkDestroySemaphore(device.device, vksemaphore.handle, nullptr);

			auto it = device.gpuResourcePool.caches.binarySemaphore[id];

			device.gpuResourcePool.caches.binarySemaphore.erase(id);
			device.gpuResourcePool.stores.binarySemaphore.erase(it);
		}
	);
}

auto Fence::info() const -> FenceInfo const&
{
	return m_info;
}

auto Fence::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
}

auto Fence::value() const -> uint64
{
	auto const& self = to_impl(*this);
	
	uint64 value = {};

	ASSERTION(self.vkdevice != nullptr && "No device referenced for current Fence.");
	ASSERTION(self.handle != VK_NULL_HANDLE && "Attempting to retrieve value of an invalid Fence.");

	if (valid())
	{
		vkGetSemaphoreCounterValue(self.vkdevice->device, self.handle, &value);
	}

	return value;
}

auto Fence::signal(uint64 const value) const -> void
{
	if (valid())
	{
		auto const& self = to_impl(*this);

		uint64 currentValue = {};

		vkGetSemaphoreCounterValue(self.vkdevice->device, self.handle, &currentValue);
		currentValue = std::max(std::max(currentValue, value), currentValue + 1);

		VkSemaphoreSignalInfo signalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = self.handle,
			.value = currentValue,
		};
		vkSignalSemaphore(self.vkdevice->device, &signalInfo);
	}
}

auto Fence::signal() const -> void
{
	if (valid())
	{
		auto const& self = to_impl(*this);

		uint64 currentValue = {};
		vkGetSemaphoreCounterValue(self.vkdevice->device, self.handle, &currentValue);

		++currentValue;

		VkSemaphoreSignalInfo signalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = self.handle,
			.value = currentValue,
		};
		vkSignalSemaphore(self.vkdevice->device, &signalInfo);
	}
}

auto Fence::wait_for_value(uint64 value, uint64 timeout) const -> bool
{
	VkResult result = VK_SUCCESS;

	if (valid())
	{
		auto const& self = to_impl(*this);

		uint64 waitValue = value;
		VkSemaphoreWaitInfo waitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &self.handle,
			.pValues = &waitValue,
		};
		result = vkWaitSemaphores(self.vkdevice->device, &waitInfo, timeout);
	}

	return result == VK_SUCCESS;
}

auto Fence::from(Device& device, FenceInfo&& info) -> handle_type
{
	auto&& vkdevice = to_device(device);

	VkSemaphoreTypeCreateInfo timelineSemaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = info.initialValue
	};

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &timelineSemaphoreInfo
	};

	VkSemaphore handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateSemaphore(vkdevice.device, &semaphoreInfo, nullptr, &handle))

	auto it = vkdevice.gpuResourcePool.stores.timelineSemaphore.emplace(vkdevice);

	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::FenceImpl>,
		.id 	= ++vkdevice.gpuResourcePool.idCounter.others
	};

	auto id = std::bit_cast<uint64>(meta);

	vkdevice.gpuResourcePool.caches.timelineSemaphore.emplace(id, it);
	vkdevice.begin_referencing(id);

	auto&& vksemaphore = *it;

	vksemaphore.m_info = std::move(info);
	vksemaphore.handle = handle;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vksemaphore);
	}

	return handle_type{ vkdevice, id };
}

auto Fence::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a Fence with an invalid id.");

	auto&& vkdevice = to_device(device);
	auto&& vksemaphore = *vkdevice.gpuResourcePool.caches.timelineSemaphore[id];

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vksemaphore, id](vk::DeviceImpl& device) -> void
		{
			vkDestroySemaphore(device.device, vksemaphore.handle, nullptr);

			auto it = device.gpuResourcePool.caches.timelineSemaphore[id];

			device.gpuResourcePool.caches.timelineSemaphore.erase(id);
			device.gpuResourcePool.stores.timelineSemaphore.erase(it);
		}
	);
}

auto Event::info() const -> EventInfo const&
{
	return m_info;
}

auto Event::valid() const -> bool
{
	auto const& vkevent = to_impl(*this);

	return vkevent.handle != VK_NULL_HANDLE;
}

auto Event::from(Device& device, EventInfo&& info) -> handle_type
{
	auto&& vkdevice = to_device(device);

	VkEventCreateInfo eventInfo{
		.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO
	};

	VkEvent handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateEvent(vkdevice.device, &eventInfo, nullptr, &handle))

	auto it = vkdevice.gpuResourcePool.stores.events.emplace();

	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::EventImpl>,
		.id 	= ++vkdevice.gpuResourcePool.idCounter.others
	};

	auto id = std::bit_cast<uint64>(meta);

	vkdevice.gpuResourcePool.caches.event.emplace(id, it);
	vkdevice.begin_referencing(id);

	auto&& vkevent = *it;

	vkevent.handle = handle;
	vkevent.m_info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkevent);
	}

	return handle_type{ vkdevice, id };
}

auto Event::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy an Event with an invalid id.");

	auto&& vkdevice = to_device(device);
	auto&& vkevent = *vkdevice.gpuResourcePool.caches.event[id];

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkevent, id] (vk::DeviceImpl& device) -> void
		{
			vkDestroyEvent(device.device, vkevent.handle, nullptr);

			auto it = device.gpuResourcePool.caches.event[id];

			device.gpuResourcePool.caches.event.erase(id);
			device.gpuResourcePool.stores.events.erase(it);
		}
	);
}

namespace vk
{
FenceImpl::FenceImpl(DeviceImpl& device) :
	vkdevice{ &device }
{}
}
}