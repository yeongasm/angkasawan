#include "SandboxRenderConfig.h"
#include "Library/Math/Math.h"
#include "Importer/Importer.h"

namespace sandbox
{
	bool CColorPass::Initialize(Ref<EngineImpl> pInEngine, Ref<IRenderSystem> pInRenderSystem, Ref<IAssetManager> pInAssetManager)
	{
		pEngine = pInEngine;
		pRenderer = pInRenderSystem;
		pAssetManager = pInAssetManager;


		IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
		const WindowInfo& windowInfo = pEngine->GetWindowInformation();

		frameGraph.Initialize({ 800, 600 });

		RenderPassCreateInfo colorPassInfo = {};
		colorPassInfo.Identifier = "Base_Color_Pass";
		colorPassInfo.Type = RenderPass_Type_Graphics;
		colorPassInfo.Depth = 1.0f;
		//colorPassInfo.Pos = windowInfo.Pos;
		colorPassInfo.Pos = { 0, 0 };
		colorPassInfo.Extent = windowInfo.Extent;

		Extent = windowInfo.Extent;

		RenderPassHnd = frameGraph.AddRenderPass(colorPassInfo);
		if (RenderPassHnd == INVALID_HANDLE) { return false; }

		//OutputAtt = frameGraph.AddColorOutput(RenderPassHnd);

		ShaderImporter importer;
		FragmentShader = importer.ImportShaderFromPath("Data/Shaders/Triangle.frag", Shader_Type_Fragment, pAssetManager);
		VertexShader = importer.ImportShaderFromPath("Data/Shaders/Triangle.vert", Shader_Type_Vertex, pAssetManager);

		if (FragmentShader == INVALID_HANDLE ||
			VertexShader == INVALID_HANDLE) { return false; }

		// Fragment shader here ...
		Ref<Shader> pFrag = pAssetManager->GetShaderWithHandle(FragmentShader);
		pFrag->Hnd = pRenderer->CreateShader(pFrag->Code, pFrag->Type);
		pFrag->Code.Release();

		if (pFrag->Hnd == INVALID_HANDLE) { return false; }

		// Fragment shader here ...
		Ref<Shader> pVert = pAssetManager->GetShaderWithHandle(VertexShader);
		pVert->Hnd = pRenderer->CreateShader(pVert->Code, pVert->Type);
		pVert->Code.Release();

		if (pVert->Hnd == INVALID_HANDLE) { return false; }

		Viewport viewport;
		viewport.x = static_cast<float32>(Pos.x);
		viewport.y = static_cast<float32>(Pos.y);
		viewport.Width = static_cast<float32>(Extent.Width);
		viewport.Height = static_cast<float32>(Extent.Height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		Rect2D scissor;
		scissor.Offset.x = 0;
		scissor.Offset.y = 0;
		scissor.Extent = Extent;

		PipelineCreateInfo colorPipelineInfo = {};
		colorPipelineInfo.VertexShaderHnd = pVert->Hnd;
		colorPipelineInfo.FragmentShaderHnd = pFrag->Hnd;
		colorPipelineInfo.BindPoint = Pipeline_Bind_Point_Graphics;
		colorPipelineInfo.CullMode = Culling_Mode_Back;
		colorPipelineInfo.FrontFace = Front_Face_Counter_Clockwise;
		colorPipelineInfo.PolygonalMode = Polygon_Mode_Fill;
		colorPipelineInfo.Samples = Sample_Count_1;
		colorPipelineInfo.Topology = Topology_Type_Triangle;
		colorPipelineInfo.Viewport = viewport;
		colorPipelineInfo.Scissor = scissor;

		PipelineHnd = pRenderer->CreatePipeline(colorPipelineInfo);
		if (PipelineHnd == INVALID_HANDLE) { return false; }

		SPipeline::VertexInputBinding vertexBindings = {};
		vertexBindings.Binding = 0;
		vertexBindings.From = 0;
		vertexBindings.To = 4;
		vertexBindings.Stride = sizeof(Vertex);
		vertexBindings.Type = Vertex_Input_Rate_Vertex;

		SPipeline::VertexInputBinding instanceBindings = {};
		instanceBindings.Binding = 1;
		instanceBindings.From = 5;
		instanceBindings.To = 8;
		instanceBindings.Stride = sizeof(math::mat4);
		instanceBindings.Type = Vertex_Input_Rate_Instance;

		pRenderer->PipelineAddVertexInputBinding(PipelineHnd, vertexBindings);
		pRenderer->PipelineAddVertexInputBinding(PipelineHnd, instanceBindings);
		pRenderer->PipelineAddRenderPass(PipelineHnd, RenderPassHnd);

		return true;
	}


	void CColorPass::Terminate()
	{
		pRenderer->DestroyPipeline(PipelineHnd);
		
		Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(FragmentShader);
		Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(VertexShader);

		pRenderer->DestroyShader(pFragment->Hnd);
		pRenderer->DestroyShader(pVertex->Hnd);

		pAssetManager->DeleteShader(FragmentShader);
		pAssetManager->DeleteShader(VertexShader);
	}

	const Handle<SRenderPass> CColorPass::GetRenderPassHandle() const
	{
		return RenderPassHnd;
	}

	const Handle<SPipeline> CColorPass::GetPipelineHandle() const
	{
		return PipelineHnd;
	}

	const Handle<Shader> CColorPass::GetShaderHandle(EShaderType Type) const
	{
		return (Type == Shader_Type_Vertex) ? VertexShader : FragmentShader;
	}

	bool RendererSetup::Initialize(Ref<EngineImpl> pInEngine, Ref<IRenderSystem> pInRenderSystem, Ref<IAssetManager> pInAssetManager)
	{
		pEngine = pInEngine;
		pRenderer = pInRenderSystem;
		pAssetManager = pInAssetManager;

		PoolHandle = pRenderer->CreateDescriptorPool();
		if (PoolHandle == INVALID_HANDLE) { return false; }

		constexpr size_t camUboSize = sizeof(CameraUbo) * 2;

		SDescriptorPool::Size dynamicUniformBuffer;
		dynamicUniformBuffer.Count = 1;
		dynamicUniformBuffer.Type = Descriptor_Type_Dynamic_Uniform_Buffer;

		//SDescriptorPool::Size bindlessTextures;
		//bindlessTextures.Count = 128;
		//bindlessTextures.Type = Descriptor_Type_Sampled_Image;

		pRenderer->DescriptorPoolAddSizeType(PoolHandle, dynamicUniformBuffer);
		//pRenderer->DescriptorPoolAddSizeType(PoolHandle, bindlessTextures);

		SetLayoutHnd = pRenderer->CreateDescriptorSetLayout();

		BufferAllocateInfo bufferInfo = {};
		bufferInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
		bufferInfo.Type.Set(Buffer_Type_Uniform);
		bufferInfo.Size = pRenderer->PadToAlignedSize(sizeof(CameraUbo)) * 2;

		CameraUboHnd = pRenderer->AllocateNewBuffer(bufferInfo);
		if (CameraUboHnd == INVALID_HANDLE) { return false; }

		DescriptorSetLayoutBindingInfo bindingInfo = {};
		bindingInfo.ShaderStages.Set(Shader_Type_Vertex);
		bindingInfo.Type = Descriptor_Type_Dynamic_Uniform_Buffer;
		bindingInfo.Stride = pRenderer->PadToAlignedSize(sizeof(CameraUbo));
		bindingInfo.DescriptorCount = 1;
		bindingInfo.BindingSlot = 0;
		bindingInfo.LayoutHnd = SetLayoutHnd;
		//layoutInfo.BufferHnd = CameraUboHnd;

		pRenderer->DescriptorSetLayoutAddBinding(bindingInfo);
		
		//layoutInfo = {};
		//layoutInfo.ShaderStages.Set(Shader_Type_Fragment);
		//layoutInfo.Type = Descriptor_Type_Sampled_Image;
		//layoutInfo.DescriptorCount = 1000;
		//layoutInfo.BindingSlot = 1;
		//layoutInfo.LayoutHnd = SetLayoutHnd;

		//pRenderer->DescriptorSetLayoutAddBinding(layoutInfo);

		DescriptorSetAllocateInfo setInfo = {};
		setInfo.LayoutHnd = SetLayoutHnd;
		setInfo.PoolHnd = PoolHandle;
		setInfo.Slot = 0;

		SetHnd = pRenderer->CreateDescriptorSet(setInfo);
		if (SetHnd == INVALID_HANDLE) { return false; }

		// Update camera ubo ...
		pRenderer->DescriptorSetMapToBuffer(SetHnd, 0, CameraUboHnd, 0, pRenderer->PadToAlignedSize(sizeof(CameraUbo)));

		if (!ColorPass.Initialize(pEngine, pRenderer, pAssetManager)) { return false; }

		pRenderer->PipelineAddDescriptorSetLayout(ColorPass.GetPipelineHandle(), SetLayoutHnd);

		return true;
	}

	const Handle<SDescriptorPool> RendererSetup::GetDescriptorPoolHandle() const
	{
		return PoolHandle;
	}

	const Handle<SDescriptorSetLayout> RendererSetup::GetDescriptorSetLayoutHandle() const
	{
		return SetLayoutHnd;
	}

	const Handle<SDescriptorSet> RendererSetup::GetDescriptorSetHandle() const
	{
		return SetHnd;
	}

	const Handle<SMemoryBuffer> RendererSetup::GetCameraUboHandle() const
	{
		return CameraUboHnd;
	}

	bool RendererSetup::Build()
	{
		IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
		if (!frameGraph.Build()) { return false; }

		pRenderer->BuildBuffer(CameraUboHnd);
		pRenderer->BuildDescriptorPool(PoolHandle);
		pRenderer->BuildDescriptorSetLayout(SetLayoutHnd);
		pRenderer->BuildDescriptorSet(SetHnd);

		Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(ColorPass.GetShaderHandle(Shader_Type_Vertex));
		Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(ColorPass.GetShaderHandle(Shader_Type_Fragment));

		if (!pRenderer->BuildShader(pFragment->Hnd)) { return false; }
		if (!pRenderer->BuildShader(pVertex->Hnd)) { return false; }
		if (!pRenderer->BuildGraphicsPipeline(ColorPass.GetPipelineHandle())) { return false; }

		return true;
	}

	void RendererSetup::Terminate()
	{
		//IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
		ColorPass.Terminate();
		//frameGraph.Terminate();
		pRenderer->DestroyBuffer(CameraUboHnd);
		pRenderer->DestroyDescriptorSet(SetHnd);
		pRenderer->DestroyDescriptorSetLayout(SetLayoutHnd);
		pRenderer->DestroyDescriptorPool(PoolHandle);
	}

	const CColorPass& RendererSetup::GetColorPass() const
	{
		return ColorPass;
	}

}