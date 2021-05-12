#include "GameAppManager.h"
#include "Library/Memory/Memory.h"

GameManager::GameManager() :
	Game(nullptr), Dll(), Paused(false), HotReloadable(false)
{}

GameManager::~GameManager() {}

void GameManager::RegisterGameDll(FilePath GameFile, bool GameIsPaused, bool EnableHotReload)
{
	Paused = GameIsPaused;
	HotReloadable = EnableHotReload;

	DllModule::FixedString tempFilename;
	tempFilename.Format("Reload.%s", GameFile.Filename());

	Dll.Filename = GameFile.C_Str();
	Dll.TempFilename = tempFilename;

	Dll.LoadDllModule();
	Dll.OnDllLoad(this);
}

void GameManager::RunGame(float32 Timestep)
{
	// NOTE(Ygsm):
	// It works for now ... (I hope nothing goes wrong!!!)
	if (HotReloadable && Dll.WasDllUpdated())
	{
		TerminateGame();
		Dll.LoadDllModule();
		Dll.OnDllLoad(this);
		return;
	}

	float32 timestep = Timestep;
	if (Paused)
	{
		timestep = 0.0f;
	}
	Game->Run(timestep);
}

void GameManager::TerminateGame()
{
	if (!Game)
	{
		return;
	}
	Game->Terminate();
	IMemory::Free(Game);
	Game = nullptr;
	Dll.UnloadDllModule();
}

void GameManager::PauseGame(bool Paused)
{
	this->Paused = Paused;
}