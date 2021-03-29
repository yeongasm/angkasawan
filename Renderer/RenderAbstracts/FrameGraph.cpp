#include "FrameGraph.h"
#include "Renderer.h"

RenderPass::RenderPass(IRFrameGraph& OwnerGraph, ERenderPassType Type, uint32 Order) :
	//Samples(Sample_Count_Max),
	Width(0.0f),
	Height(0.0f),
	Depth(0.0f),
	Order(Order),
	PassType(RenderPass_Pass_Main),
	Flags(RenderPass_Bit_None),
	//VertexStride(0),
	Owner(OwnerGraph), 
	Type(Type),
	//Topology(Topology_Type_Triangle),
	//FrontFace(Front_Face_Clockwise),
	//CullMode(Culling_Mode_None),
	//PolygonalMode(Polygon_Mode_Fill),
	//Shaders(), 
	State(RenderPass_State_New),
	//PipelineHandle(INVALID_HANDLE),
	RenderpassHandle(INVALID_HANDLE),
	FramebufferHandle(INVALID_HANDLE),
	ColorInputs(),
	ColorOutputs(),
	DepthStencilInput(),
	DepthStencilOutput()//,
	//Parent(nullptr),
	//VertexBindings(),
	//Childrens(),
	//BoundDescriptorLayouts()
{}

RenderPass::~RenderPass()
{
}

//bool RenderPass::AddShader(Shader* ShaderSrc)
//{
//	if (ShaderSrc->Handle == INVALID_HANDLE)
//	{
//		return false;
//	}
//	Shaders[ShaderSrc->Type] = ShaderSrc;
//	return true;
//}

//bool RenderPass::AddPipeline(Shader* VertexShader, Shader* FragmentShader)
//{
//	if (!VertexShader || !FragmentShader)
//	{
//		return false;
//	}
//
//	if (VertexShader->Handle == INVALID_HANDLE ||
//		FragmentShader->Handle == INVALID_HANDLE)
//	{
//		return false;
//	}
//
//	Shaders[Shader_Type_Vertex] = VertexShader;
//	Shaders[Shader_Type_Fragment] = FragmentShader;
//
//	return true;
//}

//bool RenderPass::AddVertexInputBinding(uint32 Binding, uint32 From, uint32 To, uint32 Stride, EVertexInputRateType Type)
//{
//	for (const VertexInputBinding& vtxInputBinding : VertexBindings)
//	{
//		if (vtxInputBinding.Binding == Binding)
//		{
//			return false;
//		}
//	}
//	VertexBindings.Push({ Binding, From, To, Stride, Type });
//	return true;
//}

void RenderPass::AddColorInput(const String32& Identifier, RenderPass& From)
{
	AttachmentInfo& attachment = From.ColorOutputs[Identifier];
	ColorInputs.Insert(Identifier, attachment);
}

void RenderPass::AddColorOutput(const String32& Identifier, const AttachmentCreateInfo& CreateInfo)
{
	AttachmentInfo attachment;
	FMemory::Memcpy(&attachment, &CreateInfo, sizeof(AttachmentCreateInfo));
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

//void RenderPass::SetVertexStride(uint32 Stride)
//{
//	this->VertexStride = Stride;
//}

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

//void RenderPass::SetSampleCount(ESampleCount Samples)
//{
//	this->Samples = Samples;
//}

//void RenderPass::SetTopology(ETopologyType Type)
//{
//	Topology = Type;
//}

//void RenderPass::SetCullMode(ECullingMode Mode)
//{
//	CullMode = Mode;
//}

//void RenderPass::SetPolygonMode(EPolygonMode Mode)
//{
//	PolygonalMode = Mode;
//}

//void RenderPass::SetFrontFace(EFrontFaceDir Face)
//{
//	FrontFace = Face;
//}

//bool RenderPass::AddSubpass(RenderPass& Subpass)
//{
//	if (Subpass.IsSubpass())
//	{
//		return false;
//	}
//	
//	if (!Childrens.Length())
//	{
//		Childrens.Reserve(8);
//	}
//
//	for (RenderPass* subpasses : Childrens)
//	{
//		if (subpasses == &Subpass)
//		{
//			return false;
//		}
//	}
//
//	Childrens.Push(&Subpass);
//	Subpass.PassType = RenderPass_Pass_Sub;
//
//	return true;
//}

//bool RenderPass::IsMainpass() const
//{
//	return PassType == RenderPass_Pass_Main;
//}

//bool RenderPass::IsSubpass() const
//{
//	return PassType == RenderPass_Pass_Sub;
//}

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

IRFrameGraph::IRFrameGraph(LinearAllocator& GraphAllocator) :
	Allocator(GraphAllocator),
	Name(),
	RenderPasses(),
	ColorImage(),
	DepthStencilImage(),
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
	return ColorImage.Handle;
}

Handle<HImage> IRFrameGraph::GetDepthStencilImage() const
{
	return DepthStencilImage.Handle;
}

uint32 IRFrameGraph::GetNumRenderPasses() const
{
	return static_cast<uint32>(RenderPasses.Length());
}

void IRFrameGraph::SetOutputExtent(uint32 Width, uint32 Height)
{
	uint32 width = (!Width) ? gpu::SwapchainWidth() : Width;
	uint32 height = (!Height) ? gpu::SwapchainHeight() : Height;

	ColorImage.Width = width;
	ColorImage.Height = height;
	DepthStencilImage.Width = width;
	DepthStencilImage.Height = height;
}

void IRFrameGraph::OnWindowResize()
{
	gpu::DestroyTexture(ColorImage);
	gpu::DestroyTexture(DepthStencilImage);
	gpu::CreateTexture(ColorImage);
	gpu::CreateTexture(DepthStencilImage);

	RenderPass* renderPass = nullptr;
	for (auto& pair : RenderPasses)
	{
		renderPass = pair.Value;
		gpu::DestroyFramebuffer(*renderPass);
		gpu::CreateFramebuffer(*renderPass);
	}
}

void IRFrameGraph::Destroy()
{
	for (auto& pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;
		gpu::DestroyRenderpass(*renderPass);
		gpu::DestroyFramebuffer(*renderPass);
	}
	gpu::DestroyTexture(ColorImage);
	gpu::DestroyTexture(DepthStencilImage);
}

bool IRFrameGraph::Compile()
{
	if (!RenderPasses.Length()) { return false; }
	if (Compiled()) { return false; }

	ColorImage.Channels = 4;
	ColorImage.Usage.Set(Image_Usage_Transfer_Src);
	ColorImage.Usage.Set(Image_Usage_Sampled);

	DepthStencilImage = ColorImage;

	ColorImage.Usage.Set(Image_Usage_Color_Attachment);
	DepthStencilImage.Usage.Set(Image_Usage_Depth_Stencil_Attachment);

	gpu::CreateTexture(ColorImage);
	gpu::CreateTexture(DepthStencilImage);

	for (Pair<RenderPassEnum, RenderPass*>& pair : RenderPasses)
	{
		RenderPass* renderPass = pair.Value;

		if (!gpu::CreateRenderpass(*renderPass))  { return false; }
		if (!gpu::CreateFramebuffer(*renderPass)) { return false; }
	}

	IsCompiled = true;

	return true;
}

bool IRFrameGraph::Compiled() const
{
	return IsCompiled;
}

//void IRFrameGraph::BindLayoutToRenderPass(DescriptorLayout& Layout, Handle<RenderPass> PassHandle)
//{
//	RenderPass& renderPass = GetRenderPass(PassHandle);
//	renderPass.BoundDescriptorLayouts.Push(&Layout);
//	Layout.Pipeline = &renderPass.PipelineHandle;
//}