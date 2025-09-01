#include "mimalloc-new-delete.h"
#include "sandbox.hpp"

auto main(int argc, char* argv[]) -> int
{
	mi_version();

	sandbox::SandboxApp app{ argc, argv };
	app.run();
	app.stop();

	return 0;
}
