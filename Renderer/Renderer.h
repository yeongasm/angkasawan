#pragma once
#ifndef LEARNVK_RENDERER_RENDER_SYSTEM_H
#define LEARNVK_RENDERER_RENDER_SYSTEM_H

#include "API/Context.h"
#include "RenderPlatform/API.h"
#include "Engine/Interface.h"
#include "Assets/Assets.h"
#include "Library/Containers/Node.h"
#include "RenderAbstracts/FrameGraph.h"
#include "RenderAbstracts/DrawCommand.h"
#include "RenderAbstracts/DescriptorSets.h"
#include "RenderAbstracts/RenderMemory.h"
#include "RenderAbstracts/PipelineManager.h"
#include "RenderAbstracts/MaterialManager.h"
#include "RenderAbstracts/PushConstant.h"

/**
*
* TODO(Ygsm):
* Include settings e.g render resolution and etc.
*/

class RENDERER_API RenderSystem : public SystemInterface
{
private:

	EngineImpl&				Engine;
	IRAssetManager*			AssetManager;
	IRFrameGraph*			FrameGraph;
	IRDescriptorManager*	DescriptorManager;
	IRenderMemoryManager*	MemoryManager;
	IRDrawManager*			DrawCmdManager;
	IRPipelineManager*		PipelineManager;
	IRPushConstantManager*	PushConstantManager;
	IRMaterialManager*		MaterialManager;
	Handle<ISystem>			Hnd;

	void FlushRenderer();

public:

	RenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd);
	~RenderSystem();

	DELETE_COPY_AND_MOVE(RenderSystem)

	void		OnInit			() override;
	void		OnUpdate		() override;
	void		OnTerminate		() override;
	void		FinalizeGraph	();

	IRFrameGraph&			GetFrameGraph			();
	IRAssetManager&			GetAssetManager			();
	IRDescriptorManager&	GetDescriptorManager	();
	IRenderMemoryManager&	GetRenderMemoryManager	();
	IRDrawManager&			GetDrawManager			();
	IRPipelineManager&		GetPipelineManager		();
	IRMaterialManager&		GetMaterialManager		();

	static Handle<ISystem> GetSystemHandle();
};

namespace ao
{
	RENDERER_API RenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H