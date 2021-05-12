#pragma once
#include "GameAppManager.h"

template <typename GameClass>
bool GameManager::InitializeGame()
{
	Game = reinterpret_cast<GameClass*>(IMemory::Malloc(sizeof(GameClass)));
	if (!Game)
	{
		return false;
	}
	new (Game) GameClass();
	Game->Initialize();
	return true;
}