#include "SandboxRenderConfig.h"
#include "Library/Math/Math.h"
#include "Importer/Importer.h"

namespace sandbox
{
  bool ISandboxRendererSetup::PrepareGBufferPass()
  {
    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();

    GBufferPass.Pos = { 0, 0 };
    GBufferPass.Extent = { 800, 600 };

    RenderPassCreateInfo gBufferInfo = {};
    gBufferInfo.Identifier = "G_Buffer_Pass";
    gBufferInfo.Type = RenderPass_Type_Graphics;
    gBufferInfo.Depth = 1.0f;
    gBufferInfo.Pos = GBufferPass.Pos;
    gBufferInfo.Extent = GBufferPass.Extent;

    GBufferPass.RenderPassHnd = frameGraph.AddRenderPass(gBufferInfo);
    if (GBufferPass.RenderPassHnd == INVALID_HANDLE)
    {
      return false;
    }

    // G Buffer attachment outputs.
    GBufferPass.PositionImgHnd = frameGraph.AddColorOutput(GBufferPass.RenderPassHnd);
    GBufferPass.NormalsImgHnd = frameGraph.AddColorOutput(GBufferPass.RenderPassHnd);
    GBufferPass.AlbedoImgHnd = frameGraph.AddColorOutput(GBufferPass.RenderPassHnd);

    BufferAllocateInfo cameraUboInfo = {};
    cameraUboInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
    cameraUboInfo.Type.Set(Buffer_Type_Uniform);
    cameraUboInfo.Size = pRenderer->PadToAlignedSize(sizeof(SandboxSceneCameraUbo)) * 2;

    GBufferPass.CameraUboHnd = pRenderer->AllocateNewBuffer(cameraUboInfo);
    if (GBufferPass.CameraUboHnd == INVALID_HANDLE)
    {
      return false;
    }

    // Map buffer to descriptor set
    pRenderer->DescriptorSetMapToBuffer(
      DescriptorSetHnd,
      0,
      GBufferPass.CameraUboHnd,
      0,
      sizeof(SandboxSceneCameraUbo)
    );

    ShaderImporter importer;
    GBufferPass.VertexShader = importer.ImportShaderFromPath("Data/Shaders/GBuffer.vert", Shader_Type_Vertex, pAssetManager);
    GBufferPass.FragmentShader = importer.ImportShaderFromPath("Data/Shaders/GBuffer.frag", Shader_Type_Fragment, pAssetManager);

    if (GBufferPass.VertexShader == INVALID_HANDLE ||
        GBufferPass.FragmentShader == INVALID_HANDLE)
    {
      return false;
    }

    GBufferPass.VertexShader->Hnd = pRenderer->CreateShader(GBufferPass.VertexShader->Code, GBufferPass.VertexShader->Type);
    GBufferPass.VertexShader->Code.Release();

    GBufferPass.FragmentShader->Hnd = pRenderer->CreateShader(GBufferPass.FragmentShader->Code, GBufferPass.FragmentShader->Type);
    GBufferPass.FragmentShader->Code.Release();

    if (GBufferPass.VertexShader->Hnd == INVALID_HANDLE ||
        GBufferPass.FragmentShader->Hnd == INVALID_HANDLE)
    {
      return false;
    }

    Viewport viewport;
    viewport.x = static_cast<float32>(GBufferPass.Pos.x);
    viewport.y = static_cast<float32>(GBufferPass.Pos.y);
    viewport.Width = static_cast<float32>(GBufferPass.Extent.Width);
    viewport.Height = static_cast<float32>(GBufferPass.Extent.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    Rect2D scissor;
    scissor.Offset.x = 0;
    scissor.Offset.y = 0;
    scissor.Extent = GBufferPass.Extent;

    PipelineCreateInfo gBufferPipeline = {};
    gBufferPipeline.VertexShaderHnd = GBufferPass.VertexShader->Hnd;
    gBufferPipeline.FragmentShaderHnd = GBufferPass.FragmentShader->Hnd;
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

    GBufferPass.PipelineHnd = pRenderer->CreatePipeline(gBufferPipeline);
    if (GBufferPass.PipelineHnd == INVALID_HANDLE) { return false; }

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

    pRenderer->PipelineAddVertexInputBinding(GBufferPass.PipelineHnd, vertexBindings);
    pRenderer->PipelineAddVertexInputBinding(GBufferPass.PipelineHnd, instanceBindings);
    pRenderer->PipelineAddRenderPass(GBufferPass.PipelineHnd, GBufferPass.RenderPassHnd);

    // NOTE: Do not render to default image that's in the frame graph.
    //frameGraph.NoDefaultColorRender(GBufferPass.RenderPassHnd);

    return true;
  }

  bool ISandboxRendererSetup::PrepareTextOverlayPass()
  {
    IFrameGraph& frameGraph = pRenderer->GetFrameGraph();

    TextOverlayPass.Pos = { 0, 0 };
    TextOverlayPass.Extent = { 800, 600 };

    RenderPassCreateInfo textOverlayInfo = {};
    textOverlayInfo.Identifier = "Text_Overlay_Pass";
    textOverlayInfo.Depth = 1.0f;
    textOverlayInfo.Type = RenderPass_Type_Graphics;
    textOverlayInfo.Pos = TextOverlayPass.Pos;
    textOverlayInfo.Extent = TextOverlayPass.Extent;

    TextOverlayPass.RenderPassHnd = frameGraph.AddRenderPass(textOverlayInfo);
    if (TextOverlayPass.RenderPassHnd == INVALID_HANDLE)
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

    TextOverlayPass.ImgSamplerHnd = pRenderer->CreateImageSampler(fontSamplerInfo);
    if (TextOverlayPass.ImgSamplerHnd == INVALID_HANDLE)
    {
      return false;
    }

    ShaderImporter importer;
    TextOverlayPass.VertexShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.vert", Shader_Type_Vertex, pAssetManager);
    TextOverlayPass.FragmentShader = importer.ImportShaderFromPath("Data/Shaders/TextOverlay.frag", Shader_Type_Fragment, pAssetManager);

    if (TextOverlayPass.VertexShader == INVALID_HANDLE ||
      TextOverlayPass.FragmentShader == INVALID_HANDLE)
    {
      return false;
    }

    TextOverlayPass.VertexShader->Hnd = pRenderer->CreateShader(TextOverlayPass.VertexShader->Code, TextOverlayPass.VertexShader->Type);
    TextOverlayPass.VertexShader->Code.Release();

    TextOverlayPass.FragmentShader->Hnd = pRenderer->CreateShader(TextOverlayPass.FragmentShader->Code, TextOverlayPass.FragmentShader->Type);
    TextOverlayPass.FragmentShader->Code.Release();

    if (TextOverlayPass.VertexShader->Hnd == INVALID_HANDLE ||
      TextOverlayPass.FragmentShader->Hnd == INVALID_HANDLE)
    {
      return false;
    }

    Viewport viewport;
    viewport.x = static_cast<float32>(TextOverlayPass.Pos.x);
    viewport.y = static_cast<float32>(TextOverlayPass.Pos.y);
    viewport.Width = static_cast<float32>(TextOverlayPass.Extent.Width);
    viewport.Height = static_cast<float32>(TextOverlayPass.Extent.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    Rect2D scissor;
    scissor.Offset.x = 0;
    scissor.Offset.y = 0;
    scissor.Extent = TextOverlayPass.Extent;

    PipelineCreateInfo textOverlayPipelineInfo = {};
    textOverlayPipelineInfo.VertexShaderHnd = TextOverlayPass.VertexShader->Hnd;
    textOverlayPipelineInfo.FragmentShaderHnd = TextOverlayPass.FragmentShader->Hnd;
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

    TextOverlayPass.PipelineHnd = pRenderer->CreatePipeline(textOverlayPipelineInfo);
    if (TextOverlayPass.PipelineHnd == INVALID_HANDLE)
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

    pRenderer->PipelineAddVertexInputBinding(TextOverlayPass.PipelineHnd, vertexBindings);
    pRenderer->PipelineAddVertexInputBinding(TextOverlayPass.PipelineHnd, glyphInstanceAttrBindings);
    pRenderer->PipelineAddRenderPass(TextOverlayPass.PipelineHnd, TextOverlayPass.RenderPassHnd);

    return true;
  }

  void ISandboxRendererSetup::DestroyGBufferPass()
  {
    pRenderer->DestroyPipeline(GBufferPass.PipelineHnd);
    pRenderer->DestroyShader(GBufferPass.VertexShader->Hnd);
    pRenderer->DestroyShader(GBufferPass.FragmentShader->Hnd);
    pRenderer->DestroyBuffer(GBufferPass.CameraUboHnd);

    pAssetManager->DestroyShader(GBufferPass.VertexShader);
    pAssetManager->DestroyShader(GBufferPass.FragmentShader);
  }

  void ISandboxRendererSetup::DestroyTextOverlayPass()
  {
    pRenderer->DestroyPipeline(TextOverlayPass.PipelineHnd);
    pRenderer->DestroyShader(TextOverlayPass.VertexShader->Hnd);
    pRenderer->DestroyShader(TextOverlayPass.FragmentShader->Hnd);
    pRenderer->DestroyImageSampler(TextOverlayPass.ImgSamplerHnd);

    pAssetManager->DestroyShader(TextOverlayPass.VertexShader);
    pAssetManager->DestroyShader(TextOverlayPass.FragmentShader);
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
      GBufferPass.PipelineHnd,
      DescriptorSetLayoutHnd
    );
    pRenderer->PipelineAddDescriptorSetLayout(
      TextOverlayPass.PipelineHnd,
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

  astl::Ref<IGBufferPass> ISandboxRendererSetup::GetGBufferPass()
  {
    return astl::Ref<IGBufferPass>(&GBufferPass);
  }

  astl::Ref<ITextOverlayPass> ISandboxRendererSetup::GetTextOverlayPass()
  {
    return astl::Ref<ITextOverlayPass>(&TextOverlayPass);
  }

  const Handle<SDescriptorSet> ISandboxRendererSetup::GetDescriptorSet() const
  {
    return DescriptorSetHnd;
  }

  void ISandboxRendererSetup::Terminate()
  {
    DestroyGBufferPass();
    DestroyTextOverlayPass();

    pRenderer->DestroyDescriptorSet(DescriptorSetHnd);
    pRenderer->DestroyDescriptorSetLayout(DescriptorSetLayoutHnd);
    pRenderer->DestroyDescriptorPool(DescriptorPoolHnd);
  }
}
