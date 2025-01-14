#pragma once
#ifndef RENDER_ASYNC_DEVICE_HPP
#define RENDER_ASYNC_DEVICE_HPP

#include <expected>
#include "upload_heap.hpp"

namespace render
{
class AsyncDevice : lib::non_copyable_non_movable
{
public:
	auto device() const -> gpu::Device&;
	auto command_queue() const -> CommandQueue&;
	auto upload_heap() const -> UploadHeap&;

	static auto from(gpu::DeviceInitInfo const& deviceInfo) -> std::expected<std::unique_ptr<AsyncDevice>, std::string_view>;
private:
	std::unique_ptr<gpu::Device> m_device;
	std::unique_ptr<CommandQueue> m_commandQueue;
	std::unique_ptr<UploadHeap> m_uploadHeap;
};
}

#endif // !RENDER_ASYNC_DEVICE_HPP
