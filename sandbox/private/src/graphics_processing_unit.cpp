#include "graphics_processing_unit.hpp"

namespace sandbox
{
auto GraphicsProcessingUnit::device() const -> gpu::Device&
{
	return *m_device;
}

auto GraphicsProcessingUnit::command_queue() const -> CommandQueue&
{
	return *m_commandQueue;
}

auto GraphicsProcessingUnit::upload_heap() const -> UploadHeap&
{
	return *m_uploadHeap;
}

auto GraphicsProcessingUnit::from(gpu::DeviceInitInfo const& deviceInfo) -> std::optional<std::unique_ptr<GraphicsProcessingUnit>>
{
	if (auto result = gpu::Device::from(deviceInfo); result.has_value())
	{
		auto gpu = std::make_unique<GraphicsProcessingUnit>();
		gpu->m_device = std::move(*result);

		gpu->m_commandQueue = std::make_unique<CommandQueue>(*gpu->m_device);
		gpu->m_uploadHeap	= std::make_unique<UploadHeap>(*gpu->m_device, *gpu->m_commandQueue);

		return std::make_optional(std::move(gpu));
	}

	return std::nullopt;
}
}