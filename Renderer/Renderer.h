#pragma once
#ifndef LEARNVK_RENDERER_RENDER_SYSTEM_H
#define LEARNVK_RENDERER_RENDER_SYSTEM_H

#include "API/Context.h"
#include "RenderPlatform/API.h"
#include "Engine/Interface.h"
#include "Assets/Assets.h"
#include "FrameGraph/FrameGraph.h"
#include "Library/Containers/Node.h"
#include "RenderAbstracts/DrawCommand.h"
#include "RenderAbstracts/RenderGroup.h"
#include "RenderAbstracts/DescriptorSets.h"

#define RENDERER_DRAWABLES_SLACK 16

/**
*
* TODO(Ygsm):
* Include settings e.g render resolution and etc.
*/

class RENDERER_API RenderSystem : public SystemInterface
{
private:

	using DrawCommandStore = Map<uint32, Array<DrawCommand, RENDERER_DRAWABLES_SLACK>>;

	struct BindingDescriptors
	{
		ForwardNode<DescriptorSetInstance>* Base;
		ForwardNode<DescriptorSetInstance>* Current;
		uint32 Count;
	};

	struct DescriptorInstanceStore
	{
		Array<DescriptorSetInstance>	Global;
		Map<size_t, BindingDescriptors> PerPass;
		Map<size_t, BindingDescriptors> PerObject;
	};

	EngineImpl&				Engine;
	RenderContext			Context;
	IRAssetManager			AssetManager;
	IRFrameGraph*			FrameGraph;
	IRDescriptorManager*	DescriptorManager;
	IRVertexGroupManager*	VertexGroupManager;
	DrawCommandStore		DrawCommands;
	DescriptorInstanceStore	DescriptorInstances;
	Handle<ISystem>			Hnd;

	void FlushRenderer				();
	void BindVertexAndIndexBuffer	(DrawCommand& Command, RenderPass& Pass);
	void BindPerPassDescriptors		(uint32 RenderPassId, RenderPass& Pass);
	void BindPerObjectDescriptors	(uint32 DrawCommandId, RenderPass& Pass);

public:

	RenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd);
	~RenderSystem();

	DELETE_COPY_AND_MOVE(RenderSystem)

	void		OnInit			() override;
	void		OnUpdate		() override;
	void		OnTerminate		() override;
	void		FinalizeGraph	();

	Handle<DrawCommand>		DrawModel				(Model& InModel, uint32 Pass);
	bool					UpdateDescriptorSet		(uint32 DescriptorId, Handle<DrawCommand> DrawCmdHandle, void* Data, size_t Size);
	bool					BindDescLayoutToPass	(uint32 DescriptorLayoutId, Handle<RenderPass> PassHandle);
	IRFrameGraph&			GetFrameGraph			();
	IRAssetManager&			GetAssetManager			();
	IRDescriptorManager&	GetDescriptorManager	();
	IRVertexGroupManager&	GetVertexGroupManager	();

	static Handle<ISystem> GetSystemHandle();
};

namespace ao
{
	RENDERER_API RenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H