#pragma once
#ifndef RHI_SEMAPHORE_H
#define RHI_SEMAPHORE_H

#include "resource.h"

namespace rhi
{
class Semaphore : public Resource
{
public:
	RHI_API Semaphore() = default;
	RHI_API ~Semaphore() = default;

	RHI_API Semaphore(Semaphore&&) noexcept;
	RHI_API auto operator=(Semaphore&&) noexcept -> Semaphore&;

	RHI_API auto info() const->SemaphoreInfo const&;
private:
	friend struct APIContext;
	friend class Swapchain;

	SemaphoreInfo m_info;

	Semaphore(
		SemaphoreInfo&& info,
		APIContext* context,
		void* data,
		resource_type typeId
	);
};

class Fence : public Resource
{
public:
	RHI_API Fence() = default;
	RHI_API ~Fence() = default;

	RHI_API Fence(Fence&&) noexcept;
	RHI_API auto operator=(Fence&&) noexcept -> Fence&;

	RHI_API auto info() const -> FenceInfo const&;
	RHI_API auto value() const -> uint64;
	RHI_API auto signal(uint64 val) -> void;
	RHI_API auto signal() -> void;
	RHI_API auto wait_for_value(uint64 val, uint64 timeout = UINT64_MAX) const -> bool;
private:
	friend struct APIContext;
	friend class Swapchain;

	FenceInfo m_info;

	Fence(
		FenceInfo&& info,
		APIContext* context,
		void* data,
		resource_type typeId
	);
};
}

#endif // !RHI_SEMAPHORE_H
