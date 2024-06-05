#pragma once
#ifndef SANDBOX_DEMO_APPLICATION_H
#define SANDBOX_DEMO_APPLICATION_H

#include "core/engine.h"
#include "gpu/gpu.h"
#include "gpu/util/shader_compiler.h"

namespace sandbox
{
class DemoApplication : public core::Application
{
public:
	DemoApplication() = default;

	DemoApplication(
		core::wnd::window_handle rootWindowHandle,
		gpu::util::ShaderCompiler& shaderCompiler,
		gpu::Device& device, 
		gpu::swapchain rootWindowSwapchain,
		size_t const& frameIndex 
	) :
		m_root_app_window{ rootWindowHandle },
		m_shader_compiler{ shaderCompiler },
		m_device{ device },
		m_swapchain{ rootWindowSwapchain },
		m_frame_index{ frameIndex }
	{};

	virtual ~DemoApplication() override = default;

	virtual bool initialize() = 0;
	virtual void run() = 0;
	virtual void terminate() = 0;
protected:
	core::wnd::window_handle m_root_app_window;
	gpu::util::ShaderCompiler& m_shader_compiler;
	gpu::Device& m_device;
	gpu::swapchain m_swapchain;
	size_t const& m_frame_index;
};

inline auto open_file(std::filesystem::path path) -> lib::string
{
	lib::string result = {};
	std::ifstream stream{ path, std::ios_base::in | std::ios_base::binary | std::ios_base::ate};

	if (stream.good() && 
		stream.is_open())
	{
		uint32 size = static_cast<uint32>(stream.tellg());
		stream.seekg(0, std::ios::beg);

		result.reserve(size + 1);

		stream.read(result.data(), size);
		result[size] = lib::null_terminator<lib::string::value_type>::val;

		stream.close();
	}

	return result;
}

}

#endif // !SANDBOX_DEMO_APPLICATION_H
