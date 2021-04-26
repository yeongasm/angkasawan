#pragma once
#ifndef LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H
#define LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H


#include "Library/Containers/Bitset.h"
#include "Library/Containers/Map.h"
#include "Library/Allocators/LinearAllocator.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "API/RendererFlagBits.h"
#include "API/Definitions.h"
#include "API/Device.h"

class IFrameGraph;

/**
*
*/
struct RENDERER_API RenderPass
{

	RenderPass(IFrameGraph& Graph, uint32 Order);
	~RenderPass();

	DELETE_COPY_AND_MOVE(RenderPass)

	void AddColorInput			(const String32& Identifier, RenderPass& From);
	void AddColorOutput			(const String32& Identifier, const AttachmentCreateInfo& CreateInfo);
	void AddDepthStencilInput	(const RenderPass& From);
	void AddDepthStencilOutput	();
	void NoRender				();
	void NoColorRender			();
	void NoDepthStencilRender	();

	using VulkanFramebuffer = IRenderDevice::VulkanFramebuffer;
	using OutputAttachments = Map<size_t, STexture*, XxHash<size_t>, 1>;
	using InputAttachments	= Map<size_t, STexture*, XxHash<size_t>, 1>;

	IFrameGraph& Owner;
	BitSet<ERenderPassFlagBits>	Flags;
	float32	Width;
	float32	Height;
	float32	Depth;
	uint32 Order;
	uint32 PassType;
	ERenderPassState State;
	VkRenderPass RenderPassHnd;
	VulkanFramebuffer Framebuffer;
	InputAttachments ColorInputs;
	OutputAttachments ColorOutputs;
	STexture* DepthStencilInput;
	STexture* DepthStencilOutput;
};


class RENDERER_API IFrameGraph
{
public:

	IFrameGraph(LinearAllocator& InAllocator, SRDeviceStore& InDeviceStore);
	~IFrameGraph();

	DELETE_COPY_AND_MOVE(IFrameGraph)

	Handle<RenderPass>	AddPass				(const String64& Identity);
	RenderPass&			GetRenderPass		(Handle<RenderPass> Handle);
	//Handle<HImage>		GetColorImage		()	const;
	//Handle<HImage>		GetDepthStencilImage()	const;
	uint32				GetNumRenderPasses	()	const;
	void				SetOutputExtent		(uint32 Width = 0, uint32 Height = 0);

	void OnWindowResize	();
	void Destroy		();
	bool Compile		();
	bool Compiled		() const;

private:			

	friend class RenderSystem;
	using RenderPassTable = Map<size_t, RenderPass*, XxHash<size_t>, 3>;

	LinearAllocator&	Allocator;
	//IDeviceStore&		Device;
	STexture*			ColorImage;
	STexture*			DepthStencilImage;
	RenderPassTable		RenderPasses;
	bool				IsCompiled;
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H