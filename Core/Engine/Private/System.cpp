#pragma once
#include "System.h"

void SystemManager::UnregisterAndFreeAll()
{
	for (const Pair<uint32, SystemInterface*>& pair : GameSystems)
	{
		SystemInterface* system = pair.Value;
		system->OnTerminate();
		FMemory::Free(system);
	}

	for (const Pair<uint32, SystemInterface*>& pair : EngineSystems)
	{
		SystemInterface* system = pair.Value;
		system->OnTerminate();
		FMemory::Free(system);
	}

	GameSystems.Release();
	EngineSystems.Release();
}

void SystemManager::Update()
{
	for (const Pair<uint32, SystemInterface*>& pair : GameSystems)
	{
		SystemInterface* system = pair.Value;
		system->OnUpdate();
	}

	for (const Pair<uint32, SystemInterface*>& pair : EngineSystems)
	{
		SystemInterface* system = pair.Value;
		system->OnUpdate();
	}
}

void SystemManager::OnEvent(const OS::Event& e)
{
	for (const Pair<uint32, SystemInterface*>& pair : GameSystems)
	{
		SystemInterface* system = pair.Value;
		system->OnEvent(e);
	}

	for (const Pair<uint32, SystemInterface*>& pair : EngineSystems)
	{
		SystemInterface* system = pair.Value;
		system->OnEvent(e);
	}
}

SystemInterface* SystemManager::GetSystem(uint32 SystemIdentity, SystemType Type)
{
	Map<uint32, SystemInterface*>* container = nullptr;
	SystemInterface* system = nullptr;

	if (Type == System_Engine_Type)
	{
		container = &EngineSystems;
	}
	else
	{
		container = &GameSystems;
	}

	system = container->Get(SystemIdentity);

	return system;
}