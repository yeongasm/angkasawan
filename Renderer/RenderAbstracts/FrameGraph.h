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
struct DescriptorLayout;

struct AttachmentCreateInfo
{
	ETextureUsage	Usage;
	ETextureType	Type;
};

/**
* NOTE(Ygsm):
* 30.12.2020 - Should subpasses produce output attachments?
* 04.01.2021 - No, subpasses should not produce output attachment. They will use the parent's renderpass.
* 
* A RenderPass represents a rendering pass in the program.
* Is able to output textures that will be sampled by another pass.
* Also able to receive input from other RenderPasses to be sampled.
* By default, all RenderPasses will render to the color & depth stencil output owned by the FrameGraph unless any of the No* functions are called.
* 
* *** FUTURE FEATURE ***
* 
* RenderPasses can also be converted into subpasses.
* Subpasses are RenderPasses that do not produce sampled outputs and instead writes to it's parent's output.
* They are essentially just another pipeline within the main pass.
*/
struct RENDERER_API RenderPass
{
//public:

	RenderPass(IRFrameGraph& OwnerGraph, ERenderPassType Type, uint32 Order);
	~RenderPass();

	DELETE_COPY_AND_MOVE(RenderPass)

	//bool AddShader				(Shader* ShaderSrc);
	bool AddPipeline			(Shader* VertexShader, Shader* FragmentShader);
	bool AddVertexInputBinding	(uint32 Binding, uint32 From, uint32 To, uint32 Stride, EVertexInputRateType Type);
	void AddColorInput			(const String32& Identifier, RenderPass& From);
	void AddColorOutput			(const String32& Identifier, const AttachmentCreateInfo& CreateInfo);

	//void AddDepthInput			(const RenderPass& From);
	//void AddDepthOutput			();

	//void AddStencilInput		(const RenderPass& From);
	//void AddStencilOutput		();

	void AddDepthStencilInput	(const RenderPass& From);
	void AddDepthStencilOutput	();

	/**
	* Feature not implemented yet!
	*/
	bool AddSubpass		(RenderPass& Subpass);

	/**
	* Feature not implemented yet!
	*/
	bool IsMainpass		() const;

	/**
	* Feature not implemented yet!
	*/
	bool IsSubpass		() const;

	void SetWidth		(float32 Width);
	void SetHeight		(float32 Height);
	void SetDepth		(float32 Depth);
	void SetSampleCount	(ESampleCount Samples);
	void SetTopology	(ETopologyType Type);
	void SetCullMode	(ECullingMode Mode);
	void SetPolygonMode	(EPolygonMode Mode);
	void SetFrontFace	(EFrontFaceDir Face);

	void NoRender				();
	void NoColorRender			();
	void NoDepthStencilRender	();

//private:

	struct VertexInputBinding
	{
		uint32 Binding;
		uint32 From;
		uint32 To;
		uint32 Stride;
		EVertexInputRateType Type;
	};

	using VertexInputBindings = Array<VertexInputBinding>;

	struct AttachmentInfo : public AttachmentCreateInfo
	{
		Handle<HImage> Handle;
	};

	using OutputAttachments = Map<String32, AttachmentInfo, MurmurHash<String32>, 1>;
	using InputAttachments	= Map<String32, AttachmentInfo, MurmurHash<String32>, 1>;
	using ArrayOfDescLayouts = Array<DescriptorLayout*>;

	IRFrameGraph&		Owner;
	ERenderPassType		Type;

	ESampleCount		Samples;
	ETopologyType		Topology;
	EFrontFaceDir		FrontFace;
	ECullingMode		CullMode;
	EPolygonMode		PolygonalMode;
	BitSet<uint32>		Flags;
	float32				Width;
	float32				Height;
	float32				Depth;
	uint32				Order;
	uint32				PassType;

	Shader*				Shaders[Shader_Type_Max];
	ERenderPassState	State;
	Handle<HPipeline>	PipelineHandle;
	Handle<HRenderpass> RenderpassHandle;
	Handle<HFramebuffer>FramebufferHandle;
	//Handle<HFramepass>	FramePassHandle;

	InputAttachments	ColorInputs;
	OutputAttachments	ColorOutputs;
	AttachmentInfo		DepthStencilInput;
	AttachmentInfo		DepthStencilOutput;

	RenderPass*			Parent;
	VertexInputBindings	VertexBindings;
	Array<RenderPass*>	Childrens;
	ArrayOfDescLayouts	BoundDescriptorLayouts;
};


class RENDERER_API IRFrameGraph
{
private:
	using RenderPassEnum = uint32;
public:

	IRFrameGraph(LinearAllocator& GraphAllocator);
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

	String128			Name;
	LinearAllocator&	Allocator;
	Texture				ColorImage;
	Texture				DepthStencilImage;
	//FrameImages			Images;
	RenderPassTable		RenderPasses;
	bool				IsCompiled;

	void BindLayoutToRenderPass(DescriptorLayout& Layout, Handle<RenderPass> PassHandle);
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H