#include "sandbox_api.h"
#include "core/engine.h"
#include "rhi/rhi.h"
#include "rhi/util/shader_compiler.h"

namespace sandbox
{

class SandboxApp final : public core::Application
{
public:
	SANDBOX_API SandboxApp();
	SANDBOX_API virtual ~SandboxApp() override;

	SANDBOX_API virtual bool initialize()	override;
	SANDBOX_API virtual void run()			override;
	SANDBOX_API virtual void terminate()	override;
private:
	core::wnd::window_handle m_root_app_window;
	rhi::util::ShaderCompiler m_shader_compiler;
	lib::array<std::unique_ptr<core::Application>> m_demo_applications;
	lib::ref<core::Application> m_showcased_demo;
	rhi::Instance m_instance;
	rhi::Device* m_device;
	rhi::Swapchain m_swapchain;
	size_t m_frame_index;
};

}