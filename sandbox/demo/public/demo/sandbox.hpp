#include "core.platform/application.hpp"
#include "model_demo_app.hpp"
#include "graphics_processing_unit.hpp"

namespace sandbox
{
class SandboxApp final : public core::platform::Application
{
public:
	SandboxApp(int argc, char** argv);
	~SandboxApp() = default;

	NOCOPYANDMOVE(SandboxApp)

	auto run() -> void;
	auto stop() -> void;
private:
	core::Ref<core::platform::Window> m_rootWindow;
	std::unique_ptr<GraphicsProcessingUnit> m_gpu;
	ModelDemoApp m_applet;
	bool m_isRunning;

	auto terminate() -> void;
};
}