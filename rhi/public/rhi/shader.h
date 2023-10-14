#pragma once
#ifndef RHI_SHADER_H
#define RHI_SHADER_H

#include "resource.h"

namespace rhi
{
class Shader : public Resource
{
public:
	RHI_API Shader() = default;
	RHI_API ~Shader() = default;

	RHI_API Shader(Shader&& rhs) noexcept;
	RHI_API auto operator=(Shader&& rhs) noexcept -> Shader&;

	RHI_API auto info() const -> ShaderInfo const&;
	RHI_API auto attributes() const -> std::span<ShaderAttribute const>;
private:
	friend struct APIContext;

	ShaderInfo m_info;
	lib::array<ShaderAttribute> m_attributes;

	Shader(
		ShaderInfo&& info,
		APIContext* context,
		void* data,
		resource_type typeId
	);
};
}

#endif // !RHI_SHADER_H
