#pragma once
#ifndef LEARNVK_RENDERER_RENDER_SYSTEM_H
#define LEARNVK_RENDERER_RENDER_SYSTEM_H

#include "API/Context.h"
#include "RenderPlatform/API.h"
#include "Engine/Interface.h"
#include "Assets/Assets.h"
#include "FrameGraph/FrameGraph.h"
#include "Library/Containers/List.h"
#include "RenderAbstracts/CommandBuffer.h"
#include "RenderAbstracts/RenderGroup.h"


class RENDERER_API RenderSystem : public SystemInterface
{
public:

	enum Identity : uint32 { Value = 0x00 };

	RenderSystem();
	~RenderSystem();

	DELETE_COPY_AND_MOVE(RenderSystem)

	void OnInit			() override;
	void OnUpdate		() override;
	void OnTerminate	() override;
	void OnEvent		(const OS::Event& e) override;
	
	FrameGraph*	GetFrameGraph			();
	void		FinalizeGraph			();

	void		PushCommandForRender	(DrawCommand& Command);
	void		RenderModel				(Model& InModel, uint32 RenderedPass);

	void		NewRenderGroup			(uint32 Identifier);
	void		AddModelToRenderGroup	(Model& InModel, uint32 GroupIdentifier);
	void		FinalizeRenderGroup		(uint32 Identifier);
	void		TerminateRenderGroup	(uint32 Identifier);

	RendererAssetManager& FetchAssetManager	();

private:

	using RenderGroupStore = Map<uint32, RenderGroup*>;

	RendererAssetManager	AssetManager;
	RenderContext			Context;
	FrameGraph*				Graph;
	CommandBuffer			CmdBuffer;
	RenderGroupStore		RenderGroups;

};

namespace ao
{
	RENDERER_API RenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H