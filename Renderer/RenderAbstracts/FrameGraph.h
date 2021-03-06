#pragma once
#ifndef LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H
#define LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H


#include "Library/Containers/Bitset.h"
#include "Library/Containers/Array.h"
#include "Library/Containers/String.h"
#include "Library/Containers/Pair.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "API/RendererFlagBits.h"
#include "API/Definitions.h"
#include "Engine/Interface.h"

class IFrameGraph;
class IRenderSystem;
struct SDescriptorSet;
struct SPipeline;
struct SRenderPass;
struct SImage;

using Extent2D = WindowInfo::Extent2D;
using Position = WindowInfo::Position;

using VkRenderPass = struct VkRenderPass_T*;
using VkFramebuffer = struct VkFramebuffer_T*;

struct SRenderPass
{
	struct OAttachment
	{
		Handle<SImage> Hnd;
		astl::Ref<SImage> pImg;
	};

	struct IAttachment
	{
		astl::Ref<SRenderPass> pOwner;
		uint32 AtIndex;
	};

	using OutputAttachments = astl::StaticArray<OAttachment, MAX_RENDERPASS_ATTACHMENT_COUNT>;
	using InputAttachments = astl::StaticArray<IAttachment, MAX_RENDERPASS_ATTACHMENT_COUNT>;

	Handle<SRenderPass> Hnd; // Not sure if I like this ... 
	IFrameGraph* pOwner;
	InputAttachments ColorInputs;
	OutputAttachments ColorOutputs;
	VkFramebuffer Framebuffer;
	IAttachment DepthStencilInput;
	OAttachment DepthStencilOutput;
	VkRenderPass RenderPassHnd;
	astl::BitSet<ERenderPassFlagBits> Flags;
	Extent2D Extent;
	Position Pos;
	float32 Depth;
	ERenderPassType Type;
	astl::BitSet<ERenderPassOrderFlagBits> Order;
  uint32 IndexForDraw;
	bool Rebuild = false;
};

struct RenderPassCreateInfo
{
	astl::String64 Identifier;
	Extent2D Extent;
	Position Pos;
	float32 Depth;
	ERenderPassType Type;
};

/**
* NOTE(Ygsm):
* Compute passes in the frame graph will be synchronous compute using the graphics queue.
* Create another subsystem for async compute.
* 
* The frame graph will own a descriptor set and slot 0 is reserved for it.
*/
class RENDERER_API IFrameGraph
{
private:

	friend class IRenderSystem;

	using RenderPass = astl::Pair<Handle<SRenderPass>, astl::Ref<SRenderPass>>;
	using PassContainer = astl::StaticArray<RenderPass, MAX_FRAMEGRAPH_PASS_COUNT>;

	static size_t _HashSeed;

	IRenderSystem& Renderer;
	SRenderPass::OAttachment ColorImage;
	SRenderPass::OAttachment DepthStencilImage;
	PassContainer RenderPasses;
	Extent2D Extent;
	bool Built;

	astl::Ref<SRenderPass> GetRenderPass(Handle<SRenderPass> Hnd);
	size_t HashIdentifier(const astl::String64& Identifier);

	// Create a vk framebuffer.
	bool CreateFramebuffer(astl::Ref<SRenderPass> pRenderPass);
	// Destroys a vk framebuffer.
	void DestroyFramebuffer(astl::Ref<SRenderPass> pRenderPass);

	// Creates a vk renderpass.
	bool CreateRenderPass(astl::Ref<SRenderPass> pRenderPass);
	// Destroys a vk renderpass.
	void DestroyRenderPass(astl::Ref<SRenderPass> pRenderPass);

	void SetupAttachments(astl::Ref<SImage> pColor, astl::Ref<SImage> pDepthStencil);

public:

	IFrameGraph(IRenderSystem& InRenderer);
	~IFrameGraph();

	DELETE_COPY_AND_MOVE(IFrameGraph)

	bool AddColorInputFrom(Handle<SImage> Img, Handle<SRenderPass> Src, Handle<SRenderPass> Dst);
	Handle<SImage> AddColorOutput(Handle<SRenderPass> Hnd);
	bool AddDepthStencilInputFrom(Handle<SRenderPass> Src, Handle<SRenderPass> Dst);
	bool AddDepthStencilOutput(Handle<SRenderPass> Hnd);
	bool SetRenderPassExtent(Handle<SRenderPass> Hnd, Extent2D Extent);
	bool SetRenderPassOrigin(Handle<SRenderPass> Hnd, Position Origin);

	bool NoDefaultRender(Handle<SRenderPass> Hnd);
	bool NoDefaultColorRender(Handle<SRenderPass> Hnd);
	bool NoDefaultDepthStencilRender(Handle<SRenderPass> Hnd);

	Handle<SRenderPass> AddRenderPass(const RenderPassCreateInfo& CreateInfo);

	bool GetRenderPassColorOutputCount(Handle<SRenderPass> Hnd, uint32& Count);

	size_t GetNumRenderPasses()	const;
	void SetOutputExtent(uint32 Width = 0, uint32 Height = 0);

	void OnWindowResize();
	
	// Creates a CPU side image with the given extent.
	// Does not create the resource on the GPU.
	bool Initialize(const Extent2D& Extent);
	void Terminate();
	bool Build();
	bool IsBuilt() const;
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H
