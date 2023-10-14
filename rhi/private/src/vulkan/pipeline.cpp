#include "pipeline.h"
#include "vulkan/vk_device.h"

namespace rhi
{
Pipeline::Pipeline(
	PipelineType type,
	APIContext* context,
	void* data,
	resource_type type_id
) :
	Resource{ context, data, type_id },
	m_type{ type }
{}

Pipeline::Pipeline(Pipeline&& rhs) noexcept
{
	*this = std::move(rhs);
}

Pipeline& Pipeline::operator=(Pipeline&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_type = rhs.m_type;
		Resource::operator=(std::move(rhs));
		new (&rhs) Pipeline{};
	}
	return *this;
}

auto Pipeline::type() const -> PipelineType
{
	return m_type;
}

RasterPipeline::RasterPipeline(
	RasterPipelineInfo&& info,
	APIContext* context,
	void* data,
	resource_type type_id
) :
	Pipeline{ PipelineType::Rasterization, context, data, type_id },
	m_info{ std::move(info) }
{
	m_context->setup_debug_name(*this);
}

RasterPipeline::RasterPipeline(RasterPipeline&& rhs) noexcept
{
	*this = std::move(rhs);
}

RasterPipeline& RasterPipeline::operator=(RasterPipeline&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		Pipeline::operator=(std::move(rhs));
		new (&rhs) RasterPipeline{};
	}
	return *this;
}

auto RasterPipeline::info() const -> RasterPipelineInfo const&
{
	return m_info;
}
}