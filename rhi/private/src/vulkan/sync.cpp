#include "sync.h"
#include "vulkan/vk_device.h"

namespace rhi
{
Semaphore::Semaphore(
	SemaphoreInfo&& info,
	APIContext* context,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) }
{
	m_context->setup_debug_name(*this);
}

Semaphore::Semaphore(Semaphore&& rhs) noexcept
{
	*this = std::move(rhs);
}

auto Semaphore::operator=(Semaphore&& rhs) noexcept -> Semaphore&
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		Resource::operator=(std::move(rhs));
		new (&rhs) Semaphore{};
	}
	return *this;
}

auto Semaphore::info() const -> SemaphoreInfo const&
{
	return m_info;
}

Fence::Fence(
	FenceInfo&& info,
	APIContext* context,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) }
{
	m_context->setup_debug_name(*this);
}

Fence::Fence(Fence&& rhs) noexcept
{
	*this = std::move(rhs);
}

auto Fence::operator=(Fence&& rhs) noexcept -> Fence&
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		Resource::operator=(std::move(rhs));
		new (&rhs) Fence{};
	}
	return *this;
}

auto Fence::info() const -> FenceInfo const&
{
	return m_info;
}

auto Fence::value() const -> uint64
{
	vulkan::Semaphore& semaphore = as<vulkan::Semaphore>();
	uint64 value = 0;
	vkGetSemaphoreCounterValue(m_context->device, semaphore.handle, &value);
	//semaphore.value = value;
	return value;
}

auto Fence::signal(uint64 val) -> void
{
	if (valid())
	{
		vulkan::Semaphore& semaphore = as<vulkan::Semaphore>();
		uint64 signalValue = value();
		signalValue = std::max(std::max(signalValue, val), signalValue + 1);
		VkSemaphoreSignalInfo semaphoreSignalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = semaphore.handle,
			.value = signalValue,
		};
		vkSignalSemaphore(m_context->device, &semaphoreSignalInfo);
	}
}

auto Fence::signal() -> void
{
	if (valid())
	{
		vulkan::Semaphore& semaphore = as<vulkan::Semaphore>();
		uint64 signalValue = value() + 1ull;
		VkSemaphoreSignalInfo semaphoreSignalInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = semaphore.handle,
			.value = signalValue,
		};
		vkSignalSemaphore(m_context->device, &semaphoreSignalInfo);
	}
}

auto Fence::wait_for_value(uint64 val, uint64 timeout) const -> bool
{
	VkResult result = VK_SUCCESS;
	if (valid())
	{
		vulkan::Semaphore& semaphore = as<vulkan::Semaphore>();
		uint64 value = val;
		VkSemaphoreWaitInfo semaphoreWaitInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &semaphore.handle,
			.pValues = &value,
		};
		result = vkWaitSemaphores(m_context->device, &semaphoreWaitInfo, timeout);
	}
	return result != VK_TIMEOUT;
}

}