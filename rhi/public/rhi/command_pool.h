#pragma once
#ifndef RHI_COMMAND_POOL_H
#define RHI_COMMAND_POOL_H

#include "lib/map.h"
#include "command_buffer.h"
#include "resource.h"

namespace rhi
{
struct SubmitInfo
{
	DeviceQueue queue;
	std::span<CommandBuffer> commandBuffers;
	std::span<Semaphore const> waitSemaphores;
	std::span<Semaphore const> signalSemaphores;
	std::span<std::pair<Fence const*, uint64>> waitFences;
	std::span<std::pair<Fence const*, uint64>> signalFences;
};

struct PresentInfo
{
	std::span<Swapchain> swapchains;
};

struct APIContext;

class CommandPool : public Resource
{
public:
	RHI_API CommandPool() = default;
	RHI_API ~CommandPool() = default;

	RHI_API CommandPool(CommandPool&&) noexcept;
	RHI_API auto operator=(CommandPool&&) noexcept -> CommandPool&;

	RHI_API auto info() const -> CommandPoolInfo const&;
	RHI_API auto reset() -> void;
	RHI_API auto allocate_command_buffer(CommandBufferInfo&& info) -> CommandBuffer&;
	RHI_API auto free_command_buffer(CommandBuffer& commandBuffer) -> void;
	//RHI_API std::span<CommandBuffer> allocate_command_buffers(size_t count);
private:
	friend struct APIContext;
	static constexpr size_t INVALID_COMMAND_BUFFER_INDEX = std::numeric_limits<size_t>::max();

	CommandPoolInfo m_info;
	lib::map<size_t, CommandBuffer> m_command_buffers;

	CommandPool(
		CommandPoolInfo&& info,
		APIContext* context,
		void* data,
		resource_type typeId
	);
};
}

#endif // !RHI_COMMAND_POOL_H
