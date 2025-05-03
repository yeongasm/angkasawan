#include "sandbox.hpp"

auto main(int argc, char* argv[]) -> int
{
	sandbox::SandboxApp app{ argc, argv };
	app.run();
	app.stop();

	return 0;
}
