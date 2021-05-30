#pragma once
#ifndef LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H
#define LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H


#include "Library/Containers/Bitset.h"
#include "Library/Containers/Array.h"
#include "Library/Containers/String.h"
#include "Library/Containers/Ref.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "API/RendererFlagBits.h"
#include "API/Definitions.h"
#include "API/Device.h"

class IFrameGraph;
class IRenderSystem;
struct SDescriptorSet;
struct SPipeline;

struct SRenderPass
{
	using Extent2D = WindowInfo::Extent2D;
	using Position = WindowInfo::Position;
	using Attachment = Pair<Handle<SImage>, Ref<SImage>>;
	using VulkanFramebuffer = IRenderDevice::VulkanFramebuffer;
	using AttachmentContainer = StaticArray<Attachment, MAX_RENDERPASS_ATTACHMENT_COUNT>;

	Handle<SRenderPass> Hnd; // Not sure if I like this ... 
	IFrameGraph* pOwner;
	AttachmentContainer ColorInputs;
	AttachmentContainer ColorOutputs;
	VulkanFramebuffer Framebuffer;
	Attachment DepthStencilInput;
	Attachment DepthStencilOutput;
	VkRenderPass RenderPassHnd;
	BitSet<ERenderPassFlagBits> Flags;
	//Ref<SDescriptorSet> pSet;
	//Ref<SPipeline> pPipeline;
	Extent2D Extent;
	Position Pos;
	float32 Depth;
	ERenderPassType Type;
	bool Bound = false;
};

struct RenderPassCreateInfo
{
	String64 Identifier;
	SRenderPass::Extent2D Extent;
	SRenderPass::Position Pos;
	float32 Depth;
	ERenderPassType Type;
	//Handle<SDescriptorSet> DescriptorSetHnd;
	//Handle<SPipeline> PipelineHnd;
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

	using Extent2D = SRenderPass::Extent2D;
	using RenderPass = Pair<Handle<SRenderPass>, Ref<SRenderPass>>;
	using Attachment = SRenderPass::Attachment;
	using PassContainer = StaticArray<RenderPass, MAX_FRAMEGRAPH_PASS_COUNT>;

	static size_t _HashSeed;

	IRenderSystem& Renderer;
	Attachment ColorImage;
	Attachment DepthStencilImage;
	PassContainer RenderPasses;
	Extent2D Extent;
	bool Built;

	Ref<SRenderPass> GetRenderPass(Handle<SRenderPass> Hnd);
	void ResetRenderPassBindState();
	size_t HashIdentifier(const String64& Identifier);

	// Create a vk framebuffer.
	bool CreateFramebuffer(Ref<SRenderPass> pRenderPass);
	// Destroys a vk framebuffer.
	void DestroyFramebuffer(Ref<SRenderPass> pRenderPass);

	// Creates a vk renderpass.
	bool CreateRenderPass(Ref<SRenderPass> pRenderPass);
	// Destroys a vk renderpass.
	void DestroyRenderPass(Ref<SRenderPass> pRenderPass);


public:

	IFrameGraph(IRenderSystem& InRenderer);
	~IFrameGraph();

	DELETE_COPY_AND_MOVE(IFrameGraph)

	bool AddColorInputFrom(const String64& AttId, Handle<SRenderPass> Src, Handle<SRenderPass> Dst);
	bool AddColorOutput(const String64& AttId, Handle<SRenderPass> Hnd);
	bool AddDepthStencilInputFrom(Handle<SRenderPass> Src, Handle<SRenderPass> Dst);
	bool AddDepthStencilOutput(Handle<SRenderPass> Hnd);
	bool SetRenderPassExtent(Handle<SRenderPass> Hnd, WindowInfo::Extent2D Extent);
	bool SetRenderPassOrigin(Handle<SRenderPass> Hnd, WindowInfo::Position Origin);

	bool NoDefaultRender(Handle<SRenderPass> Hnd);
	bool NoDefaultColorRender(Handle<SRenderPass> Hnd);
	bool NoDefaultDepthStencilRender(Handle<SRenderPass> Hnd);

	Handle<SRenderPass> AddRenderPass(const RenderPassCreateInfo& CreateInfo);

	bool GetRenderPassColorOutputCount(Handle<SRenderPass> Hnd, uint32& Count);

	size_t GetNumRenderPasses()	const;
	void SetOutputExtent(uint32 Width = 0, uint32 Height = 0);

	//void OnWindowResize();
	// 
	// Creates a CPU side image with the given extent.
	// Does not create the resource on the GPU.
	bool Initialize(const Extent2D& Extent);
	void Terminate();
	bool Build();
	bool IsBuilt() const;
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H