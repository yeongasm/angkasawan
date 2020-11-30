#include "Context.h"
#include "Vk/ShaderToSPIRVCompiler.h"
#include <vector>

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
	
	if (!InitializeDriver())
	{
		return false;
	}

	return true;
}

void RenderContext::TerminateContext()
{
	TerminateDriver();
}

void RenderContext::Render(DrawCommand& Command)
{
	vk::HwCmdBufferRecordInfo recordInfo;
	FMemory::InitializeObject(recordInfo);

	RenderPass& renderPass = *Command.BindedPass;

	recordInfo.CommandBufferHandle = &renderPass.CmdBufferHandle;
	recordInfo.PipelineHandle = &renderPass.PipelineHandle;
	recordInfo.ClearColor[Color_Channel_Red]	= 0.0f;
	recordInfo.ClearColor[Color_Channel_Green]	= 0.0f;
	recordInfo.ClearColor[Color_Channel_Blue]	= 0.0f;
	recordInfo.ClearColor[Color_Channel_Alpha]	= 1.0f;

	RecordCommandBuffer(recordInfo);
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
			printf("%s\n\n", shaderCompiler.GetLastErrorMessage());
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

		shaderInfo.Handle = shader->Handle;
		shaderInfo.Type	  = shader->Type;

		pipelineCreateInfo.Shaders.Push(shaderInfo);
	}

	pipelineCreateInfo.CullMode = Pass.CullMode;
	pipelineCreateInfo.FrontFace = Pass.FrontFace;
	pipelineCreateInfo.Handle	= &Pass.PipelineHandle;
	pipelineCreateInfo.PolyMode = Pass.PolygonalMode;
	pipelineCreateInfo.Topology = Pass.Topology;

	if (!CreatePipeline(pipelineCreateInfo))
	{
		// TODO(Ygsm):
		// Include a logging system.
		return false;
	}

	Pass.State = RenderPass_State_Built;

	return true;
}

bool RenderContext::RegisterRenderPassCmdBuffer(RenderPass& Pass)
{
	vk::HwCmdBufferAllocInfo cmdBufferAllocInfo;
	FMemory::InitializeObject(cmdBufferAllocInfo);

	cmdBufferAllocInfo.Handle = &Pass.CmdBufferHandle;

	if (!AddCommandBufferEntry(cmdBufferAllocInfo))
	{
		// TODO(Ygsm):
		// Include a logging system.
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