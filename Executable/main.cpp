#include "Library/Memory/Memory.h"
#include "Config/ConfigParser.h"
#include "Renderer.h"

bool InitializeEngine(EngineImpl* Engine, const EngineCreationInfo& CreateInfo)
{
	Engine->InitializeEngine(CreateInfo);

	Handle<ISystem> hnd;
	IRenderSystem* renderer = reinterpret_cast<IRenderSystem*>(
		Engine->AllocateAndRegisterSystem(
			sizeof(IRenderSystem), 
			System_Engine_Type, 
			&hnd
		)
	);
	astl::IMemory::InitializeObject(renderer, *Engine, hnd);
	renderer->OnInit();
	Engine->RegisterGame(CreateInfo);
	return true;
}

int main(int argc, char* argv[])
{
	ConfigFileParser configParser;
	EngineCreationInfo createInfo = {};
	
	if (!configParser.InitializeFromConfigFile(createInfo))
	{
		return -1;
	}

	EngineImpl* engine = reinterpret_cast<EngineImpl*>(astl::IMemory::Malloc(sizeof(EngineImpl)));
	astl::IMemory::InitializeObject(engine);

	if (!InitializeEngine(engine, createInfo))
	{
		goto TerminateEngine;
	}

	engine->Run();

TerminateEngine:

	engine->TerminateEngine();

	return 0;
}
