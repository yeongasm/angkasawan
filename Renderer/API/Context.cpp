#include "Context.h"
#include "Vk/ShaderToSPIRVCompiler.h"

struct RenderContextGlobalVariables
{
	Array<uint32> ShaderBinaries;
	using ConstShaderTypes = const shaderc_shader_kind;
	ConstShaderTypes ShaderTypes[4] = { shaderc_vertex_shader, shaderc_fragment_shader, shaderc_compute_shader, shaderc_geometry_shader };
} GlobalVars;

RenderContext::RenderContext() : GraphicsDriver() {}

RenderContext::~RenderContext() {}

bool RenderContext::InitializeContext()
{
	if (!InitializeDriver()) { return false; }
	return true;
}

void RenderContext::TerminateContext()
{
	TerminateDriver();
}

void RenderContext::BindRenderPass(RenderPass& Pass)
{
	vk::HwCmdBufferRecordInfo recordInfo = {};
	recordInfo.FramePassHandle	= Pass.FramePassHandle;
	recordInfo.PipelineHandle	= Pass.PipelineHandle;

	RecordCommandBuffer(recordInfo);
}

void RenderContext::UnbindRenderPass(RenderPass& Pass)
{
	vk::HwCmdBufferRecordInfo recordInfo = {};
	recordInfo.FramePassHandle = Pass.FramePassHandle;
	recordInfo.PipelineHandle = Pass.PipelineHandle;

	UnrecordCommandBuffer(recordInfo);
	PushCmdBufferForSubmit(Pass.FramePassHandle);
}

void RenderContext::SubmitForRender(const Drawable& DrawObj, RenderPass& Pass)
{
	vk::HwDrawInfo drawInfo = {};

	drawInfo.Vbo				= DrawObj.Vbo;
	drawInfo.Ebo				= DrawObj.Ebo;
	drawInfo.VertexOffset		= DrawObj.VertexOffset;
	drawInfo.IndexOffset		= DrawObj.IndexOffset;
	drawInfo.FramePassHandle	= Pass.FramePassHandle;
	
	// NOTE(Ygsm):
	// Indexed draw is not implemented yet.
	Draw(drawInfo);
}

bool RenderContext::NewFrameImages(FrameImages& Images)
{
	vk::HwImageCreateStruct image = {};

	image.Handle		= &Images.Color;
	image.Width			= SwapChain.Extent.width;
	image.Height		= SwapChain.Extent.height;
	image.MipLevels		= 1;
	image.Depth			= 1;
	image.Samples		= Sample_Count_1;
	image.Type			= Texture_Type_2D;
	image.Usage			= Texture_Usage_Color;
	image.UsageFlagBits = Image_Usage_Color_Attachment | Image_Usage_Transfer_Src | Image_Usage_Sampled;

	if (!CreateImage(image)) { return false; }

	image.Handle		= &Images.DepthStencil;
	image.Usage			= Texture_Usage_Depth_Stencil;
	image.UsageFlagBits = Image_Usage_Depth_Stencil_Attachment | Image_Usage_Sampled;

	if (!CreateImage(image)) { return false; }

	return true;
}

void RenderContext::DestroyFrameImages(FrameImages& Images)
{
	DestroyImage(Images.Color);
	DestroyImage(Images.DepthStencil);
}

bool RenderContext::NewGraphicsPipeline(RenderPass& Pass)
{
	//if (!ValidateGraphicsPipelineCreateInfo(CreateInfo))
	//{
	//	return false;
	//}

	vk::HwPipelineCreateInfo pipelineCreateInfo;

	// Compile shaders if they're not compiled.
	vk::ShaderToSPIRVCompiler shaderCompiler;

	for (Shader* shader : Pass.Shaders)
	{
		if (shader->Handle != -1)
		{
			continue;
		}

		GlobalVars.ShaderBinaries.Empty();;
		if (!shaderCompiler.CompileShader(
			shader->Name.C_Str(),
			GlobalVars.ShaderTypes[shader->Type],
			shader->Code.C_Str(),
			GlobalVars.ShaderBinaries
		))
		{
			// TODO(Ygsm):
			// Include logging system.
			// printf("%s\n\n", shaderCompiler.GetLastErrorMessage());
			return false;
		}

		vk::HwShaderCreateInfo createInfo;
		FMemory::InitializeObject(&createInfo);

		createInfo.DWordBuf = &GlobalVars.ShaderBinaries;
		createInfo.Handle = &shader->Handle;

		if (!CreateShader(createInfo))
		{
			return false;
		}

		vk::HwPipelineCreateInfo::ShaderInfo shaderInfo;
		FMemory::InitializeObject(shaderInfo);

		shaderInfo.Handle			= shader->Handle;
		shaderInfo.Type				= shader->Type;
		shaderInfo.Attributes		= shader->Attributes.First();
		shaderInfo.AttributeCount	= shader->Attributes.Length();

		pipelineCreateInfo.Shaders.Push(shaderInfo);
	}

	pipelineCreateInfo.VertexStride		= sizeof(Vertex);
	pipelineCreateInfo.FramePassHandle	= Pass.FramePassHandle;
	pipelineCreateInfo.Handle			= &Pass.PipelineHandle;
	pipelineCreateInfo.CullMode			= Pass.CullMode;
	pipelineCreateInfo.FrontFace		= Pass.FrontFace;
	pipelineCreateInfo.PolyMode			= Pass.PolygonalMode;
	pipelineCreateInfo.Topology			= Pass.Topology;
	pipelineCreateInfo.HasDepthStencil	= Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output) || !Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render);

	if (!CreatePipeline(pipelineCreateInfo))
	{
		// TODO(Ygsm):
		// Include a logging system.
		return false;
	}

	for (Shader* shader : Pass.Shaders)
	{
		DestroyShader(shader->Handle);
	}

	Pass.State = RenderPass_State_Built;

	return true;
}

bool RenderContext::NewRenderPassFramebuffer(RenderPass& Pass)
{
	vk::HwFramebufferCreateInfo framebufferCreateInfo = {};

	framebufferCreateInfo.Width		= Pass.Width;
	framebufferCreateInfo.Height	= Pass.Height;
	framebufferCreateInfo.Depth		= Pass.Depth;
	framebufferCreateInfo.Samples	= Pass.Samples;
	framebufferCreateInfo.Handle	= Pass.FramePassHandle;

	framebufferCreateInfo.DefaultOutputs[vk::Default_Output_Color]			= Pass.Owner.GetColorImage();
	framebufferCreateInfo.DefaultOutputs[vk::Default_Output_DepthStencil]	= Pass.Owner.GetDepthStencilImage();

	if (Pass.Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		framebufferCreateInfo.DefaultOutputs[vk::Default_Output_Color] = INVALID_HANDLE;
	}

	if (Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
		framebufferCreateInfo.DefaultOutputs[vk::Default_Output_DepthStencil] = INVALID_HANDLE;
	}

	uint32 i = 0;

	for (auto& pair : Pass.ColorOutputs)
	{
		auto& passAttachment = pair.Value;
		vk::HwAttachmentInfo& colorAttachment = framebufferCreateInfo.Outputs[i++];
		FMemory::InitializeObject(colorAttachment);

		colorAttachment.Handle		= &passAttachment.Handle;
		colorAttachment.Type		= passAttachment.Type;
		colorAttachment.Usage		= passAttachment.Usage;

		colorAttachment.UsageFlagBits = Image_Usage_Color_Attachment | Image_Usage_Sampled;
	}

	framebufferCreateInfo.NumColorOutputs = i;


	if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output))
	{
		vk::HwAttachmentInfo& depthStencil = framebufferCreateInfo.Outputs[i++];

		depthStencil.Handle	= &Pass.DepthStencilOutput.Handle;
		depthStencil.Type = Pass.DepthStencilOutput.Type;
		depthStencil.Usage = Pass.DepthStencilOutput.Usage;
		depthStencil.UsageFlagBits	= Image_Usage_Depth_Stencil_Attachment | Image_Usage_Sampled;
	}

	framebufferCreateInfo.NumOutputs = i;

	return CreateFramebuffer(framebufferCreateInfo);
}

void RenderContext::DestroyRenderPassFramebuffer(RenderPass& Pass)
{
	for (auto& pair : Pass.ColorOutputs)
	{
		DestroyImage(pair.Value.Handle);
	}

	if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output))
	{
		DestroyImage(Pass.DepthStencilOutput.Handle);
	}

	DestroyFramebuffer(Pass.FramePassHandle);
}

bool RenderContext::NewRenderPassRenderpass(RenderPass& Pass)
{
	const uint32 finalOrder = Pass.Owner.GetNumRenderPasses() - 1;

	vk::HwRenderpassCreateInfo createInfo = {};

	createInfo.Handle = &Pass.FramePassHandle;
	createInfo.Samples = Pass.Samples;
	createInfo.NumColorOutputs = static_cast<uint32>(Pass.ColorOutputs.Length());
	createInfo.DefaultOutputs  = Pass.Flags;
	createInfo.HasDepthStencilAttachment = Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output);

	if (!Pass.Order)
	{
		createInfo.Order = RenderPass_Order_First;
	}

	if (Pass.Order == finalOrder)
	{
		createInfo.Order = RenderPass_Order_Last;
	}
	else
	{
		createInfo.Order = RenderPass_Order_InBetween;
	}
	
	return CreateRenderPass(createInfo);
}

void RenderContext::DestroyRenderPassRenderpass(RenderPass& Pass)
{
	DestroyRenderPass(Pass.FramePassHandle);
}

bool RenderContext::CreateCmdPoolAndBuffers()
{
	if (!CreateCommandPool())		{ return false; }
	if (!AllocateCommandBuffers())	{ return false; }
	return true;
}

bool RenderContext::NewVertexBuffer(Handle<HBuffer>& Vbo, void* Data, size_t Count)
{
	//if (Vbo != 0 && Vbo != INVALID_HANDLE) { return true; }

	vk::HwBufferCreateInfo vboCreateInfo;

	vboCreateInfo.Handle = &Vbo;
	vboCreateInfo.Data	= Data;
	vboCreateInfo.Count = Count;
	vboCreateInfo.Size = sizeof(Vertex);
	vboCreateInfo.Type = Buffer_Type_Vertex;

	if (!CreateBuffer(vboCreateInfo)) { return false; }

	return true;
}

bool RenderContext::NewIndexBuffer(Handle<HBuffer>& Ebo, void* Data, size_t Count)
{
	//if (Ebo != 0 && Ebo != INVALID_HANDLE) { return true; }

	vk::HwBufferCreateInfo eboCreateInfo;

	eboCreateInfo.Handle = &Ebo;
	eboCreateInfo.Data	= Data;
	eboCreateInfo.Count = Count;
	eboCreateInfo.Size = sizeof(uint32);
	eboCreateInfo.Type = Buffer_Type_Index;

	if (!CreateBuffer(eboCreateInfo)) { return false; }

	return true;
}

//Handle<HFramePass> RenderContext::GetDefaultFramebuffer() const
//{
//	return DefaultFramebuffer;
//}

void RenderContext::FinalizeRenderPass(RenderPass& Pass)
{
	PushCmdBufferForSubmit(Pass.FramePassHandle);
}

//bool RenderContext::ValidateGraphicsPipelineCreateInfo(const GraphicsPipelineCreateInfo& CreateInfo)
//{
//	bool valid = true;
//	if (!CreateInfo.VertexShader || 
//		!CreateInfo.FragmentShader)
//	{
//		valid = false;
//	}
//
//	return valid;
//}