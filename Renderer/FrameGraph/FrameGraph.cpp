#include "FrameGraph.h"
#include "Renderer.h"

RenderPass::RenderPass(FrameGraph& OwnerGraph, RenderPassType Type, uint32 Order) :
	Samples(Sample_Count_Max),
	Width(0.0f),
	Height(0.0f),
	Depth(0.0f),
	Order(Order),
	Flags(RenderPass_Bit_None),
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
	ColorOutputs(),
	DepthStencilInput(),
	DepthStencilOutput()
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
	AttachmentInfo* attachment = &From.ColorOutputs[Identifier];
	ColorInputs.Insert(Identifier, attachment);
}

void RenderPass::AddColorOutput(const String32& Identifier, const AttachmentCreateInfo& Info)
{
	AttachmentInfo attachment;
	FMemory::Memzero(&attachment, sizeof(AttachmentInfo));
	FMemory::Memcpy(&attachment, &Info, sizeof(AttachmentCreateInfo));
	ColorOutputs.Insert(Identifier, Move(attachment));
}

void RenderPass::AddDepthStencilInput(const RenderPass& From)
{
	if (Flags.Has(RenderPass_Bit_DepthStencil_Input))
	{
		return;
	}

	DepthStencilInput = From.DepthStencilInput;
	Flags.Set(RenderPass_Bit_DepthStencil_Input);
}

void RenderPass::AddDepthStencilOutput()
{
	if (Flags.Has(RenderPass_Bit_DepthStencil_Output)) 
	{ 
		return; 
	}
	
	FMemory::Memzero(&DepthStencilOutput, sizeof(AttachmentInfo));

	DepthStencilOutput.Type = Texture_Type_2D;
	DepthStencilOutput.Usage = Texture_Usage_Depth_Stencil;
	DepthStencilOutput.Handle = INVALID_HANDLE;

	Flags.Set(RenderPass_Bit_DepthStencil_Output);
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


void RenderPass::NoRender()
{
	Flags.Set(RenderPass_Bit_No_Color_Render);
	Flags.Set(RenderPass_Bit_No_DepthStencil_Render);
}

void RenderPass::NoColorRender()
{
	Flags.Set(RenderPass_Bit_No_Color_Render);
}

void RenderPass::NoDepthStencilRender()
{
	Flags.Set(RenderPass_Bit_No_DepthStencil_Render);
}

FrameGraph::FrameGraph(RenderContext& Context, LinearAllocator& GraphAllocator) :
	Context(Context),
	Allocator(GraphAllocator),
	Name(),
	RenderPasses(),
	Images(),
	IsCompiled(false)
{}

FrameGraph::~FrameGraph()
{
	RenderPasses.Release();
}

Handle<RenderPass> FrameGraph::AddPass(PassNameEnum PassIdentity, RenderPassType Type)
{
	uint32 order = static_cast<uint32>(RenderPasses.Length());

	RenderPass* renderPass = reinterpret_cast<RenderPass*>(Allocator.Malloc(sizeof(RenderPass)));
	FMemory::InitializeObject(renderPass, *this, Type, order);
	
	RenderPasses.Insert(PassIdentity, renderPass);

	return Handle<RenderPass>(PassIdentity);
}

RenderPass& FrameGraph::GetRenderPass(Handle<RenderPass> Handle)
{
	VKT_ASSERT("Handle supplied into function is invalid" && Handle != INVALID_HANDLE);
	RenderPass* renderPass = RenderPasses[Handle];
	return *renderPass;
}

Handle<HImage> FrameGraph::GetColorImage() const
{
	return Images.Color;
}

Handle<HImage> FrameGraph::GetDepthStencilImage() const
{
	return Images.DepthStencil;
}

uint32 FrameGraph::GetNumRenderPasses() const
{
	return static_cast<uint32>(RenderPasses.Length());
}

void FrameGraph::BlitToDefault()
{

}

void FrameGraph::Destroy()
{
	for (auto pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;

		Context.DestroyRenderPassRenderpass(*renderPass);
		Context.DestroyRenderPassFramebuffer(*renderPass);
		Context.DestroyPipeline(renderPass->PipelineHandle);
	}

	Context.DestroyFrameImages(Images);
}

bool FrameGraph::Compile()
{
	Context.NewFrameImages(Images);

	for (Pair<PassNameEnum, RenderPass*>& pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;

		if (renderPass->FramePassHandle == INVALID_HANDLE)
		{
			if (!Context.NewRenderPassRenderpass(*renderPass)) 
			{
				return false;
			}

			if (!Context.NewRenderPassFramebuffer(*renderPass)) 
			{ 
				return false; 
			}
		}

		if (!Context.NewGraphicsPipeline(*renderPass)) 
		{ 
			return false; 
		}
	}

	Context.CreateCmdPoolAndBuffers();
	IsCompiled = true;

	return true;
}

bool FrameGraph::Compiled() const
{
	return IsCompiled;
}