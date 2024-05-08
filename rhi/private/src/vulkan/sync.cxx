module;

#include <mutex>

#include "vulkan/vk.h"
#include "lib/common.h"

module forge;

namespace frg
{
auto Semaphore::info() const -> SemaphoreInfo const&
{
	return m_info;
}

auto Semaphore::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto Semaphore::from(Device& device, SemaphoreInfo&& info) -> Resource<Semaphore>
{
	auto&& [index, semaphore] = device.m_gpuResourcePool.semaphores.emplace(device);

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkResult result = vkCreateSemaphore(device.m_apiContext.device, &semaphoreInfo, nullptr, &semaphore.m_impl.handle);

	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.semaphores.erase(index);

		return null_resource;
	}

	info.name.format("<semaphore>:{}", info.name.c_str());

	semaphore.m_info = std::move(info);
	semaphore.m_impl.type = VK_SEMAPHORE_TYPE_BINARY;

	return Resource{ index, semaphore };
}

auto Semaphore::destroy(Semaphore const& resource, id_type id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a refernece to itself and can be safely deleted.
	 */
	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<Semaphore> });
}

Semaphore::Semaphore(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{}
{}

auto Fence::info() const -> FenceInfo const&
{
	return m_info;
}

auto Fence::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto Fence::value() const -> uint64
{
	uint64 value = {};

	ASSERTION(m_device != nullptr && "No device referenced for current Fence.");
	ASSERTION(m_impl.handle != VK_NULL_HANDLE && "Attempting to retrieve value of an invalid Fence.");

	if (m_device != nullptr && 
		m_impl.handle != VK_NULL_HANDLE)
	{
		vkGetSemaphoreCounterValue(m_device->m_apiContext.device, m_impl.handle, &value);
	}
	return value;
}

auto Fence::signal(uint64 const value) const -> void
{
	if (m_device != nullptr &&
		m_impl.handle != VK_NULL_HANDLE)
	{
		uint64 currentValue = {};
		vkGetSemaphoreCounterValue(m_device->m_apiContext.device, m_impl.handle, &currentValue);
		currentValue = std::max(std::max(currentValue, value), currentValue + 1);

		VkSemaphoreSignalInfo signalInfo{
			.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore	= m_impl.handle,
			.value		= currentValue,
		};
		vkSignalSemaphore(m_device->m_apiContext.device, &signalInfo);
	}
}

auto Fence::signal() const -> void
{
	if (m_device != nullptr &&
		m_impl.handle != VK_NULL_HANDLE)
	{
		uint64 currentValue = {};
		vkGetSemaphoreCounterValue(m_device->m_apiContext.device, m_impl.handle, &currentValue);

		++currentValue;

		VkSemaphoreSignalInfo signalInfo{
			.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore	= m_impl.handle,
			.value		= currentValue,
		};
		vkSignalSemaphore(m_device->m_apiContext.device, &signalInfo);
	}
}

auto Fence::wait_for_value(uint64 value, uint64 timeout) const -> bool
{
	VkResult result = VK_SUCCESS;

	if (m_device != nullptr &&
		m_impl.handle != VK_NULL_HANDLE)
	{
		uint64 waitValue = value;
		VkSemaphoreWaitInfo waitInfo{
			.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores	= &m_impl.handle,
			.pValues		= &waitValue,
		};
		result = vkWaitSemaphores(m_device->m_apiContext.device, &waitInfo, timeout);
	}

	return result == VK_SUCCESS;
}

auto Fence::from(Device& device, FenceInfo&& info) -> Resource<Fence>
{
	auto&& [index, fence] = device.m_gpuResourcePool.fences.emplace(device);

	VkSemaphoreTypeCreateInfo timelineSemaphoreInfo{
		.sType			= VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType	= VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue	= info.initialValue
	};

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &timelineSemaphoreInfo
	};

	VkResult result = vkCreateSemaphore(device.m_apiContext.device, &semaphoreInfo, nullptr, &fence.m_impl.handle);

	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.fences.erase(index);

		return null_resource;
	}

	info.name.format("<fence>:{}", info.name.c_str());

	fence.m_info = std::move(info);
	fence.m_impl.type = VK_SEMAPHORE_TYPE_TIMELINE;

	return Resource{ index, fence };
}

auto Fence::destroy(Fence const& resource, id_type id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a refernece to itself and can be safely deleted.
	 */
	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<Fence> });
}

Fence::Fence(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{}
{}
}