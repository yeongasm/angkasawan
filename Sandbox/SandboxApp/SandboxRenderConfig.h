#pragma once
#ifndef LEARNVK_SANDBOX_SANDBOX_APP_SANDBOX_RENDER_CONFIG_H
#define LEARNVK_SANDBOX_SANDBOX_APP_SANDBOX_RENDER_CONFIG_H

#include "Library/Templates/Types.h"
#include "Engine/Interface.h"
#include "Renderer.h"
#include "Asset/Assets.h"
#include "Library/Math/Math.h"

namespace sandbox
{
	using Extent2D = WindowInfo::Extent2D;
	using Position = WindowInfo::Position;

  struct SandboxSceneCameraUbo
  {
    math::mat4 ViewProj;
    math::mat4 View;
  };

  struct ISandboxFramePass
  {
    WindowInfo::Position  Pos;
    WindowInfo::Extent2D  Extent;
    Handle<SPipeline>     PipelineHnd;
    Handle<SRenderPass>   RenderPassHnd;
    RefHnd<Shader>        VertexShader;
    RefHnd<Shader>        FragmentShader;
    void*                 pNext;
  };

  struct IGBufferPass : public ISandboxFramePass
  {
    Handle<SMemoryBuffer> CameraUboHnd;
    Handle<SImage> PositionImgHnd;
    Handle<SImage> NormalsImgHnd;
    Handle<SImage> AlbedoImgHnd;
  };

  struct ITextOverlayPass : public ISandboxFramePass
  {
    Handle<SImageSampler> ImgSamplerHnd;
  };

  class ISandboxRendererSetup
  {
  private:

    Handle<SDescriptorPool> DescriptorPoolHnd;
    Handle<SDescriptorSetLayout> DescriptorSetLayoutHnd;
    Handle<SDescriptorSet> DescriptorSetHnd;
    astl::Ref<EngineImpl> pEngine;
    astl::Ref<IRenderSystem> pRenderer;
    astl::Ref<IAssetManager> pAssetManager;
    IGBufferPass GBufferPass;
    ITextOverlayPass TextOverlayPass;

    bool PrepareGBufferPass();
    bool PrepareTextOverlayPass();
    void DestroyGBufferPass();
    void DestroyTextOverlayPass();

  public:

    bool Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager);
    astl::Ref<IGBufferPass> GetGBufferPass();
    astl::Ref<ITextOverlayPass> GetTextOverlayPass();
    const Handle<SDescriptorSet> GetDescriptorSet() const;
    void Terminate();
  };
}

#endif // !LEARNVK_SANDBOX_SANDBOX_APP_SANDBOX_RENDER_CONFIG_H
