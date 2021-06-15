#include "Sandbox.h"
#include "SandboxApp/SandboxApp.h"
#include "SubSystem/Game/GameAppManager.h"

extern "C"
{
	void OnDllLoad(void* Param)
	{
		// TODO(Ygsm):
		// Tell the engine to terminate when the game fails to load.
		GameManager* gameCoordinator = reinterpret_cast<GameManager*>(Param);
		if (gameCoordinator->InitializeGame<sandbox::SandboxApp>())
		{
			return;
		}
	}

	void OnDllUnload(void* Param) {}
}