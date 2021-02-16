#include "FrameGraph.h"
#include "Renderer.h"

RenderPass::RenderPass(IRFrameGraph& OwnerGraph, ERenderPassType Type, uint32 Order) :
	Samples(Sample_Count_Max),
	Width(0.0f),
	Height(0.0f),
	Depth(0.0f),
	Order(Order),
	PassType(RenderPass_Pass_Main),
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
	DepthStencilOutput(),
	Parent(nullptr),
	Childrens(),
	BoundDescriptorLayouts()
{}

RenderPass::~RenderPass()
{
	Shaders.Release();
}

bool RenderPass::AddShader(Shader* ShaderSrc, EShaderType Type)
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

void RenderPass::SetSampleCount(ESampleCount Samples)
{
	this->Samples = Samples;
}

void RenderPass::SetTopology(ETopologyType Type)
{
	Topology = Type;
}

void RenderPass::SetCullMode(ECullingMode Mode)
{
	CullMode = Mode;
}

void RenderPass::SetPolygonMode(EPolygonMode Mode)
{
	PolygonalMode = Mode;
}

void RenderPass::SetFrontFace(EFrontFaceDir Face)
{
	FrontFace = Face;
}

bool RenderPass::AddSubpass(RenderPass& Subpass)
{
	if (Subpass.IsSubpass())
	{
		return false;
	}
	
	if (!Childrens.Length())
	{
		Childrens.Reserve(8);
	}

	for (RenderPass* subpasses : Childrens)
	{
		if (subpasses == &Subpass)
		{
			return false;
		}
	}

	Childrens.Push(&Subpass);
	Subpass.PassType = RenderPass_Pass_Sub;

	return true;
}

bool RenderPass::IsMainpass() const
{
	return PassType == RenderPass_Pass_Main;
}

bool RenderPass::IsSubpass() const
{
	return PassType == RenderPass_Pass_Sub;
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

IRFrameGraph::IRFrameGraph(RenderContext& Context, LinearAllocator& GraphAllocator) :
	Context(Context),
	Allocator(GraphAllocator),
	Name(),
	RenderPasses(),
	Images(),
	IsCompiled(false)
{}

IRFrameGraph::~IRFrameGraph()
{
	RenderPasses.Release();
}

Handle<RenderPass> IRFrameGraph::AddPass(RenderPassEnum PassIdentity, ERenderPassType Type)
{
	uint32 order = static_cast<uint32>(RenderPasses.Length());

	RenderPass* renderPass = reinterpret_cast<RenderPass*>(Allocator.Malloc(sizeof(RenderPass)));
	FMemory::InitializeObject(renderPass, *this, Type, order);
	
	RenderPasses.Insert(PassIdentity, renderPass);

	return Handle<RenderPass>(PassIdentity);
}

RenderPass& IRFrameGraph::GetRenderPass(Handle<RenderPass> Handle)
{
	VKT_ASSERT("Handle supplied into function is invalid" && Handle != INVALID_HANDLE);
	RenderPass* renderPass = RenderPasses[Handle];
	return *renderPass;
}

Handle<HImage> IRFrameGraph::GetColorImage() const
{
	return Images.Color;
}

Handle<HImage> IRFrameGraph::GetDepthStencilImage() const
{
	return Images.DepthStencil;
}

uint32 IRFrameGraph::GetNumRenderPasses() const
{
	return static_cast<uint32>(RenderPasses.Length());
}

void IRFrameGraph::OnWindowResize()
{
	Context.DestroyFrameImages(Images);
	Context.NewFrameImages(Images);

	RenderPass* renderPass = nullptr;
	for (auto& pair : RenderPasses)
	{
		renderPass = pair.Value;
		Context.DestroyRenderPassFramebuffer(*renderPass);
		Context.NewRenderPassFramebuffer(*renderPass);
		Context.DestroyGraphicsPipeline(*renderPass);
		Context.NewGraphicsPipeline(*renderPass);
	}
}

void IRFrameGraph::Destroy()
{
	for (auto& pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;
		Context.DestroyRenderPassRenderpass(*renderPass);
		Context.DestroyRenderPassFramebuffer(*renderPass, true);
		Context.DestroyGraphicsPipeline(*renderPass, true);
	}
	Context.DestroyFrameImages(Images);
}

bool IRFrameGraph::Compile()
{
	if (Compiled()) { return false; }

	Context.NewFrameImages(Images);

	for (Pair<RenderPassEnum, RenderPass*>& pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;

		if (renderPass->IsMainpass() && (renderPass->FramePassHandle == INVALID_HANDLE))
		{
			if (!Context.NewRenderPassRenderpass(*renderPass))	{ return false; }
			if (!Context.NewRenderPassFramebuffer(*renderPass)) { return false; }
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

bool IRFrameGraph::Compiled() const
{
	return IsCompiled;
}

void IRFrameGraph::BindLayoutToRenderPass(DescriptorLayout& Layout, Handle<RenderPass> PassHandle)
{
	RenderPass& renderPass = GetRenderPass(PassHandle);
	renderPass.BoundDescriptorLayouts.Push(&Layout);
}
