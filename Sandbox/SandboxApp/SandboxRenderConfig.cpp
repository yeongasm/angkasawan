#include "SandboxRenderConfig.h"
#include "Library/Math/Math.h"
#include "Importer/Importer.h"

namespace sandbox
{

  IGBufferPassExtension pGBufferExt;
  ITextOverlayPassExtension pTextOverlayExt;

  bool ISandboxRendererSetup::PrepareGBufferPass()
  {
    ISandboxFramePass gBufferPass = {};
    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();

    gBufferPass.Pos = { 0, 0 };
    gBufferPass.Extent = { 800, 600 };
    gBufferPass.pNext = &pGBufferExt;

    RenderPassCreateInfo gBufferInfo = {};
    gBufferInfo.Identifier = "G_Buffer_Pass";
    gBufferInfo.Type = RenderPass_Type_Graphics;
    gBufferInfo.Depth = 1.0f;
    gBufferInfo.Pos = gBufferPass.Pos;
    gBufferInfo.Extent = gBufferPass.Extent;

    gBufferPass.RenderPassHnd = frameGraph.AddRenderPass(gBufferInfo);
    if (gBufferPass.RenderPassHnd == INVALID_HANDLE)
    {
      return false;
    }

    // G Buffer attachment outputs.
    pGBufferExt.PositionImgHnd = frameGraph.AddColorOutput(gBufferPass.RenderPassHnd);
    pGBufferExt.NormalsImgHnd = frameGraph.AddColorOutput(gBufferPass.RenderPassHnd);
    pGBufferExt.AlbedoImgHnd  = frameGraph.AddColorOutput(gBufferPass.RenderPassHnd);

    BufferAllocateInfo cameraUboInfo = {};
    cameraUboInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
    cameraUboInfo.Type.Set(Buffer_Type_Uniform);
    cameraUboInfo.Size = pRenderer->PadToAlignedSize(sizeof(SandboxSceneCameraUbo)) * 2;

    pGBufferExt.CameraUboHnd = pRenderer->AllocateNewBuffer(cameraUboInfo);
    if (pGBufferExt.CameraUboHnd == INVALID_HANDLE)
    {
      return false;
    }

    // Map buffer to descriptor set
    pRenderer->DescriptorSetMapToBuffer(
      DescriptorSetHnd,
      0,
      pGBufferExt.CameraUboHnd,
      0,
      sizeof(SandboxSceneCameraUbo)
    );

    ShaderImporter importer;
    gBufferPass.VertexShader = importer.ImportShaderFromPath("Data/Shaders/GBuffer.vert", Shader_Type_Vertex, pAssetManager);
    gBufferPass.FragmentShader = importer.ImportShaderFromPath("Data/Shaders/GBuffer.frag", Shader_Type_Fragment, pAssetManager);

    if (gBufferPass.VertexShader == INVALID_HANDLE ||
        gBufferPass.FragmentShader == INVALID_HANDLE)
    {
      return false;
    }

    gBufferPass.VertexShader->Hnd = pRenderer->CreateShader(gBufferPass.VertexShader->Code, gBufferPass.VertexShader->Type);
    gBufferPass.VertexShader->Code.Release();

    gBufferPass.FragmentShader->Hnd = pRenderer->CreateShader(gBufferPass.FragmentShader->Code, gBufferPass.FragmentShader->Type);
    gBufferPass.FragmentShader->Code.Release();

    if (gBufferPass.VertexShader->Hnd == INVALID_HANDLE ||
        gBufferPass.FragmentShader->Hnd == INVALID_HANDLE)
    {
      return false;
    }

    Viewport viewport;
    viewport.x = static_cast<float32>(gBufferPass.Pos.x);
    viewport.y = static_cast<float32>(gBufferPass.Pos.y);
    viewport.Width = static_cast<float32>(gBufferPass.Extent.Width);
    viewport.Height = static_cast<float32>(gBufferPass.Extent.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    Rect2D scissor;
    scissor.Offset.x = 0;
    scissor.Offset.y = 0;
    scissor.Extent = gBufferPass.Extent;

    PipelineCreateInfo gBufferPipeline = {};
    gBufferPipeline.VertexShaderHnd = gBufferPass.VertexShader->Hnd;
    gBufferPipeline.FragmentShaderHnd = gBufferPass.FragmentShader->Hnd;
    gBufferPipeline.BindPoint = Pipeline_Bind_Point_Graphics;
    gBufferPipeline.CullMode = Culling_Mode_Back;
    gBufferPipeline.FrontFace = Front_Face_Counter_Clockwise;
    gBufferPipeline.PolygonalMode = Polygon_Mode_Fill;
    gBufferPipeline.Samples = Sample_Count_1;
    gBufferPipeline.Topology = Topology_Type_Triangle;
    gBufferPipeline.Viewport = viewport;
    gBufferPipeline.Scissor = scissor;
    gBufferPipeline.SrcColorBlendFactor = Blend_Factor_One;
    gBufferPipeline.DstColorBlendFactor = Blend_Factor_Zero;
    gBufferPipeline.ColorBlendOp = Blend_Op_Add;
    gBufferPipeline.SrcAlphaBlendFactor = Blend_Factor_One;
    gBufferPipeline.DstAlphaBlendFactor = Blend_Factor_Zero;
    gBufferPipeline.AlphaBlendOp = Blend_Op_Add;

    gBufferPass.PipelineHnd = pRenderer->CreatePipeline(gBufferPipeline);
    if (gBufferPass.PipelineHnd == INVALID_HANDLE) { return false; }

    SPipeline::VertexInputBinding vertexBindings = {};
    vertexBindings.Binding = 0;
    vertexBindings.From = 0;
    vertexBindings.To = 3;
    vertexBindings.Stride = sizeof(Vertex);
    vertexBindings.Type = Vertex_Input_Rate_Vertex;

    SPipeline::VertexInputBinding instanceBindings = {};
    instanceBindings.Binding = 1;
    instanceBindings.From = 4;
    instanceBindings.To = 7;
    instanceBindings.Stride = sizeof(math::mat4);
    instanceBindings.Type = Vertex_Input_Rate_Instance;

    pRenderer->PipelineAddVertexInputBinding(gBufferPass.PipelineHnd, vertexBindings);
    pRenderer->PipelineAddVertexInputBinding(gBufferPass.PipelineHnd, instanceBindings);
    pRenderer->PipelineAddRenderPass(gBufferPass.PipelineHnd, gBufferPass.RenderPassHnd);

    // NOTE: Do not render to default image that's in the frame graph.
    frameGraph.NoDefaultColorRender(gBufferPass.RenderPassHnd);

    FramePasses.Emplace(ESandboxFrames::Sandbox_Frame_GBuffer, astl::Move(gBufferPass));

    return true;
  }

  bool ISandboxRendererSetup::PrepareTextOverlayPass()
  {
    ISandboxFramePass textOverlay = {};
    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();

    textOverlay.Pos = { 0, 0 };
    textOverlay.Extent = { 800, 600 };
    textOverlay.pNext = &pTextOverlayExt;

    RenderPassCreateInfo textOverlayInfo = {};
    textOverlayInfo.Identifier = "Text_Overlay_Pass";
    textOverlayInfo.Depth = 1.0f;
    textOverlayInfo.Type = RenderPass_Type_Graphics;
    textOverlayInfo.Pos = textOverlay.Pos;
    textOverlayInfo.Extent = textOverlay.Extent;

    textOverlay.RenderPassHnd = frameGraph.AddRenderPass(textOverlayInfo);
    if (textOverlay.RenderPassHnd == INVALID_HANDLE)
    {
      return false;
    }

    ImageSamplerCreateInfo fontSamplerInfo = {};
    fontSamplerInfo.AddressModeU = Sampler_Address_Mode_Clamp_To_Edge;
    fontSamplerInfo.AddressModeV = Sampler_Address_Mode_Clamp_To_Edge;
    fontSamplerInfo.AddressModeW = Sampler_Address_Mode_Clamp_To_Edge;
    fontSamplerInfo.AnisotropyLvl = 8.0f;
    fontSamplerInfo.CompareOp = Compare_Op_Never;
    fontSamplerInfo.MaxLod = 4.0f;
    fontSamplerInfo.MinFilter = Sampler_Filter_Nearest;
    fontSamplerInfo.MagFilter = Sampler_Filter_Nearest;

    pTextOverlayExt.ImgSamplerHnd = pRenderer->CreateImageSampler(fontSamplerInfo);
    if (pTextOverlayExt.ImgSamplerHnd == INVALID_HANDLE)
    {
      return false;
    }

    ShaderImporter importer;
    textOverlay.VertexShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.vert", Shader_Type_Vertex, pAssetManager);
    textOverlay.FragmentShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.frag", Shader_Type_Fragment, pAssetManager);

    if (textOverlay.VertexShader == INVALID_HANDLE ||
        textOverlay.FragmentShader == INVALID_HANDLE)
    {
      return false;
    }

    textOverlay.VertexShader->Hnd = pRenderer->CreateShader(textOverlay.VertexShader->Code, textOverlay.VertexShader->Type);
    textOverlay.VertexShader->Code.Release();

    textOverlay.FragmentShader->Hnd = pRenderer->CreateShader(textOverlay.FragmentShader->Code, textOverlay.FragmentShader->Type);
    textOverlay.FragmentShader->Code.Release();

    if (textOverlay.VertexShader->Hnd == INVALID_HANDLE ||
        textOverlay.FragmentShader->Hnd == INVALID_HANDLE)
    {
      return false;
    }

    Viewport viewport;
    viewport.x = static_cast<float32>(textOverlay.Pos.x);
    viewport.y = static_cast<float32>(textOverlay.Pos.y);
    viewport.Width = static_cast<float32>(textOverlay.Extent.Width);
    viewport.Height = static_cast<float32>(textOverlay.Extent.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    Rect2D scissor;
    scissor.Offset.x = 0;
    scissor.Offset.y = 0;
    scissor.Extent = textOverlay.Extent;

    PipelineCreateInfo textOverlayPipelineInfo = {};
    textOverlayPipelineInfo.VertexShaderHnd = textOverlay.VertexShader->Hnd;
    textOverlayPipelineInfo.FragmentShaderHnd = textOverlay.FragmentShader->Hnd;
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

    textOverlay.PipelineHnd = pRenderer->CreatePipeline(textOverlayPipelineInfo);
    if (textOverlay.PipelineHnd == INVALID_HANDLE)
    {
      return false;
    }

    SPipeline::VertexInputBinding vertexBindings = {};
    vertexBindings.Binding = 0;
    vertexBindings.From = 0;
    vertexBindings.To = 3;
    vertexBindings.Stride = sizeof(Vertex);
    vertexBindings.Type = Vertex_Input_Rate_Vertex;

    SPipeline::VertexInputBinding glyphInstanceAttrBindings = {};
    glyphInstanceAttrBindings.Binding = 2;
    glyphInstanceAttrBindings.From = 4;
    glyphInstanceAttrBindings.To = 12;
    glyphInstanceAttrBindings.Stride = sizeof(math::mat4) + (sizeof(math::vec2) * 4) + sizeof(math::vec3);
    glyphInstanceAttrBindings.Type = Vertex_Input_Rate_Instance;

    pRenderer->PipelineAddVertexInputBinding(textOverlay.PipelineHnd, vertexBindings);
    pRenderer->PipelineAddVertexInputBinding(textOverlay.PipelineHnd, glyphInstanceAttrBindings);
    pRenderer->PipelineAddRenderPass(textOverlay.PipelineHnd, textOverlay.RenderPassHnd);

    FramePasses.Emplace(ESandboxFrames::Sandbox_Frame_TextOverlay, astl::Move(textOverlay));

    return true;
  }

  bool ISandboxRendererSetup::Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager)
  {
    pEngine = pInEngine;
    pRenderer = pInRenderSystem;
    pAssetManager = pInAssetManager;

    // Create global descriptor pool.
    DescriptorPoolHnd = pRenderer->CreateDescriptorPool();
    if (DescriptorPoolHnd == INVALID_HANDLE)
    {
      return false;
    }

    // Register dynamic uniform buffer for camera information into descriptor pool.
    SDescriptorPool::Size dynamicUniformBuffer;
    dynamicUniformBuffer.Count = 1;
    dynamicUniformBuffer.Type = Descriptor_Type_Dynamic_Uniform_Buffer;

    // Register bindless textures into descriptor pool.
    // Textures that are binded to a descriptor set allocated from this pool.
    SDescriptorPool::Size bindlessTextures;
    bindlessTextures.Count = 130; // We only need 129 but added an extra one in just in case.
    bindlessTextures.Type = Descriptor_Type_Sampled_Image;

    pRenderer->DescriptorPoolAddSizeType(DescriptorPoolHnd, dynamicUniformBuffer);
    pRenderer->DescriptorPoolAddSizeType(DescriptorPoolHnd, bindlessTextures);

    // Create a global descriptor set layout.
    DescriptorSetLayoutHnd = pRenderer->CreateDescriptorSetLayout();
    if (DescriptorSetLayoutHnd == INVALID_HANDLE)
    {
      return false;
    }

    // Add buffer binding information into descriptor set layout.
    DescriptorSetLayoutBindingInfo bindingInfo = {};
    bindingInfo.ShaderStages.Set(Shader_Type_Vertex);
    bindingInfo.Type = Descriptor_Type_Dynamic_Uniform_Buffer;
    bindingInfo.Stride = pRenderer->PadToAlignedSize(sizeof(SandboxSceneCameraUbo));
    bindingInfo.DescriptorCount = 1;
    bindingInfo.BindingSlot = 0;
    bindingInfo.LayoutHnd = DescriptorSetLayoutHnd;

    pRenderer->DescriptorSetLayoutAddBinding(bindingInfo);

    // Add bindless textures binding information into descriptor set layout.
    bindingInfo = {};
    bindingInfo.ShaderStages.Set(Shader_Type_Fragment);
    bindingInfo.Type = Descriptor_Type_Sampled_Image;
    bindingInfo.DescriptorCount = 128;
    bindingInfo.BindingSlot = 1;
    bindingInfo.LayoutHnd = DescriptorSetLayoutHnd;

    pRenderer->DescriptorSetLayoutAddBinding(bindingInfo);

    // Add font texture atlas binding information into descriptor set layout.
    bindingInfo = {};
    bindingInfo.ShaderStages.Set(Shader_Type_Fragment);
    bindingInfo.Type = Descriptor_Type_Sampled_Image;
    bindingInfo.DescriptorCount = 1;
    bindingInfo.BindingSlot = 2;
    bindingInfo.LayoutHnd = DescriptorSetLayoutHnd;

    pRenderer->DescriptorSetLayoutAddBinding(bindingInfo);

    DescriptorSetAllocateInfo setInfo = {};
    setInfo.LayoutHnd = DescriptorSetLayoutHnd;
    setInfo.PoolHnd = DescriptorPoolHnd;
    setInfo.Slot = 0;

    DescriptorSetHnd = pRenderer->CreateDescriptorSet(setInfo);
    if (DescriptorSetHnd == INVALID_HANDLE)
    {
      return false;
    }

    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
    frameGraph.Initialize({ 800, 600 });

    // Initialize passes.
    if (!PrepareGBufferPass())      { return false; }
    if (!PrepareTextOverlayPass())  { return false; }

    pRenderer->PipelineAddDescriptorSetLayout(
      FramePasses[static_cast<uint32>(ESandboxFrames::Sandbox_Frame_GBuffer)].PipelineHnd,
      DescriptorSetLayoutHnd
    );
    pRenderer->PipelineAddDescriptorSetLayout(
      FramePasses[static_cast<uint32>(ESandboxFrames::Sandbox_Frame_TextOverlay)].PipelineHnd,
      DescriptorSetLayoutHnd
    );

    //ImageSamplerCreateInfo samplerInfo = {};
    //samplerInfo.AnisotropyLvl = 16.0f;
    //samplerInfo.AddressModeU = Sampler_Address_Mode_Clamp_To_Edge;
    //samplerInfo.AddressModeV = Sampler_Address_Mode_Clamp_To_Edge;
    //samplerInfo.AddressModeW = Sampler_Address_Mode_Clamp_To_Edge;
    //samplerInfo.CompareOp = Compare_Op_Less_Or_Equal;
    //samplerInfo.MinFilter = Sampler_Filter_Nearest;
    //samplerInfo.MagFilter = Sampler_Filter_Linear;
    //samplerInfo.MaxLod = 3.0f;

    //ModelTexImgSamplerHnd = pRenderer->CreateImageSampler(samplerInfo);
    //if (ModelTexImgSamplerHnd == INVALID_HANDLE) { return false; }

    frameGraph.Build();

    return true;
  }

  //const ISandboxFramePass& ISandboxRendererSetup::GetSandboxFramePass(ESandboxFrames Frame) const
  //{
  //  return FramePasses[static_cast<uint32>(Frame)];
  //}

  astl::Ref<ISandboxFramePass> ISandboxRendererSetup::GetFramePass(ESandboxFrames FrameId)
  {
    return &FramePasses[static_cast<uint32>(FrameId)];
  }

  const Handle<SDescriptorSet> ISandboxRendererSetup::GetDescriptorSet() const
  {
    return DescriptorSetHnd;
  }

  void ISandboxRendererSetup::Terminate()
  {
    for (auto& [key, pass] : FramePasses)
    {
      pRenderer->DestroyPipeline(pass.PipelineHnd);
      pRenderer->DestroyShader(pass.VertexShader->Hnd);
      pRenderer->DestroyShader(pass.FragmentShader->Hnd);
      pAssetManager->DestroyShader(pass.VertexShader);
      pAssetManager->DestroyShader(pass.FragmentShader);
    }

    // Destroy g buffer camera ubo.
    pRenderer->DestroyBuffer(pGBufferExt.CameraUboHnd);
    // Destroy text overlay image sampler.
    pRenderer->DestroyImageSampler(pTextOverlayExt.ImgSamplerHnd);

    pRenderer->DestroyDescriptorSet(DescriptorSetHnd);
    pRenderer->DestroyDescriptorSetLayout(DescriptorSetLayoutHnd);
    pRenderer->DestroyDescriptorPool(DescriptorPoolHnd);
  }

	/*bool CColorPass::Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager)
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
	}*/


	/*void CColorPass::Terminate()
	{
		pRenderer->DestroyPipeline(PipelineHnd);
		
		astl::Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(FragmentShader);
		astl::Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(VertexShader);

		pRenderer->DestroyShader(pFragment->Hnd);
		pRenderer->DestroyShader(pVertex->Hnd);

		pAssetManager->DestroyShader(FragmentShader);
		pAssetManager->DestroyShader(VertexShader);
	}*/

	/*const Handle<SRenderPass> CColorPass::GetRenderPassHandle() const
	{
		return RenderPassHnd;
	}*/

	/*const Handle<SPipeline> CColorPass::GetPipelineHandle() const
	{
		return PipelineHnd;
	}*/

	/*const Handle<Shader> CColorPass::GetShaderHandle(EShaderType Type) const
	{
		return (Type == Shader_Type_Vertex) ? VertexShader : FragmentShader;
	}*/

	/*bool RendererSetup::Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager)
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
	}*/

	/*const Handle<SDescriptorPool> RendererSetup::GetDescriptorPoolHandle() const
	{
		return PoolHandle;
	}*/

	/*const Handle<SDescriptorSetLayout> RendererSetup::GetDescriptorSetLayoutHandle() const
	{
		return SetLayoutHnd;
	}*/

	/*const Handle<SDescriptorSet> RendererSetup::GetDescriptorSetHandle() const
	{
		return SetHnd;
	}*/

	/*const Handle<SMemoryBuffer> RendererSetup::GetCameraUboHandle() const
	{
		return CameraUboHnd;
	}*/

  /*const Handle<SImageSampler> RendererSetup::GetTextureImageSampler() const
  {
    return ModelTexImgSamplerHnd;
  }*/

	//bool RendererSetup::Build()
	//{
	//	IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
	//	if (!frameGraph.Build()) { return false; }

	//	//pRenderer->BuildBuffer(CameraUboHnd);
	//	//pRenderer->BuildDescriptorPool(PoolHandle);
	//	//pRenderer->BuildDescriptorSetLayout(SetLayoutHnd);
	//	//pRenderer->BuildDescriptorSet(SetHnd);

	//	//astl::Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(ColorPass.GetShaderHandle(Shader_Type_Vertex));
	//	//astl::Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(ColorPass.GetShaderHandle(Shader_Type_Fragment));

	//	//if (!pRenderer->BuildShader(pFragment->Hnd)) { return false; }
	//	//if (!pRenderer->BuildShader(pVertex->Hnd)) { return false; }
	//	//if (!pRenderer->BuildGraphicsPipeline(ColorPass.GetPipelineHandle())) { return false; }

 // //  pFragment = pAssetManager->GetShaderWithHandle(TextOverlay.GetShaderHandle(Shader_Type_Vertex));
 // //  pVertex = pAssetManager->GetShaderWithHandle(TextOverlay.GetShaderHandle(Shader_Type_Fragment));

 // //  if (!pRenderer->BuildShader(pFragment->Hnd)) { return false; }
 // //  if (!pRenderer->BuildShader(pVertex->Hnd)) { return false; }
 // //  if (!pRenderer->BuildGraphicsPipeline(TextOverlay.GetPipelineHandle())) { return false; }

 // //  if (!pRenderer->BuildImageSampler(ModelTexImgSamplerHnd)) { return false; }

	//	return true;
	//}

	/*void RendererSetup::Terminate()
	{
		//ColorPass.Terminate();
  //  TextOverlay.Terminate();
    pRenderer->DestroyImageSampler(ModelTexImgSamplerHnd);
		pRenderer->DestroyBuffer(CameraUboHnd);
		pRenderer->DestroyDescriptorSet(SetHnd);
		pRenderer->DestroyDescriptorSetLayout(SetLayoutHnd);
		pRenderer->DestroyDescriptorPool(PoolHandle);
	}*/

	/*const CColorPass& RendererSetup::GetColorPass() const
	{
		return ColorPass;
	}*/

  /*const CTextOverlayPass& RendererSetup::GetTexOverlayPass() const
  {
    return TextOverlay;
  }*/

  //bool CTextOverlayPass::Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager)
  //{
  //  pEngine = pInEngine;
  //  pRenderer = pInRenderSystem;
  //  pAssetManager = pInAssetManager;

  //  IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
  //  const WindowInfo& windowInfo = pEngine->GetWindowInformation();

  //  RenderPassCreateInfo textOverlayPass = {};
  //  textOverlayPass.Identifier = "Text_Overlay_Pass";
  //  textOverlayPass.Type = RenderPass_Type_Graphics;
  //  textOverlayPass.Depth = 0.0f;
  //  textOverlayPass.Pos = { 0, 0 };
  //  textOverlayPass.Extent = { 800, 600 };

  //  Extent = windowInfo.Extent;

  //  RenderPassHnd = frameGraph.AddRenderPass(textOverlayPass);
  //  frameGraph.NoDefaultDepthStencilRender(RenderPassHnd);

  //  if (RenderPassHnd == INVALID_HANDLE) { return false; }

  //  // Import shaders required for text overlay pass.
  //  ShaderImporter importer;
  //  FragmentShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.frag", Shader_Type_Fragment, pAssetManager);
  //  VertexShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.vert", Shader_Type_Vertex, pAssetManager);

  //  if (FragmentShader == INVALID_HANDLE ||
  //    VertexShader == INVALID_HANDLE) {
  //    return false;
  //  }

  //  // Fragment shader for text overlay pass is created here.
  //  astl::Ref<Shader> pFrag = pAssetManager->GetShaderWithHandle(FragmentShader);
  //  pFrag->Hnd = pRenderer->CreateShader(pFrag->Code, pFrag->Type);
  //  pFrag->Code.Release();

  //  if (pFrag->Hnd == INVALID_HANDLE) { return false; }

  //  // Vertex shader for text overlay pass is created here.
  //  astl::Ref<Shader> pVert = pAssetManager->GetShaderWithHandle(VertexShader);
  //  pVert->Hnd = pRenderer->CreateShader(pVert->Code, pVert->Type);
  //  pVert->Code.Release();

  //  if (pVert->Hnd == INVALID_HANDLE) { return false; }

  //  Viewport viewport;
  //  viewport.x = static_cast<float32>(Pos.x);
  //  viewport.y = static_cast<float32>(Pos.y);
  //  viewport.Width = static_cast<float32>(Extent.Width);
  //  viewport.Height = static_cast<float32>(Extent.Height);
  //  viewport.MinDepth = 0.0f;
  //  viewport.MaxDepth = 1.0f;

  //  Rect2D scissor;
  //  scissor.Offset.x = 0;
  //  scissor.Offset.y = 0;
  //  scissor.Extent = Extent;

  //  PipelineCreateInfo textOverlayPipelineInfo = {};
  //  textOverlayPipelineInfo.VertexShaderHnd = pVert->Hnd;
  //  textOverlayPipelineInfo.FragmentShaderHnd = pFrag->Hnd;
  //  textOverlayPipelineInfo.BindPoint = Pipeline_Bind_Point_Graphics;
  //  textOverlayPipelineInfo.CullMode = Culling_Mode_None;
  //  textOverlayPipelineInfo.FrontFace = Front_Face_Counter_Clockwise;
  //  textOverlayPipelineInfo.PolygonalMode = Polygon_Mode_Fill;
  //  textOverlayPipelineInfo.Samples = Sample_Count_1;
  //  textOverlayPipelineInfo.Topology = Topology_Type_Triangle;
  //  textOverlayPipelineInfo.Viewport = viewport;
  //  textOverlayPipelineInfo.Scissor = scissor;
  //  textOverlayPipelineInfo.SrcColorBlendFactor = Blend_Factor_Src_Alpha;
  //  textOverlayPipelineInfo.DstColorBlendFactor = Blend_Factor_One_Minus_Src_Alpha;
  //  textOverlayPipelineInfo.ColorBlendOp = Blend_Op_Add;
  //  textOverlayPipelineInfo.SrcAlphaBlendFactor = Blend_Factor_One_Minus_Src_Alpha;
  //  textOverlayPipelineInfo.DstAlphaBlendFactor = Blend_Factor_Zero;
  //  textOverlayPipelineInfo.AlphaBlendOp = Blend_Op_Add;

  //  PipelineHnd = pRenderer->CreatePipeline(textOverlayPipelineInfo);
  //  if (PipelineHnd == INVALID_HANDLE) { return false; }

  //  SPipeline::VertexInputBinding vertexBindings = {};
  //  vertexBindings.Binding = 0;
  //  vertexBindings.From = 0;
  //  vertexBindings.To = 4;
  //  vertexBindings.Stride = sizeof(Vertex);
  //  vertexBindings.Type = Vertex_Input_Rate_Vertex;

  //  SPipeline::VertexInputBinding glyphInstanceAttrBindings = {};
  //  glyphInstanceAttrBindings.Binding = 2;
  //  glyphInstanceAttrBindings.From = 5;
  //  glyphInstanceAttrBindings.To = 13;
  //  glyphInstanceAttrBindings.Stride = sizeof(math::mat4) + (sizeof(math::vec2) * 4) + sizeof(math::vec3);
  //  glyphInstanceAttrBindings.Type = Vertex_Input_Rate_Instance;

  //  //SPipeline::VertexInputBinding glyphAttribBindings = {};
  //  //glyphAttribBindings.Binding = 2;
  //  //glyphAttribBindings.From = 9;
  //  //glyphAttribBindings.To = 13;
  //  //glyphAttribBindings.Stride = sizeof(math::vec2) * 4 + sizeof(math::vec3);
  //  //glyphAttribBindings.Type = Vertex_Input_Rate_Instance;

  //  pRenderer->PipelineAddVertexInputBinding(PipelineHnd, vertexBindings);
  //  pRenderer->PipelineAddVertexInputBinding(PipelineHnd, glyphInstanceAttrBindings);
  //  //pRenderer->PipelineAddVertexInputBinding(PipelineHnd, glyphAttribBindings);
  //  pRenderer->PipelineAddRenderPass(PipelineHnd, RenderPassHnd);

  //  ImageSamplerCreateInfo fontSamplerInfo = {};
  //  fontSamplerInfo.AddressModeU = Sampler_Address_Mode_Clamp_To_Edge;
  //  fontSamplerInfo.AddressModeV = Sampler_Address_Mode_Clamp_To_Edge;
  //  fontSamplerInfo.AddressModeW = Sampler_Address_Mode_Clamp_To_Edge;
  //  fontSamplerInfo.AnisotropyLvl = 8.0f;
  //  fontSamplerInfo.CompareOp = Compare_Op_Never;
  //  fontSamplerInfo.MaxLod = 4.0f;
  //  fontSamplerInfo.MinFilter = Sampler_Filter_Nearest;
  //  fontSamplerInfo.MagFilter = Sampler_Filter_Nearest;

  //  FontSamplerHnd = pRenderer->CreateImageSampler(fontSamplerInfo);
  //  VKT_ASSERT(FontSamplerHnd != INVALID_HANDLE);

  //  return true;
  //}

  /*void CTextOverlayPass::Terminate()
  {
    pRenderer->DestroyPipeline(PipelineHnd);

    astl::Ref<Shader> pFragment = pAssetManager->GetShaderWithHandle(FragmentShader);
    astl::Ref<Shader> pVertex = pAssetManager->GetShaderWithHandle(VertexShader);

    pRenderer->DestroyShader(pFragment->Hnd);
    pRenderer->DestroyShader(pVertex->Hnd);
    pRenderer->DestroyImageSampler(FontSamplerHnd);

    pAssetManager->DestroyShader(FragmentShader);
    pAssetManager->DestroyShader(VertexShader);
  }*/

  /*const Handle<SRenderPass> CTextOverlayPass::GetRenderPassHandle() const
  {
    return RenderPassHnd;
  }*/

  /*const Handle<SPipeline> CTextOverlayPass::GetPipelineHandle() const
  {
    return PipelineHnd;
  }*/

  /*const Handle<Shader> CTextOverlayPass::GetShaderHandle(EShaderType Type) const
  {
    return (Type == Shader_Type_Vertex) ? VertexShader : FragmentShader;
  }*/

  /*const Handle<SImageSampler> CTextOverlayPass::GetFontSampler() const
  {
    return FontSamplerHnd;
  }*/

}
