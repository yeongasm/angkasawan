#pragma once
#ifndef GRAPHICS_PROCESSING_UNIT_H
#define GRAPHICS_PROCESSING_UNIT_H

#include <expected>
#include "upload_heap.hpp"

namespace sandbox
{
class GraphicsProcessingUnit
{
public:
	GraphicsProcessingUnit() = default;
	~GraphicsProcessingUnit() = default;

	NOCOPYANDMOVE(GraphicsProcessingUnit)

	auto device() const -> gpu::Device&;
	auto command_queue() const -> CommandQueue&;
	auto upload_heap() const -> UploadHeap&;

	static auto from(gpu::DeviceInitInfo const& deviceInfo) -> std::expected<std::unique_ptr<GraphicsProcessingUnit>, std::string_view>;
private:
	std::unique_ptr<gpu::Device> m_device;
	std::unique_ptr<CommandQueue> m_commandQueue;
	std::unique_ptr<UploadHeap> m_uploadHeap;
};
}

#endif // !GRAPHICS_PROCESSING_UNIT_H