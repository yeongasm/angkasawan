#include "core.platform/application.hpp"
#include "model_demo_app.hpp"

namespace sandbox
{
class SandboxApp final : public core::platform::Application, lib::non_copyable_non_movable
{
public:
	SandboxApp(int32 argc, char** argv);
	~SandboxApp() = default;

	auto run() -> void;
	auto stop() -> void;
private:
	core::Ref<core::platform::Window> m_rootWindow;
	std::unique_ptr<render::AsyncDevice> m_gpu;
	ModelDemoApp m_applet;
	bool m_isRunning;

	auto terminate() -> void;
};
}