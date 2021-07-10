#include "SandboxRenderConfig.h"
#include "Library/Math/Math.h"
#include "Importer/Importer.h"

namespace sandbox
{
	bool CColorPass::Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager)
	{
		pEngine = pInEngine;
		pRenderer = pInRenderSystem;
		pAssetManager = pInAssetManager;

		IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
		const WindowInfo& windowInfo = pEngine->GetWindowInformation();

		RenderPassCreateInfo colorPassInfo = {};
		colorPassInfo.Identifier = "Base_Color_Pass";
		colorPassInfo.Type = RenderPass_Type_Graphics;
		colorPassInfo.Depth = 1.0f;
		//colorPassInfo.Pos = windowInfo.Pos;
		colorPassInfo.Pos = { 0, 0 };
		colorPassInfo.Extent = { 800, 600 };

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
		astl::Ref<Shader> pFrag = pAssetManager->GetShaderWithHandle(FragmentShader);
		pFrag->Hnd = pRenderer->CreateShader(pFrag->Code, pFrag->Type);
		pFrag->Code.Release();

		if (pFrag->Hnd == INVALID_HANDLE) { return false; }

		// Fragment shader here ...
		astl::Ref<Shader> pVert = pAssetManager->GetShaderWithHandle(VertexShader);
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
    colorPipelineInfo.SrcColorBlendFactor = Blend_Factor_One;
    colorPipelineInfo.DstColorBlendFactor = Blend_Factor_Zero;
    colorPipelineInfo.ColorBlendOp = Blend_Op_Add;
    colorPipelineInfo.SrcAlphaBlendFactor = Blend_Factor_One;
    colorPipelineInfo.DstAlphaBlendFactor = Blend_Factor_Zero;
    colorPipelineInfo.AlphaBlendOp = Blend_Op_Add;

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
		
		astl::Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(FragmentShader);
		astl::Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(VertexShader);

		pRenderer->DestroyShader(pFragment->Hnd);
		pRenderer->DestroyShader(pVertex->Hnd);

		pAssetManager->DestroyShader(FragmentShader);
		pAssetManager->DestroyShader(VertexShader);
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

	bool RendererSetup::Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager)
	{
		pEngine = pInEngine;
		pRenderer = pInRenderSystem;
		pAssetManager = pInAssetManager;

    // Create global descriptor pool.
		PoolHandle = pRenderer->CreateDescriptorPool();
		if (PoolHandle == INVALID_HANDLE) { return false; }

		constexpr size_t camUboSize = sizeof(CameraUbo) * 2;

    // Register dynamic uniform buffer for camera information into descriptor pool.
		SDescriptorPool::Size dynamicUniformBuffer;
		dynamicUniformBuffer.Count = 1;
		dynamicUniformBuffer.Type = Descriptor_Type_Dynamic_Uniform_Buffer;

    // Register bindless textures into descriptor pool.
    // Textures that are binded to a descriptor set allocated from this pool.
		SDescriptorPool::Size bindlessTextures;
		bindlessTextures.Count = 130; // We only need 129 but added an extra one in just in case.
		bindlessTextures.Type = Descriptor_Type_Sampled_Image;

		pRenderer->DescriptorPoolAddSizeType(PoolHandle, dynamicUniformBuffer);
		pRenderer->DescriptorPoolAddSizeType(PoolHandle, bindlessTextures);

    // Create a global descriptor set layout.
		SetLayoutHnd = pRenderer->CreateDescriptorSetLayout();

    // Allocate a buffer to store camera information.
    // Buffer is twice the size of the struct since no. of frames in flights are taken into account.
		BufferAllocateInfo bufferInfo = {};
		bufferInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
		bufferInfo.Type.Set(Buffer_Type_Uniform);
		bufferInfo.Size = pRenderer->PadToAlignedSize(sizeof(CameraUbo)) * 2;

		CameraUboHnd = pRenderer->AllocateNewBuffer(bufferInfo);
		if (CameraUboHnd == INVALID_HANDLE) { return false; }

    // Add buffer binding information into descriptor set layout.
		DescriptorSetLayoutBindingInfo bindingInfo = {};
		bindingInfo.ShaderStages.Set(Shader_Type_Vertex);
		bindingInfo.Type = Descriptor_Type_Dynamic_Uniform_Buffer;
		bindingInfo.Stride = pRenderer->PadToAlignedSize(sizeof(CameraUbo));
		bindingInfo.DescriptorCount = 1;
		bindingInfo.BindingSlot = 0;
		bindingInfo.LayoutHnd = SetLayoutHnd;

		pRenderer->DescriptorSetLayoutAddBinding(bindingInfo);

    // Add bindless textures binding information into descriptor set layout.
    bindingInfo = {};
    bindingInfo.ShaderStages.Set(Shader_Type_Fragment);
    bindingInfo.Type = Descriptor_Type_Sampled_Image;
    bindingInfo.DescriptorCount = 128;
    bindingInfo.BindingSlot = 1;
    bindingInfo.LayoutHnd = SetLayoutHnd;

    pRenderer->DescriptorSetLayoutAddBinding(bindingInfo);

    // Add texture atlas binding information into descriptor set layout.
    bindingInfo = {};
    bindingInfo.ShaderStages.Set(Shader_Type_Fragment);
    bindingInfo.Type = Descriptor_Type_Sampled_Image;
    bindingInfo.DescriptorCount = 1;
    bindingInfo.BindingSlot = 2;
    bindingInfo.LayoutHnd = SetLayoutHnd;

    pRenderer->DescriptorSetLayoutAddBinding(bindingInfo);

		DescriptorSetAllocateInfo setInfo = {};
		setInfo.LayoutHnd = SetLayoutHnd;
		setInfo.PoolHnd = PoolHandle;
		setInfo.Slot = 0;

		SetHnd = pRenderer->CreateDescriptorSet(setInfo);
		if (SetHnd == INVALID_HANDLE) { return false; }

    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
    frameGraph.Initialize({ 800, 600 });

    // Initialize passes.
		if (!ColorPass.Initialize(pEngine, pRenderer, pAssetManager)) { return false; }
    if (!TextOverlay.Initialize(pEngine, pRenderer, pAssetManager)) { return false; }

		pRenderer->PipelineAddDescriptorSetLayout(ColorPass.GetPipelineHandle(), SetLayoutHnd);
    pRenderer->PipelineAddDescriptorSetLayout(TextOverlay.GetPipelineHandle(), SetLayoutHnd);

    ImageSamplerCreateInfo samplerInfo = {};
    samplerInfo.AnisotropyLvl = 16.0f;
    samplerInfo.AddressModeU = Sampler_Address_Mode_Clamp_To_Edge;
    samplerInfo.AddressModeV = Sampler_Address_Mode_Clamp_To_Edge;
    samplerInfo.AddressModeW = Sampler_Address_Mode_Clamp_To_Edge;
    samplerInfo.CompareOp = Compare_Op_Less_Or_Equal;
    samplerInfo.MinFilter = Sampler_Filter_Nearest;
    samplerInfo.MagFilter = Sampler_Filter_Linear;
    samplerInfo.MaxLod = 3.0f;

    ModelTexImgSamplerHnd = pRenderer->CreateImageSampler(samplerInfo);
    if (ModelTexImgSamplerHnd == INVALID_HANDLE) { return false; }

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

  const Handle<SImageSampler> RendererSetup::GetTextureImageSampler() const
  {
    return ModelTexImgSamplerHnd;
  }

	bool RendererSetup::Build()
	{
		IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
		if (!frameGraph.Build()) { return false; }

		//pRenderer->BuildBuffer(CameraUboHnd);
		//pRenderer->BuildDescriptorPool(PoolHandle);
		//pRenderer->BuildDescriptorSetLayout(SetLayoutHnd);
		//pRenderer->BuildDescriptorSet(SetHnd);

		//astl::Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(ColorPass.GetShaderHandle(Shader_Type_Vertex));
		//astl::Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(ColorPass.GetShaderHandle(Shader_Type_Fragment));

		//if (!pRenderer->BuildShader(pFragment->Hnd)) { return false; }
		//if (!pRenderer->BuildShader(pVertex->Hnd)) { return false; }
		//if (!pRenderer->BuildGraphicsPipeline(ColorPass.GetPipelineHandle())) { return false; }

  //  pFragment = pAssetManager->GetShaderWithHandle(TextOverlay.GetShaderHandle(Shader_Type_Vertex));
  //  pVertex = pAssetManager->GetShaderWithHandle(TextOverlay.GetShaderHandle(Shader_Type_Fragment));

  //  if (!pRenderer->BuildShader(pFragment->Hnd)) { return false; }
  //  if (!pRenderer->BuildShader(pVertex->Hnd)) { return false; }
  //  if (!pRenderer->BuildGraphicsPipeline(TextOverlay.GetPipelineHandle())) { return false; }

  //  if (!pRenderer->BuildImageSampler(ModelTexImgSamplerHnd)) { return false; }

		return true;
	}

	void RendererSetup::Terminate()
	{
		ColorPass.Terminate();
    TextOverlay.Terminate();
    pRenderer->DestroyImageSampler(ModelTexImgSamplerHnd);
		pRenderer->DestroyBuffer(CameraUboHnd);
		pRenderer->DestroyDescriptorSet(SetHnd);
		pRenderer->DestroyDescriptorSetLayout(SetLayoutHnd);
		pRenderer->DestroyDescriptorPool(PoolHandle);
	}

	const CColorPass& RendererSetup::GetColorPass() const
	{
		return ColorPass;
	}

  const CTextOverlayPass& RendererSetup::GetTexOverlayPass() const
  {
    return TextOverlay;
  }

  bool CTextOverlayPass::Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager)
  {
    pEngine = pInEngine;
    pRenderer = pInRenderSystem;
    pAssetManager = pInAssetManager;

    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
    const WindowInfo& windowInfo = pEngine->GetWindowInformation();

    RenderPassCreateInfo textOverlayPass = {};
    textOverlayPass.Identifier = "Text_Overlay_Pass";
    textOverlayPass.Type = RenderPass_Type_Graphics;
    textOverlayPass.Depth = 0.0f;
    textOverlayPass.Pos = { 0, 0 };
    textOverlayPass.Extent = { 800, 600 };

    Extent = windowInfo.Extent;

    RenderPassHnd = frameGraph.AddRenderPass(textOverlayPass);
    frameGraph.NoDefaultDepthStencilRender(RenderPassHnd);

    if (RenderPassHnd == INVALID_HANDLE) { return false; }

    // Import shaders required for text overlay pass.
    ShaderImporter importer;
    FragmentShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.frag", Shader_Type_Fragment, pAssetManager);
    VertexShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.vert", Shader_Type_Vertex, pAssetManager);

    if (FragmentShader == INVALID_HANDLE ||
      VertexShader == INVALID_HANDLE) {
      return false;
    }

    // Fragment shader for text overlay pass is created here.
    astl::Ref<Shader> pFrag = pAssetManager->GetShaderWithHandle(FragmentShader);
    pFrag->Hnd = pRenderer->CreateShader(pFrag->Code, pFrag->Type);
    pFrag->Code.Release();

    if (pFrag->Hnd == INVALID_HANDLE) { return false; }

    // Vertex shader for text overlay pass is created here.
    astl::Ref<Shader> pVert = pAssetManager->GetShaderWithHandle(VertexShader);
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

    PipelineCreateInfo textOverlayPipelineInfo = {};
    textOverlayPipelineInfo.VertexShaderHnd = pVert->Hnd;
    textOverlayPipelineInfo.FragmentShaderHnd = pFrag->Hnd;
    textOverlayPipelineInfo.BindPoint = Pipeline_Bind_Point_Graphics;
    textOverlayPipelineInfo.CullMode = Culling_Mode_None;
    textOverlayPipelineInfo.FrontFace = Front_Face_Counter_Clockwise;
    textOverlayPipelineInfo.PolygonalMode = Polygon_Mode_Fill;
    textOverlayPipelineInfo.Samples = Sample_Count_1;
    textOverlayPipelineInfo.Topology = Topology_Type_Triangle;
    textOverlayPipelineInfo.Viewport = viewport;
    textOverlayPipelineInfo.Scissor = scissor;
    textOverlayPipelineInfo.SrcColorBlendFactor = Blend_Factor_Src_Alpha;
    textOverlayPipelineInfo.DstColorBlendFactor = Blend_Factor_One_Minus_Src_Alpha;
    textOverlayPipelineInfo.ColorBlendOp = Blend_Op_Add;
    textOverlayPipelineInfo.SrcAlphaBlendFactor = Blend_Factor_One_Minus_Src_Alpha;
    textOverlayPipelineInfo.DstAlphaBlendFactor = Blend_Factor_Zero;
    textOverlayPipelineInfo.AlphaBlendOp = Blend_Op_Add;

    PipelineHnd = pRenderer->CreatePipeline(textOverlayPipelineInfo);
    if (PipelineHnd == INVALID_HANDLE) { return false; }

    SPipeline::VertexInputBinding vertexBindings = {};
    vertexBindings.Binding = 0;
    vertexBindings.From = 0;
    vertexBindings.To = 4;
    vertexBindings.Stride = sizeof(Vertex);
    vertexBindings.Type = Vertex_Input_Rate_Vertex;

    SPipeline::VertexInputBinding glyphInstanceAttrBindings = {};
    glyphInstanceAttrBindings.Binding = 2;
    glyphInstanceAttrBindings.From = 5;
    glyphInstanceAttrBindings.To = 13;
    glyphInstanceAttrBindings.Stride = sizeof(math::mat4) + (sizeof(math::vec2) * 4) + sizeof(math::vec3);
    glyphInstanceAttrBindings.Type = Vertex_Input_Rate_Instance;

    //SPipeline::VertexInputBinding glyphAttribBindings = {};
    //glyphAttribBindings.Binding = 2;
    //glyphAttribBindings.From = 9;
    //glyphAttribBindings.To = 13;
    //glyphAttribBindings.Stride = sizeof(math::vec2) * 4 + sizeof(math::vec3);
    //glyphAttribBindings.Type = Vertex_Input_Rate_Instance;

    pRenderer->PipelineAddVertexInputBinding(PipelineHnd, vertexBindings);
    pRenderer->PipelineAddVertexInputBinding(PipelineHnd, glyphInstanceAttrBindings);
    //pRenderer->PipelineAddVertexInputBinding(PipelineHnd, glyphAttribBindings);
    pRenderer->PipelineAddRenderPass(PipelineHnd, RenderPassHnd);

    ImageSamplerCreateInfo fontSamplerInfo = {};
    fontSamplerInfo.AddressModeU = Sampler_Address_Mode_Clamp_To_Edge;
    fontSamplerInfo.AddressModeV = Sampler_Address_Mode_Clamp_To_Edge;
    fontSamplerInfo.AddressModeW = Sampler_Address_Mode_Clamp_To_Edge;
    fontSamplerInfo.AnisotropyLvl = 8.0f;
    fontSamplerInfo.CompareOp = Compare_Op_Never;
    fontSamplerInfo.MaxLod = 4.0f;
    fontSamplerInfo.MinFilter = Sampler_Filter_Nearest;
    fontSamplerInfo.MagFilter = Sampler_Filter_Nearest;

    FontSamplerHnd = pRenderer->CreateImageSampler(fontSamplerInfo);
    VKT_ASSERT(FontSamplerHnd != INVALID_HANDLE);

    return true;
  }

  void CTextOverlayPass::Terminate()
  {
    pRenderer->DestroyPipeline(PipelineHnd);

    astl::Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(FragmentShader);
    astl::Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(VertexShader);

    pRenderer->DestroyShader(pFragment->Hnd);
    pRenderer->DestroyShader(pVertex->Hnd);
    pRenderer->DestroyImageSampler(FontSamplerHnd);

    pAssetManager->DestroyShader(FragmentShader);
    pAssetManager->DestroyShader(VertexShader);
  }

  const Handle<SRenderPass> CTextOverlayPass::GetRenderPassHandle() const
  {
    return RenderPassHnd;
  }

  const Handle<SPipeline> CTextOverlayPass::GetPipelineHandle() const
  {
    return PipelineHnd;
  }

  const Handle<Shader> CTextOverlayPass::GetShaderHandle(EShaderType Type) const
  {
    return (Type == Shader_Type_Vertex) ? VertexShader : FragmentShader;
  }

  const Handle<SImageSampler> CTextOverlayPass::GetFontSampler() const
  {
    return FontSamplerHnd;
  }

}
