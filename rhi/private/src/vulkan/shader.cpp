#include "vulkan/vk_device.h"
#include "shader.h"

namespace rhi
{

Shader::Shader(
	ShaderInfo&& info,
	APIContext* context,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) },
	m_attributes{}
{
	m_context->setup_debug_name(*this);
}

Shader::Shader(Shader&& rhs) noexcept
{
	*this = std::move(rhs);
}

auto Shader::operator=(Shader&& rhs) noexcept -> Shader&
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		m_attributes = std::move(rhs.m_attributes);
		Resource::operator=(std::move(rhs));
		new (&rhs) Shader{};
	}
	return *this;
}

auto Shader::info() const -> ShaderInfo const&
{
	return m_info;
}

auto Shader::attributes() const -> std::span<ShaderAttribute const>
{
	return std::span{ m_attributes.data(), m_attributes.size() };
}

}