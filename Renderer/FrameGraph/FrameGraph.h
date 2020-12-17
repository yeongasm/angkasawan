#pragma once
#ifndef LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H
#define LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H

#include "Library/Containers/Map.h"
#include "Library/Allocators/LinearAllocator.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderPlatform/API.h"
#include "API/RendererFlagBits.h"
#include "Assets/Shader.h"

class RenderSystem;
class FrameGraph;
class RenderContext;

struct AttachmentCreateInfo
{
	AttachmentType	Type;
	TextureUsage	Usage;
	TextureType		Dimensions;
};

// NOTE(Ygsm):
// ON HOLD!!!
struct SubPass
{

};

/**
* NOTE(Ygsm):
* A render pass represents a single framebuffer / render-pass / graphics pipeline in the hardware.
*/
class RENDERER_API RenderPass
{
public:

	RenderPass(FrameGraph& OwnerGraph, RenderPassType Type);
	~RenderPass();

	DELETE_COPY_AND_MOVE(RenderPass)

	bool AddShader				(Shader* ShaderSrc, ShaderType Type);
	void AddColorInput			(const String32& Identifier, RenderPass& From);
	//void AddDepthStencilInput	(const String32& Identifier, const RenderPass& From);
	void AddColorOutput			(const String32& Identifier, const AttachmentCreateInfo& Info);
	//void AddDepthStencilOutput	(const String32& Identifier, const AttachmentInfo& Info);

	void SetWidth			(float32 Width);
	void SetHeight			(float32 Height);
	void SetDepth			(float32 Depth);
	void SetSampleCount		(SampleCount Samples);
	void SetTopology		(TopologyType Type);
	void SetCullMode		(CullingMode Mode);
	void SetPolygonMode		(PolygonMode Mode);
	void SetFrontFace		(FrontFaceDir Face);

private:

	struct RenderPassAttachment : public AttachmentCreateInfo
	{
		Handle<HImage> Handle;
	};

	friend class FrameGraph;
	friend class RenderContext;

	using OutputAttachments = Map<String32, RenderPassAttachment, MurmurHash<String32>, 1>;
	using InputAttachments	= Map<String32, RenderPassAttachment*, MurmurHash<String32>, 1>;

	FrameGraph&			Owner;
	RenderPassType		Type;

	SampleCount		Samples;
	TopologyType	Topology;
	FrontFaceDir	FrontFace;
	CullingMode		CullMode;
	PolygonMode		PolygonalMode;
	float32			Width;
	float32			Height;
	float32			Depth;

	Array<Shader*>		Shaders;
	RenderPassState		State;
	Handle<HPipeline>	PipelineHandle;
	Handle<HFramePass>	FramePassHandle;

	InputAttachments	ColorInputs;
	OutputAttachments	ColorOutputs;

};

class RENDERER_API FrameGraph
{
private:
	using PassNameEnum = uint32;
public:

	FrameGraph(RenderContext& Context, LinearAllocator& GraphAllocator);
	~FrameGraph();

	DELETE_COPY_AND_MOVE(FrameGraph)

	Handle<RenderPass>	AddPass				(PassNameEnum PassIdentity, RenderPassType Type);
	RenderPass&			GetRenderPass		(Handle<RenderPass> Handle);

	void OutputRenderPassToScreen(RenderPass& Pass);

	void Destroy();
	bool Compile();
	bool Compiled() const;

private:
	friend class RenderSystem;
	using RenderPassTable = Map<PassNameEnum, RenderPass*>;
	RenderContext&		Context;
	LinearAllocator&	Allocator;
	String128			Name;
	RenderPassTable		RenderPasses;
	bool				IsCompiled;
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H