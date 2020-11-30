#pragma once
#ifndef LEARNVK_ENGINE_PRIVATE_SYSTEM
#define LEARNVK_ENGINE_PRIVATE_SYSTEM

#include "Platform/Minimal.h"
#include "Library/Containers/Map.h"
#include "Platform/EngineAPI.h"

enum SystemType : uint32
{
	System_Engine_Type	= 0x00,
	System_Game_Type	= 0x01
};

struct ENGINE_API SystemInterface : public ApplicationInterface
{
	//virtual bool	Initialize	()	= 0;
	virtual void	OnUpdate	()	= 0;
	//virtual void	Terminate	()	= 0;

	//virtual void OnInit() = delete;

};

class ENGINE_API SystemManager
{
private:

	friend class EngineImpl;

public:

	SystemManager()  = default;
	~SystemManager() = default;

	DELETE_COPY_AND_MOVE(SystemManager)

	template <typename SystemClass>
	SystemClass* RegisterSystem	(SystemType Type);

	void		UnregisterAndFreeAll	();
	void		Update					();
	void		OnEvent					(const OS::Event& e);

	SystemInterface* GetSystem(uint32 Identity, SystemType Type);

private:
	Map<uint32, SystemInterface*> GameSystems;
	Map<uint32, SystemInterface*> EngineSystems;
};

#include "System.inl"
#endif // !LEARNVK_ENGINE_PRIVATE_SYSTEM