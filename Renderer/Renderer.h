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

	/**
	* NOTE(Ygsm):
	* There is a flaw with using indices as handles. When objects are dynamically added or removed, the indices will be invalidated.
	*/
	Handle<FrameGraph>	NewFrameGraph			(const char* GraphName);
	FrameGraph&			GetFrameGraph			(Handle<FrameGraph> Handle);
	FrameGraph*			GetFrameGraphWithName	(const char* GraphName);
	void				PushCommandForRender	(const DrawCommand& Command);

	RendererAssetManager& FetchAssetManager		();

private:
	RendererAssetManager	AssetManager;
	RenderContext			Context;
	Array<FrameGraph*>		FrameGraphs;
	CommandBuffer			CmdBuffer;
};

namespace ao
{
	RENDERER_API RenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H