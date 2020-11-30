#include "Abstraction.h"

EngineImpl* g_EngineCtx = nullptr;

void EngineImpl::InitializeGlobalContext(EngineImpl* Engine)
{
	g_EngineCtx = Engine;
}

EngineImpl& FetchEngineContext()
{
	// Did you initialize the engine yet?
	VKT_ASSERT(g_EngineCtx);
	return *g_EngineCtx;
}