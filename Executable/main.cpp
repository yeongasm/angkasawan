#include "Library/Memory/Memory.h"
#include "Config/ConfigParser.h"
#include "Renderer.h"

bool InitializeEngine(EngineImpl* Engine, const EngineCreationInfo& CreateInfo)
{
	Engine->InitializeEngine(CreateInfo);

	Handle<ISystem> hnd;
	size_t renderSystemSize = sizeof(RenderSystem);
	RenderSystem* renderer = reinterpret_cast<RenderSystem*>(Engine->AllocateAndRegisterSystem(renderSystemSize, System_Engine_Type, &hnd));
	FMemory::InitializeObject(renderer, *Engine, hnd);
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