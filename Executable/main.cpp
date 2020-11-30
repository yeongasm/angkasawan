#include "Library/Memory/Memory.h"
#include "Config/ConfigParser.h"
#include "Renderer.h"

bool InitializeEngine(EngineImpl* Engine, const EngineCreationInfo& CreateInfo)
{
	Engine->InitializeEngine(CreateInfo);
	RenderSystem* renderer = Engine->Systems.RegisterSystem<RenderSystem>(System_Engine_Type);
	if (!renderer) { return false; }
	Engine->RegisterGame(CreateInfo);
	return true;
}

int main(int argc, char* argv[])
{
	ConfigFileParser configParser;
	EngineCreationInfo createInfo;
	FMemory::InitializeObject(createInfo);
	
	if (!configParser.InitializeFromConfigFile(createInfo))
	{
		return -1;
	}

	EngineImpl* engine = reinterpret_cast<EngineImpl*>(FMemory::Malloc(sizeof(EngineImpl)));
	FMemory::InitializeObject(engine);

	if (!InitializeEngine(engine, createInfo))
	{
		goto TerminateEngine;
	}

	engine->Run();

TerminateEngine:

	engine->TerminateEngine();


	return 0;
}