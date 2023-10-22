#include "sampler.h"
#include "vulkan/vk_device.h"

namespace rhi
{
Sampler::Sampler(
	SamplerInfo&& info,
	uint64 packedInfoUint64,
	APIContext* context,
	void* data,
	resource_type type_id
) :
	Resource{ context, data, type_id },
	m_info{ std::move(info) },
	m_packed_info{ packedInfoUint64 }
{
	m_context->setup_debug_name(*this);
}

Sampler::Sampler(Sampler&& rhs) noexcept
{
	*this = std::move(rhs);
}

Sampler& Sampler::operator=(Sampler&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		m_packed_info = rhs.m_packed_info;
		Resource::operator=(std::move(rhs));
		new (&rhs) Sampler{};
	}
	return *this;
}

auto Sampler::info() const -> SamplerInfo const&
{
	return m_info;
}

auto Sampler::info_packed() const -> uint64
{
	return m_packed_info;
}
}