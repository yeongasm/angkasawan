#pragma once
#ifndef LEARNVK_CORE_SUBSYSTEM_GAME_GAME_APP_MANAGER
#define LEARNVK_CORE_SUBSYSTEM_GAME_GAME_APP_MANAGER

#include "GameApp.h"
#include "Library/Containers/Path.h"

class ENGINE_API GameManager
{
private:

	GameAppBase*	Game;
	DllModule		Dll;
	bool			Paused;
	bool			HotReloadable;

public:

	GameManager();
	~GameManager();

	DELETE_COPY_AND_MOVE(GameManager)

	void RegisterGameDll(FilePath GameFile, bool GameIsPaused, bool EnableHotReload);

	// NOTE:
	// To be called from within the game.
	// Not too sure about this approach.
	template <typename GameClass> 
	bool InitializeGame();
	
	void RunGame(float32 Timestep);
	void TerminateGame();

	void PauseGame(bool Paused);
};

#include "GameAppManager.inl"
#endif // !LEARNVK_CORE_SUBSYSTEM_GAME_GAME_APP_MANAGER