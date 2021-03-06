#pragma once
#include "System.h"

template <typename SystemClass>
SystemClass* SystemManager::RegisterSystem(SystemType Type)
{
	using Identity = typename SystemClass::Identity;
	SystemClass* system = reinterpret_cast<SystemClass*>(IMemory::Malloc(sizeof(SystemClass)));
	IMemory::InitializeObject(system);

	if (Type == System_Engine_Type)
	{
		EngineSystems.Insert(Identity::Value, system);
	}
	else
	{
		GameSystems.Insert(Identity::Value, system);
	}

	system->OnInit();

	return system;
}