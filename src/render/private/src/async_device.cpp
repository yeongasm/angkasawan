#include "async_device.hpp"

namespace render
{
auto AsyncDevice::device() const -> gpu::Device&
{
	return *m_device;
}

auto AsyncDevice::command_queue() const -> CommandQueue&
{
	return *m_commandQueue;
}

auto AsyncDevice::upload_heap() const -> UploadHeap&
{
	return *m_uploadHeap;
}

auto AsyncDevice::pipeline_cache() const -> PipelineCache&
{
	return *m_pipelineCache;
}

auto AsyncDevice::from(AsyncDeviceInitInfo const& info) -> std::expected<std::unique_ptr<AsyncDevice>, std::string_view>
{
	if (auto result = gpu::Device::from(info.deviceInfo); result)
	{
		auto gpu = std::make_unique<AsyncDevice>();
		gpu->m_device = std::move(*result);

		gpu->m_commandQueue = std::make_unique<CommandQueue>(*gpu->m_device);
		gpu->m_uploadHeap = std::make_unique<UploadHeap>(*gpu->m_device, *gpu->m_commandQueue);
		gpu->m_pipelineCache = PipelineCache::create(gpu->device(), { .cachePath = info.pipelineCachePath });

		return gpu;
	}

	return std::unexpected{ "Failed to create GPU device interface" };
}
}