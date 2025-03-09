#include "vulkan/vkgpu.hpp"

namespace gpu
{
Semaphore::Semaphore(Device& device) :
	DeviceResource{ device },
	m_info{}
{}

auto Semaphore::info() const -> SemaphoreInfo const&
{
	return m_info;
}

auto Semaphore::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto Semaphore::from(Device& device, SemaphoreInfo&& info) -> Resource<Semaphore>
{
	auto&& vkdevice = to_device(device);
		
	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkSemaphore handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateSemaphore(vkdevice.device, &semaphoreInfo, nullptr, &handle))

	auto it = vkdevice.gpuResourcePool.binarySemaphore.emplace(vkdevice);

	auto&& vksemaphore = *it;

	if (info.name.size())
	{
		info.name.format("<semaphore>:{}", info.name.c_str());
	}

	vksemaphore.handle = handle;
	vksemaphore.m_info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vksemaphore);
	}

	return Resource<Semaphore>{ vksemaphore };
}

auto Semaphore::destroy(Semaphore& resource) -> void
{
	auto&& vkdevice = to_device(resource.m_device);
	auto&& vksemaphore = to_impl(resource);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, &vksemaphore, vk::ResourceType::Semaphore);
}

Fence::Fence(Device& device) :
	DeviceResource{ device },
	m_info{}
{}

auto Fence::info() const -> FenceInfo const&
{
	return m_info;
}

auto Fence::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto Fence::value() const -> uint64
{
	auto const& self = to_impl(*this);
	auto const& vkdevice = to_device(m_device);

	uint64 value = {};

	ASSERTION(m_device != nullptr && "No device referenced for current Fence.");
	ASSERTION(self.handle != VK_NULL_HANDLE && "Attempting to retrieve value of an invalid Fence.");

	if (valid())
	{
		vkGetSemaphoreCounterValue(vkdevice.device, self.handle, &value);
	}

	return value;
}

auto Fence::signal(fence_value_t const value) const -> void
{
	if (valid())
	{
		auto const& self = to_impl(*this);
		auto const& vkdevice = to_device(m_device);

		uint64 currentValue = {};

		vkGetSemaphoreCounterValue(vkdevice.device, self.handle, &currentValue);
		currentValue = std::max(std::max(currentValue, value), currentValue + 1);

		VkSemaphoreSignalInfo signalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = self.handle,
			.value = currentValue,
		};
		vkSignalSemaphore(vkdevice.device, &signalInfo);
	}
}

auto Fence::signal() const -> void
{
	if (valid())
	{
		auto const& self = to_impl(*this);
		auto const& vkdevice = to_device(m_device);

		uint64 currentValue = {};
		vkGetSemaphoreCounterValue(vkdevice.device, self.handle, &currentValue);

		++currentValue;

		VkSemaphoreSignalInfo signalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = self.handle,
			.value = currentValue,
		};
		vkSignalSemaphore(vkdevice.device, &signalInfo);
	}
}

auto Fence::wait_for_value(fence_value_t value, uint64 timeout) const -> bool
{
	VkResult result = VK_SUCCESS;

	if (valid())
	{
		auto const& self = to_impl(*this);
		auto const& vkdevice = to_device(m_device);

		uint64 waitValue = value;
		VkSemaphoreWaitInfo waitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &self.handle,
			.pValues = &waitValue,
		};
		result = vkWaitSemaphores(vkdevice.device, &waitInfo, timeout);
	}

	return result == VK_SUCCESS;
}

auto Fence::from(Device& device, FenceInfo&& info) -> Resource<Fence>
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

	auto it = vkdevice.gpuResourcePool.timelineSemaphore.emplace(vkdevice);

	auto&& vksemaphore = *it;

	if (info.name.size())
	{
		info.name.format("<fence>:{}", info.name.c_str());
	}

	vksemaphore.m_info = std::move(info);
	vksemaphore.handle = handle;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vksemaphore);
	}

	return Resource<Fence>{ vksemaphore };
}

auto Fence::destroy(Fence const& resource) -> void
{
	auto&& vkdevice = to_device(resource.m_device);
	auto&& vksemaphore = to_impl(resource);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, &vksemaphore, vk::ResourceType::Fence);
}

Event::Event(Device& device) :
	DeviceResource{ device },
	m_info{}
{}

auto Event::info() const -> EventInfo const&
{
	return m_info;
}

auto Event::valid() const -> bool
{
	auto const& vkevent = to_impl(*this);

	return vkevent.handle != VK_NULL_HANDLE;
}

auto Event::state() const -> EventState
{
	auto const& vkevent = to_impl(*this);
	auto const& vkdevice = to_device(m_device);

	if (vkGetEventStatus(vkdevice.device, vkevent.handle) == VK_EVENT_SET)
	{
		return EventState::Signaled;
	}

	return EventState::Unsignaled;
}

auto Event::signal() const -> void
{
	auto const& vkevent = to_impl(*this);
	auto const& vkdevice = to_device(m_device);

	vkSetEvent(vkdevice.device, vkevent.handle);
}

auto Event::reset() const -> void
{
	auto&& vkevent = to_impl(*this);
	auto&& vkdevice = to_device(m_device);

	vkResetEvent(vkdevice.device, vkevent.handle);
}

auto Event::from(Device& device, EventInfo&& info) -> Resource<Event>
{
	auto&& vkdevice = to_device(device);

	VkEventCreateInfo eventInfo{
		.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO
	};

	VkEvent handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateEvent(vkdevice.device, &eventInfo, nullptr, &handle))

	auto it = vkdevice.gpuResourcePool.events.emplace(vkdevice);

	auto&& vkevent = *it;

	if (info.name.size())
	{
		info.name.format("<event>:{}", info.name.c_str());
	}

	vkevent.handle = handle;
	vkevent.m_info = std::move(info);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkevent);
	}

	return Resource<Event>{ vkevent };
}

auto Event::destroy(Event const& resource) -> void
{
	auto&& vkdevice = to_device(resource.m_device);
	auto&& vkevent = to_impl(resource);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, &vkevent, vk::ResourceType::Event);
}

namespace vk
{
SemaphoreImpl::SemaphoreImpl(DeviceImpl& device) :
	Semaphore{ device }
{}

FenceImpl::FenceImpl(DeviceImpl& device) :
	Fence{ device }
{}

EventImpl::EventImpl(DeviceImpl& device) :
	Event{ device }
{}
}
}