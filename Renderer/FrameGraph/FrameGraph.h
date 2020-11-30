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

// Most games only use a single subpass in a renderpass.

struct AttachmentInfo
{
	AttachmentUsage		Usage;
	AttachmentType		Type;
	AttachmentDimension	Dimension;
	SampleCount			Samples;
	float32				Width;
	float32				Height;
	Handle<HImage>		ImageHandle;
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

	bool AddShader			(Shader* ShaderSrc, ShaderType Type);
	void AddOutputAttachment(const String32& Identifier, const AttachmentInfo& Info);
	void AddInputAttachment	(const String32& Identifier, const RenderPass& From);

	void SetTopology		(TopologyType Type);
	void SetCullMode		(CullingMode Mode);
	void SetPolygonMode		(PolygonMode Mode);
	void SetFrontFace		(FrontFaceDir Face);

private:
	friend class FrameGraph;
	friend class RenderContext;

	using ConstAttachment = const AttachmentInfo;

	FrameGraph&			Owner;
	RenderPassType		Type;

	TopologyType		Topology;
	FrontFaceDir		FrontFace;
	CullingMode			CullMode;
	PolygonMode			PolygonalMode;

	Array<Shader*>		Shaders;
	RenderPassState		State;
	Handle<HPipeline>	PipelineHandle;
	Handle<HCmdBuffer>	CmdBufferHandle;
	Handle<HFrameParam> FramePrmHandle;

	Map<String32, AttachmentInfo>	OutAttachments;
	Map<String32, ConstAttachment*>	InAttachments;
};

class RENDERER_API FrameGraph
{
private:
	using PassNameEnum = uint32;
public:

	FrameGraph(RenderContext& Context, LinearAllocator& GraphAllocator);
	~FrameGraph();

	DELETE_COPY_AND_MOVE(FrameGraph)

	Handle<RenderPass>	AddPass(PassNameEnum PassIdentity, RenderPassType Type);
	RenderPass&			GetRenderPass(Handle<RenderPass> Handle);

	void Destroy();
	bool Compile();

private:
	friend class RenderSystem;
	using RenderPassTable = Map<PassNameEnum, RenderPass*>;
	RenderContext&		Context;
	LinearAllocator&	Allocator;
	String128			Name;
	RenderPassTable		RenderPasses;
};

#endif // !LEARNVK_RENDERER_RENDERGRAPH_RENDER_GRAPH_H