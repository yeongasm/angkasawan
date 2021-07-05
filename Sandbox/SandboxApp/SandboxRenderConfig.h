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

	class RendererSetup;

	class IBasePass
	{
	public:

		Position Pos;
		Extent2D Extent;
		astl::Ref<EngineImpl> pEngine;
		astl::Ref<IRenderSystem> pRenderer;
		astl::Ref<IAssetManager> pAssetManager;
		Handle<SPipeline> PipelineHnd;
		Handle<SRenderPass> RenderPassHnd;
	};

	class CColorPass : protected IBasePass
	{
	private:

		RefHnd<Shader> VertexShader;
		RefHnd<Shader> FragmentShader;
		//Handle<SImage> OutputAtt;

	public:
		
		using IBasePass::Pos;
		using IBasePass::Extent;

		bool Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager);
		void Terminate();

		const Handle<SRenderPass> GetRenderPassHandle() const;
		const Handle<SPipeline> GetPipelineHandle() const;
		const Handle<Shader> GetShaderHandle(EShaderType Type) const;
	};

  class CTextOverlayPass : protected IBasePass
  {
  private:
    RefHnd<Shader> VertexShader;
    RefHnd<Shader> FragmentShader;

  public:
    bool Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager);
    void Terminate();

    const Handle<SRenderPass> GetRenderPassHandle() const;
    const Handle<SPipeline> GetPipelineHandle() const;
    const Handle<Shader> GetShaderHandle(EShaderType Type) const;
  };

	class RendererSetup
	{
	private:

		Handle<SDescriptorPool> PoolHandle;
		Handle<SDescriptorSetLayout> SetLayoutHnd;
		Handle<SDescriptorSet> SetHnd;
		Handle<SMemoryBuffer> CameraUboHnd;
    Handle<SImageSampler> ModelTexImgSamplerHnd;
		astl::Ref<EngineImpl> pEngine;
		astl::Ref<IRenderSystem> pRenderer;
		astl::Ref<IAssetManager> pAssetManager;
		CColorPass ColorPass;
    CTextOverlayPass TextOverlay;

	public:

		struct CameraUbo
		{
			math::mat4 ViewProj;
			math::mat4 View;
		};

		bool Initialize(astl::Ref<EngineImpl> pInEngine, astl::Ref<IRenderSystem> pInRenderSystem, astl::Ref<IAssetManager> pInAssetManager);
		const Handle<SDescriptorPool> GetDescriptorPoolHandle() const;
		const Handle<SDescriptorSetLayout> GetDescriptorSetLayoutHandle() const;
		const Handle<SDescriptorSet> GetDescriptorSetHandle() const;
		const Handle<SMemoryBuffer> GetCameraUboHandle() const;
    const Handle<SImageSampler> GetTextureImageSampler() const;
		bool Build();
		void Terminate();

		const CColorPass& GetColorPass() const;
    const CTextOverlayPass& GetTexOverlayPass() const;
	};

}

#endif // !LEARNVK_SANDBOX_SANDBOX_APP_SANDBOX_RENDER_CONFIG_H
