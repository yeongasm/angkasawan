#pragma once
#ifndef LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H
#define LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H

#include "Library/Containers/Bitset.h"
#include "Library/Containers/Map.h"
#include "Library/Allocators/LinearAllocator.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "API/RendererFlagBits.h"
#include "RenderAbstracts/Primitives.h"

#define MAX_FRAMES_IN_FLIGHT 2

class RenderSystem;
class IRFrameGraph;
struct SRDeviceStore;

struct AttachmentCreateInfo
{
	ETextureUsage	Usage;
	ETextureType	Type;
};

/**
* NOTE(Ygsm):
* A RenderPass represents a rendering pass in the program.
* Is able to output textures that will be sampled by another pass.
* Also able to receive input from other RenderPasses to be sampled.
* By default, all RenderPasses will render to the color & depth stencil output owned by the FrameGraph unless any of the No* functions are called.
* 
*
*/
struct RENDERER_API RenderPass
{

	RenderPass(IRFrameGraph& OwnerGraph, ERenderPassType Type, uint32 Order);
	~RenderPass();

	DELETE_COPY_AND_MOVE(RenderPass)

	void AddColorInput			(const String32& Identifier, RenderPass& From);
	void AddColorOutput			(const String32& Identifier, const AttachmentCreateInfo& CreateInfo);
	void AddDepthStencilInput	(const RenderPass& From);
	void AddDepthStencilOutput	();

	void SetWidth		(float32 Width);
	void SetHeight		(float32 Height);
	void SetDepth		(float32 Depth);

	void NoRender				();
	void NoColorRender			();
	void NoDepthStencilRender	();

	struct AttachmentInfo : public AttachmentCreateInfo
	{
		Handle<HImage> Handle;
	};

	using OutputAttachments = Map<String32, AttachmentInfo, MurmurHash<String32>, 1>;
	using InputAttachments	= Map<String32, AttachmentInfo, MurmurHash<String32>, 1>;

	IRFrameGraph&		Owner;
	ERenderPassType		Type;

	BitSet<uint32>		Flags;
	float32				Width;
	float32				Height;
	float32				Depth;
	uint32				Order;
	uint32				PassType;

	ERenderPassState	State;
	Handle<HRenderpass> RenderpassHandle;
	Handle<HFramebuffer>FramebufferHandle;

	InputAttachments	ColorInputs;
	OutputAttachments	ColorOutputs;
	AttachmentInfo		DepthStencilInput;
	AttachmentInfo		DepthStencilOutput;
};


class RENDERER_API IRFrameGraph
{
private:
	using RenderPassEnum = uint32;
public:

	IRFrameGraph(LinearAllocator& GraphAllocator, SRDeviceStore& InDeviceStore);
	~IRFrameGraph();

	DELETE_COPY_AND_MOVE(IRFrameGraph)

	Handle<RenderPass>	AddPass				(RenderPassEnum PassIdentity, ERenderPassType Type);
	RenderPass&			GetRenderPass		(Handle<RenderPass> Handle);
	Handle<HImage>		GetColorImage		()	const;
	Handle<HImage>		GetDepthStencilImage()	const;
	uint32				GetNumRenderPasses	()	const;
	void				SetOutputExtent		(uint32 Width = 0, uint32 Height = 0);

	void OnWindowResize	();
	void Destroy		();
	bool Compile		();
	bool Compiled		() const;

private:			

	friend class RenderSystem;
	using RenderPassTable	= Map<RenderPassEnum, RenderPass*>;

	LinearAllocator&	Allocator;
	SRDeviceStore&		Device;
	Texture				ColorImage;
	Texture				DepthStencilImage;
	RenderPassTable		RenderPasses;
	bool				IsCompiled;
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H