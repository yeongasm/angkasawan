#pragma once
#ifndef RHI_PIPELINE_H
#define RHI_PIPELINE_H

#include "shader.h"

namespace rhi
{
/**
* TODO(afiq):
* 
* [ ] - Add support for tesselation control shader, tesselation evaluation shader, geometry shader and mesh shaders.
*/

class Pipeline : public Resource
{
public:
	RHI_API Pipeline() = default;
	RHI_API ~Pipeline() = default;

	RHI_API Pipeline(Pipeline&& rhs) noexcept;
	RHI_API Pipeline& operator=(Pipeline&& rhs) noexcept;

	RHI_API auto type() const -> PipelineType;
protected:
	friend struct APIContext;

	PipelineType m_type;

	Pipeline(
		PipelineType type,
		APIContext* context,
		void* data,
		resource_type type_id
	);
};

class RasterPipeline final : public Pipeline
{
public:
	RHI_API RasterPipeline() = default;
	RHI_API ~RasterPipeline() = default;

	RHI_API RasterPipeline(RasterPipeline&& rhs) noexcept;
	RHI_API RasterPipeline& operator=(RasterPipeline&& rhs) noexcept;

	RHI_API auto info() const -> RasterPipelineInfo const&;
private:
	friend struct APIContext;
	friend struct ResourceDeleter<RasterPipeline>;

	RasterPipelineInfo m_info;

	RasterPipeline(
		RasterPipelineInfo&& info,
		APIContext* context,
		void* data,
		resource_type type_id
	);
};
}

#endif // !RHI_PIPELINE_H
