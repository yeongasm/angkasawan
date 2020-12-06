#include "FrameGraph.h"
#include "Renderer.h"

RenderPass::RenderPass(FrameGraph& OwnerGraph, RenderPassType Type) :
	Owner(OwnerGraph), 
	Type(Type), 
	Shaders(), 
	State(RenderPass_State_New)
{}

RenderPass::~RenderPass()
{
	Shaders.Release();
}

bool RenderPass::AddShader(Shader* ShaderSrc, ShaderType Type)
{
	for (Shader* shader : Shaders)
	{
		if (shader->Type == Type)
		{
			return false;
		}

		if (shader == ShaderSrc)
		{
			return false;
		}
	}

	Shaders.Push(ShaderSrc);

	return true;
}

//void RenderPass::AddOutputAttachment(const String32& Identifier, const AttachmentInfo& Info)
//{
//	OutAttachments.Insert(Identifier, Info);
//}
//
//void RenderPass::AddInputAttachment(const String32& Identifier, const RenderPass& From)
//{
//	const AttachmentInfo& attachment = From.OutAttachments[Identifier];
//	InAttachments.Insert(Identifier, &attachment);
//}

void RenderPass::SetTopology(TopologyType Type)
{
	Topology = Type;
}

void RenderPass::SetCullMode(CullingMode Mode)
{
	CullMode = Mode;
}

void RenderPass::SetPolygonMode(PolygonMode Mode)
{
	PolygonalMode = Mode;
}

void RenderPass::SetFrontFace(FrontFaceDir Face)
{
	FrontFace = Face;
}


FrameGraph::FrameGraph(RenderContext& Context, LinearAllocator& GraphAllocator) :
	Context(Context), Allocator(GraphAllocator), Name(), RenderPasses()
{}

FrameGraph::~FrameGraph()
{
	RenderPasses.Release();
}

Handle<RenderPass> FrameGraph::AddPass(PassNameEnum PassIdentity, RenderPassType Type)
{
	RenderPass* renderPass = reinterpret_cast<RenderPass*>(Allocator.Malloc(sizeof(RenderPass)));
	new (renderPass) RenderPass(*this, Type);

	RenderPasses.Insert(PassIdentity, renderPass);
	Context.RegisterRenderPassCmdBuffer(*renderPass);

	return Handle<RenderPass>(PassIdentity);
}

RenderPass& FrameGraph::GetRenderPass(Handle<RenderPass> Handle)
{
	VKT_ASSERT("Handle supplied into function is invalid" && Handle != INVALID_HANDLE);
	// NOTE(Ygsm):
	// Don't need to check if render pass exist because the container would throw an assertion.
	RenderPass* renderPass = RenderPasses[Handle];
	return *renderPass;
}

void FrameGraph::Destroy()
{
	for (auto pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;
		for (Shader* shader : renderPass->Shaders)
		{
			Context.DestroyShader(shader->Handle);
		}
		Context.DestroyPipeline(renderPass->PipelineHandle);
		Context.FreeCommandBuffer(renderPass->CmdBufferHandle);
	}
}

bool FrameGraph::Compile()
{
	for (Pair<PassNameEnum, RenderPass*>& pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;
		if (!Context.NewGraphicsPipeline(*renderPass))
		{
			// TODO(Ygsm):
			// Include a logger.
			return false;
		}
	}
	Context.CreateCmdPoolAndBuffers();
	return true;
}