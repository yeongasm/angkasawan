#pragma once
#ifndef LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H
#define LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H


#include "Library/Containers/Bitset.h"
#include "Library/Containers/Array.h"
#include "Library/Containers/String.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "API/RendererFlagBits.h"
#include "API/Definitions.h"
#include "API/Device.h"

class IFrameGraph;
class IRenderSystem;

struct SRenderPass
{
	using Extent2D = WindowInfo::Extent2D;
	using Position = WindowInfo::Position;
	using Attachment = Pair<Handle<SImage>, SImage*>;
	using VulkanFramebuffer = IRenderDevice::VulkanFramebuffer;
	using AttachmentContainer = StaticArray<Attachment, MAX_RENDERPASS_ATTACHMENT_COUNT>;

	//enum ERenderPassOrder : uint32
	//{
	//	Render_Pass_Order_First,
	//	Render_Pass_Order_In_Between,
	//	Render_Pass_Order_Final
	//};

	IFrameGraph* pOwner;
	AttachmentContainer ColorInputs;
	AttachmentContainer ColorOutputs;
	VulkanFramebuffer Framebuffer;
	Attachment DepthStencilInput;
	Attachment DepthStencilOutput;
	VkRenderPass RenderPassHnd;
	BitSet<ERenderPassFlagBits> Flags;
	Extent2D Extent;
	Position Pos;
	float32 Depth;
	//ERenderPassOrder Order;
	ERenderPassType Type;
};

struct RenderPassCreateInfo
{
	String64 Identifier;
	SRenderPass::Extent2D Extent;
	SRenderPass::Position Pos;
	float32 Depth;
	ERenderPassType Type;
};

/**
* NOTE(Ygsm):
* Compute passes in the frame graph will be synchronous compute using the graphics queue.
* Create another subsystem for async compute.
*/
class RENDERER_API IFrameGraph
{
private:

	friend class IRenderSystem;

	using Extent2D = SRenderPass::Extent2D;
	using RenderPass = Pair<Handle<SRenderPass>, SRenderPass*>;
	using Attachment = SRenderPass::Attachment;
	using PassContainer = StaticArray<RenderPass, MAX_FRAMEGRAPH_PASS_COUNT>;

	static size_t _HashSeed;

	IRenderSystem& Renderer;
	Attachment ColorImage;
	Attachment DepthStencilImage;
	PassContainer RenderPasses;
	Extent2D Extent;
	bool Built;

	SRenderPass* GetRenderPass(Handle<SRenderPass> Hnd);
	size_t HashIdentifier(const String64& Identifier);

	// Create a vk framebuffer.
	bool CreateFramebuffer(SRenderPass* pRenderPass);
	// Destroys a vk framebuffer.
	void DestroyFramebuffer(SRenderPass* pRenderPass);

	// Creates a vk renderpass.
	bool CreateRenderPass(SRenderPass* pRenderPass);
	// Destroys a vk renderpass.
	void DestroyRenderPass(SRenderPass* pRenderPass);


public:

	IFrameGraph(IRenderSystem& InRenderer);
	~IFrameGraph();

	DELETE_COPY_AND_MOVE(IFrameGraph)

	void BeginRenderPass(SRenderPass* pRenderPass);
	void EndRenderPass(SRenderPass* pRenderPass);

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
	size_t GetNumRenderPasses()	const;
	void SetOutputExtent(uint32 Width = 0, uint32 Height = 0);

	//void OnWindowResize();
	bool Initialize();
	void Terminate();
	bool Build();
	bool IsBuilt() const;
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H