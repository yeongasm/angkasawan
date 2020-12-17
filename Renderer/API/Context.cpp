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
	drawInfo.VertexCount		= DrawObj.VertexCount;
	drawInfo.IndexCount			= DrawObj.IndexCount;
	drawInfo.FramePassHandle	= Pass.FramePassHandle;
	
	// NOTE(Ygsm):
	// Indexed draw is not implemented yet.
	Draw(drawInfo);
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

	if (!CreatePipeline(pipelineCreateInfo))
	{
		// TODO(Ygsm):
		// Include a logging system.
		return false;
	}

	Pass.State = RenderPass_State_Built;

	return true;
}

bool RenderContext::NewRenderPassFramebuffer(RenderPass& Pass)
{
	vk::HwFramebufferCreateInfo framebufferCreateInfo;
	FMemory::InitializeObject(framebufferCreateInfo);

	framebufferCreateInfo.Width		= Pass.Width;
	framebufferCreateInfo.Height	= Pass.Height;
	framebufferCreateInfo.Depth		= Pass.Depth;
	framebufferCreateInfo.Samples	= Pass.Samples;
	framebufferCreateInfo.Handle	= &Pass.FramePassHandle;

	size_t i = 0;
	for (auto& pair : Pass.ColorOutputs)
	{
		auto& passAttachment = pair.Value;
		vk::HwAttachmentInfo& colorAttachment = framebufferCreateInfo.ColorAttachments[i];
		FMemory::InitializeObject(colorAttachment);

		colorAttachment.Handle		= &passAttachment.Handle;
		colorAttachment.Dimension	= passAttachment.Dimensions;
		colorAttachment.Type		= passAttachment.Type;
		colorAttachment.Usage		= passAttachment.Usage;

		i++;
		framebufferCreateInfo.NumColorAttachments++;
	}

	if (!CreateFramebuffer(framebufferCreateInfo)) 
	{ 
		return false; 
	}

	return true;
}

bool RenderContext::CreateCmdPoolAndBuffers()
{
	if (!CreateCommandPool())		{ return false; }
	if (!AllocateCommandBuffers())	{ return false; }
	return true;
}

bool RenderContext::NewVertexBuffer(Mesh& InMesh)
{
	if (InMesh.Vbo != 0 && InMesh.Vbo != INVALID_HANDLE)
	{
		return true;
	}

	vk::HwVertexBufferCreateInfo vboCreateInfo;

	vboCreateInfo.Handle	= &InMesh.Vbo;
	vboCreateInfo.Data		= InMesh.Vertices.First();
	vboCreateInfo.Size		= InMesh.Vertices.Length() * sizeof(Vertex);

	if (!CreateVertexBuffer(vboCreateInfo))
	{
		return false;
	}

	return true;
}

Handle<HFramePass> RenderContext::GetDefaultFramebuffer() const
{
	return DefaultFramebuffer;
}

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