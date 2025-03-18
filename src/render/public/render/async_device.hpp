#pragma once
#ifndef RENDER_ASYNC_DEVICE_HPP
#define RENDER_ASYNC_DEVICE_HPP

#include <expected>
#include "upload_heap.hpp"
#include "pipeline_cache.hpp"

namespace render
{
struct AsyncDeviceInitInfo
{
	gpu::DeviceInitInfo deviceInfo;
	std::filesystem::path pipelineCachePath;
};

class AsyncDevice : lib::non_copyable_non_movable
{
public:
	auto device() const -> gpu::Device&;
	auto command_queue() const -> CommandQueue&;
	auto upload_heap() const -> UploadHeap&;
	auto pipeline_cache() const -> PipelineCache&;

	static auto from(AsyncDeviceInitInfo const& info) -> std::expected<std::unique_ptr<AsyncDevice>, std::string_view>;
private:
	std::unique_ptr<gpu::Device> m_device;
	std::unique_ptr<CommandQueue> m_commandQueue;
	std::unique_ptr<UploadHeap> m_uploadHeap;
	std::unique_ptr<PipelineCache> m_pipelineCache;
};
}

#endif // !RENDER_ASYNC_DEVICE_HPP
