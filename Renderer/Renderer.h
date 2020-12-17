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


class RENDERER_API RenderSystem : public SystemInterface
{
public:

	enum Identity : uint32 { Value = 0x00 };

	RenderSystem();
	~RenderSystem();

	DELETE_COPY_AND_MOVE(RenderSystem)

	//bool Initialize();

	void OnInit			() override;
	void OnUpdate		() override;
	void OnTerminate	() override;
	void OnEvent		(const OS::Event& e) override;
	//void Terminate();

	FrameGraph*	GetFrameGraph			();
	void		FinalizeGraph			();
	
	/**
	* I think in practice you will always only render models.
	*/
	void		PushCommandForRender	(DrawCommand& Command);
	void		RenderModel				(Model& InModel, uint32 RenderedPass);

	RendererAssetManager& FetchAssetManager		();

	/**
	* Builds the vertex and index buffers for each mesh in the model
	*/
	bool FinalizeModel	(Model& InModel);

private:
	RendererAssetManager	AssetManager;
	RenderContext			Context;
	FrameGraph*				Graph;
	CommandBuffer			CmdBuffer;
};

namespace ao
{
	RENDERER_API RenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H