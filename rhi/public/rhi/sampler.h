#pragma once
#ifndef RHI_SAMPLER_H
#define RHI_SAMPLER_H

#include "resource.h"

namespace rhi
{
class Sampler : public Resource
{
public:
	Sampler() = default;
	~Sampler() = default;

	RHI_API Sampler(Sampler&& rhs) noexcept;
	RHI_API Sampler& operator=(Sampler&& rhs) noexcept;

	RHI_API auto info() const -> SamplerInfo const&;
	RHI_API auto info_packed() const -> uint64;
private:
	friend struct APIContext;
	SamplerInfo m_info;
	uint64 m_packed_info;

	Sampler(
		SamplerInfo&& info,
		uint64 packedInfoUint64,
		APIContext* context,
		void* data,
		resource_type type_id
	);
};
}

#endif // !RHI_SAMPLER_H
