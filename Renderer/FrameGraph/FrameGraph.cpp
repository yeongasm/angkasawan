#include "FrameGraph.h"
#include "Renderer.h"

RenderPass::RenderPass(FrameGraph& OwnerGraph, RenderPassType Type) :
	Samples(Sample_Count_Max),
	Width(0.0f),
	Height(0.0f),
	Depth(0.0f),
	Owner(OwnerGraph), 
	Type(Type),
	Topology(Topology_Type_Triangle),
	FrontFace(Front_Face_Clockwise),
	CullMode(Culling_Mode_None),
	PolygonalMode(Polygon_Mode_Fill),
	Shaders(), 
	State(RenderPass_State_New),
	PipelineHandle(INVALID_HANDLE),
	FramePassHandle(INVALID_HANDLE),
	ColorInputs(),
	ColorOutputs()
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

void RenderPass::AddColorInput(const String32& Identifier, RenderPass& From)
{
	RenderPassAttachment* attachment = &From.ColorOutputs[Identifier];
	ColorInputs.Insert(Identifier, attachment);
}

void RenderPass::AddColorOutput(const String32& Identifier, const AttachmentCreateInfo& Info)
{
	RenderPassAttachment attachment;
	FMemory::InitializeObject(attachment);
	FMemory::Memcpy(&attachment, &Info, sizeof(AttachmentCreateInfo));
	ColorOutputs.Insert(Identifier, Move(attachment));
}

void RenderPass::SetWidth(float32 Width)
{
	this->Width = Width;
}

void RenderPass::SetHeight(float32 Height)
{
	this->Height = Height;
}

void RenderPass::SetDepth(float32 Depth)
{
	this->Depth = Depth;
}

void RenderPass::SetSampleCount(SampleCount Samples)
{
	this->Samples = Samples;
}

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
	Context(Context),
	Allocator(GraphAllocator),
	Name(),
	RenderPasses(),
	//PresentationImage(nullptr),
	IsCompiled(false)
{}

FrameGraph::~FrameGraph()
{
	RenderPasses.Release();
}

Handle<RenderPass> FrameGraph::AddPass(PassNameEnum PassIdentity, RenderPassType Type)
{
	RenderPass* renderPass = reinterpret_cast<RenderPass*>(Allocator.Malloc(sizeof(RenderPass)));
	FMemory::InitializeObject(renderPass, *this, Type);
	//new (renderPass) RenderPass(*this, Type);

	RenderPasses.Insert(PassIdentity, renderPass);

	return Handle<RenderPass>(PassIdentity);
}

RenderPass& FrameGraph::GetRenderPass(Handle<RenderPass> Handle)
{
	VKT_ASSERT("Handle supplied into function is invalid" && Handle != INVALID_HANDLE);
	RenderPass* renderPass = RenderPasses[Handle];
	return *renderPass;
}

void FrameGraph::OutputRenderPassToScreen(RenderPass& Pass)
{
	Pass.FramePassHandle = Context.GetDefaultFramebuffer();
}

void FrameGraph::Destroy()
{
	for (auto pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;
		for (auto& pair : renderPass->ColorOutputs)
		{
			auto& attachment = pair.Value;
			Context.DestroyImage(attachment.Handle);
		}
		for (Shader* shader : renderPass->Shaders)
		{
			Context.DestroyShader(shader->Handle);
		}
		Context.DestroyPipeline(renderPass->PipelineHandle);
		Context.DestroyRenderPass(renderPass->FramePassHandle);
		Context.DestroyFramebuffer(renderPass->FramePassHandle);
	}
}

bool FrameGraph::Compile()
{
	//
	// TODO(Ygsm): 
	// Include a logger.
	//
	for (Pair<PassNameEnum, RenderPass*>& pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;

		if (renderPass->FramePassHandle == INVALID_HANDLE)
		{
			if (!Context.NewRenderPassFramebuffer(*renderPass)) 
			{ 
				return false; 
			}
		}

		if (!Context.NewGraphicsPipeline(*renderPass))		{ return false; }
	}
	Context.CreateCmdPoolAndBuffers();
	IsCompiled = true;
	return true;
}

bool FrameGraph::Compiled() const
{
	return IsCompiled;
}